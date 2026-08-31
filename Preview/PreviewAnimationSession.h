#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <SpectralShipGen/AnimationSamplingPlanner.h>
#include <SpectralShipGen/GeneratedShip.h>
#include <SpectralShipGen/ShipAnimationStateCoordinator.h>
#include <SpectralShipGen/ShipFiringAnimation.h>
#include <SpectralShipGen/ShipFiringAnimator.h>
#include <SpectralShipGen/ShipIdleAnimation.h>
#include <SpectralShipGen/ShipIdleAnimator.h>
#include <SpectralShipGen/ShipLateralMovementAnimator.h>
#include <SpectralShipGen/ShipLongitudinalMovementAnimator.h>
#include <SpectralShipGen/ShipMovementAnimation.h>

namespace SpectralShipGenStudioPreview
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
        PreviewAnimationActionResult resetForGeneratedShip(const SpectralShipGen::GeneratedShip& ship);
        PreviewAnimationActionResult regenerateSelectedAnimation(const SpectralShipGen::GeneratedShip& ship);
        PreviewAnimationActionResult cycleAnimationType(const SpectralShipGen::GeneratedShip& ship);
        PreviewAnimationActionResult cycleBaseMovementState(const SpectralShipGen::GeneratedShip& ship);
        PreviewAnimationActionResult cyclePlaybackSpeed();
        PreviewAnimationActionResult cycleMovementPhase();
        PreviewAnimationActionResult cycleFiringTarget(const SpectralShipGen::GeneratedShip& ship);
        PreviewAnimationActionResult applySelectedState(const SpectralShipGen::GeneratedShip& ship);
        PreviewAnimationActionResult returnToIdle(const SpectralShipGen::GeneratedShip& ship);
        PreviewAnimationActionResult beginComposedFiringEvent(const SpectralShipGen::GeneratedShip& ship);
        PreviewAnimationActionResult triggerFiringEvent(const SpectralShipGen::GeneratedShip& ship);
        PreviewAnimationAdvanceResult advancePlayback(const SpectralShipGen::GeneratedShip& ship, double elapsedMicroseconds);

        void resetPlaybackAccumulator();
        bool moveFrame(int32_t delta);
        void setFrameIndex(uint32_t frameIndex);
        bool setNormalizedTime(double normalizedTime);

        const std::vector<SpectralShipGen::Image>& getActiveFrames() const;
        const SpectralShipGen::ShipMovementAnimationClip* getActiveMovementClip() const;
        double getActiveFrameDurationMilliseconds() const;
        uint64_t getActiveSeed() const;
        const SpectralShipGen::AnimationSamplingPlan& getActiveSampling() const;
        uint32_t getActiveDurationMilliseconds() const;
        double getActiveNormalizedTime() const;
        double getPlaybackSpeed() const { return m_PlaybackSpeed; }
        bool isActiveLooping() const;
        uint32_t getActiveAnimatedComponentCount() const;
        std::string getSemanticPhaseDisplay() const;
        std::string getEffectDisplay() const;

        SpectralShipGen::ShipAnimationType getSelectedAnimationType() const { return m_SelectedAnimationType; }
        SpectralShipGen::ShipMovementAnimationPhase getMovementPhase() const { return m_MovementAnimationPhase; }
        uint32_t getFrameIndex() const { return m_AnimationFrameIndex; }
        SpectralShipGen::ShipAnimationType getRuntimeMovementType() const { return m_RuntimeMovementType; }
        SpectralShipGen::ShipAnimationType getPendingMovementType() const { return m_PendingMovementType; }
        bool isMovementTransitionPending() const { return m_MovementTransitionPending; }
        bool isTransientStatePreviewActive() const { return m_TransientStatePreviewActive; }
        double getRuntimeMovementNormalizedTime() const { return m_RuntimeMovementNormalizedTime; }
        const std::vector<SpectralShipGen::ShipFiringAnimationTarget>& getFiringTargets() const { return m_FiringTargets; }
        uint32_t getSelectedFiringTargetIndex() const { return m_SelectedFiringTargetIndex; }

        const SpectralShipGen::ShipIdleAnimationSettings& getIdleSettings() const { return m_IdleAnimationSettings; }
        SpectralShipGen::ShipIdleAnimationSettings& getIdleSettings() { return m_IdleAnimationSettings; }
        const SpectralShipGen::ShipIdleAnimation& getIdleAnimation() const { return m_IdleAnimation; }
        const SpectralShipGen::ShipMovementAnimationSettings& getMovementSettings() const { return m_MovementAnimationSettings; }
        SpectralShipGen::ShipMovementAnimationSettings& getMovementSettings() { return m_MovementAnimationSettings; }
        const SpectralShipGen::ShipMovementAnimation& getMovementAnimation() const { return m_MovementAnimation; }
        const SpectralShipGen::ShipFiringAnimationSettings& getFiringSettings() const { return m_FiringAnimationSettings; }
        SpectralShipGen::ShipFiringAnimationSettings& getFiringSettings() { return m_FiringAnimationSettings; }
        const SpectralShipGen::ShipFiringAnimation& getFiringAnimation() const { return m_FiringAnimation; }

    private:
        static bool isMovementAnimationType(SpectralShipGen::ShipAnimationType type);
        static double wrapNormalizedAnimationTime(double normalizedTime);
        const std::vector<double>& getActiveNormalizedSampleTimes() const;
        PreviewAnimationActionResult generateSelected(const SpectralShipGen::GeneratedShip& ship);

        SpectralShipGen::ShipIdleAnimator m_IdleAnimator;
        SpectralShipGen::ShipAnimationStateCoordinator m_AnimationStateCoordinator;
        SpectralShipGen::ShipFiringAnimator m_FiringAnimator;
        SpectralShipGen::ShipLateralMovementAnimator m_LateralMovementAnimator;
        SpectralShipGen::ShipLongitudinalMovementAnimator m_LongitudinalMovementAnimator;

        SpectralShipGen::ShipIdleAnimationSettings m_IdleAnimationSettings;
        SpectralShipGen::ShipIdleAnimation m_IdleAnimation;
        SpectralShipGen::ShipMovementAnimationSettings m_MovementAnimationSettings;
        SpectralShipGen::ShipMovementAnimation m_MovementAnimation;
        SpectralShipGen::ShipFiringAnimationSettings m_FiringAnimationSettings;
        SpectralShipGen::ShipFiringAnimation m_FiringAnimation;
        std::vector<SpectralShipGen::ShipFiringAnimationTarget> m_FiringTargets;
        uint32_t m_SelectedFiringTargetIndex = 0u;

        SpectralShipGen::ShipAnimationType m_SelectedAnimationType = SpectralShipGen::ShipAnimationType::IDLE;
        SpectralShipGen::ShipMovementAnimationPhase m_MovementAnimationPhase = SpectralShipGen::ShipMovementAnimationPhase::ENTER;
        uint32_t m_AnimationFrameIndex = 0u;
        double m_AnimationPlaybackAccumulatorMicroseconds = 0.0;

        SpectralShipGen::ShipAnimationType m_RuntimeMovementType = SpectralShipGen::ShipAnimationType::IDLE;
        SpectralShipGen::ShipAnimationType m_PendingMovementType = SpectralShipGen::ShipAnimationType::IDLE;
        bool m_MovementTransitionPending = false;
        bool m_TransientStatePreviewActive = false;
        double m_RuntimeMovementNormalizedTime = 0.0;
        double m_ResumeMovementNormalizedTime = 0.0;
        double m_StatePreviewFrameDurationMilliseconds = 0.0;
        double m_PlaybackSpeed = 1.0;
        std::vector<SpectralShipGen::Image> m_StatePreviewFrames;
    };
}
