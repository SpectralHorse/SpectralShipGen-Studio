#include "PreviewRegressionSuites.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>

#include "PreviewAnimationSession.h"
#include "PreviewGenerationRecipe.h"
#include "PreviewWorkspace.h"
#include <PixelShipGenerator/ShipFactionProfile.h>
#include <PixelShipGenerator/ShipFiringAnimator.h>
#include <PixelShipGenerator/ShipGenerationProfile.h>
#include <PixelShipGenerator/ShipGenerationSeeds.h>
#include <PixelShipGenerator/ShipGenerationSettings.h>
#include <PixelShipGenerator/ShipGenerator.h>
#include <PixelShipGenerator/ShipSpritesheetUtils.h>

namespace
{
    bool imagesEqual(const PixelShipGenerator::Image& first, const PixelShipGenerator::Image& second)
    {
        return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
    }

    PixelShipGenerator::GeneratedShip generateAnimatedShip()
    {
        PixelShipGenerator::ShipGenerator generator;
        PixelShipGenerator::ShipFiringAnimator firingAnimator;
        for (uint64_t seed = 1u; seed <= 256u; ++seed)
        {
            PixelShipGenerator::ShipGenerationSettings settings;
            settings.Seed = seed;
            settings.Dimensions = { 96u, 96u };
            settings.Style = PixelShipGenerator::ShipStyle::FIGHTER;
            settings.Faction = PixelShipGenerator::ShipFactionType::MILITARY;
            PixelShipGenerator::GeneratedShip ship = generator.generate(settings);
            if (!firingAnimator.getAvailableTargets(ship).empty()) { return ship; }
        }
        return {};
    }

    PixelShipGeneratorPreview::PreviewGenerationRecipe makeCustomRecipe()
    {
        using namespace PixelShipGenerator;
        PixelShipGeneratorPreview::PreviewGenerationRecipe recipe;
        recipe.Seeds = deriveShipGenerationSeeds(0x9900000000000001ull);
        recipe.Dimensions = { 96u, 64u };
        recipe.StructuralSource = ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        recipe.Style = ShipStyle::SHIP_STYLE_END;
        recipe.StructuralProfile = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
        recipe.StructuralProfile.LargeWeaponChance = 88u;
        recipe.FactionSource = ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        recipe.Faction = ShipFactionType::SHIP_FACTION_TYPE_END;
        recipe.FactionProfile = getShipFactionProfile(ShipFactionType::CORPORATE);
        recipe.FactionProfile.SurfaceDetails.DetailDensityPercent = 79u;
        recipe.PaletteConfiguration.Mode = ShipPaletteSourceMode::FIXED;
        recipe.PaletteConfiguration.Fixed.HullBase = Color(42u, 70u, 106u, 255u);
        recipe.PaletteConfiguration.Fixed.HullAccent = Color(214u, 90u, 145u, 255u);
        return recipe;
    }

    bool checkSampledTime(const PixelShipGeneratorPreview::PreviewAnimationSession& session)
    {
        const double time = session.getActiveNormalizedTime();
        return time >= 0.0 && time <= 1.0 && session.getFrameIndex() < session.getActiveFrames().size();
    }
}

int PixelShipGeneratorTests::runPreviewAnimationLabRegression()
{
    using namespace PixelShipGeneratorPreview;
    using namespace PixelShipGenerator;

    const GeneratedShip ship = generateAnimatedShip();
    if (ship.FinalImage.empty()) { std::cerr << "animation_lab_ship failed.\n"; return 1; }

    PreviewAnimationSession session;
    if (!session.resetForGeneratedShip(ship).Success || session.getActiveFrames().empty()) { return 1; }
    if (session.getSelectedAnimationType() != ShipAnimationType::IDLE || !imagesEqual(session.getActiveFrames().front(), ship.FinalImage)) { return 1; }
    if (session.getActiveSampling().ActualFrameCount != session.getActiveFrames().size() || !session.isActiveLooping()) { return 1; }
    if (session.getActiveNormalizedTime() != 0.0 || session.getSemanticPhaseDisplay() != "LOOP") { return 1; }

    if (!session.setNormalizedTime(0.5) || !checkSampledTime(session)) { return 1; }
    const uint32_t scrubbedIndex = session.getFrameIndex();
    const Image scrubbedFrame = session.getActiveFrames()[scrubbedIndex];
    if (!session.setNormalizedTime(0.5) || session.getFrameIndex() != scrubbedIndex || !imagesEqual(session.getActiveFrames()[session.getFrameIndex()], scrubbedFrame)) { return 1; }
    if (!session.setNormalizedTime(1.0) || session.getActiveNormalizedTime() >= 1.0) { return 1; }

    constexpr ShipAnimationType ExpectedTypes[] = { ShipAnimationType::MOVE_LEFT, ShipAnimationType::MOVE_RIGHT, ShipAnimationType::MOVE_UP, ShipAnimationType::MOVE_DOWN, ShipAnimationType::FIRE, ShipAnimationType::IDLE };
    for (ShipAnimationType expected : ExpectedTypes)
    {
        const PreviewAnimationActionResult result = session.cycleAnimationType(ship);
        if (!result.Success || session.getSelectedAnimationType() != expected) { return 1; }
    }

    session.cycleAnimationType(ship); // MOVE_LEFT
    if (session.getSemanticPhaseDisplay() != "ENTER") { return 1; }
    session.cycleMovementPhase(); // SUSTAIN
    if (session.getSemanticPhaseDisplay() != "SUSTAIN" || !session.isActiveLooping()) { return 1; }
    session.cycleMovementPhase(); // EXIT
    if (session.getSemanticPhaseDisplay() != "EXIT" || session.isActiveLooping()) { return 1; }

    session.cycleAnimationType(ship); // MOVE_RIGHT
    session.cycleAnimationType(ship); // MOVE_UP
    if (session.getSemanticPhaseDisplay().find("ACCEL") == std::string::npos) { return 1; }
    session.cycleMovementPhase(); // sustain
    session.cycleMovementPhase(); // exit
    if (session.getSemanticPhaseDisplay().find("BRAKE") == std::string::npos) { return 1; }

    session.returnToIdle(ship);
    session.advancePlayback(ship, 2000000.0);
    const auto originalFrames = session.getActiveFrames();
    const auto originalSampling = session.getActiveSampling();
    session.cyclePlaybackSpeed(); // 2x
    if (std::abs(session.getPlaybackSpeed() - 2.0) > 0.0001 || session.getActiveFrames().size() != originalFrames.size() || session.getActiveSampling().ActualFrameCount != originalSampling.ActualFrameCount) { return 1; }
    for (std::size_t i = 0u; i < originalFrames.size(); ++i) { if (!imagesEqual(originalFrames[i], session.getActiveFrames()[i])) { return 1; } }

    session.cycleBaseMovementState(ship); // MOVE_LEFT
    session.advancePlayback(ship, 2000000.0);
    if (session.getRuntimeMovementType() != ShipAnimationType::MOVE_LEFT || session.getMovementPhase() != ShipMovementAnimationPhase::SUSTAIN) { return 1; }
    const PreviewAnimationActionResult fire = session.triggerFiringEvent(ship);
    if (!fire.Success || !session.isTransientStatePreviewActive() || session.getSelectedAnimationType() != ShipAnimationType::FIRE || session.isActiveLooping()) { return 1; }
    if (session.getSemanticPhaseDisplay().empty()) { return 1; }
    session.advancePlayback(ship, 4000000.0);
    if (session.isTransientStatePreviewActive() || session.getRuntimeMovementType() != ShipAnimationType::MOVE_LEFT || session.getSelectedAnimationType() != ShipAnimationType::MOVE_LEFT || session.getMovementPhase() != ShipMovementAnimationPhase::SUSTAIN) { return 1; }

    GeneratedShip weaponless = ship;
    weaponless.IdleAnimationMetadata.WeaponComponents.clear();
    PreviewAnimationSession weaponlessSession;
    if (!weaponlessSession.resetForGeneratedShip(weaponless).Success) { return 1; }
    const PreviewAnimationActionResult weaponlessFire = weaponlessSession.triggerFiringEvent(weaponless);
    if (weaponlessFire.Success || weaponlessFire.StatusMessage.empty()) { return 1; }

    const auto customRecipe = makeCustomRecipe();
    const GeneratedShip customShip = ShipGenerator{}.generate(customRecipe);
    PreviewAnimationSession customSession;
    if (!customSession.resetForGeneratedShip(customShip).Success || customSession.getActiveFrames().empty() || !imagesEqual(customSession.getActiveFrames().front(), customShip.FinalImage)) { return 1; }

    PreviewWorkspaceSession workspaces;
    PreviewMode mode = workspaces.switchTo(PreviewWorkspace::ANIMATION, PreviewMode::STATIC);
    mode = workspaces.switchTo(PreviewWorkspace::GENERATE, mode);
    if (workspaces.getActiveWorkspace() != PreviewWorkspace::GENERATE || mode != PreviewMode::STATIC) { return 1; }

    const Image idleSheet = createHorizontalSpritesheet(session.getIdleAnimation());
    if (idleSheet.empty() || idleSheet.getHeight() != ship.FinalImage.getHeight()) { return 1; }

    std::cout << "preview_animation_lab passed.\n";
    return 0;
}
