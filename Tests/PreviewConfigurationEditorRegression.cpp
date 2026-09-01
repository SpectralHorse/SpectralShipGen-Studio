#include "PreviewRegressionSuites.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

#include "ConfigurationEditorControls.h"
#include "PreviewConfigurationEditor.h"
#include "RuntimeCustomPresetWorkspace.h"
#include "StructuralProfileSelection.h"
#include "UserPresetPersistence.h"

#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipGenerationProfileValidation.h>
#include <SpectralShipGen/ShipGenerationSettings.h>
#include <SpectralShipGen/ShipGenerator.h>
#include <SpectralShipGen/ShipPaletteConfiguration.h>

namespace SpectralShipGenStudioTests
{
    namespace
    {
        int fail(const std::string& message)
        {
            std::cerr << "Preview configuration editor regression failed: " << message << '\n';
            return 1;
        }

        bool clickEditorAction(SpectralShipGenStudioPreview::PreviewConfigurationEditor& editor, SpectralShipGenStudioPreview::ConfigurationEditorAction action, SpectralShipGenStudioPreview::ConfigurationEditorEvent& outEvent)
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

        bool hasValidationError(const SpectralShipGen::ValidationResult& result, const std::string& field)
        {
            for (const SpectralShipGen::ValidationIssue& issue : result.Errors) { if (issue.Field == field) { return true; } }
            return false;
        }

        bool imagesEqual(const SpectralShipGen::Image& first, const SpectralShipGen::Image& second)
        {
            return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
        }
    }

    int runPreviewConfigurationEditorRegression()
    {
        using namespace SpectralShipGenStudioPreview;
        using namespace SpectralShipGen;

        ConfigurationIntegerControl probability;
        probability.configure("CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 5, 50);
        probability.setRowBounds({ 0.0f, 0.0f, 400.0f, 30.0f });
        if (probability.valueForTrackPosition(probability.TrackBounds.Left) != 0) { return fail("slider left edge did not resolve to minimum"); }
        if (probability.valueForTrackPosition(probability.TrackBounds.Left + probability.TrackBounds.Width) != 100) { return fail("slider right edge did not resolve to maximum"); }
        if (probability.valueForTrackPosition(probability.TrackBounds.Left + probability.TrackBounds.Width * 0.5f) != 50) { return fail("slider midpoint did not preserve stepped value math"); }
        if (probability.getDisplayValue() != "50%") { return fail("probability display lost probability semantics"); }

        ConfigurationIntegerControl signedOffset;
        signedOffset.configure("OFFSET", ConfigurationNumericSemantic::SIGNED_OFFSET, -20, 20, 2, -4);
        signedOffset.increment(); signedOffset.increment(); signedOffset.increment();
        if (signedOffset.Value != 2 || signedOffset.getDisplayValue() != "+2") { return fail("signed value stepping/display is incorrect"); }

        ConfigurationIntegerControl multiplier;
        multiplier.configure("SCALE", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, 2000, 5, 140);
        if (multiplier.Value != 140 || multiplier.getDisplayValue() != "140% x") { return fail("multiplier values above 100 were not preserved"); }

        ConfigurationRangeControl range;
        range.configure("RANGE", 0, 100, 1, 20, 80);
        range.setRowBounds({ 0.0f, 0.0f, 720.0f, 30.0f });
        if (range.minimumValueForTrackPosition(range.MinimumTrackBounds.Left) != 0 ||
            range.maximumValueForTrackPosition(range.MaximumTrackBounds.Left + range.MaximumTrackBounds.Width) != 100)
        {
            return fail("range slider endpoints did not resolve exactly");
        }
        const float minimum75X = range.MinimumTrackBounds.Left + range.MinimumTrackBounds.Width * 0.75f;
        if (!range.beginPointer(minimum75X, range.MinimumTrackBounds.Top - 4.0f) || range.MinimumValue != 75) { return fail("range minimum slider press/thumb hit did not update exact integer value"); }
        if (!range.updatePointer(range.MinimumTrackBounds.Left + range.MinimumTrackBounds.Width) || range.MinimumValue != 80) { return fail("range minimum drag did not clamp at current maximum"); }
        if (!range.endPointer(range.MinimumTrackBounds.Left, range.MinimumTrackBounds.Top) || range.MinimumValue != 0 || range.DraggingEndpoint != -1) { return fail("range minimum drag/release did not resolve endpoint/reset drag state"); }
        const float maximum25X = range.MaximumTrackBounds.Left + range.MaximumTrackBounds.Width * 0.25f;
        if (!range.beginPointer(maximum25X, range.MaximumTrackBounds.Top) || range.MaximumValue != 25) { return fail("range maximum slider did not update exact middle integer value"); }
        if (!range.endPointer(range.MaximumTrackBounds.Left, range.MaximumTrackBounds.Top) || range.MaximumValue != range.MinimumValue) { return fail("range maximum drag did not preserve Min <= Max at the lower endpoint"); }
        range.setValues(60, 40);
        if (range.MinimumValue != 60 || range.MaximumValue != 60) { return fail("range control did not preserve existing Min <= Max normalization"); }

        ConfigurationToggleControl toggle;
        toggle.configure("TOGGLE", false);
        toggle.setRowBounds({ 0.0f, 0.0f, 400.0f, 30.0f });
        if (!toggle.activate(toggle.ToggleBounds.Left + 2.0f, toggle.ToggleBounds.Top + 2.0f) || !toggle.Value) { return fail("toggle control did not change semantic boolean value"); }

        ConfigurationChoiceControl choice;
        choice.configure("CHOICE", { "A", "B", "C" }, 1u);
        choice.setRowBounds({ 0.0f, 0.0f, 400.0f, 30.0f });
        if (!choice.activate(choice.NextBounds.Left + 2.0f, choice.NextBounds.Top + 2.0f) || choice.getDisplayValue() != "C") { return fail("choice control did not advance enum value"); }

        ConfigurationWeightGroupControl weights;
        std::array<std::string, ConfigurationWeightGroupControl::MaximumRows> labels = {};
        std::array<uint32_t, ConfigurationWeightGroupControl::MaximumRows> weightValues = {};
        labels[0u] = "A"; labels[1u] = "B"; labels[2u] = "C";
        weightValues[0u] = 20u; weightValues[1u] = 30u; weightValues[2u] = 50u;
        weights.configure("WEIGHTS", labels, weightValues, 3u, 10000u);
        if (weights.getRows()[0u].ProbabilityPercent != 20u || weights.getRows()[1u].ProbabilityPercent != 30u || weights.getRows()[2u].ProbabilityPercent != 50u) { return fail("relative-weight probabilities are incorrect"); }

        const ShipGenerationProfile fighterBefore = getShipGenerationProfile(ShipStyle::FIGHTER);
        PreviewConfigurationEditor editor;
        editor.setPanelBounds({ 880.0f, 0.0f, 760.0f, 1000.0f });
        editor.openStructuralProfile("FIGHTER Copy", fighterBefore);
        if (!editor.isOpen()) { return fail("editor did not open"); }
        if (!editor.getValidationResult().isValid()) { return fail("valid built-in structural draft reported validation errors"); }
        if (editor.getProfileSections().size() < 18u || editor.getBoundValueCount() < 130u) { return fail("full public structural profile was not mapped into semantic editor sections"); }
        if (editor.findIntegerField("LargeWeaponChance") == nullptr || editor.findRangeField("NoseWidthPercent") == nullptr || editor.findWeightGroup("WingWeights") == nullptr || editor.findToggleField("VisualHierarchyEnabled") == nullptr || editor.findChoiceField("CoreRaisedSurfaceTone") == nullptr || editor.findIntegerField("AnimationTraits.Firing.ResponseStrengthPercent") == nullptr)
        {
            return fail("representative structural/animation field bindings are missing");
        }
        if (editor.getMaximumScrollOffset() <= 0.0f) { return fail("large grouped editor form did not expose scroll state"); }
        const auto expandOnly = [&](std::size_t sectionIndex)
        {
            for (std::size_t index = 0u; index < editor.getProfileSections().size(); ++index) { editor.setSectionExpanded(index, index == sectionIndex); }
            editor.onMouseWheelScrolled(1000.0f);
        };

        const ConfigurationTextField& initialName = editor.getNameField();
        editor.onMousePress(initialName.Bounds.Left + 10.0f, initialName.Bounds.Top + 10.0f);
        editor.onTextEntered('!');
        if (editor.getName() != "FIGHTER Copy!") { return fail("name editing did not append text"); }
        editor.onTextEntered(8u);

        expandOnly(10u); // Weapons.
        StructuralIntegerFieldBinding* weaponChance = editor.findIntegerField("LargeWeaponChance");
        if (weaponChance == nullptr) { return fail("weapon chance binding lookup failed"); }
        editor.onMouseRelease(weaponChance->Control.IncrementBounds.Left + 2.0f, weaponChance->Control.IncrementBounds.Top + 2.0f);
        if (!editor.hasUnsavedChanges() || editor.getDraftProfile().LargeWeaponChance == fighterBefore.LargeWeaponChance) { return fail("probability edit did not update the real public draft profile"); }

        expandOnly(3u); // Wings.
        StructuralWeightGroupBinding* wingWeights = editor.findWeightGroup("WingWeights");
        if (wingWeights == nullptr) { return fail("wing weight binding lookup failed"); }
        auto& wingRows = wingWeights->Control.getRows();
        const uint32_t originalBroadShare = wingRows[3u].ProbabilityPercent;
        editor.onMouseRelease(wingRows[3u].Control.IncrementBounds.Left + 2.0f, wingRows[3u].Control.IncrementBounds.Top + 2.0f);
        if (editor.getDraftProfile().BroadWingWeight == fighterBefore.BroadWingWeight || wingRows[3u].ProbabilityPercent == originalBroadShare) { return fail("relative-weight edit did not update weight and normalized readout"); }

        expandOnly(2u); // Hull dimensions.
        StructuralRangeFieldBinding* noseEnd = editor.findRangeField("NoseEndPercent");
        StructuralRangeFieldBinding* noseWidth = editor.findRangeField("NoseWidthPercent");
        StructuralRangeFieldBinding* upperWidth = editor.findRangeField("UpperFuselageWidthPercent");
        if (noseEnd == nullptr || noseWidth == nullptr || upperWidth == nullptr) { return fail("representative structural range binding lookup failed"); }
        const int32_t originalUpperWidthMinimum = upperWidth->Control.MinimumValue;
        const int32_t noseWidthTarget = noseWidth->Control.MinimumValue < noseWidth->Control.MaximumValue ? noseWidth->Control.MinimumValue + 1 : noseWidth->Control.MinimumValue;
        const float noseWidthTargetX = noseWidth->Control.MinimumTrackBounds.Left +
            noseWidth->Control.MinimumTrackBounds.Width * static_cast<float>(noseWidthTarget - noseWidth->Control.MinimumLimit) /
            static_cast<float>(std::max(1, noseWidth->Control.MaximumLimit - noseWidth->Control.MinimumLimit));
        editor.onMousePress(noseWidthTargetX, noseWidth->Control.MinimumTrackBounds.Top - 4.0f);
        if (static_cast<int32_t>(editor.getDraftProfile().NoseWidthPercent.Min) != noseWidth->Control.MinimumValue) { return fail("structural range slider press did not synchronize the authoritative draft immediately"); }
        editor.onMouseRelease(0.0f, 0.0f);
        if (noseWidth->Control.DraggingEndpoint != -1 ||
            static_cast<int32_t>(editor.getDraftProfile().NoseWidthPercent.Min) != noseWidth->Control.MinimumValue ||
            upperWidth->Control.MinimumValue != originalUpperWidthMinimum)
        {
            return fail("structural range slider outside release did not preserve synchronized state or leaked into another control");
        }
        RuntimeCustomPresetWorkspace sliderPersistenceWorkspace;
        const RuntimeCustomPresetId sliderPresetId = sliderPersistenceWorkspace.addStructural("Slider Persistence", editor.getDraftProfile());
        const UserPresetLibraryLoadResult sliderReloaded = deserializeUserPresetLibrary(serializeUserPresetLibrary(sliderPersistenceWorkspace));
        const RuntimeStructuralPreset* sliderReloadedPreset = sliderReloaded.Workspace.findStructural(sliderPresetId);
        if (!sliderReloaded.Success || sliderReloadedPreset == nullptr || sliderReloadedPreset->Profile.NoseWidthPercent.Min != editor.getDraftProfile().NoseWidthPercent.Min)
        {
            return fail("slider-edited structural range did not persist exactly through the current user-preset schema");
        }
        noseEnd->Control.setValues(70, 80);
        expandOnly(0u);
        StructuralToggleFieldBinding* hierarchyToggle = editor.findToggleField("VisualHierarchyEnabled");
        editor.onMouseRelease(hierarchyToggle->Control.ToggleBounds.Left + 2.0f, hierarchyToggle->Control.ToggleBounds.Top + 2.0f);
        if (!hasValidationError(editor.getValidationResult(), "LongitudinalHullSegments")) { return fail("invalid longitudinal range was not reported by Core validation"); }
        if (editor.getActionButtons()[0u].Enabled) { return fail("Apply remained enabled for invalid Core configuration"); }

        const auto resetButton = editor.getActionButtons()[2u];
        editor.onMouseRelease(resetButton.Bounds.Left + 2.0f, resetButton.Bounds.Top + 2.0f);
        if (!editor.getValidationResult().isValid() || editor.hasUnsavedChanges()) { return fail("Reset did not restore the opening valid public profile"); }

        expandOnly(10u);
        weaponChance = editor.findIntegerField("LargeWeaponChance");
        editor.onMouseRelease(weaponChance->Control.IncrementBounds.Left + 2.0f, weaponChance->Control.IncrementBounds.Top + 2.0f);
        ConfigurationEditorEvent actionEvent;
        if (!clickEditorAction(editor, ConfigurationEditorAction::APPLY, actionEvent) || actionEvent.Action != ConfigurationEditorAction::APPLY) { return fail("Apply action was not reported for valid edited profile"); }
        if (!clickEditorAction(editor, ConfigurationEditorAction::DUPLICATE, actionEvent) || actionEvent.Action != ConfigurationEditorAction::DUPLICATE) { return fail("Duplicate action was not reported"); }

        RuntimeCustomPresetWorkspace workspace;
        PreviewConfigurationEditor defaultEditor;
        defaultEditor.setPanelBounds({ 880.0f, 0.0f, 760.0f, 1000.0f });
        defaultEditor.openStructuralProfile("Custom Profile", ShipGenerationProfile{});
        if (!defaultEditor.getValidationResult().isValid()) { return fail("default-constructed public structural profile is not a valid add-profile starting point"); }
        const RuntimeCustomPresetId defaultCopy = workspace.addStructural(defaultEditor.getName(), defaultEditor.getDraftProfile());
        if (workspace.findStructural(defaultCopy) == nullptr || workspace.findStructural(defaultCopy)->Name != "Custom Profile") { return fail("create-from-default runtime structural profile workflow failed"); }

        const RuntimeCustomPresetId fighterCopy = workspace.addStructural("FIGHTER Copy", getShipGenerationProfile(ShipStyle::FIGHTER));
        const RuntimeCustomPresetId industrialCopy = workspace.addStructural("INDUSTRIAL Copy", getShipGenerationProfile(ShipStyle::INDUSTRIAL));
        const RuntimeCustomPresetId spearheadCopy = workspace.addStructural("SPEARHEAD Copy", getShipGenerationProfile(ShipStyle::SPEARHEAD));
        if (workspace.getStructuralPresets().size() != 4u) { return fail("default/built-in duplicate-to-runtime workflows did not create four custom structural profiles"); }
        ShipGenerationProfile updatedIndustrial = workspace.findStructural(industrialCopy)->Profile;
        updatedIndustrial.LargeWeaponChance = 100u;
        if (!workspace.updateStructural(industrialCopy, "INDUSTRIAL Arsenal", updatedIndustrial) || workspace.findStructural(industrialCopy)->Name != "INDUSTRIAL Arsenal") { return fail("existing runtime structural profile could not be edited in place"); }
        if (getShipGenerationProfile(ShipStyle::FIGHTER).LargeWeaponChance != fighterBefore.LargeWeaponChance) { return fail("canonical built-in FIGHTER profile was mutated by runtime editing"); }

        const auto entries = buildStructuralProfileSelection(workspace);
        if (entries.size() != 11u || entries.front().Kind != StructuralProfileSelectionKind::BUILT_IN || entries[6u].Kind != StructuralProfileSelectionKind::RUNTIME_CUSTOM || entries.back().Kind != StructuralProfileSelectionKind::ADD_PROFILE || entries.back().Label != "+ ADD PROFILE")
        {
            return fail("built-in/custom/+ADD structural selector ordering is incorrect");
        }
        ShipGenerationRecipe customRecipe;
        customRecipe.StructuralPreset.reset();
        customRecipe.StructuralProfile = workspace.findStructural(spearheadCopy)->Profile;
        const std::size_t selectedCustomIndex = findStructuralProfileSelectionIndex(entries, customRecipe, spearheadCopy);
        if (entries[selectedCustomIndex].CustomPresetId != spearheadCopy) { return fail("runtime custom structural selection identity was not retained"); }

        ShipGenerationProfile distinctProfile = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
        distinctProfile.LargeWeaponChance = 85u;
        distinctProfile.MaximumLargeWeaponGroups = 2u;
        distinctProfile.LargeWeaponScalePercent = 140u;
        if (!validateShipGenerationProfile(distinctProfile).isValid()) { return fail("notably different manual-review profile is unexpectedly invalid"); }

        ShipGenerationConfiguration configuration;
        configuration.Seed = 0xC001000000000001ull;
        configuration.Dimensions = { 96u, 64u };
        configuration.Faction = ShipFactionType::FRONTIER;
        configuration.DetailDensity = 57u;
        configuration.AsymmetricDetailChance = 13u;
        ShipGenerator generator;
        const GeneratedShip customA = generator.generate(configuration, distinctProfile);
        const GeneratedShip customB = generator.generate(configuration, distinctProfile);
        const GeneratedShip builtIn = generator.generate(configuration, getShipGenerationProfile(ShipStyle::INDUSTRIAL));
        if (!imagesEqual(customA.FinalImage, customB.FinalImage)) { return fail("edited custom structural profile was not deterministic for the same seed"); }
        if (imagesEqual(customA.FinalImage, builtIn.FinalImage)) { return fail("notably different custom structural profile did not affect generated output"); }
        if (customA.Provenance.StructuralPreset.has_value()) { return fail("custom structural generation pretended to be a built-in style"); }

        const RuntimeCustomPresetId factionId = workspace.addFaction("My Faction", getShipFactionProfile(ShipFactionType::RELIC));
        ShipPaletteConfiguration palette;
        palette.Mode = ShipPaletteSourceMode::FIXED;
        const RuntimeCustomPresetId paletteId = workspace.addPalette("My Palette", palette);
        if (!workspace.duplicateStructural(fighterCopy).has_value() || !workspace.duplicateFaction(factionId).has_value() || !workspace.duplicatePalette(paletteId).has_value()) { return fail("runtime workspace duplicate behavior failed"); }

        std::cout << "Preview configuration editor regression passed.\n";
        return 0;
    }
}
