#include "PreviewRegressionSuites.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

#include <SpectralShipGen/BuiltInPresetCatalog.h>
#include <SpectralShipGen/GenerationDomain.h>
#include "ShipGenerationRecipeSerializer.h"
#include <SpectralShipGen/ShipGenerator.h>
#include <SpectralShipGen/ShipPaletteGenerationProfile.h>
#include <SpectralShipGen/ShipPaletteGenerationProfileValidation.h>

#include "ConfigurationEditorControls.h"
#include "PaletteProfileSelection.h"
#include "PreviewCommand.h"
#include "PreviewConfigurationEditor.h"
#include "RuntimeCustomPresetWorkspace.h"
#include "ShipPaletteConfigurationEditorBindings.h"

namespace
{
    using namespace SpectralShipGen;
    using namespace SpectralShipGenStudioPreview;

    bool imagesEqual(const Image& first, const Image& second)
    {
        return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
    }

    bool masksEqual(const PixelMask& first, const PixelMask& second)
    {
        if (first.getWidth() != second.getWidth() || first.getHeight() != second.getHeight()) { return false; }
        for (uint32_t y = 0u; y < first.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < first.getWidth(); ++x)
            {
                if (first.get(x, y) != second.get(x, y)) { return false; }
            }
        }
        return true;
    }

    bool geometryEqual(const GeneratedShip& first, const GeneratedShip& second)
    {
        return masksEqual(first.HullMask, second.HullMask) && masksEqual(first.CockpitMask, second.CockpitMask) &&
            masksEqual(first.EngineMask, second.EngineMask) && masksEqual(first.EngineExhaustMask, second.EngineExhaustMask) &&
            masksEqual(first.AttachmentMask, second.AttachmentMask) && masksEqual(first.AccentMask, second.AccentMask) &&
            masksEqual(first.MechanicalDetailMask, second.MechanicalDetailMask) && masksEqual(first.LightMask, second.LightMask);
    }

    bool palettesEqual(const ShipPalette& a, const ShipPalette& b)
    {
        return a.Transparent == b.Transparent && a.Outline == b.Outline && a.HullDeepShadow == b.HullDeepShadow && a.HullShadow == b.HullShadow &&
            a.HullBase == b.HullBase && a.HullHighlight == b.HullHighlight && a.HullSecondary == b.HullSecondary && a.HullEdgeHighlight == b.HullEdgeHighlight &&
            a.CockpitDark == b.CockpitDark && a.CockpitBase == b.CockpitBase && a.CockpitHighlight == b.CockpitHighlight && a.CockpitGlint == b.CockpitGlint &&
            a.EngineDark == b.EngineDark && a.EngineBase == b.EngineBase && a.EngineHighlight == b.EngineHighlight && a.EngineHotCore == b.EngineHotCore &&
            a.ExhaustBase == b.ExhaustBase && a.ExhaustHighlight == b.ExhaustHighlight && a.ExhaustHotCore == b.ExhaustHotCore &&
            a.HullAccentDark == b.HullAccentDark && a.HullAccent == b.HullAccent && a.HullAccentHighlight == b.HullAccentHighlight &&
            a.MechanicalDark == b.MechanicalDark && a.MechanicalBase == b.MechanicalBase && a.LightBase == b.LightBase && a.LightHighlight == b.LightHighlight;
    }

    ShipPalette makeFixedPalette(uint8_t offset)
    {
        ShipPalette p;
        uint8_t value = offset;
        auto next = [&]() { value = static_cast<uint8_t>(value + 7u); return value; };
        p.Transparent = Color(next(), next(), next(), 0u);
        p.Outline = Color(next(), next(), next(), 255u);
        p.HullDeepShadow = Color(next(), next(), next(), 255u);
        p.HullShadow = Color(next(), next(), next(), 255u);
        p.HullBase = Color(next(), next(), next(), 255u);
        p.HullHighlight = Color(next(), next(), next(), 255u);
        p.HullSecondary = Color(next(), next(), next(), 255u);
        p.HullEdgeHighlight = Color(next(), next(), next(), 255u);
        p.CockpitDark = Color(next(), next(), next(), 255u);
        p.CockpitBase = Color(next(), next(), next(), 255u);
        p.CockpitHighlight = Color(next(), next(), next(), 255u);
        p.CockpitGlint = Color(next(), next(), next(), 255u);
        p.EngineDark = Color(next(), next(), next(), 255u);
        p.EngineBase = Color(next(), next(), next(), 255u);
        p.EngineHighlight = Color(next(), next(), next(), 255u);
        p.EngineHotCore = Color(next(), next(), next(), 255u);
        p.ExhaustBase = Color(next(), next(), next(), 255u);
        p.ExhaustHighlight = Color(next(), next(), next(), 255u);
        p.ExhaustHotCore = Color(next(), next(), next(), 255u);
        p.HullAccentDark = Color(next(), next(), next(), 255u);
        p.HullAccent = Color(next(), next(), next(), 255u);
        p.HullAccentHighlight = Color(next(), next(), next(), 255u);
        p.MechanicalDark = Color(next(), next(), next(), 255u);
        p.MechanicalBase = Color(next(), next(), next(), 255u);
        p.LightBase = Color(next(), next(), next(), 255u);
        p.LightHighlight = Color(next(), next(), next(), 255u);
        return p;
    }

    float centerX(const ConfigurationEditorRect& bounds) { return bounds.Left + bounds.Width * 0.5f; }
    float centerY(const ConfigurationEditorRect& bounds) { return bounds.Top + bounds.Height * 0.5f; }


    bool checkCommandMetadataAlignment()
    {
        const auto& table = getPreviewCommandDataTable();
        for (std::size_t index = 0u; index < table.size(); ++index)
        {
            if (table[index].Type != static_cast<PreviewCommandType>(index)) { return false; }
        }
        return getPreviewCommandData(PreviewCommandType::PREVIOUS_PALETTE).Type == PreviewCommandType::PREVIOUS_PALETTE &&
            getPreviewCommandData(PreviewCommandType::NEXT_PALETTE).Type == PreviewCommandType::NEXT_PALETTE &&
            getPreviewCommandData(PreviewCommandType::SELECT_RESOLUTION).Type == PreviewCommandType::SELECT_RESOLUTION;
    }

    bool checkColorControl()
    {
        ConfigurationColorControl control;
        control.configure("TEST", 12u, 34u, 56u, 78u);
        control.setRowBounds({ 10.0f, 20.0f, 700.0f, 62.0f });
        if (control.Red != 12u || control.Green != 34u || control.Blue != 56u || control.Alpha != 78u) { return false; }
        const auto& redTrack = control.TrackBounds[0u];
        const float redX = redTrack.Left + redTrack.Width * (200.0f / 255.0f);
        if (!control.beginPointer(redX, redTrack.Top) || !control.endPointer(redX, redTrack.Top) || control.Red != 200u) { return false; }
        const auto& alphaTrack = control.TrackBounds[3u];
        if (!control.beginPointer(alphaTrack.Left, alphaTrack.Top) || !control.endPointer(alphaTrack.Left, alphaTrack.Top) || control.Alpha != 0u) { return false; }
        return control.SwatchBounds.Width > 0.0f && control.SwatchBounds.Height > 0.0f;
    }

    bool checkBuiltInPaletteRoundTrip()
    {
        ShipPaletteConfigurationEditorBindings bindings;
        for (const BuiltInPalettePreset& preset : getBuiltInPalettePresetCatalog())
        {
            ShipPaletteConfiguration configuration;
            configuration.Mode = ShipPaletteSourceMode::EXPLICIT_GENERATED;
            configuration.Generated = getBuiltInPalettePresetProfile(preset.FactionPreset);
            configuration.Fixed = makeFixedPalette(static_cast<uint8_t>(17u + static_cast<uint32_t>(preset.FactionPreset) * 9u));

            PreviewConfigurationEditor editor;
            editor.setPanelBounds({ 880.0f, 0.0f, 760.0f, 1000.0f });
            editor.openPaletteConfiguration(preset.StableId, configuration);
            if (!bindings.equivalent(configuration, editor.getDraftPaletteConfiguration())) { return false; }

            const ShipPaletteGenerationProfile before = getBuiltInPalettePresetProfile(preset.FactionPreset);
            editor.setSectionExpanded(1u, true);
            PaletteRangeFieldBinding* hue = editor.findPaletteRangeField("Generated.Ranges.HullHue");
            if (hue == nullptr) { return false; }
            if (hue->Control.MinimumValue < hue->Control.MaximumValue)
            {
                const int32_t target = hue->Control.MinimumValue + 1;
                const float targetX = hue->Control.MinimumTrackBounds.Left + hue->Control.MinimumTrackBounds.Width *
                    static_cast<float>(target - hue->Control.MinimumLimit) / static_cast<float>(std::max(1, hue->Control.MaximumLimit - hue->Control.MinimumLimit));
                editor.onMousePress(targetX, hue->Control.MinimumTrackBounds.Top);
                editor.onMouseRelease(targetX, hue->Control.MinimumTrackBounds.Top);
            }
            const ShipPaletteGenerationProfile after = getBuiltInPalettePresetProfile(preset.FactionPreset);
            if (before.Ranges.HullHue.Min != after.Ranges.HullHue.Min || before.Ranges.HullHue.Max != after.Ranges.HullHue.Max) { return false; }
        }
        return true;
    }

    bool checkEditorAndValidation()
    {
        ShipPaletteConfiguration configuration;
        configuration.Mode = ShipPaletteSourceMode::EXPLICIT_GENERATED;
        configuration.Generated = getShipPaletteGenerationProfile(ShipFactionType::CORPORATE);
        configuration.Fixed = makeFixedPalette(13u);
        const ShipPaletteGenerationProfile canonicalCorporate = getShipPaletteGenerationProfile(ShipFactionType::CORPORATE);

        PreviewConfigurationEditor editor;
        editor.setPanelBounds({ 880.0f, 0.0f, 760.0f, 1000.0f });
        editor.openPaletteConfiguration("Corporate Copy", configuration);
        if (editor.getProfileKind() != ConfigurationEditorProfileKind::PALETTE || editor.getBoundValueCount() != 148u || !editor.getValidationResult().isValid()) { return false; }

        editor.setSectionExpanded(1u, true);
        PaletteRangeFieldBinding* hullHue = editor.findPaletteRangeField("Generated.Ranges.HullHue");
        if (hullHue == nullptr) { return false; }
        const int32_t originalMin = hullHue->Control.MinimumValue;
        if (originalMin < hullHue->Control.MaximumValue)
        {
            const int32_t target = originalMin + 1;
            const float targetX = hullHue->Control.MinimumTrackBounds.Left + hullHue->Control.MinimumTrackBounds.Width *
                static_cast<float>(target - hullHue->Control.MinimumLimit) / static_cast<float>(std::max(1, hullHue->Control.MaximumLimit - hullHue->Control.MinimumLimit));
            editor.onMousePress(targetX, hullHue->Control.MinimumTrackBounds.Top);
            editor.onMouseRelease(targetX, hullHue->Control.MinimumTrackBounds.Top);
            if (editor.getDraftPaletteConfiguration().Generated.Ranges.HullHue.Min != static_cast<uint32_t>(target)) { return false; }
        }

        PaletteChoiceFieldBinding* mode = editor.findPaletteChoiceField("Mode");
        if (mode == nullptr) { return false; }
        editor.onMouseRelease(centerX(mode->Control.NextBounds), centerY(mode->Control.NextBounds));
        if (editor.getDraftPaletteConfiguration().Mode != ShipPaletteSourceMode::FIXED) { return false; }
        editor.setSectionExpanded(5u, true);
        PaletteColorFieldBinding* hullBase = editor.findPaletteColorField("Fixed.HullBase");
        if (hullBase == nullptr) { return false; }
        const ConfigurationEditorRect redTrack = hullBase->Control.TrackBounds[0u];
        const float redX = redTrack.Left + redTrack.Width * (201.0f / 255.0f);
        editor.onMousePress(redX, redTrack.Top);
        editor.onMouseRelease(redX, redTrack.Top);
        if (editor.getDraftPaletteConfiguration().Fixed.HullBase.R != 201u) { return false; }

        if (getShipPaletteGenerationProfile(ShipFactionType::CORPORATE).Ranges.HullHue.Min != canonicalCorporate.Ranges.HullHue.Min) { return false; }

        ShipPaletteConfiguration invalid = configuration;
        invalid.Generated.Ranges.HullHue = { 320u, 20u };
        editor.openPaletteConfiguration("Invalid", invalid);
        if (editor.getValidationResult().isValid() || editor.getActionButtons()[0u].Enabled) { return false; }
        return true;
    }

    bool checkWorkspaceAndSelection()
    {
        RuntimeCustomPresetWorkspace workspace;
        ShipPaletteConfiguration generated;
        generated.Mode = ShipPaletteSourceMode::EXPLICIT_GENERATED;
        generated.Generated = getShipPaletteGenerationProfile(ShipFactionType::MILITARY);
        const RuntimeCustomPresetId id = workspace.addPalette("Blue Steel", generated);
        const auto duplicate = workspace.duplicatePalette(id);
        if (!duplicate.has_value() || workspace.getPalettePresets().size() != 2u) { return false; }
        generated.Generated.Ranges.HullHue = { 200u, 240u };
        if (!workspace.updatePalette(id, "Blue Steel Edited", generated)) { return false; }

        const std::vector<PaletteProfileSelectionEntry> entries = buildPaletteProfileSelection(workspace);
        if (entries.size() != 10u || entries.front().Kind != PaletteProfileSelectionKind::FACTION_DEFAULT || entries.back().Kind != PaletteProfileSelectionKind::ADD_PALETTE) { return false; }
        for (std::size_t index = 1u; index <= 6u; ++index) { if (entries[index].Kind != PaletteProfileSelectionKind::BUILT_IN_GENERATED) { return false; } }

        ShipGenerationRecipe recipe;
        recipe.PaletteConfiguration = generated;
        const std::size_t customIndex = findPaletteProfileSelectionIndex(entries, recipe, std::nullopt, id);
        return entries[customIndex].Kind == PaletteProfileSelectionKind::RUNTIME_CUSTOM && entries[customIndex].CustomPresetId == id;
    }

    bool checkPaletteGenerationSemantics()
    {
        ShipGenerator generator;
        ShipGenerationProfile structural = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
        structural.LargeWeaponChance = 82u;
        ShipFactionProfile faction = getShipFactionProfile(ShipFactionType::RELIC);
        faction.Weapons.ChancePercent = 135u;

        ShipPaletteGenerationProfile generatedProfile = getShipPaletteGenerationProfile(ShipFactionType::XENO);
        generatedProfile.Ranges.HullHue = { 278u, 315u };
        generatedProfile.Ranges.HullSaturation = { 62u, 94u };
        generatedProfile.Ranges.Accent.HueOffset = { 95, 150 };
        generatedProfile.Behavior.MinimumAccentHueDistance = 70u;
        if (!validateShipPaletteGenerationProfile(generatedProfile).isValid()) { return false; }

        ShipGenerationRecipe recipe;
        recipe.Seeds = deriveShipGenerationSeeds(0x9200ABCDEF123456ull);
        recipe.Dimensions = { 96u, 64u };
        recipe.StructuralPreset.reset();
        recipe.StructuralProfile = structural;
        recipe.FactionPreset.reset();
        recipe.FactionProfile = faction;
        recipe.PaletteConfiguration.Mode = ShipPaletteSourceMode::EXPLICIT_GENERATED;
        recipe.PaletteConfiguration.Generated = generatedProfile;
        recipe.DetailDensity = 59u;
        recipe.AsymmetricDetailChance = 17u;

        const GeneratedShip first = generator.generate(recipe);
        const GeneratedShip repeated = generator.generate(recipe);
        if (!imagesEqual(first.FinalImage, repeated.FinalImage) || first.Provenance.StructuralPreset.has_value() || first.Provenance.FactionPreset.has_value()) { return false; }

        ShipGenerationRecipe changedConfiguration = recipe;
        changedConfiguration.PaletteConfiguration.Generated.Ranges.HullHue = { 18u, 42u };
        changedConfiguration.PaletteConfiguration.Generated.Ranges.Accent.HueOffset = { 150, 190 };
        const GeneratedShip changedColors = generator.generate(changedConfiguration);
        if (!geometryEqual(first, changedColors) || imagesEqual(first.FinalImage, changedColors.FinalImage)) { return false; }

        ShipGenerationRecipe paletteRerolled = recipe;
        paletteRerolled.DomainSeedOverrides.set(GenerationDomain::PALETTE, 0x9200111122223333ull);
        const GeneratedShip recolored = generator.generate(paletteRerolled);
        if (!geometryEqual(first, recolored) || imagesEqual(first.FinalImage, recolored.FinalImage) || palettesEqual(first.Palette, recolored.Palette)) { return false; }

        ShipGenerationRecipeDocument document;
        document.Recipe = recipe;
        const ShipGenerationRecipeLoadResult loaded = deserializeShipGenerationRecipe(serializeShipGenerationRecipe(document));
        if (!loaded.Success) { return false; }
        const GeneratedShip roundTripped = generator.generate(loaded.Document.Recipe);
        if (!imagesEqual(first.FinalImage, roundTripped.FinalImage) || !geometryEqual(first, roundTripped)) { return false; }

        ShipGenerationRecipe fixed = recipe;
        fixed.PaletteConfiguration.Mode = ShipPaletteSourceMode::FIXED;
        fixed.PaletteConfiguration.Fixed = makeFixedPalette(41u);
        fixed.DomainSeedOverrides.set(GenerationDomain::PALETTE, 0x9200444455556666ull);
        const GeneratedShip fixedFirst = generator.generate(fixed);
        if (!geometryEqual(first, fixedFirst)) { return false; }
        ShipGenerationRecipe fixedRerolled = fixed;
        fixedRerolled.DomainSeedOverrides.set(GenerationDomain::PALETTE, 0x9200777788889999ull);
        const GeneratedShip fixedSecond = generator.generate(fixedRerolled);
        if (!palettesEqual(fixedFirst.Palette, fixed.PaletteConfiguration.Fixed) || !imagesEqual(fixedFirst.FinalImage, fixedSecond.FinalImage) || !geometryEqual(fixedFirst, fixedSecond) ||
            fixedFirst.Provenance.StructuralPreset.has_value() || fixedFirst.Provenance.FactionPreset.has_value()) {
            return false;
        }

        ShipGenerationRecipeDocument fixedDocument;
        fixedDocument.Recipe = fixed;
        const ShipGenerationRecipeLoadResult fixedLoaded = deserializeShipGenerationRecipe(serializeShipGenerationRecipe(fixedDocument));
        if (!fixedLoaded.Success || fixedLoaded.Document.Recipe.PaletteConfiguration.Mode != ShipPaletteSourceMode::FIXED || !palettesEqual(fixedLoaded.Document.Recipe.PaletteConfiguration.Fixed, fixed.PaletteConfiguration.Fixed)) { return false; }
        const GeneratedShip fixedRoundTripped = generator.generate(fixedLoaded.Document.Recipe);
        return imagesEqual(fixedFirst.FinalImage, fixedRoundTripped.FinalImage) && geometryEqual(fixedFirst, fixedRoundTripped);
    }
}

namespace SpectralShipGenStudioTests
{
    int runPreviewPaletteEditorRegression()
    {
        if (!checkCommandMetadataAlignment()) { std::cerr << "Task 92 regression failed: PreviewCommand metadata alignment.\n"; return 1; }
        if (!checkColorControl()) { std::cerr << "Task 92 regression failed: color control math/swatch state.\n"; return 1; }
        if (!checkBuiltInPaletteRoundTrip()) { std::cerr << "Task 92 regression failed: built-in palette editor round-trip/immutability.\n"; return 1; }
        if (!checkEditorAndValidation()) { std::cerr << "Task 92 regression failed: palette editor binding/validation/immutability.\n"; return 1; }
        if (!checkWorkspaceAndSelection()) { std::cerr << "Task 92 regression failed: runtime palette workspace/selection.\n"; return 1; }
        if (!checkPaletteGenerationSemantics()) { std::cerr << "Task 92 regression failed: generated/fixed palette generation semantics.\n"; return 1; }
        return 0;
    }
}
