#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "GenerationCalibration.h"
#include "PreviewCommand.h"
#include "PreviewResolution.h"
#include "PreviewState.h"

namespace PixelShipGeneratorPreview
{
    enum class PreviewCommandPanelMode : uint32_t
    {
        GENERATE = 0u,
        PROFILES,
        INSPECT,
        FAVORITES,
        ANIMATION,
        CALIBRATION,
        REROLL_STUDIO
    };

    struct PreviewCommandPanelButton
    {
        PreviewCommand Command;
        sf::FloatRect Bounds;
        std::string Label;
        bool Enabled = true;
        bool Active = false;
    };

    struct PreviewCommandPanelSelector
    {
        std::string Label;
        std::string Value;
        sf::FloatRect ValueBounds;
        PreviewCommand PreviousCommand;
        PreviewCommand NextCommand;
    };

    struct PreviewCommandPanelSlider
    {
        std::string Label;
        sf::FloatRect ValueBounds;
        sf::FloatRect TrackBounds;
        PreviewCommandType ApplyCommand = PreviewCommandType::PREVIEW_COMMAND_TYPE_END;
        uint32_t Minimum = MinimumPreviewResolution;
        uint32_t Maximum = MaximumPreviewResolution;
        uint32_t Step = PreviewResolutionStep;
        uint32_t Value = 64u;
        uint32_t ValueTag = 0u;
        std::string DetailText;
        bool Enabled = true;
        bool Dragging = false;
    };


    struct PreviewCalibrationWeightRowState
    {
        std::string Label;
        uint32_t CurrentWeight = 0u;
        uint32_t DefaultWeight = 0u;
        uint32_t SuggestedWeight = 0u;
        uint32_t ProbabilityPercent = 0u;
        uint32_t Maximum = 300u;
        bool Valid = false;
    };

    struct PreviewCommandPanelGroupHeader
    {
        std::string Label;
        sf::Vector2f Position;
    };

    struct PreviewCommandPanelState
    {
        std::array<bool, static_cast<std::size_t>(PreviewCommandType::PREVIEW_COMMAND_TYPE_END)> Enabled = {};
        std::array<bool, static_cast<std::size_t>(PreviewCommandType::PREVIEW_COMMAND_TYPE_END)> Active = {};
        std::array<PixelShipGenerator::ShipDimensions, MaximumResolutionBookmarks> ResolutionBookmarks = {};
        uint32_t ResolutionBookmarkCount = 0u;
        PixelShipGenerator::ShipDimensions CurrentDimensions;
        bool AspectRatioLocked = true;
        std::string StyleValue;
        std::string FactionValue;
        std::string PaletteValue;
        std::string ConfigurationBundleValue;
        std::string ProfilesSectionValue;
        std::string ProfilesItemValue;
        std::string InspectionGroupValue;
        std::string InspectionViewValue;
        std::string InspectionPresentationValue;
        PreviewCommandPanelMode Mode = PreviewCommandPanelMode::GENERATE;
        std::array<bool, PixelShipGenerator::GenerationDomainCount> RerollStudioSelectedDomains = {};
        std::string CalibrationGroupValue;
        std::string CalibrationEvidenceValue;
        std::array<PreviewCalibrationWeightRowState, 6u> CalibrationWeightRows = {};
    };

    class PreviewCommandPanel
    {
    public:
        PreviewCommandPanel();

        void updateState(const PreviewCommandPanelState& state);
        void onMouseMove(sf::Vector2f position);
        void onMousePress(sf::Vector2f position);
        std::optional<PreviewCommand> onMouseRelease(sf::Vector2f position);
        void cancelPress();

        const std::vector<PreviewCommandPanelButton>& getButtons() const;
        const std::vector<PreviewCommandPanelSelector>& getSelectors() const;
        const PreviewCommandPanelSlider& getWidthSlider() const;
        const PreviewCommandPanelSlider& getHeightSlider() const;
        const std::vector<PreviewCommandPanelSlider>& getCalibrationSliders() const;
        const std::vector<PreviewCommandPanelGroupHeader>& getGroupHeaders() const;
        PreviewCommandPanelMode getMode() const;
        int32_t getHoveredButtonIndex() const;
        int32_t getPressedButtonIndex() const;
        bool isDimensionSliderDragging() const;
        const PreviewCommandData* getHoveredCommandData() const;

    private:
        void addButton(const PreviewCommand& command, float x, float y, float width, float height, std::string label = {});
        void addFullButton(const PreviewCommand& command, float& y);
        void addGroupHeader(const char* label, float& y);
        void addPairButtons(const PreviewCommand& leftCommand, const PreviewCommand& rightCommand, float& y);
        void addQuadButtons(const PreviewCommand& first, const PreviewCommand& second, const PreviewCommand& third, const PreviewCommand& fourth, float& y);
        void addBookmarkButtons(float& y);
        void addSelector(const char* label, const PreviewCommand& previousCommand, const PreviewCommand& nextCommand, float& y, bool compact = false);
        void addDimensionSliders(float& y);
        void addDimensionControlButtons(float& y);
        void addCalibrationWeightSliders(float& y);
        void addRerollStudioDomainButton(PixelShipGenerator::GenerationDomain domain, float x, float y, float width, float height);
        void buildLayout(PreviewCommandPanelMode mode = PreviewCommandPanelMode::GENERATE);
        int32_t findButtonIndex(sf::Vector2f position) const;
        PreviewCommandPanelSlider* findSlider(sf::Vector2f position);
        const PreviewCommandPanelSlider* findSlider(sf::Vector2f position) const;
        uint32_t getSliderValueForPosition(const PreviewCommandPanelSlider& slider, float x) const;

    private:
        std::vector<PreviewCommandPanelButton> m_Buttons;
        std::vector<PreviewCommandPanelSelector> m_Selectors;
        PreviewCommandPanelSlider m_WidthSlider;
        PreviewCommandPanelSlider m_HeightSlider;
        std::vector<PreviewCommandPanelSlider> m_CalibrationSliders;
        std::vector<PreviewCommandPanelGroupHeader> m_GroupHeaders;
        PreviewCommandPanelMode m_Mode = PreviewCommandPanelMode::GENERATE;
        int32_t m_HoveredButtonIndex = -1;
        int32_t m_PressedButtonIndex = -1;
    };
}
