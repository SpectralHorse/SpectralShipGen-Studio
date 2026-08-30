#include "PreviewAnimationSession.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace PixelShipGeneratorPreview
{
    namespace
    {
        std::string getAnimationTypeDisplayName(PixelShipGenerator::ShipAnimationType type)
        {
            switch (type)
            {
            case PixelShipGenerator::ShipAnimationType::IDLE: return "IDLE";
            case PixelShipGenerator::ShipAnimationType::MOVE_LEFT: return "MOVE LEFT";
            case PixelShipGenerator::ShipAnimationType::MOVE_RIGHT: return "MOVE RIGHT";
            case PixelShipGenerator::ShipAnimationType::MOVE_UP: return "MOVE UP";
            case PixelShipGenerator::ShipAnimationType::MOVE_DOWN: return "MOVE DOWN";
            case PixelShipGenerator::ShipAnimationType::FIRE: return "FIRE";
            default: return "UNSUPPORTED";
            }
        }

        std::string getMovementPhaseDisplayName(PixelShipGenerator::ShipMovementAnimationPhase phase)
        {
            switch (phase)
            {
            case PixelShipGenerator::ShipMovementAnimationPhase::ENTER: return "ENTER";
            case PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN: return "SUSTAIN";
            case PixelShipGenerator::ShipMovementAnimationPhase::EXIT: return "EXIT";
            default: return "UNKNOWN";
            }
        }

        std::string getFiringPhaseDisplayName(PixelShipGenerator::ShipFiringAnimationPhase phase)
        {
            switch (phase)
            {
            case PixelShipGenerator::ShipFiringAnimationPhase::REST: return "REST";
            case PixelShipGenerator::ShipFiringAnimationPhase::PRE_FIRE: return "PREFIRE";
            case PixelShipGenerator::ShipFiringAnimationPhase::RECOIL: return "RECOIL";
            case PixelShipGenerator::ShipFiringAnimationPhase::RECOVERY: return "RECOVERY";
            default: return "UNKNOWN";
            }
        }

        std::string getWeaponTypeDisplayName(PixelShipGenerator::ShipWeaponType type)
        {
            switch (type)
            {
            case PixelShipGenerator::ShipWeaponType::SINGLE_CANNON: return "SINGLE CANNON";
            case PixelShipGenerator::ShipWeaponType::TWIN_CANNON: return "TWIN CANNON";
            case PixelShipGenerator::ShipWeaponType::COMPACT_TURRET: return "COMPACT TURRET";
            case PixelShipGenerator::ShipWeaponType::RAIL_WEAPON: return "RAIL WEAPON";
            case PixelShipGenerator::ShipWeaponType::WEAPON_POD: return "WEAPON POD";
            default: return "WEAPON";
            }
        }
    }

    bool PreviewAnimationSession::isMovementAnimationType(PixelShipGenerator::ShipAnimationType type)
    {
        return type == PixelShipGenerator::ShipAnimationType::MOVE_LEFT || type == PixelShipGenerator::ShipAnimationType::MOVE_RIGHT || type == PixelShipGenerator::ShipAnimationType::MOVE_UP || type == PixelShipGenerator::ShipAnimationType::MOVE_DOWN;
    }

    double PreviewAnimationSession::wrapNormalizedAnimationTime(double normalizedTime)
    {
        double wrapped = normalizedTime - std::floor(normalizedTime);
        if (wrapped < 0.0) { wrapped += 1.0; }
        return wrapped;
    }

    PreviewAnimationActionResult PreviewAnimationSession::resetForGeneratedShip(const PixelShipGenerator::GeneratedShip& ship)
    {
        m_RuntimeMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
        m_PendingMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
        m_MovementTransitionPending = false;
        m_TransientStatePreviewActive = false;
        m_RuntimeMovementNormalizedTime = 0.0;
        m_ResumeMovementNormalizedTime = 0.0;
        m_StatePreviewFrames.clear();
        m_AnimationFrameIndex = 0u;
        return generateSelected(ship);
    }

    PreviewAnimationActionResult PreviewAnimationSession::regenerateSelectedAnimation(const PixelShipGenerator::GeneratedShip& ship)
    {
        return generateSelected(ship);
    }

    PreviewAnimationActionResult PreviewAnimationSession::generateSelected(const PixelShipGenerator::GeneratedShip& ship)
    {
        PreviewAnimationActionResult result;
        result.ActiveFramesChanged = true;
        m_IdleAnimation = m_IdleAnimator.generate(ship, m_IdleAnimationSettings);

        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::MOVE_LEFT || m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::MOVE_RIGHT)
        {
            m_MovementAnimation = m_LateralMovementAnimator.generate(ship, m_SelectedAnimationType, m_MovementAnimationSettings);
        }
        else if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::MOVE_UP || m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::MOVE_DOWN)
        {
            m_MovementAnimation = m_LongitudinalMovementAnimator.generate(ship, m_SelectedAnimationType, m_MovementAnimationSettings);
        }
        else if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE)
        {
            m_FiringTargets = m_FiringAnimator.getAvailableTargets(ship);
            if (m_FiringTargets.empty())
            {
                m_FiringAnimation = {};
                m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
                result.StatusMessage = "FIRE unavailable: generated ship has no movable weapon component.";
                return result;
            }
            m_SelectedFiringTargetIndex %= static_cast<uint32_t>(m_FiringTargets.size());
            m_FiringAnimation = m_FiringAnimator.generate(ship, m_FiringTargets[m_SelectedFiringTargetIndex], m_FiringAnimationSettings);
        }

        m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
        if (m_AnimationFrameIndex >= getActiveFrames().size()) { m_AnimationFrameIndex = 0u; }
        return result;
    }

    PreviewAnimationActionResult PreviewAnimationSession::cycleAnimationType(const PixelShipGenerator::GeneratedShip& ship)
    {
        m_TransientStatePreviewActive = false;
        m_StatePreviewFrames.clear();
        constexpr std::array<PixelShipGenerator::ShipAnimationType, 6u> Types = {
            PixelShipGenerator::ShipAnimationType::IDLE,
            PixelShipGenerator::ShipAnimationType::MOVE_LEFT,
            PixelShipGenerator::ShipAnimationType::MOVE_RIGHT,
            PixelShipGenerator::ShipAnimationType::MOVE_UP,
            PixelShipGenerator::ShipAnimationType::MOVE_DOWN,
            PixelShipGenerator::ShipAnimationType::FIRE
        };
        const auto iterator = std::find(Types.begin(), Types.end(), m_SelectedAnimationType);
        const std::size_t currentIndex = iterator == Types.end() ? 0u : static_cast<std::size_t>(std::distance(Types.begin(), iterator));
        m_SelectedAnimationType = Types[(currentIndex + 1u) % Types.size()];
        m_MovementAnimationPhase = PixelShipGenerator::ShipMovementAnimationPhase::ENTER;
        m_AnimationFrameIndex = 0u;
        PreviewAnimationActionResult result = generateSelected(ship);
        if (result.StatusMessage.empty()) { result.StatusMessage = "Animation type: " + getAnimationTypeDisplayName(m_SelectedAnimationType); }
        return result;
    }

    PreviewAnimationActionResult PreviewAnimationSession::cycleBaseMovementState(const PixelShipGenerator::GeneratedShip& ship)
    {
        constexpr std::array<PixelShipGenerator::ShipAnimationType, 5u> BaseStates = {
            PixelShipGenerator::ShipAnimationType::IDLE,
            PixelShipGenerator::ShipAnimationType::MOVE_LEFT,
            PixelShipGenerator::ShipAnimationType::MOVE_RIGHT,
            PixelShipGenerator::ShipAnimationType::MOVE_UP,
            PixelShipGenerator::ShipAnimationType::MOVE_DOWN
        };
        const PixelShipGenerator::ShipAnimationType reference = m_MovementTransitionPending ? m_PendingMovementType : m_RuntimeMovementType;
        const auto iterator = std::find(BaseStates.begin(), BaseStates.end(), reference);
        const std::size_t currentIndex = iterator == BaseStates.end() ? 0u : static_cast<std::size_t>(std::distance(BaseStates.begin(), iterator));
        const PixelShipGenerator::ShipAnimationType target = BaseStates[(currentIndex + 1u) % BaseStates.size()];
        m_SelectedAnimationType = target;
        m_AnimationFrameIndex = 0u;
        PreviewAnimationActionResult result = target == PixelShipGenerator::ShipAnimationType::IDLE ? returnToIdle(ship) : applySelectedState(ship);
        if (result.Success) { result.StatusMessage = "Base movement: " + getAnimationTypeDisplayName(target); }
        return result;
    }

    PreviewAnimationActionResult PreviewAnimationSession::cyclePlaybackSpeed()
    {
        constexpr std::array<double, 5u> Speeds = { 0.25, 0.5, 1.0, 2.0, 4.0 };
        auto iterator = std::find(Speeds.begin(), Speeds.end(), m_PlaybackSpeed);
        const std::size_t currentIndex = iterator == Speeds.end() ? 2u : static_cast<std::size_t>(std::distance(Speeds.begin(), iterator));
        m_PlaybackSpeed = Speeds[(currentIndex + 1u) % Speeds.size()];
        m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
        PreviewAnimationActionResult result;
        result.StatusMessage = "Playback speed: " + std::to_string(m_PlaybackSpeed) + "x";
        return result;
    }

    PreviewAnimationActionResult PreviewAnimationSession::cycleMovementPhase()
    {
        PreviewAnimationActionResult result;
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE || m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE) { return result; }
        const uint32_t phaseCount = static_cast<uint32_t>(PixelShipGenerator::ShipMovementAnimationPhase::SHIP_MOVEMENT_ANIMATION_PHASE_END);
        m_MovementAnimationPhase = static_cast<PixelShipGenerator::ShipMovementAnimationPhase>((static_cast<uint32_t>(m_MovementAnimationPhase) + 1u) % phaseCount);
        m_AnimationFrameIndex = 0u;
        m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
        result.ActiveFramesChanged = true;
        result.StatusMessage = "Movement phase: " + getMovementPhaseDisplayName(m_MovementAnimationPhase);
        return result;
    }

    PreviewAnimationActionResult PreviewAnimationSession::cycleFiringTarget(const PixelShipGenerator::GeneratedShip& ship)
    {
        PreviewAnimationActionResult result;
        if (m_SelectedAnimationType != PixelShipGenerator::ShipAnimationType::FIRE || m_FiringTargets.empty()) { return result; }
        m_SelectedFiringTargetIndex = (m_SelectedFiringTargetIndex + 1u) % static_cast<uint32_t>(m_FiringTargets.size());
        m_AnimationFrameIndex = 0u;
        m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
        if (m_TransientStatePreviewActive)
        {
            result = beginComposedFiringEvent(ship);
        }
        else
        {
            result = generateSelected(ship);
        }
        if (result.Success && m_FiringAnimation.Diagnostics.ValidTarget && !m_FiringAnimation.Diagnostics.Weapons.empty())
        {
            const auto& weapon = m_FiringAnimation.Diagnostics.Weapons.front();
            result.StatusMessage = "Firing target: " + getWeaponTypeDisplayName(weapon.Type) + " component " + std::to_string(m_FiringAnimation.Target.WeaponComponentIndex);
        }
        return result;
    }

    PreviewAnimationActionResult PreviewAnimationSession::applySelectedState(const PixelShipGenerator::GeneratedShip& ship)
    {
        m_TransientStatePreviewActive = false;
        m_StatePreviewFrames.clear();
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE) { return beginComposedFiringEvent(ship); }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE) { return returnToIdle(ship); }

        PreviewAnimationActionResult result;
        if (!isMovementAnimationType(m_SelectedAnimationType)) { return result; }
        const PixelShipGenerator::ShipAnimationType target = m_SelectedAnimationType;
        if (m_RuntimeMovementType == target)
        {
            m_MovementTransitionPending = false;
            m_PendingMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
            m_MovementAnimationPhase = PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN;
            result = generateSelected(ship);
            const auto* clip = getActiveMovementClip();
            if (clip != nullptr && !clip->Frames.empty())
            {
                m_AnimationFrameIndex = static_cast<uint32_t>(std::floor(wrapNormalizedAnimationTime(m_RuntimeMovementNormalizedTime) * static_cast<double>(clip->Frames.size()))) % static_cast<uint32_t>(clip->Frames.size());
            }
            result.StartPlayback = true;
            result.StatusMessage = "State: " + getAnimationTypeDisplayName(target) + " SUSTAIN";
            return result;
        }

        if (m_RuntimeMovementType == PixelShipGenerator::ShipAnimationType::IDLE)
        {
            m_RuntimeMovementType = target;
            m_MovementTransitionPending = false;
            m_PendingMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
            m_MovementAnimationPhase = PixelShipGenerator::ShipMovementAnimationPhase::ENTER;
            m_RuntimeMovementNormalizedTime = 0.0;
            result = generateSelected(ship);
            result.StartPlayback = true;
            result.StatusMessage = "State transition: IDLE -> " + getAnimationTypeDisplayName(target);
            return result;
        }

        const PixelShipGenerator::ShipAnimationType current = m_RuntimeMovementType;
        const PixelShipGenerator::ShipMovementTransitionPlan plan = m_AnimationStateCoordinator.planMovementTransition(current, target, m_MovementAnimationSettings);
        m_PendingMovementType = target;
        m_MovementTransitionPending = plan.ExitCurrentMovement && plan.EnterTargetMovement;
        m_SelectedAnimationType = current;
        m_MovementAnimationPhase = PixelShipGenerator::ShipMovementAnimationPhase::EXIT;
        result = generateSelected(ship);
        result.StartPlayback = true;
        result.StatusMessage = "State transition: " + getAnimationTypeDisplayName(current) + " -> NEUTRAL -> " + getAnimationTypeDisplayName(target);
        return result;
    }

    PreviewAnimationActionResult PreviewAnimationSession::returnToIdle(const PixelShipGenerator::GeneratedShip& ship)
    {
        m_TransientStatePreviewActive = false;
        m_StatePreviewFrames.clear();
        PreviewAnimationActionResult result;
        if (m_RuntimeMovementType == PixelShipGenerator::ShipAnimationType::IDLE)
        {
            m_SelectedAnimationType = PixelShipGenerator::ShipAnimationType::IDLE;
            m_MovementTransitionPending = false;
            m_PendingMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
            result = generateSelected(ship);
            result.StartPlayback = true;
            result.StatusMessage = "State: IDLE";
            return result;
        }

        const PixelShipGenerator::ShipAnimationType current = m_RuntimeMovementType;
        const PixelShipGenerator::ShipMovementTransitionPlan plan = m_AnimationStateCoordinator.planMovementTransition(current, PixelShipGenerator::ShipAnimationType::IDLE, m_MovementAnimationSettings);
        m_PendingMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
        m_MovementTransitionPending = plan.ExitCurrentMovement;
        m_SelectedAnimationType = current;
        m_MovementAnimationPhase = PixelShipGenerator::ShipMovementAnimationPhase::EXIT;
        result = generateSelected(ship);
        result.StartPlayback = true;
        result.StatusMessage = "State transition: " + getAnimationTypeDisplayName(current) + " -> IDLE";
        return result;
    }

    PreviewAnimationActionResult PreviewAnimationSession::beginComposedFiringEvent(const PixelShipGenerator::GeneratedShip& ship)
    {
        PreviewAnimationActionResult result;
        m_FiringTargets = m_FiringAnimator.getAvailableTargets(ship);
        if (m_FiringTargets.empty())
        {
            result.Success = false;
            result.StatusMessage = "FIRE unavailable: generated ship has no movable weapon component.";
            return result;
        }

        m_SelectedFiringTargetIndex %= static_cast<uint32_t>(m_FiringTargets.size());
        const PixelShipGenerator::ShipFiringAnimationTarget target = m_FiringTargets[m_SelectedFiringTargetIndex];
        m_FiringAnimation = m_FiringAnimator.generate(ship, target, m_FiringAnimationSettings);
        if (m_FiringAnimation.Frames.empty()) { result.Success = false; return result; }

        PixelShipGenerator::ShipMovementAnimation movement;
        uint32_t sustainDurationMilliseconds = 0u;
        if (m_RuntimeMovementType != PixelShipGenerator::ShipAnimationType::IDLE)
        {
            if (m_RuntimeMovementType == PixelShipGenerator::ShipAnimationType::MOVE_LEFT || m_RuntimeMovementType == PixelShipGenerator::ShipAnimationType::MOVE_RIGHT)
            {
                movement = m_LateralMovementAnimator.generate(ship, m_RuntimeMovementType, m_MovementAnimationSettings);
            }
            else
            {
                movement = m_LongitudinalMovementAnimator.generate(ship, m_RuntimeMovementType, m_MovementAnimationSettings);
            }
            sustainDurationMilliseconds = movement.Sustain.DurationMilliseconds;
        }

        m_StatePreviewFrames.clear();
        m_StatePreviewFrames.reserve(m_FiringAnimation.NormalizedSampleTimes.size());
        for (double firingTime : m_FiringAnimation.NormalizedSampleTimes)
        {
            PixelShipGenerator::ShipAnimationStateRequest request;
            request.UnderlyingMovementType = m_RuntimeMovementType;
            request.MovementPhase = PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN;
            if (sustainDurationMilliseconds > 0u)
            {
                const double elapsedMilliseconds = firingTime * static_cast<double>(m_FiringAnimation.DurationMilliseconds);
                request.MovementNormalizedTime = wrapNormalizedAnimationTime(m_RuntimeMovementNormalizedTime + elapsedMilliseconds / static_cast<double>(sustainDurationMilliseconds));
            }
            request.FireActive = true;
            request.FiringTarget = target;
            request.FiringNormalizedTime = firingTime;
            m_StatePreviewFrames.push_back(m_AnimationStateCoordinator.evaluate(ship, request, m_MovementAnimationSettings, m_FiringAnimationSettings).Pose.Frame);
        }

        if (sustainDurationMilliseconds > 0u)
        {
            m_ResumeMovementNormalizedTime = wrapNormalizedAnimationTime(m_RuntimeMovementNormalizedTime + static_cast<double>(m_FiringAnimation.DurationMilliseconds) / static_cast<double>(sustainDurationMilliseconds));
        }
        else { m_ResumeMovementNormalizedTime = 0.0; }

        m_StatePreviewFrameDurationMilliseconds = m_FiringAnimation.FrameDurationMilliseconds;
        m_TransientStatePreviewActive = true;
        m_AnimationFrameIndex = 0u;
        m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
        result.ActiveFramesChanged = true;
        result.StartPlayback = true;
        result.StatusMessage = std::string("Transient FIRE over ") + (m_RuntimeMovementType == PixelShipGenerator::ShipAnimationType::IDLE ? "NEUTRAL" : getAnimationTypeDisplayName(m_RuntimeMovementType));
        return result;
    }

    PreviewAnimationActionResult PreviewAnimationSession::triggerFiringEvent(const PixelShipGenerator::GeneratedShip& ship)
    {
        m_SelectedAnimationType = PixelShipGenerator::ShipAnimationType::FIRE;
        m_AnimationFrameIndex = 0u;
        return beginComposedFiringEvent(ship);
    }

    PreviewAnimationAdvanceResult PreviewAnimationSession::advancePlayback(const PixelShipGenerator::GeneratedShip& ship, double elapsedMicroseconds)
    {
        PreviewAnimationAdvanceResult result;
        const std::vector<PixelShipGenerator::Image>& frames = getActiveFrames();
        if (frames.empty()) { return result; }
        const double frameDurationMicroseconds = std::max(1.0, getActiveFrameDurationMilliseconds() * 1000.0);
        m_AnimationPlaybackAccumulatorMicroseconds += std::max(0.0, elapsedMicroseconds) * m_PlaybackSpeed;
        if (m_AnimationPlaybackAccumulatorMicroseconds < frameDurationMicroseconds) { return result; }

        const uint32_t elapsedFrames = std::max(1u, static_cast<uint32_t>(m_AnimationPlaybackAccumulatorMicroseconds / frameDurationMicroseconds));
        m_AnimationPlaybackAccumulatorMicroseconds -= static_cast<double>(elapsedFrames) * frameDurationMicroseconds;
        const uint32_t frameCount = static_cast<uint32_t>(frames.size());

        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE)
        {
            const uint64_t targetFrame = static_cast<uint64_t>(m_AnimationFrameIndex) + static_cast<uint64_t>(elapsedFrames);
            if (targetFrame < frameCount)
            {
                m_AnimationFrameIndex = static_cast<uint32_t>(targetFrame);
                result.FrameChanged = true;
            }
            else if (m_TransientStatePreviewActive)
            {
                m_TransientStatePreviewActive = false;
                m_StatePreviewFrames.clear();
                m_AnimationFrameIndex = 0u;
                m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
                if (m_RuntimeMovementType != PixelShipGenerator::ShipAnimationType::IDLE)
                {
                    m_SelectedAnimationType = m_RuntimeMovementType;
                    m_MovementAnimationPhase = PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN;
                    m_RuntimeMovementNormalizedTime = m_ResumeMovementNormalizedTime;
                    generateSelected(ship);
                    const auto* sustain = getActiveMovementClip();
                    if (sustain != nullptr && !sustain->Frames.empty())
                    {
                        m_AnimationFrameIndex = static_cast<uint32_t>(std::floor(m_RuntimeMovementNormalizedTime * static_cast<double>(sustain->Frames.size()))) % static_cast<uint32_t>(sustain->Frames.size());
                    }
                }
                else
                {
                    m_SelectedAnimationType = PixelShipGenerator::ShipAnimationType::IDLE;
                    generateSelected(ship);
                    m_AnimationFrameIndex = 0u;
                }
                result.ActiveFramesChanged = true;
                result.FrameChanged = true;
            }
            else
            {
                m_AnimationFrameIndex = 0u;
                m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
                result.ReturnToStatic = true;
            }
            return result;
        }

        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE || m_MovementAnimationPhase == PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN)
        {
            m_AnimationFrameIndex = (m_AnimationFrameIndex + elapsedFrames) % frameCount;
            result.FrameChanged = true;
            if (m_SelectedAnimationType == m_RuntimeMovementType && isMovementAnimationType(m_SelectedAnimationType))
            {
                const auto* sustain = getActiveMovementClip();
                if (sustain != nullptr && m_AnimationFrameIndex < sustain->NormalizedSampleTimes.size()) { m_RuntimeMovementNormalizedTime = sustain->NormalizedSampleTimes[m_AnimationFrameIndex]; }
            }
            return result;
        }

        const uint64_t targetFrame = static_cast<uint64_t>(m_AnimationFrameIndex) + static_cast<uint64_t>(elapsedFrames);
        if (m_MovementAnimationPhase == PixelShipGenerator::ShipMovementAnimationPhase::ENTER)
        {
            if (targetFrame < frameCount)
            {
                m_AnimationFrameIndex = static_cast<uint32_t>(targetFrame);
            }
            else
            {
                const uint64_t leftoverFrames = targetFrame - frameCount;
                m_MovementAnimationPhase = PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN;
                const auto* sustain = getActiveMovementClip();
                const uint32_t sustainCount = sustain != nullptr ? static_cast<uint32_t>(sustain->Frames.size()) : 0u;
                if (sustainCount > 0u)
                {
                    m_AnimationFrameIndex = static_cast<uint32_t>(leftoverFrames % sustainCount);
                    if (m_AnimationFrameIndex < sustain->NormalizedSampleTimes.size()) { m_RuntimeMovementNormalizedTime = sustain->NormalizedSampleTimes[m_AnimationFrameIndex]; }
                }
                result.ActiveFramesChanged = true;
            }
            result.FrameChanged = true;
            return result;
        }

        if (targetFrame < frameCount)
        {
            m_AnimationFrameIndex = static_cast<uint32_t>(targetFrame);
            result.FrameChanged = true;
        }
        else if (m_MovementTransitionPending)
        {
            const PixelShipGenerator::ShipAnimationType target = m_PendingMovementType;
            m_MovementTransitionPending = false;
            m_PendingMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
            m_AnimationFrameIndex = 0u;
            m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
            m_RuntimeMovementNormalizedTime = 0.0;
            if (target == PixelShipGenerator::ShipAnimationType::IDLE)
            {
                m_RuntimeMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
                m_SelectedAnimationType = PixelShipGenerator::ShipAnimationType::IDLE;
            }
            else
            {
                m_RuntimeMovementType = target;
                m_SelectedAnimationType = target;
                m_MovementAnimationPhase = PixelShipGenerator::ShipMovementAnimationPhase::ENTER;
            }
            generateSelected(ship);
            result.ActiveFramesChanged = true;
            result.FrameChanged = true;
        }
        else
        {
            m_AnimationFrameIndex = 0u;
            m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
            result.ReturnToStatic = true;
        }
        return result;
    }

    void PreviewAnimationSession::resetPlaybackAccumulator() { m_AnimationPlaybackAccumulatorMicroseconds = 0.0; }

    bool PreviewAnimationSession::moveFrame(int32_t delta)
    {
        const auto& frames = getActiveFrames();
        if (frames.empty()) { return false; }
        const int32_t frameCount = static_cast<int32_t>(frames.size());
        int32_t frameIndex = static_cast<int32_t>(m_AnimationFrameIndex) + delta;
        if (frameIndex < 0) { frameIndex = frameCount - 1; }
        if (frameIndex >= frameCount) { frameIndex = 0; }
        m_AnimationFrameIndex = static_cast<uint32_t>(frameIndex);
        return true;
    }

    void PreviewAnimationSession::setFrameIndex(uint32_t frameIndex)
    {
        const auto& frames = getActiveFrames();
        m_AnimationFrameIndex = frames.empty() ? 0u : std::min(frameIndex, static_cast<uint32_t>(frames.size() - 1u));
    }

    const std::vector<double>& PreviewAnimationSession::getActiveNormalizedSampleTimes() const
    {
        if (m_TransientStatePreviewActive) { return m_FiringAnimation.NormalizedSampleTimes; }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE) { return m_IdleAnimation.NormalizedSampleTimes; }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE) { return m_FiringAnimation.NormalizedSampleTimes; }
        const auto* clip = getActiveMovementClip();
        if (clip != nullptr) { return clip->NormalizedSampleTimes; }
        static const std::vector<double> EmptyTimes;
        return EmptyTimes;
    }

    bool PreviewAnimationSession::setNormalizedTime(double normalizedTime)
    {
        const std::vector<double>& sampleTimes = getActiveNormalizedSampleTimes();
        if (sampleTimes.empty()) { return false; }
        const double target = std::clamp(normalizedTime, 0.0, 1.0);
        auto iterator = std::lower_bound(sampleTimes.begin(), sampleTimes.end(), target);
        std::size_t index = iterator == sampleTimes.end() ? sampleTimes.size() - 1u : static_cast<std::size_t>(std::distance(sampleTimes.begin(), iterator));
        if (index > 0u && std::abs(sampleTimes[index - 1u] - target) <= std::abs(sampleTimes[index] - target)) { --index; }
        m_AnimationFrameIndex = static_cast<uint32_t>(index);
        if (!m_TransientStatePreviewActive && m_SelectedAnimationType == m_RuntimeMovementType && isMovementAnimationType(m_SelectedAnimationType) && m_MovementAnimationPhase == PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN)
        {
            m_RuntimeMovementNormalizedTime = sampleTimes[index];
        }
        m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
        return true;
    }

    const PixelShipGenerator::ShipMovementAnimationClip* PreviewAnimationSession::getActiveMovementClip() const
    {
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE || m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE || m_MovementAnimation.Type != m_SelectedAnimationType) { return nullptr; }
        return &PixelShipGenerator::getMovementAnimationClip(m_MovementAnimation, m_MovementAnimationPhase);
    }

    const std::vector<PixelShipGenerator::Image>& PreviewAnimationSession::getActiveFrames() const
    {
        if (m_TransientStatePreviewActive) { return m_StatePreviewFrames; }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE) { return m_IdleAnimation.Frames; }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE) { return m_FiringAnimation.Frames; }
        const auto* clip = getActiveMovementClip();
        if (clip != nullptr) { return clip->Frames; }
        static const std::vector<PixelShipGenerator::Image> EmptyFrames;
        return EmptyFrames;
    }

    double PreviewAnimationSession::getActiveFrameDurationMilliseconds() const
    {
        if (m_TransientStatePreviewActive) { return m_StatePreviewFrameDurationMilliseconds; }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE) { return m_IdleAnimation.FrameDurationMilliseconds; }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE) { return m_FiringAnimation.FrameDurationMilliseconds; }
        const auto* clip = getActiveMovementClip();
        return clip != nullptr ? clip->FrameDurationMilliseconds : 0.0;
    }

    uint64_t PreviewAnimationSession::getActiveSeed() const
    {
        if (m_TransientStatePreviewActive || m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE) { return m_FiringAnimation.Seed; }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE) { return m_IdleAnimation.Seed; }
        return m_MovementAnimation.Seed;
    }

    const PixelShipGenerator::AnimationSamplingPlan& PreviewAnimationSession::getActiveSampling() const
    {
        if (m_TransientStatePreviewActive || m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE) { return m_FiringAnimation.Sampling; }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE) { return m_IdleAnimation.Sampling; }
        const auto* clip = getActiveMovementClip();
        return clip != nullptr ? clip->Sampling : m_IdleAnimation.Sampling;
    }

    uint32_t PreviewAnimationSession::getActiveDurationMilliseconds() const
    {
        if (m_TransientStatePreviewActive || m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE) { return m_FiringAnimation.DurationMilliseconds; }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE) { return m_IdleAnimation.DurationMilliseconds; }
        const auto* clip = getActiveMovementClip();
        return clip != nullptr ? clip->DurationMilliseconds : 0u;
    }

    double PreviewAnimationSession::getActiveNormalizedTime() const
    {
        const std::vector<double>& sampleTimes = getActiveNormalizedSampleTimes();
        return m_AnimationFrameIndex < sampleTimes.size() ? sampleTimes[m_AnimationFrameIndex] : 0.0;
    }

    bool PreviewAnimationSession::isActiveLooping() const
    {
        if (m_TransientStatePreviewActive || m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE) { return false; }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE) { return true; }
        const auto* clip = getActiveMovementClip();
        return clip != nullptr && clip->Looping;
    }

    uint32_t PreviewAnimationSession::getActiveAnimatedComponentCount() const
    {
        return getActiveSampling().ActiveAnimatedComponentCount;
    }

    std::string PreviewAnimationSession::getSemanticPhaseDisplay() const
    {
        if (m_TransientStatePreviewActive || m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE)
        {
            return getFiringPhaseDisplayName(PixelShipGenerator::getFiringAnimationPhase(getActiveNormalizedTime()));
        }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE) { return "LOOP"; }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::MOVE_UP || m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::MOVE_DOWN)
        {
            if (m_MovementAnimationPhase == PixelShipGenerator::ShipMovementAnimationPhase::ENTER) { return "ENTER / ACCEL"; }
            if (m_MovementAnimationPhase == PixelShipGenerator::ShipMovementAnimationPhase::EXIT) { return "EXIT / BRAKE"; }
        }
        return getMovementPhaseDisplayName(m_MovementAnimationPhase);
    }

    std::string PreviewAnimationSession::getEffectDisplay() const
    {
        std::string effects;
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE)
        {
            if (m_IdleAnimationSettings.EngineFlicker) { effects += "E"; }
            if (m_IdleAnimationSettings.LightBlinking) { effects += "L"; }
            if (m_IdleAnimationSettings.MechanicalMicroMovement) { effects += "M"; }
            if (m_IdleAnimationSettings.HoverOffset) { effects += "H"; }
            if (m_IdleAnimationSettings.SmallDetailVariation) { effects += "D"; }
        }
        else if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE)
        {
            effects += "W";
            if (m_FiringAnimation.Diagnostics.PreFireMotion) { effects += "P"; }
        }
        else
        {
            if (m_MovementAnimationSettings.EngineVectoring) { effects += "E"; }
            if (m_MovementAnimationSettings.WeaponStabilization) { effects += "W"; }
            if (m_MovementAnimationSettings.AttachmentArticulation) { effects += "A"; }
        }
        return effects.empty() ? "NONE" : effects;
    }
}
