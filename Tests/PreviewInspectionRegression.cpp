#include "PreviewRegressionSuites.h"

#include <iostream>
#include <string>

#include "PreviewInspection.h"
#include "PreviewWorkspace.h"
#include <SpectralShipGen/ShipGenerationRecipe.h>
#include <SpectralShipGen/ShipGenerationSettings.h>
#include <SpectralShipGen/ShipGenerator.h>

namespace SpectralShipGenStudioTests
{
    namespace
    {
        int fail(const std::string& message)
        {
            std::cerr << "Preview inspection regression failed: " << message << '\n';
            return 1;
        }

        void setMaskPixel(SpectralShipGen::PixelMask& mask, uint32_t x, uint32_t y)
        {
            mask.set(x, y, true);
        }

        SpectralShipGen::GeneratedShip makeSyntheticShip()
        {
            SpectralShipGen::GeneratedShip ship;
            const SpectralShipGen::ShipGenerationSeeds seeds = SpectralShipGen::deriveShipGenerationSeeds(101u);
            ship.reset(8u, 8u, seeds);
            ship.FinalImage.reset(8u, 8u, SpectralShipGen::Color(20u, 30u, 40u, 255u));
            setMaskPixel(ship.HullMask, 1u, 1u);
            setMaskPixel(ship.CockpitMask, 2u, 1u);
            setMaskPixel(ship.EngineMask, 3u, 1u);
            setMaskPixel(ship.EngineExhaustMask, 4u, 1u);
            setMaskPixel(ship.AttachmentMask, 5u, 1u);
            SpectralShipGen::ShipAttachmentPlacement attachment;
            attachment.AnchorX = 6u;
            attachment.AnchorY = 2u;
            attachment.MinimumX = 5u;
            attachment.MaximumX = 7u;
            attachment.MinimumY = 1u;
            attachment.MaximumY = 3u;
            ship.AttachmentPlacements.push_back(attachment);
            setMaskPixel(ship.AccentMask, 1u, 2u);
            setMaskPixel(ship.MechanicalDetailMask, 2u, 2u);
            setMaskPixel(ship.LightMask, 3u, 2u);
            return ship;
        }

        SpectralShipGen::ShipGenerationDebugInfo makeSyntheticDebug()
        {
            SpectralShipGen::ShipGenerationDebugInfo debug;
            const auto reset = [](SpectralShipGen::PixelMask& mask) { mask.reset(8u, 8u); };
            reset(debug.HullLayerMask);
            reset(debug.HullLayerUpperMask);
            reset(debug.CoreRegionMask);
            reset(debug.CoreSecondaryMaterialMask);
            reset(debug.CoreRaisedMask);
            reset(debug.CoreRecessedMask);
            reset(debug.CoreLuminousMask);
            reset(debug.WeaponOccupiedMask);
            reset(debug.MaterialSecondaryHullMask);
            reset(debug.MaterialMechanicalMask);
            reset(debug.LiveryPrimaryMask);
            reset(debug.LiverySecondaryMask);
            reset(debug.PrimaryDetailMotifMask);
            reset(debug.SecondaryDetailMotifMask);
            reset(debug.MacroAsymmetryMask);
            reset(debug.ReservedNegativeSpaceMask);

            setMaskPixel(debug.HullLayerMask, 1u, 3u);
            setMaskPixel(debug.HullLayerUpperMask, 2u, 3u);
            setMaskPixel(debug.CoreRegionMask, 3u, 3u);
            setMaskPixel(debug.CoreRaisedMask, 4u, 3u);
            setMaskPixel(debug.WeaponOccupiedMask, 2u, 4u);
            setMaskPixel(debug.MaterialSecondaryHullMask, 3u, 4u);
            setMaskPixel(debug.MaterialMechanicalMask, 4u, 4u);
            setMaskPixel(debug.LiveryPrimaryMask, 5u, 4u);
            setMaskPixel(debug.LiverySecondaryMask, 6u, 4u);
            setMaskPixel(debug.PrimaryDetailMotifMask, 1u, 5u);
            setMaskPixel(debug.SecondaryDetailMotifMask, 2u, 5u);
            setMaskPixel(debug.MacroAsymmetryMask, 3u, 5u);
            setMaskPixel(debug.ReservedNegativeSpaceMask, 4u, 5u);

            SpectralShipGen::WeaponUnitDebugInfo weapon;
            weapon.AnchorX = 1u;
            weapon.AnchorY = 6u;
            weapon.BodyMinX = 1u;
            weapon.BodyMaxX = 3u;
            weapon.BodyMinY = 4u;
            weapon.BodyMaxY = 6u;
            weapon.BarrelMinX = 3u;
            weapon.BarrelMaxX = 5u;
            weapon.BarrelMinY = 5u;
            weapon.BarrelMaxY = 5u;
            weapon.MuzzleX = 5u;
            weapon.MuzzleY = 5u;
            debug.WeaponUnits.push_back(weapon);

            debug.SpatialRegionMapWidth = 8u;
            debug.SpatialRegionMapHeight = 8u;
            debug.SpatialRegionMap.assign(64u, static_cast<uint8_t>(SpectralShipGen::GenerationSpatialRegion::MID_FUSELAGE));
            const std::size_t midFuselage = static_cast<std::size_t>(SpectralShipGen::GenerationSpatialRegion::MID_FUSELAGE);
            debug.SpatialRegionCapacities[midFuselage] = 100u;
            debug.SpatialRegionLoads[midFuselage] = 80u;

            SpectralShipGen::PixelMask stage(8u, 8u);
            stage.set(7u, 7u);
            debug.HullStages.emplace_back(SpectralShipGen::ShipGenerationDebugStageType::BASE_HULL, stage);
            return debug;
        }
    }

    int runPreviewInspectionRegression()
    {
        using namespace SpectralShipGenStudioPreview;

        if (getWrappedPreviewInspectionGroup(PreviewInspectionGroup::STRUCTURE, -1) != PreviewInspectionGroup::CONSTRAINTS ||
            getWrappedPreviewInspectionGroup(PreviewInspectionGroup::CONSTRAINTS, 1) != PreviewInspectionGroup::STRUCTURE)
        {
            return fail("inspection group selector does not wrap");
        }
        if (getWrappedDiagnosticView(PreviewInspectionGroup::STRUCTURE, DiagnosticViewMode::HULL, -1) != DiagnosticViewMode::COMBINED ||
            getWrappedDiagnosticView(PreviewInspectionGroup::COMPOSITION, DiagnosticViewMode::MACRO_ASYMMETRY, 1) != DiagnosticViewMode::DETAILS ||
            getWrappedDiagnosticView(PreviewInspectionGroup::CONSTRAINTS, DiagnosticViewMode::NEGATIVE_SPACE, -1) != DiagnosticViewMode::SEMANTIC_LOAD)
        {
            return fail("semantic view selection does not wrap within its group");
        }
        if (getDiagnosticViewGroup(DiagnosticViewMode::WEAPONS) != PreviewInspectionGroup::STRUCTURE ||
            getDiagnosticViewGroup(DiagnosticViewMode::LIVERY) != PreviewInspectionGroup::COMPOSITION ||
            getDiagnosticViewGroup(DiagnosticViewMode::NEGATIVE_SPACE) != PreviewInspectionGroup::CONSTRAINTS)
        {
            return fail("semantic views are assigned to the wrong inspection group");
        }

        SpectralShipGen::GeneratedShip emptyShip;
        if (hasPreviewInspectionShip(emptyShip))
        {
            return fail("empty generated state is incorrectly inspectable");
        }

        const SpectralShipGen::GeneratedShip syntheticShip = makeSyntheticShip();
        const SpectralShipGen::ShipGenerationDebugInfo syntheticDebug = makeSyntheticDebug();
        if (!hasPreviewInspectionShip(syntheticShip))
        {
            return fail("valid generated ship is not recognized by the inspection model");
        }
        const SpectralShipGen::Image overlay = createPreviewInspectionImage(syntheticShip, syntheticDebug, DiagnosticViewMode::HULL, PreviewInspectionPresentation::OVERLAY);
        const SpectralShipGen::Image isolate = createPreviewInspectionImage(syntheticShip, syntheticDebug, DiagnosticViewMode::HULL, PreviewInspectionPresentation::ISOLATE);
        if (overlay.getWidth() != 8u || overlay.getHeight() != 8u || isolate.getWidth() != 8u || isolate.getHeight() != 8u)
        {
            return fail("inspection image changed native dimensions");
        }
        if (overlay.getPixel(0u, 0u) != syntheticShip.FinalImage.getPixel(0u, 0u) || overlay.getPixel(1u, 1u) == syntheticShip.FinalImage.getPixel(1u, 1u))
        {
            return fail("overlay presentation does not preserve normal pixels while highlighting selected semantics");
        }
        if (isolate.getPixel(0u, 0u).A != 0u || isolate.getPixel(1u, 1u) != PreviewDiagnosticColors::Hull)
        {
            return fail("isolate presentation does not isolate the selected semantic mask");
        }

        const SpectralShipGen::Image attachments = createPreviewInspectionImage(syntheticShip, syntheticDebug, DiagnosticViewMode::ATTACHMENTS, PreviewInspectionPresentation::ISOLATE);
        if (attachments.getPixel(6u, 2u) != PreviewDiagnosticColors::AttachmentRoot || attachments.getPixel(7u, 3u) != PreviewDiagnosticColors::AttachmentBounds)
        {
            return fail("attachment roots/bounds are not visible in the inspection image");
        }

        const SpectralShipGen::Image material = createPreviewInspectionImage(syntheticShip, syntheticDebug, DiagnosticViewMode::MATERIALS, PreviewInspectionPresentation::ISOLATE);
        const SpectralShipGen::Image livery = createPreviewInspectionImage(syntheticShip, syntheticDebug, DiagnosticViewMode::LIVERY, PreviewInspectionPresentation::ISOLATE);
        const SpectralShipGen::Image negativeSpace = createPreviewInspectionImage(syntheticShip, syntheticDebug, DiagnosticViewMode::NEGATIVE_SPACE, PreviewInspectionPresentation::ISOLATE);
        const SpectralShipGen::Image weapons = createPreviewInspectionImage(syntheticShip, syntheticDebug, DiagnosticViewMode::WEAPONS, PreviewInspectionPresentation::ISOLATE);
        if (material.getPixel(3u, 4u) != PreviewDiagnosticColors::MaterialSecondary || material.getPixel(4u, 4u) != PreviewDiagnosticColors::MaterialMechanical)
        {
            return fail("material semantic masks are not routed to inspection");
        }
        if (livery.getPixel(5u, 4u) != PreviewDiagnosticColors::LiveryPrimary || livery.getPixel(6u, 4u) != PreviewDiagnosticColors::LiverySecondary)
        {
            return fail("livery semantic masks are not routed to inspection");
        }
        if (negativeSpace.getPixel(4u, 5u) != PreviewDiagnosticColors::NegativeSpace)
        {
            return fail("reserved negative-space mask is not routed to inspection");
        }
        if (weapons.getPixel(1u, 6u) != PreviewDiagnosticColors::WeaponRoot || weapons.getPixel(5u, 5u) != PreviewDiagnosticColors::WeaponMuzzle)
        {
            return fail("weapon root/muzzle geometry is not visible in the inspection image");
        }

        const SpectralShipGen::Image stage = createPreviewInspectionImage(syntheticShip, syntheticDebug, DiagnosticViewMode::HULL, PreviewInspectionPresentation::ISOLATE, true, 0u);
        if (stage.getPixel(7u, 7u) != PreviewDiagnosticColors::Hull || stage.getPixel(1u, 1u).A != 0u)
        {
            return fail("captured hull-generation stage routing is incorrect");
        }

        SpectralShipGen::ShipGenerationSettings settings;
        settings.Seed = 0x97A5B3C1u;
        settings.Dimensions = { 64u, 64u };
        settings.Style = SpectralShipGen::ShipStyle::INDUSTRIAL;
        settings.Faction = SpectralShipGen::ShipFactionType::CORPORATE;
        settings.DomainSeedOverrides.set(SpectralShipGen::GenerationDomain::WEAPONS, 0x123456789ABCDEF0ull);
        const SpectralShipGen::ShipGenerationRecipe recipe = SpectralShipGen::makeShipGenerationRecipe(settings);
        const SpectralShipGen::ShipGenerationRecipe recipeBeforeInspection = recipe;
        SpectralShipGen::ShipGenerationDebugInfo debug;
        SpectralShipGen::ShipGenerator generator;
        const SpectralShipGen::GeneratedShip ship = generator.generate(recipe, &debug);
        if (ship.Seed != recipe.Seeds.Master || ship.FinalImage.getWidth() != recipe.Dimensions.Width || ship.FinalImage.getHeight() != recipe.Dimensions.Height)
        {
            return fail("current generated ship/recipe routing is inconsistent");
        }
        if (ship.DomainSeeds.get(SpectralShipGen::GenerationDomain::WEAPONS) != *recipe.DomainSeedOverrides.get(SpectralShipGen::GenerationDomain::WEAPONS))
        {
            return fail("effective domain seed/override information is not retained on the current ship");
        }
        if (!ship.Provenance.StructuralPreset.has_value() || !ship.Provenance.FactionPreset.has_value())
        {
            return fail("built-in provenance was not retained for truthful inspection summary");
        }
        if (debug.PrimaryVisualAnchor == SpectralShipGen::ShipVisualAnchorType::SHIP_VISUAL_ANCHOR_TYPE_END || debug.ComplexityInitialBudget == 0u)
        {
            return fail("one-ship hierarchy/complexity debug data is not retained for inspection");
        }

        const struct RepresentativeInspectionCase
        {
            uint32_t Width;
            uint32_t Height;
            SpectralShipGen::ShipStyle Style;
        } representativeCases[] = {
            { 32u, 32u, SpectralShipGen::ShipStyle::SPEARHEAD },
            { 44u, 44u, SpectralShipGen::ShipStyle::DELTA },
            { 64u, 64u, SpectralShipGen::ShipStyle::INDUSTRIAL },
            { 96u, 96u, SpectralShipGen::ShipStyle::SPEARHEAD },
            { 128u, 128u, SpectralShipGen::ShipStyle::DELTA }
        };
        uint64_t representativeSeed = 0x9700000000000000ull;
        for (const RepresentativeInspectionCase& representative : representativeCases)
        {
            SpectralShipGen::ShipGenerationSettings representativeSettings;
            representativeSettings.Seed = ++representativeSeed;
            representativeSettings.Dimensions = { representative.Width, representative.Height };
            representativeSettings.Style = representative.Style;
            representativeSettings.Faction = SpectralShipGen::ShipFactionType::MILITARY;
            const SpectralShipGen::ShipGenerationRecipe representativeRecipe = SpectralShipGen::makeShipGenerationRecipe(representativeSettings);
            SpectralShipGen::ShipGenerationDebugInfo representativeDebug;
            const SpectralShipGen::GeneratedShip representativeShip = generator.generate(representativeRecipe, &representativeDebug);
            const SpectralShipGen::Image representativeOverlay = createPreviewInspectionImage(representativeShip, representativeDebug, DiagnosticViewMode::COMBINED, PreviewInspectionPresentation::OVERLAY);
            const SpectralShipGen::Image representativeIsolate = createPreviewInspectionImage(representativeShip, representativeDebug, DiagnosticViewMode::SEMANTIC_LOAD, PreviewInspectionPresentation::ISOLATE);
            if (representativeOverlay.getWidth() != representative.Width || representativeOverlay.getHeight() != representative.Height ||
                representativeIsolate.getWidth() != representative.Width || representativeIsolate.getHeight() != representative.Height)
            {
                return fail("representative inspection views resampled native ship dimensions");
            }
        }

        SpectralShipGen::ShipGenerationRecipe customRecipe = recipe;
        customRecipe.StructuralPreset.reset();
        customRecipe.StructuralProfile = SpectralShipGen::getShipGenerationProfile(SpectralShipGen::ShipStyle::INDUSTRIAL);
        customRecipe.StructuralPreset.reset();
        customRecipe.FactionPreset.reset();
        customRecipe.FactionProfile = SpectralShipGen::getShipFactionProfile(SpectralShipGen::ShipFactionType::CORPORATE);
        customRecipe.FactionPreset.reset();
        SpectralShipGen::ShipGenerationDebugInfo customDebug;
        const SpectralShipGen::GeneratedShip customShip = generator.generate(customRecipe, &customDebug);
        if (customShip.Provenance.StructuralPreset.has_value() || customShip.Provenance.FactionPreset.has_value() ||
            createPreviewInspectionImage(customShip, customDebug, DiagnosticViewMode::MATERIALS, PreviewInspectionPresentation::OVERLAY).getWidth() != customRecipe.Dimensions.Width)
        {
            return fail("custom configuration inspection fabricated built-in provenance or changed native dimensions");
        }

        PreviewWorkspaceSession workspaceSession;
        const SpectralShipGen::ShipGenerationRecipe sharedRecipe = recipe;
        workspaceSession.switchTo(PreviewWorkspace::INSPECT, PreviewMode::STATIC);
        workspaceSession.switchTo(PreviewWorkspace::REROLL, PreviewMode::STATIC);
        workspaceSession.switchTo(PreviewWorkspace::ANIMATION, PreviewMode::REROLL_STUDIO);
        workspaceSession.switchTo(PreviewWorkspace::INSPECT, PreviewMode::FRAME_INSPECTION);
        if (sharedRecipe != recipe || recipe != recipeBeforeInspection)
        {
            return fail("inspection/cross-workspace state mutated the shared recipe");
        }

        std::cout << "Preview inspection workspace regression passed.\n";
        return 0;
    }
}
