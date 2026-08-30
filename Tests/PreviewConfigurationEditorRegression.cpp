#include "RegressionSuites.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>

#include "ConfigurationEditorControls.h"
#include "PreviewConfigurationEditor.h"
#include "RuntimeCustomPresetWorkspace.h"

#include "ShipFactionProfile.h"
#include "ShipGenerationProfile.h"
#include "ShipGenerationProfileValidation.h"
#include "ShipPaletteConfiguration.h"

namespace PixelShipGeneratorTests
{
    namespace
    {
        int fail(const std::string& message)
        {
            std::cerr << "Preview configuration editor regression failed: " << message << '\n';
            return 1;
        }

        bool clickEditorAction(PixelShipGeneratorPreview::PreviewConfigurationEditor& editor, PixelShipGeneratorPreview::ConfigurationEditorAction action, PixelShipGeneratorPreview::ConfigurationEditorEvent& outEvent)
        {
            for (const auto& button : editor.getActionButtons())
            {
                if (button.Action != action) { continue; }
                const float x = button.Bounds.Left + button.Bounds.Width * 0.5f;
                const float y = button.Bounds.Top + button.Bounds.Height * 0.5f;
                const auto event = editor.onMouseRelease(x, y);
                if (!event.has_value()) { return false; }
                outEvent = *event;
                return true;
            }
            return false;
        }
    }

    int runPreviewConfigurationEditorRegression()
    {
        using namespace PixelShipGeneratorPreview;
        using namespace PixelShipGenerator;

        ConfigurationIntegerControl probability;
        probability.configure("CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 5, 50);
        probability.setRowBounds({ 0.0f, 0.0f, 400.0f, 30.0f });
        if (probability.valueForTrackPosition(probability.TrackBounds.Left) != 0) { return fail("slider left edge did not resolve to minimum"); }
        if (probability.valueForTrackPosition(probability.TrackBounds.Left + probability.TrackBounds.Width) != 100) { return fail("slider right edge did not resolve to maximum"); }
        const int32_t midpoint = probability.valueForTrackPosition(probability.TrackBounds.Left + probability.TrackBounds.Width * 0.5f);
        if (midpoint != 50) { return fail("slider midpoint did not preserve stepped value math"); }
        if (probability.getDisplayValue() != "50%") { return fail("probability display lost probability semantics"); }

        ConfigurationIntegerControl signedOffset;
        signedOffset.configure("OFFSET", ConfigurationNumericSemantic::SIGNED_OFFSET, -20, 20, 2, -4);
        signedOffset.increment();
        signedOffset.increment();
        signedOffset.increment();
        if (signedOffset.Value != 2 || signedOffset.getDisplayValue() != "+2") { return fail("signed value stepping/display is incorrect"); }

        ConfigurationIntegerControl multiplier;
        multiplier.configure("SCALE", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 25, 300, 5, 140);
        if (multiplier.Value != 140 || multiplier.getDisplayValue() != "140% x") { return fail("multiplier values above 100 were not preserved"); }

        ConfigurationRangeControl range;
        range.configure("RANGE", 0, 100, 1, 20, 30);
        range.setValues(60, 40);
        if (range.MinimumValue != 60 || range.MaximumValue != 60) { return fail("range control did not enforce Min <= Max"); }
        range.setRowBounds({ 0.0f, 0.0f, 400.0f, 30.0f });
        range.activate(range.MaximumIncrementBounds.Left + 2.0f, range.MaximumIncrementBounds.Top + 2.0f);
        if (range.MaximumValue != 61) { return fail("range maximum stepper did not increment"); }

        ConfigurationWeightGroupControl weights;
        std::array<std::string, ConfigurationWeightGroupControl::MaximumRows> labels = {};
        std::array<uint32_t, ConfigurationWeightGroupControl::MaximumRows> weightValues = {};
        labels[0u] = "A";
        labels[1u] = "B";
        labels[2u] = "C";
        weightValues[0u] = 20u;
        weightValues[1u] = 30u;
        weightValues[2u] = 50u;
        weights.configure("WEIGHTS", labels, weightValues, 3u, 500u);
        if (weights.getRows()[0u].ProbabilityPercent != 20u || weights.getRows()[1u].ProbabilityPercent != 30u || weights.getRows()[2u].ProbabilityPercent != 50u)
        {
            return fail("relative-weight probabilities are incorrect");
        }
        weights.getRows()[0u].Control.setValue(50);
        weights.refreshProbabilities();
        if (weights.getRows()[0u].ProbabilityPercent == 20u) { return fail("relative-weight probability did not respond to weight changes"); }

        ShipGenerationProfile base = getShipGenerationProfile(ShipStyle::FIGHTER);
        PreviewConfigurationEditor editor;
        editor.setPanelBounds({ 880.0f, 0.0f, 760.0f, 1000.0f });
        editor.openStructuralProfile("FIGHTER Copy", base);
        if (!editor.isOpen()) { return fail("editor did not open"); }
        if (!editor.getValidationResult().isValid()) { return fail("valid built-in structural draft reported validation errors"); }
        if (editor.getMaximumScrollOffset() <= 0.0f) { return fail("large grouped editor form did not expose scroll state"); }
        const float originalScroll = editor.getScrollOffset();
        editor.onMouseWheelScrolled(-3.0f);
        if (editor.getScrollOffset() <= originalScroll) { return fail("scroll wheel did not advance editor content"); }
        editor.onMouseWheelScrolled(100.0f);
        if (editor.getScrollOffset() != 0.0f) { return fail("scroll state did not clamp at top"); }

        const ConfigurationTextField& initialName = editor.getNameField();
        const float nameX = initialName.Bounds.Left + 10.0f;
        const float nameY = initialName.Bounds.Top + 10.0f;
        editor.onMousePress(nameX, nameY);
        editor.onTextEntered('!');
        if (editor.getName() != "FIGHTER Copy!") { return fail("name editing did not append text"); }
        editor.onTextEntered(8u);
        if (editor.getName() != "FIGHTER Copy") { return fail("name editing backspace did not remove text"); }

        const auto weaponChance = editor.getWeaponChanceControl();
        editor.onMouseRelease(weaponChance.IncrementBounds.Left + 2.0f, weaponChance.IncrementBounds.Top + 2.0f);
        if (!editor.hasUnsavedChanges() || editor.getDraftProfile().LargeWeaponChance == base.LargeWeaponChance) { return fail("representative integer edit did not update draft profile"); }

        PixelShipGenerator::ValidationResult displayValidation;
        displayValidation.Errors.push_back({ "Example.Field", "Example validation error", ValidationSeverity::ERROR });
        displayValidation.Warnings.push_back({ "Example.Warning", "Example validation warning", ValidationSeverity::WARNING });
        editor.setValidationResult(displayValidation);
        if (editor.getValidationResult().Errors.size() != 1u || editor.getValidationResult().Warnings.size() != 1u) { return fail("generic validation-message state did not preserve Core errors/warnings"); }
        editor.setValidationResult(validateShipGenerationProfile(editor.getDraftProfile()));

        ConfigurationEditorEvent event;
        if (!clickEditorAction(editor, ConfigurationEditorAction::APPLY, event) || event.Action != ConfigurationEditorAction::APPLY) { return fail("Apply action was not reported to controller"); }
        if (!clickEditorAction(editor, ConfigurationEditorAction::DUPLICATE, event) || event.Action != ConfigurationEditorAction::DUPLICATE) { return fail("Duplicate action was not reported to controller"); }

        const auto resetButton = editor.getActionButtons()[2u];
        editor.onMouseRelease(resetButton.Bounds.Left + 2.0f, resetButton.Bounds.Top + 2.0f);
        if (editor.getDraftProfile().LargeWeaponChance != base.LargeWeaponChance || editor.hasUnsavedChanges()) { return fail("Reset did not restore the opening draft"); }
        if (editor.createCancelEvent().Action != ConfigurationEditorAction::CANCEL) { return fail("Cancel event semantics are incorrect"); }

        const bool wasExpanded = editor.getSections()[4u].Expanded;
        editor.setSectionExpanded(4u, !wasExpanded);
        if (editor.getSections()[4u].Expanded == wasExpanded) { return fail("section grouping state did not change"); }
        editor.setSectionExpanded(4u, wasExpanded);

        RuntimeCustomPresetWorkspace workspace;
        const RuntimeCustomPresetId structuralId = workspace.addStructural("My Structural", base);
        const RuntimeCustomPresetId factionId = workspace.addFaction("My Faction", getShipFactionProfile(ShipFactionType::RELIC));
        ShipPaletteConfiguration palette;
        palette.Mode = ShipPaletteSourceMode::FIXED;
        const RuntimeCustomPresetId paletteId = workspace.addPalette("My Palette", palette);
        if (workspace.getStructuralPresets().size() != 1u || workspace.getFactionPresets().size() != 1u || workspace.getPalettePresets().size() != 1u) { return fail("runtime workspace add behavior is incorrect"); }

        const auto structuralCopy = workspace.duplicateStructural(structuralId);
        const auto factionCopy = workspace.duplicateFaction(factionId);
        const auto paletteCopy = workspace.duplicatePalette(paletteId);
        if (!structuralCopy.has_value() || !factionCopy.has_value() || !paletteCopy.has_value()) { return fail("runtime workspace duplicate behavior failed"); }
        if (workspace.findStructural(*structuralCopy)->Name == workspace.findStructural(structuralId)->Name) { return fail("duplicated preset did not receive a unique session name"); }
        if (!workspace.removeStructural(structuralId) || !workspace.removeFaction(factionId) || !workspace.removePalette(paletteId)) { return fail("runtime workspace remove behavior failed"); }
        if (workspace.findStructural(structuralId) != nullptr || workspace.findFaction(factionId) != nullptr || workspace.findPalette(paletteId) != nullptr) { return fail("removed runtime presets remained addressable"); }

        std::cout << "Preview configuration editor regression passed.\n";
        return 0;
    }
}
