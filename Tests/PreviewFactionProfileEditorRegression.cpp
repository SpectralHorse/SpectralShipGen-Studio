#include "PreviewRegressionSuites.h"

#include <cstdint>
#include <iostream>
#include <string>

#include "FactionProfileSelection.h"
#include "PreviewConfigurationEditor.h"
#include "RuntimeCustomPresetWorkspace.h"

#include <PixelShipGenerator/ShipFactionProfile.h>
#include <PixelShipGenerator/ShipFactionProfileValidation.h>
#include <PixelShipGenerator/ShipFiringAnimator.h>
#include <PixelShipGenerator/ShipGenerationProfile.h>
#include <PixelShipGenerator/ShipGenerationSettings.h>
#include <PixelShipGenerator/ShipGenerator.h>
#include <PixelShipGenerator/ShipIdleAnimator.h>
#include <PixelShipGenerator/ShipLateralMovementAnimator.h>
#include <PixelShipGenerator/ShipLongitudinalMovementAnimator.h>

namespace PixelShipGeneratorTests
{
    namespace
    {
        int fail(const std::string& message)
        {
            std::cerr << "Preview faction profile editor regression failed: " << message << '\n';
            return 1;
        }

        bool imagesEqual(const PixelShipGenerator::Image& first, const PixelShipGenerator::Image& second)
        {
            return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
        }

        bool hasValidationErrorPrefix(const PixelShipGenerator::ValidationResult& result, const std::string& fieldPrefix)
        {
            for (const PixelShipGenerator::ValidationIssue& issue : result.Errors)
            {
                if (issue.Field.rfind(fieldPrefix, 0u) == 0u) { return true; }
            }
            return false;
        }

        bool deterministicAnimationEqual(const PixelShipGenerator::GeneratedShip& first, const PixelShipGenerator::GeneratedShip& second)
        {
            using namespace PixelShipGenerator;
            ShipIdleAnimator idle;
            ShipLateralMovementAnimator lateral;
            ShipLongitudinalMovementAnimator longitudinal;
            ShipFiringAnimator firing;

            if (!imagesEqual(idle.evaluateFrameAtNormalizedTime(first, 0.37), idle.evaluateFrameAtNormalizedTime(second, 0.37))) { return false; }
            if (!imagesEqual(lateral.evaluateFrameAtNormalizedTime(first, ShipAnimationType::MOVE_LEFT, ShipMovementAnimationPhase::SUSTAIN, 0.43), lateral.evaluateFrameAtNormalizedTime(second, ShipAnimationType::MOVE_LEFT, ShipMovementAnimationPhase::SUSTAIN, 0.43))) { return false; }
            if (!imagesEqual(longitudinal.evaluateFrameAtNormalizedTime(first, ShipAnimationType::MOVE_UP, ShipMovementAnimationPhase::ENTER, 0.39), longitudinal.evaluateFrameAtNormalizedTime(second, ShipAnimationType::MOVE_UP, ShipMovementAnimationPhase::ENTER, 0.39))) { return false; }

            const auto firstTargets = firing.getAvailableTargets(first);
            const auto secondTargets = firing.getAvailableTargets(second);
            if (firstTargets.empty() || firstTargets.size() != secondTargets.size()) { return false; }
            if (!imagesEqual(firing.evaluateFrameAtNormalizedTime(first, firstTargets.front(), 0.31), firing.evaluateFrameAtNormalizedTime(second, secondTargets.front(), 0.31))) { return false; }
            return true;
        }
    }

    int runPreviewFactionProfileEditorRegression()
    {
        using namespace PixelShipGeneratorPreview;
        using namespace PixelShipGenerator;

        RuntimeCustomPresetWorkspace workspace;
        const ShipFactionProfile relicBefore = getShipFactionProfile(ShipFactionType::RELIC);
        const ShipFactionProfile militaryBefore = getShipFactionProfile(ShipFactionType::MILITARY);

        ShipFactionProfileEditorBindings bindingRoundTrip;
        for (const ShipFactionType faction : { ShipFactionType::FRONTIER, ShipFactionType::MILITARY, ShipFactionType::ASCENDANT, ShipFactionType::XENO, ShipFactionType::CORPORATE, ShipFactionType::RELIC })
        {
            const ShipFactionProfile& builtIn = getShipFactionProfile(faction);
            ShipFactionProfile roundTrip = builtIn;
            bindingRoundTrip.load(builtIn);
            bindingRoundTrip.write(roundTrip);
            if (!bindingRoundTrip.equivalent(builtIn, roundTrip)) { return fail("editor controls altered a built-in faction profile during load/write round-trip"); }
        }

        PreviewConfigurationEditor editor;
        editor.setPanelBounds({ 880.0f, 0.0f, 760.0f, 1000.0f });
        editor.openFactionProfile("RELIC Copy", relicBefore);
        if (!editor.isOpen() || editor.getProfileKind() != ConfigurationEditorProfileKind::FACTION) { return fail("faction editor did not open in faction mode"); }
        if (!editor.getValidationResult().isValid()) { return fail("valid built-in faction draft reported validation errors"); }
        if (editor.getFactionProfileSections().size() != 16u || editor.getBoundValueCount() != 241u) { return fail("unified faction profile was not mapped into the expected semantic editor surface"); }
        if (editor.findFactionRangeField("Palette.HullHue") == nullptr ||
            editor.findFactionIntegerField("SurfaceDetails.DetailDensityPercent") == nullptr ||
            editor.findFactionWeightGroup("Weapons.WeightMultipliersPercent") == nullptr ||
            editor.findFactionWeightGroup("Materials.ZoneWeightMultipliersPercent") == nullptr ||
            editor.findFactionChoiceField("Finish.WeaponMuzzleRole") == nullptr ||
            editor.findFactionIntegerField("Animation.Firing.DurationScale.Numerator") == nullptr ||
            editor.findFactionChoiceField("Animation.Idle.SynchronizeEngines") == nullptr)
        {
            return fail("representative palette/surface/weapon/material/finish/animation bindings are missing");
        }
        if (editor.getMaximumScrollOffset() <= 0.0f) { return fail("large faction form did not expose scroll state"); }

        const auto expandOnly = [&](std::size_t sectionIndex)
        {
            for (std::size_t index = 0u; index < editor.getFactionProfileSections().size(); ++index) { editor.setSectionExpanded(index, index == sectionIndex); }
            editor.onMouseWheelScrolled(1000.0f);
        };

        expandOnly(1u); // Surface details.
        FactionIntegerFieldBinding* detailDensity = editor.findFactionIntegerField("SurfaceDetails.DetailDensityPercent");
        editor.onMouseRelease(detailDensity->Control.IncrementBounds.Left + 2.0f, detailDensity->Control.IncrementBounds.Top + 2.0f);
        if (editor.getDraftFactionProfile().SurfaceDetails.DetailDensityPercent == relicBefore.SurfaceDetails.DetailDensityPercent) { return fail("surface multiplier edit did not update the real faction draft"); }

        expandOnly(4u); // Weapons.
        FactionWeightGroupBinding* weaponWeights = editor.findFactionWeightGroup("Weapons.WeightMultipliersPercent");
        auto& weaponRows = weaponWeights->Control.getRows();
        editor.onMouseRelease(weaponRows[3u].Control.IncrementBounds.Left + 2.0f, weaponRows[3u].Control.IncrementBounds.Top + 2.0f);
        if (editor.getDraftFactionProfile().Weapons.WeightMultipliersPercent.RailWeapon == relicBefore.Weapons.WeightMultipliersPercent.RailWeapon || weaponRows[3u].ProbabilityPercent > 100u) { return fail("weapon relative-weight multiplier edit did not update raw value/read-only share"); }

        expandOnly(9u); // Materials.
        FactionWeightGroupBinding* materialWeights = editor.findFactionWeightGroup("Materials.ZoneWeightMultipliersPercent");
        auto& materialRows = materialWeights->Control.getRows();
        editor.onMouseRelease(materialRows[5u].Control.IncrementBounds.Left + 2.0f, materialRows[5u].Control.IncrementBounds.Top + 2.0f);
        if (editor.getDraftFactionProfile().Materials.ZoneWeightMultipliersPercent.HardpointSurround == relicBefore.Materials.ZoneWeightMultipliersPercent.HardpointSurround) { return fail("material multiplier edit did not update public faction profile"); }

        expandOnly(15u); // Firing animation.
        FactionIntegerFieldBinding* durationNumerator = editor.findFactionIntegerField("Animation.Firing.DurationScale.Numerator");
        editor.onMouseRelease(durationNumerator->Control.IncrementBounds.Left + 2.0f, durationNumerator->Control.IncrementBounds.Top + 2.0f);
        if (editor.getDraftFactionProfile().Animation.Firing.DurationScale.Numerator == relicBefore.Animation.Firing.DurationScale.Numerator) { return fail("animation-affecting faction value did not update public faction profile"); }

        if (getShipFactionProfile(ShipFactionType::RELIC).Animation.Firing.DurationScale.Numerator != relicBefore.Animation.Firing.DurationScale.Numerator ||
            getShipFactionProfile(ShipFactionType::RELIC).Weapons.WeightMultipliersPercent.RailWeapon != relicBefore.Weapons.WeightMultipliersPercent.RailWeapon)
        {
            return fail("canonical built-in RELIC faction was mutated by editor working-copy changes");
        }

        ShipFactionProfile invalidFaction = relicBefore;
        invalidFaction.Animation.Firing.DurationScale.Denominator = 0u;
        editor.openFactionProfile("Invalid Faction", invalidFaction);
        if (editor.getValidationResult().isValid() || !hasValidationErrorPrefix(editor.getValidationResult(), "Animation.Firing.DurationScale") || editor.getActionButtons()[0u].Enabled)
        {
            return fail("Core faction validation was not surfaced/used to block Apply");
        }

        PreviewConfigurationEditor defaultEditor;
        defaultEditor.setPanelBounds({ 880.0f, 0.0f, 760.0f, 1000.0f });
        defaultEditor.openFactionProfile("Custom Faction", ShipFactionProfile{});
        if (!defaultEditor.getValidationResult().isValid()) { return fail("default-constructed public faction profile is not a valid add-faction starting point"); }

        const RuntimeCustomPresetId defaultId = workspace.addFaction(defaultEditor.getName(), defaultEditor.getDraftFactionProfile());
        const RuntimeCustomPresetId frontierId = workspace.addFaction("FRONTIER Copy", getShipFactionProfile(ShipFactionType::FRONTIER));
        const RuntimeCustomPresetId militaryId = workspace.addFaction("MILITARY Copy", militaryBefore);
        const RuntimeCustomPresetId ascendantId = workspace.addFaction("ASCENDANT Copy", getShipFactionProfile(ShipFactionType::ASCENDANT));
        const RuntimeCustomPresetId xenoId = workspace.addFaction("XENO Copy", getShipFactionProfile(ShipFactionType::XENO));
        const RuntimeCustomPresetId corporateId = workspace.addFaction("CORPORATE Copy", getShipFactionProfile(ShipFactionType::CORPORATE));
        const RuntimeCustomPresetId relicId = workspace.addFaction("RELIC Copy", relicBefore);
        if (workspace.getFactionPresets().size() != 7u || !workspace.duplicateFaction(relicId).has_value()) { return fail("default/built-in faction duplicate workflows failed"); }

        ShipFactionProfile updatedMilitary = workspace.findFaction(militaryId)->Profile;
        updatedMilitary.Weapons.ChancePercent = 135u;
        if (!workspace.updateFaction(militaryId, "MILITARY Arsenal", updatedMilitary) || workspace.findFaction(militaryId)->Name != "MILITARY Arsenal") { return fail("existing runtime faction could not be updated in place"); }
        if (getShipFactionProfile(ShipFactionType::MILITARY).Weapons.ChancePercent != militaryBefore.Weapons.ChancePercent) { return fail("canonical built-in MILITARY faction changed during runtime editing"); }

        const auto entries = buildFactionProfileSelection(workspace);
        if (entries.size() != 15u || entries.front().Kind != FactionProfileSelectionKind::BUILT_IN || entries[6u].Kind != FactionProfileSelectionKind::RUNTIME_CUSTOM || entries.back().Kind != FactionProfileSelectionKind::ADD_FACTION || entries.back().Label != "+ ADD FACTION")
        {
            return fail("built-in/custom/+ADD faction selector ordering is incorrect");
        }
        ShipGenerationRecipe selectedRecipe;
        selectedRecipe.FactionSource = ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        selectedRecipe.Faction = ShipFactionType::SHIP_FACTION_TYPE_END;
        selectedRecipe.FactionProfile = workspace.findFaction(xenoId)->Profile;
        const std::size_t selectedIndex = findFactionProfileSelectionIndex(entries, selectedRecipe, xenoId);
        if (entries[selectedIndex].CustomPresetId != xenoId) { return fail("runtime custom faction selection identity was not retained"); }

        ShipGenerationProfile structural = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
        structural.LargeWeaponChance = 100u;
        structural.MaximumLargeWeaponGroups = 2u;
        structural.LargeWeaponScalePercent = 140u;

        ShipFactionProfile customFaction = relicBefore;
        customFaction.SurfaceDetails.DetailDensityPercent = 145u;
        customFaction.Weapons.ChancePercent = 160u;
        customFaction.Weapons.WeightMultipliersPercent.RailWeapon = 450u;
        customFaction.Materials.ZoneWeightMultipliersPercent.HardpointSurround = 325u;
        customFaction.Finish.WeaponMuzzleRole = ShipFactionPaintColorRole::LIGHT_HIGHLIGHT;
        customFaction.Animation.Idle.TechPulseStrength = 7u;
        customFaction.Animation.Idle.AlternateTechCorePhases = true;
        customFaction.Animation.LateralMovement.ResponseStrengthScale = { 3u, 2u };
        customFaction.Animation.Firing.DurationScale = { 5u, 4u };
        customFaction.Animation.Firing.HeavyResponse = ShipFactionAnimationBooleanOverride::ENABLE;
        if (!validateShipFactionProfile(customFaction).isValid()) { return fail("representative custom faction unexpectedly failed Core validation"); }

        ShipGenerationSettings sourceSettings;
        sourceSettings.Seed = 0x9100C00100000001ull;
        sourceSettings.Dimensions = { 96u, 64u };
        sourceSettings.Style = ShipStyle::INDUSTRIAL;
        sourceSettings.Faction = ShipFactionType::RELIC;
        sourceSettings.DetailDensity = 63u;
        sourceSettings.AsymmetricDetailChance = 19u;
        sourceSettings.AttachmentsEnabled = true;
        ShipGenerationRecipe customRecipe = makeShipGenerationRecipe(sourceSettings);
        customRecipe.StructuralSource = ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        customRecipe.Style = ShipStyle::SHIP_STYLE_END;
        customRecipe.StructuralProfile = structural;
        const ShipGenerationRecipe builtInFactionRecipe = customRecipe;
        customRecipe.FactionSource = ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        customRecipe.Faction = ShipFactionType::SHIP_FACTION_TYPE_END;
        customRecipe.FactionProfile = customFaction;

        ShipGenerator generator;
        const GeneratedShip builtInFactionShip = generator.generate(builtInFactionRecipe);
        const GeneratedShip first = generator.generate(customRecipe);
        const GeneratedShip second = generator.generate(customRecipe);
        if (!imagesEqual(first.FinalImage, second.FinalImage)) { return fail("custom structural + custom faction generation is not deterministic"); }
        if (imagesEqual(first.FinalImage, builtInFactionShip.FinalImage)) { return fail("edited custom faction did not affect generated static output for the deterministic review fixture"); }
        if (first.Style != ShipStyle::SHIP_STYLE_END || first.Faction != ShipFactionType::SHIP_FACTION_TYPE_END) { return fail("custom composition pretended to have built-in style/faction identity"); }
        if (first.FactionAnimationProfile.Idle.TechPulseStrength != customFaction.Animation.Idle.TechPulseStrength ||
            first.FactionAnimationProfile.LateralMovement.ResponseStrengthScale.Numerator != customFaction.Animation.LateralMovement.ResponseStrengthScale.Numerator ||
            first.FactionAnimationProfile.Firing.DurationScale.Numerator != customFaction.Animation.Firing.DurationScale.Numerator ||
            first.FactionAnimationProfile.Firing.HeavyResponse != customFaction.Animation.Firing.HeavyResponse)
        {
            return fail("custom faction animation semantics were not carried into GeneratedShip without built-in identity");
        }
        if (!deterministicAnimationEqual(first, second)) { return fail("custom faction idle/movement/firing animation was not deterministic/profile-driven"); }

        (void)defaultId;
        (void)frontierId;
        (void)ascendantId;
        (void)corporateId;
        std::cout << "Preview faction profile editor regression passed.\n";
        return 0;
    }
}
