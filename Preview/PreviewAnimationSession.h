#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "AnimationSamplingPlanner.h"
#include "GeneratedShip.h"
#include "ShipAnimationStateCoordinator.h"
#include "ShipFiringAnimation.h"
#include "ShipFiringAnimator.h"
#include "ShipIdleAnimation.h"
#include "ShipIdleAnimator.h"
#include "ShipLateralMovementAnimator.h"
#include "ShipLongitudinalMovementAnimator.h"
#include "ShipMovementAnimation.h"

namespace PixelShipGeneratorPreview
{
    struct PreviewAnimationActionResult
    {
        bool Success = true;
        bool ActiveFramesChanged = false;
        bool StartPlayback = false;
        std::string StatusMessage;
    };

    struct PreviewAnimationAdvanceResult
    {
        bool ActiveFramesChanged = false;
        bool FrameChanged = false;
        bool ReturnToStatic = false;
    };

    class PreviewAnimationSession
    {
    public:
        PreviewAnimationActionResult resetForGeneratedShip(const PixelShipGenerator::GeneratedShip& ship);
        PreviewAnimationActionResult regenerateSelectedAnimation(const PixelShipGenerator::GeneratedShip& ship);
        PreviewAnimationActionResult cycleAnimationType(const PixelShipGenerator::GeneratedShip& ship);
        PreviewAnimationActionResult cycleBaseMovementState(const PixelShipGenerator::GeneratedShip& ship);
        PreviewAnimationActionResult cyclePlaybackSpeed();
        PreviewAnimationActionResult cycleMovementPhase();
        PreviewAnimationActionResult cycleFiringTarget(const PixelShipGenerator::GeneratedShip& ship);
        PreviewAnimationActionResult applySelectedState(const PixelShipGenerator::GeneratedShip& ship);
        PreviewAnimationActionResult returnToIdle(const PixelShipGenerator::GeneratedShip& ship);
        PreviewAnimationActionResult beginComposedFiringEvent(const PixelShipGenerator::GeneratedShip& ship);
        PreviewAnimationActionResult triggerFiringEvent(const PixelShipGenerator::GeneratedShip& ship);
        PreviewAnimationAdvanceResult advancePlayback(const PixelShipGenerator::GeneratedShip& ship, double elapsedMicroseconds);

        void resetPlaybackAccumulator();
        bool moveFrame(int32_t delta);
        void setFrameIndex(uint32_t frameIndex);
        bool setNormalizedTime(double normalizedTime);

        const std::vector<PixelShipGenerator::Image>& getActiveFrames() const;
        const PixelShipGenerator::ShipMovementAnimationClip* getActiveMovementClip() const;
        double getActiveFrameDurationMilliseconds() const;
        uint64_t getActiveSeed() const;
        const PixelShipGenerator::AnimationSamplingPlan& getActiveSampling() const;
        uint32_t getActiveDurationMilliseconds() const;
        double getActiveNormalizedTime() const;
        double getPlaybackSpeed() const { return m_PlaybackSpeed; }
        bool isActiveLooping() const;
        uint32_t getActiveAnimatedComponentCount() const;
        std::string getSemanticPhaseDisplay() const;
        std::string getEffectDisplay() const;

        PixelShipGenerator::ShipAnimationType getSelectedAnimationType() const { return m_SelectedAnimationType; }
        PixelShipGenerator::ShipMovementAnimationPhase getMovementPhase() const { return m_MovementAnimationPhase; }
        uint32_t getFrameIndex() const { return m_AnimationFrameIndex; }
        PixelShipGenerator::ShipAnimationType getRuntimeMovementType() const { return m_RuntimeMovementType; }
        PixelShipGenerator::ShipAnimationType getPendingMovementType() const { return m_PendingMovementType; }
        bool isMovementTransitionPending() const { return m_MovementTransitionPending; }
        bool isTransientStatePreviewActive() const { return m_TransientStatePreviewActive; }
        double getRuntimeMovementNormalizedTime() const { return m_RuntimeMovementNormalizedTime; }
        const std::vector<PixelShipGenerator::ShipFiringAnimationTarget>& getFiringTargets() const { return m_FiringTargets; }
        uint32_t getSelectedFiringTargetIndex() const { return m_SelectedFiringTargetIndex; }

        const PixelShipGenerator::ShipIdleAnimationSettings& getIdleSettings() const { return m_IdleAnimationSettings; }
        PixelShipGenerator::ShipIdleAnimationSettings& getIdleSettings() { return m_IdleAnimationSettings; }
        const PixelShipGenerator::ShipIdleAnimation& getIdleAnimation() const { return m_IdleAnimation; }
        const PixelShipGenerator::ShipMovementAnimationSettings& getMovementSettings() const { return m_MovementAnimationSettings; }
        PixelShipGenerator::ShipMovementAnimationSettings& getMovementSettings() { return m_MovementAnimationSettings; }
        const PixelShipGenerator::ShipMovementAnimation& getMovementAnimation() const { return m_MovementAnimation; }
        const PixelShipGenerator::ShipFiringAnimationSettings& getFiringSettings() const { return m_FiringAnimationSettings; }
        PixelShipGenerator::ShipFiringAnimationSettings& getFiringSettings() { return m_FiringAnimationSettings; }
        const PixelShipGenerator::ShipFiringAnimation& getFiringAnimation() const { return m_FiringAnimation; }

    private:
        static bool isMovementAnimationType(PixelShipGenerator::ShipAnimationType type);
        static double wrapNormalizedAnimationTime(double normalizedTime);
        const std::vector<double>& getActiveNormalizedSampleTimes() const;
        PreviewAnimationActionResult generateSelected(const PixelShipGenerator::GeneratedShip& ship);

        PixelShipGenerator::ShipIdleAnimator m_IdleAnimator;
        PixelShipGenerator::ShipAnimationStateCoordinator m_AnimationStateCoordinator;
        PixelShipGenerator::ShipFiringAnimator m_FiringAnimator;
        PixelShipGenerator::ShipLateralMovementAnimator m_LateralMovementAnimator;
        PixelShipGenerator::ShipLongitudinalMovementAnimator m_LongitudinalMovementAnimator;

        PixelShipGenerator::ShipIdleAnimationSettings m_IdleAnimationSettings;
        PixelShipGenerator::ShipIdleAnimation m_IdleAnimation;
        PixelShipGenerator::ShipMovementAnimationSettings m_MovementAnimationSettings;
        PixelShipGenerator::ShipMovementAnimation m_MovementAnimation;
        PixelShipGenerator::ShipFiringAnimationSettings m_FiringAnimationSettings;
        PixelShipGenerator::ShipFiringAnimation m_FiringAnimation;
        std::vector<PixelShipGenerator::ShipFiringAnimationTarget> m_FiringTargets;
        uint32_t m_SelectedFiringTargetIndex = 0u;

        PixelShipGenerator::ShipAnimationType m_SelectedAnimationType = PixelShipGenerator::ShipAnimationType::IDLE;
        PixelShipGenerator::ShipMovementAnimationPhase m_MovementAnimationPhase = PixelShipGenerator::ShipMovementAnimationPhase::ENTER;
        uint32_t m_AnimationFrameIndex = 0u;
        double m_AnimationPlaybackAccumulatorMicroseconds = 0.0;

        PixelShipGenerator::ShipAnimationType m_RuntimeMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
        PixelShipGenerator::ShipAnimationType m_PendingMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
        bool m_MovementTransitionPending = false;
        bool m_TransientStatePreviewActive = false;
        double m_RuntimeMovementNormalizedTime = 0.0;
        double m_ResumeMovementNormalizedTime = 0.0;
        double m_StatePreviewFrameDurationMilliseconds = 0.0;
        double m_PlaybackSpeed = 1.0;
        std::vector<PixelShipGenerator::Image> m_StatePreviewFrames;
    };
}
