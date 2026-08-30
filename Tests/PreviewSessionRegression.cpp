#include "RegressionSuites.h"

#include <cstdint>
#include <iostream>
#include <optional>

#include "PreviewAnimationSession.h"
#include "PreviewCollectionSession.h"
#include "ShipFiringAnimator.h"
#include "ShipIdleAnimator.h"
#include "ShipLateralMovementAnimator.h"
#include "ShipGenerationSettings.h"
#include "ShipGenerator.h"

namespace
{
    bool imagesEqual(const PixelShipGenerator::Image& first, const PixelShipGenerator::Image& second)
    {
        return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
    }

    PixelShipGeneratorPreview::PreviewGenerationRecipe makeRecipe(uint64_t seed, uint32_t width = 64u, uint32_t height = 64u)
    {
        PixelShipGeneratorPreview::PreviewGenerationRecipe recipe;
        recipe.Seeds = PixelShipGenerator::deriveShipGenerationSeeds(seed);
        recipe.Dimensions = { width, height };
        recipe.Style = PixelShipGenerator::ShipStyle::FIGHTER;
        recipe.Faction = PixelShipGenerator::ShipFactionType::MILITARY;
        return recipe;
    }

    PixelShipGenerator::GeneratedShip generateAnimatedShip()
    {
        PixelShipGenerator::ShipGenerator generator;
        PixelShipGenerator::ShipFiringAnimator firingAnimator;
        for (uint64_t seed = 1u; seed <= 128u; ++seed)
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

    bool runCollectionChecks()
    {
        using namespace PixelShipGeneratorPreview;
        const PreviewGenerationRecipe first = makeRecipe(101u);
        const PreviewGenerationRecipe second = makeRecipe(202u, 96u, 64u);
        const PreviewGenerationRecipe third = makeRecipe(303u, 44u, 64u);
        const PreviewGenerationRecipe fourth = makeRecipe(404u, 128u, 96u);

        PreviewCollectionSession session(first, 3u);
        if (session.getHistoryCount() != 1u || session.getCurrentRecipe() != first) { return false; }
        session.appendHistoryEntry(second);
        session.appendHistoryEntry(third);
        if (!session.moveHistoryPrevious() || session.getCurrentRecipe() != second) { return false; }
        session.appendHistoryEntry(fourth);
        if (session.getHistoryCount() != 3u || session.getCurrentRecipe() != fourth || session.moveHistoryNext()) { return false; }
        if (!session.moveHistoryPrevious() || session.getCurrentRecipe() != second) { return false; }

        if (!session.addFavorite(first) || !session.addFavorite(third) || session.addFavorite(first)) { return false; }
        if (!session.isFavorite(third) || session.findFavoriteIndex(third) != 1u) { return false; }
        if (session.getFavorite(1u) == nullptr || *session.getFavorite(1u) != third) { return false; }
        if (!session.removeFavorite(first) || session.isFavorite(first) || session.getFavorites().size() != 1u) { return false; }

        session.beginGallery(999u, second);
        session.addGalleryRecipe(third);
        session.addInvalidGalleryRecipe();
        session.addGalleryRecipe(fourth);
        if (session.getGalleryBatchSeed() != 999u || session.getGalleryTemplateRecipe() != second) { return false; }
        if (session.getGalleryRecipe(0u) == nullptr || *session.getGalleryRecipe(0u) != third) { return false; }
        if (session.getGalleryRecipe(1u) != nullptr || session.getGalleryRecipe(2u) == nullptr || *session.getGalleryRecipe(2u) != fourth) { return false; }
        if (!session.isGalleryFavorite(0u) || session.isGalleryFavorite(1u) || session.isGalleryFavorite(2u)) { return false; }
        const PreviewGenerationRecipe currentBeforeGalleryFavorite = session.getCurrentRecipe();
        const std::optional<bool> fourthFavorite = session.toggleGalleryFavorite(2u);
        if (!fourthFavorite.has_value() || !*fourthFavorite || !session.isFavorite(fourth) || !session.isGalleryFavorite(2u)) { return false; }
        if (session.getFavorites().size() != 2u || session.getCurrentRecipe() != currentBeforeGalleryFavorite) { return false; }
        if (session.toggleGalleryFavorite(1u).has_value()) { return false; }
        session.clearGallery();
        if (!session.getGalleryRecipes().empty() || !session.isFavorite(third) || !session.isFavorite(fourth)) { return false; }

        session.beginGallery(1000u, second);
        session.addGalleryRecipe(third);
        session.addGalleryRecipe(fourth);
        const std::optional<bool> thirdFavorite = session.toggleGalleryFavorite(0u);
        if (!thirdFavorite.has_value() || *thirdFavorite || session.isFavorite(third) || session.isGalleryFavorite(0u) || !session.isGalleryFavorite(1u)) { return false; }
        session.clearGallery();
        if (!session.isFavorite(fourth) || session.isFavorite(third)) { return false; }

        if (!session.addResolutionBookmark({ 96u, 64u })) { return false; }
        if (!session.addResolutionBookmark({ 44u, 44u })) { return false; }
        if (session.addResolutionBookmark({ 96u, 64u })) { return false; }
        if (!session.hasResolutionBookmark({ 44u, 44u })) { return false; }
        if (session.getResolutionBookmarks().size() != 2u || session.getResolutionBookmarks().front() != PixelShipGenerator::ShipDimensions{ 44u, 44u }) { return false; }
        if (!session.removeResolutionBookmark({ 44u, 44u }) || session.hasResolutionBookmark({ 44u, 44u })) { return false; }
        return true;
    }

    bool runAnimationChecks()
    {
        using namespace PixelShipGeneratorPreview;
        const PixelShipGenerator::GeneratedShip ship = generateAnimatedShip();
        if (ship.FinalImage.empty()) { return false; }

        PreviewAnimationSession first;
        PreviewAnimationSession second;
        const PreviewAnimationActionResult firstReset = first.resetForGeneratedShip(ship);
        const PreviewAnimationActionResult secondReset = second.resetForGeneratedShip(ship);
        if (!firstReset.Success || !secondReset.Success || first.getActiveFrames().empty()) { return false; }
        if (!imagesEqual(first.getActiveFrames().front(), ship.FinalImage)) { return false; }
        if (first.getActiveFrames().size() != second.getActiveFrames().size()) { return false; }
        for (std::size_t index = 0u; index < first.getActiveFrames().size(); ++index)
        {
            if (!imagesEqual(first.getActiveFrames()[index], second.getActiveFrames()[index])) { return false; }
        }

        const PixelShipGenerator::ShipIdleAnimation directIdle = PixelShipGenerator::ShipIdleAnimator{}.generate(ship, first.getIdleSettings());
        if (directIdle.Frames.size() != first.getIdleAnimation().Frames.size()) { return false; }
        for (std::size_t index = 0u; index < directIdle.Frames.size(); ++index)
        {
            if (!imagesEqual(directIdle.Frames[index], first.getIdleAnimation().Frames[index])) { return false; }
        }

        const PreviewAnimationActionResult leftSelect = first.cycleAnimationType(ship);
        if (!leftSelect.Success || first.getSelectedAnimationType() != PixelShipGenerator::ShipAnimationType::MOVE_LEFT || first.getMovementPhase() != PixelShipGenerator::ShipMovementAnimationPhase::ENTER) { return false; }
        const PixelShipGenerator::ShipMovementAnimation directLeft = PixelShipGenerator::ShipLateralMovementAnimator{}.generate(ship, PixelShipGenerator::ShipAnimationType::MOVE_LEFT, first.getMovementSettings());
        if (directLeft.Enter.Frames.size() != first.getMovementAnimation().Enter.Frames.size()) { return false; }
        for (std::size_t index = 0u; index < directLeft.Enter.Frames.size(); ++index)
        {
            if (!imagesEqual(directLeft.Enter.Frames[index], first.getMovementAnimation().Enter.Frames[index])) { return false; }
        }

        const PreviewAnimationActionResult leftApply = first.applySelectedState(ship);
        if (!leftApply.Success || !leftApply.StartPlayback || first.getRuntimeMovementType() != PixelShipGenerator::ShipAnimationType::MOVE_LEFT) { return false; }
        const PreviewAnimationAdvanceResult enterAdvance = first.advancePlayback(ship, 1000000.0);
        if (!enterAdvance.FrameChanged || first.getMovementPhase() != PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN) { return false; }

        if (!first.moveFrame(1) || first.getFrameIndex() >= first.getActiveFrames().size()) { return false; }
        if (!first.moveFrame(-1) || first.getFrameIndex() >= first.getActiveFrames().size()) { return false; }

        const PreviewAnimationActionResult rightSelect = first.cycleAnimationType(ship);
        if (!rightSelect.Success || first.getSelectedAnimationType() != PixelShipGenerator::ShipAnimationType::MOVE_RIGHT) { return false; }
        const PreviewAnimationActionResult reverse = first.applySelectedState(ship);
        if (!reverse.Success || !first.isMovementTransitionPending() || first.getMovementPhase() != PixelShipGenerator::ShipMovementAnimationPhase::EXIT) { return false; }
        first.advancePlayback(ship, 1000000.0);
        if (first.getRuntimeMovementType() != PixelShipGenerator::ShipAnimationType::MOVE_RIGHT || first.getMovementPhase() != PixelShipGenerator::ShipMovementAnimationPhase::ENTER) { return false; }
        first.advancePlayback(ship, 1000000.0);
        if (first.getMovementPhase() != PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN) { return false; }

        first.cycleAnimationType(ship); // MOVE_UP
        first.cycleAnimationType(ship); // MOVE_DOWN
        const PreviewAnimationActionResult fireSelect = first.cycleAnimationType(ship); // FIRE
        if (!fireSelect.Success || first.getSelectedAnimationType() != PixelShipGenerator::ShipAnimationType::FIRE || first.getFiringTargets().empty()) { return false; }
        const PixelShipGenerator::ShipFiringAnimation directFire = PixelShipGenerator::ShipFiringAnimator{}.generate(ship, first.getFiringTargets()[first.getSelectedFiringTargetIndex()], first.getFiringSettings());
        if (directFire.Frames.size() != first.getFiringAnimation().Frames.size()) { return false; }
        for (std::size_t index = 0u; index < directFire.Frames.size(); ++index)
        {
            if (!imagesEqual(directFire.Frames[index], first.getFiringAnimation().Frames[index])) { return false; }
        }

        const PreviewAnimationActionResult firing = first.applySelectedState(ship);
        if (!firing.Success || !firing.StartPlayback || !first.isTransientStatePreviewActive()) { return false; }
        if (first.getActiveFrames().empty() || first.getFiringAnimation().Frames.empty()) { return false; }
        first.advancePlayback(ship, 2000000.0);
        if (first.isTransientStatePreviewActive() || first.getSelectedAnimationType() != PixelShipGenerator::ShipAnimationType::MOVE_RIGHT || first.getRuntimeMovementType() != PixelShipGenerator::ShipAnimationType::MOVE_RIGHT || first.getMovementPhase() != PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN) { return false; }

        const PreviewAnimationActionResult returnIdle = first.returnToIdle(ship);
        if (!returnIdle.Success || !returnIdle.StartPlayback || first.getMovementPhase() != PixelShipGenerator::ShipMovementAnimationPhase::EXIT) { return false; }
        first.advancePlayback(ship, 1000000.0);
        if (first.getSelectedAnimationType() != PixelShipGenerator::ShipAnimationType::IDLE || first.getRuntimeMovementType() != PixelShipGenerator::ShipAnimationType::IDLE) { return false; }
        return true;
    }
}

namespace PixelShipGeneratorTests
{
    int runPreviewSessionRegression()
    {
        if (!runCollectionChecks())
        {
            std::cerr << "preview_session_collection failed.\n";
            return 1;
        }
        if (!runAnimationChecks())
        {
            std::cerr << "preview_session_animation failed.\n";
            return 1;
        }
        std::cout << "preview_session passed.\n";
        return 0;
    }
}
