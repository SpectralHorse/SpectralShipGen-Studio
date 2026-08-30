#include "PreviewCommandPanel.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
    constexpr float PanelPadding = 10.0f;
    constexpr float ButtonHeight = 22.0f;
    constexpr float BookmarkButtonHeight = 18.0f;
    constexpr float RowSpacing = 3.0f;
    constexpr float SelectorSpacing = 4.0f;
    constexpr float GroupSpacing = 5.0f;
    constexpr float GroupHeaderHeight = 15.0f;
    constexpr float SelectorLabelWidth = 78.0f;
    constexpr float SelectorButtonWidth = 42.0f;
    constexpr float PairSpacing = 4.0f;

    const char* getRerollStudioDomainLabel(PixelShipGenerator::GenerationDomain domain)
    {
        using PixelShipGenerator::GenerationDomain;
        switch (domain)
        {
        case GenerationDomain::HULL: return "Hull Shape";
        case GenerationDomain::WINGS: return "Wings";
        case GenerationDomain::COCKPIT: return "Cockpit";
        case GenerationDomain::ENGINES: return "Engines";
        case GenerationDomain::HULL_LAYERS: return "Hull Layers";
        case GenerationDomain::MAJOR_FEATURES: return "Major Features";
        case GenerationDomain::MACRO_ASYMMETRY: return "Macro-Asymmetry";
        case GenerationDomain::WEAPONS: return "Weapons";
        case GenerationDomain::ATTACHMENTS: return "Attachments";
        case GenerationDomain::PALETTE: return "Palette";
        case GenerationDomain::DETAILS: return "Details";
        default: return "Unknown";
        }
    }
}

namespace PixelShipGeneratorPreview
{
    PreviewCommandPanel::PreviewCommandPanel()
    {
        buildLayout(PreviewCommandPanelMode::GENERATE);
    }

    void PreviewCommandPanel::updateState(const PreviewCommandPanelState& state)
    {
        if (m_Mode != state.Mode)
        {
            buildLayout(state.Mode);
        }

        for (PreviewCommandPanelButton& button : m_Buttons)
        {
            const std::size_t index = static_cast<std::size_t>(button.Command.Type);
            button.Enabled = index < state.Enabled.size() ? state.Enabled[index] : false;
            button.Active = index < state.Active.size() ? state.Active[index] : false;

            if (button.Command.Type == PreviewCommandType::REROLL_STUDIO_TOGGLE_DOMAIN)
            {
                const std::size_t domainIndex = static_cast<std::size_t>(button.Command.Value);
                const bool selected = domainIndex < state.RerollStudioSelectedDomains.size() && state.RerollStudioSelectedDomains[domainIndex];
                button.Active = selected;
                const PixelShipGenerator::GenerationDomain domain = domainIndex < PixelShipGenerator::GenerationDomainCount ? static_cast<PixelShipGenerator::GenerationDomain>(domainIndex) : PixelShipGenerator::GenerationDomain::GENERATION_DOMAIN_END;
                button.Label = std::string(selected ? "[X] " : "[ ] ") + getRerollStudioDomainLabel(domain);
            }

            if (button.Command.Type == PreviewCommandType::TOGGLE_INSPECTION_PRESENTATION)
            {
                button.Label = state.InspectionPresentationValue.empty() ? "Overlay / Isolate" : state.InspectionPresentationValue;
            }

            if (button.Command.Type == PreviewCommandType::CYCLE_ANIMATION_TYPE && !state.AnimationTypeValue.empty()) { button.Label = "Clip: " + state.AnimationTypeValue; }
            if (button.Command.Type == PreviewCommandType::CYCLE_ANIMATION_BASE_STATE && !state.AnimationBaseStateValue.empty()) { button.Label = "Base: " + state.AnimationBaseStateValue; }
            if (button.Command.Type == PreviewCommandType::CYCLE_MOVEMENT_PHASE && !state.AnimationPhaseValue.empty()) { button.Label = "Phase: " + state.AnimationPhaseValue; }
            if (button.Command.Type == PreviewCommandType::CYCLE_ANIMATION_PLAYBACK_SPEED && !state.AnimationPlaybackSpeedValue.empty()) { button.Label = "Speed: " + state.AnimationPlaybackSpeedValue; }

            if (button.Command.Type == PreviewCommandType::SELECT_RESOLUTION_BOOKMARK)
            {
                const uint32_t slot = button.Command.Value;
                const bool validSlot = slot < state.ResolutionBookmarkCount && slot < state.ResolutionBookmarks.size();
                button.Enabled = button.Enabled && validSlot;
                if (validSlot)
                {
                    const PixelShipGenerator::ShipDimensions& dimensions = state.ResolutionBookmarks[slot];
                    button.Label = std::to_string(dimensions.Width) + "x" + std::to_string(dimensions.Height);
                    button.Active = dimensions == state.CurrentDimensions;
                }
                else
                {
                    button.Label = "-";
                    button.Active = false;
                }
            }
        }

        if (state.Mode == PreviewCommandPanelMode::CALIBRATION)
        {
            if (!m_Selectors.empty()) { m_Selectors[0u].Value = state.CalibrationGroupValue; }
            for (std::size_t index = 0u; index < m_CalibrationSliders.size(); ++index)
            {
                PreviewCommandPanelSlider& slider = m_CalibrationSliders[index];
                const PreviewCalibrationWeightRowState& row = state.CalibrationWeightRows[index];
                slider.Enabled = row.Valid && state.Enabled[static_cast<std::size_t>(PreviewCommandType::CALIBRATION_SET_WEIGHT)];
                slider.Label = row.Valid ? row.Label : "-";
                slider.Maximum = row.Maximum;
                if (!slider.Dragging) { slider.Value = row.CurrentWeight; }
                slider.DetailText = row.Valid ? ("D " + std::to_string(row.DefaultWeight) + "  S " + std::to_string(row.SuggestedWeight) + "  " + std::to_string(row.ProbabilityPercent) + "%") : std::string();
            }
        }
        else if (state.Mode == PreviewCommandPanelMode::GENERATE || state.Mode == PreviewCommandPanelMode::PROFILES || state.Mode == PreviewCommandPanelMode::INSPECT)
        {
            if (state.Mode == PreviewCommandPanelMode::GENERATE && m_Selectors.size() >= 4u)
            {
                m_Selectors[0u].Value = state.StyleValue;
                m_Selectors[1u].Value = state.FactionValue;
                m_Selectors[2u].Value = state.PaletteValue;
                m_Selectors[3u].Value = state.ConfigurationBundleValue;
            }
            else if (state.Mode == PreviewCommandPanelMode::PROFILES && m_Selectors.size() >= 2u)
            {
                m_Selectors[0u].Value = state.ProfilesSectionValue;
                m_Selectors[1u].Value = state.ProfilesItemValue;
            }
            else if (state.Mode == PreviewCommandPanelMode::INSPECT && m_Selectors.size() >= 2u)
            {
                m_Selectors[0u].Value = state.InspectionGroupValue;
                m_Selectors[1u].Value = state.InspectionViewValue;
            }

            if (state.Mode == PreviewCommandPanelMode::GENERATE)
            {
                m_WidthSlider.Enabled = state.Enabled[static_cast<std::size_t>(PreviewCommandType::SET_WIDTH)];
                m_HeightSlider.Enabled = state.Enabled[static_cast<std::size_t>(PreviewCommandType::SET_HEIGHT)];

                if (state.AspectRatioLocked)
                {
                    m_WidthSlider.Minimum = MinimumPreviewResolution;
                    m_WidthSlider.Maximum = MaximumPreviewResolution;
                    m_HeightSlider.Minimum = MinimumPreviewResolution;
                    m_HeightSlider.Maximum = MaximumPreviewResolution;
                }
                else
                {
                    m_WidthSlider.Minimum = getMinimumPreviewWidthForHeight(state.CurrentDimensions.Height);
                    m_WidthSlider.Maximum = getMaximumPreviewWidthForHeight(state.CurrentDimensions.Height);
                    m_HeightSlider.Minimum = getMinimumPreviewHeightForWidth(state.CurrentDimensions.Width);
                    m_HeightSlider.Maximum = getMaximumPreviewHeightForWidth(state.CurrentDimensions.Width);
                }

                if (!m_WidthSlider.Dragging) { m_WidthSlider.Value = state.CurrentDimensions.Width; }
                if (!m_HeightSlider.Dragging) { m_HeightSlider.Value = state.CurrentDimensions.Height; }
            }
        }

        if (state.Mode == PreviewCommandPanelMode::ANIMATION)
        {
            m_AnimationTimelineSlider.Enabled = state.Enabled[static_cast<std::size_t>(PreviewCommandType::SET_ANIMATION_NORMALIZED_TIME)];
            if (!m_AnimationTimelineSlider.Dragging) { m_AnimationTimelineSlider.Value = state.AnimationTimelineValue; }
            m_AnimationTimelineSlider.DetailText = state.AnimationTimelineDetail;
        }

        if (m_HoveredButtonIndex >= 0 && !m_Buttons[static_cast<std::size_t>(m_HoveredButtonIndex)].Enabled) { m_HoveredButtonIndex = -1; }
        if (m_PressedButtonIndex >= 0 && !m_Buttons[static_cast<std::size_t>(m_PressedButtonIndex)].Enabled) { m_PressedButtonIndex = -1; }
    }

    void PreviewCommandPanel::onMouseMove(sf::Vector2f position)
    {
        for (PreviewCommandPanelSlider& slider : m_CalibrationSliders)
        {
            if (slider.Dragging)
            {
                slider.Value = getSliderValueForPosition(slider, position.x);
                m_HoveredButtonIndex = -1;
                return;
            }
        }

        if (m_AnimationTimelineSlider.Dragging)
        {
            m_AnimationTimelineSlider.Value = getSliderValueForPosition(m_AnimationTimelineSlider, position.x);
            m_HoveredButtonIndex = -1;
            return;
        }

        if (m_WidthSlider.Dragging)
        {
            m_WidthSlider.Value = getSliderValueForPosition(m_WidthSlider, position.x);
            m_HoveredButtonIndex = -1;
            return;
        }

        if (m_HeightSlider.Dragging)
        {
            m_HeightSlider.Value = getSliderValueForPosition(m_HeightSlider, position.x);
            m_HoveredButtonIndex = -1;
            return;
        }

        m_HoveredButtonIndex = findButtonIndex(position);
    }

    void PreviewCommandPanel::onMousePress(sf::Vector2f position)
    {
        PreviewCommandPanelSlider* slider = findSlider(position);
        if (slider != nullptr && slider->Enabled)
        {
            slider->Dragging = true;
            slider->Value = getSliderValueForPosition(*slider, position.x);
            m_PressedButtonIndex = -1;
            m_HoveredButtonIndex = -1;
            return;
        }

        const int32_t buttonIndex = findButtonIndex(position);
        if (buttonIndex < 0 || !m_Buttons[static_cast<std::size_t>(buttonIndex)].Enabled)
        {
            m_PressedButtonIndex = -1;
            return;
        }

        m_PressedButtonIndex = buttonIndex;
    }

    std::optional<PreviewCommand> PreviewCommandPanel::onMouseRelease(sf::Vector2f position)
    {
        PreviewCommandPanelSlider* draggingSlider = nullptr;
        for (PreviewCommandPanelSlider& slider : m_CalibrationSliders) { if (slider.Dragging) { draggingSlider = &slider; break; } }
        if (draggingSlider == nullptr && m_AnimationTimelineSlider.Dragging) { draggingSlider = &m_AnimationTimelineSlider; }
        if (draggingSlider == nullptr) { draggingSlider = m_WidthSlider.Dragging ? &m_WidthSlider : m_HeightSlider.Dragging ? &m_HeightSlider : nullptr; }
        if (draggingSlider != nullptr)
        {
            draggingSlider->Value = getSliderValueForPosition(*draggingSlider, position.x);
            draggingSlider->Dragging = false;
            m_PressedButtonIndex = -1;
            const uint32_t commandValue = draggingSlider->ApplyCommand == PreviewCommandType::CALIBRATION_SET_WEIGHT ? ((draggingSlider->ValueTag & 0xFFFFu) << 16u) | (draggingSlider->Value & 0xFFFFu) : draggingSlider->Value;
            return PreviewCommand{ draggingSlider->ApplyCommand, commandValue };
        }

        const int32_t releaseButtonIndex = findButtonIndex(position);
        const int32_t pressedButtonIndex = m_PressedButtonIndex;
        m_PressedButtonIndex = -1;

        if (pressedButtonIndex < 0 || releaseButtonIndex != pressedButtonIndex) { return std::nullopt; }
        const PreviewCommandPanelButton& button = m_Buttons[static_cast<std::size_t>(pressedButtonIndex)];
        return button.Enabled ? std::optional<PreviewCommand>(button.Command) : std::nullopt;
    }

    void PreviewCommandPanel::cancelPress()
    {
        m_PressedButtonIndex = -1;
        m_WidthSlider.Dragging = false;
        m_HeightSlider.Dragging = false;
        m_AnimationTimelineSlider.Dragging = false;
        for (PreviewCommandPanelSlider& slider : m_CalibrationSliders) { slider.Dragging = false; }
    }

    const std::vector<PreviewCommandPanelButton>& PreviewCommandPanel::getButtons() const { return m_Buttons; }
    const std::vector<PreviewCommandPanelSelector>& PreviewCommandPanel::getSelectors() const { return m_Selectors; }
    const PreviewCommandPanelSlider& PreviewCommandPanel::getWidthSlider() const { return m_WidthSlider; }
    const PreviewCommandPanelSlider& PreviewCommandPanel::getHeightSlider() const { return m_HeightSlider; }
    const std::vector<PreviewCommandPanelSlider>& PreviewCommandPanel::getCalibrationSliders() const { return m_CalibrationSliders; }
    const PreviewCommandPanelSlider& PreviewCommandPanel::getAnimationTimelineSlider() const { return m_AnimationTimelineSlider; }
    const std::vector<PreviewCommandPanelGroupHeader>& PreviewCommandPanel::getGroupHeaders() const { return m_GroupHeaders; }
    PreviewCommandPanelMode PreviewCommandPanel::getMode() const { return m_Mode; }
    int32_t PreviewCommandPanel::getHoveredButtonIndex() const { return m_HoveredButtonIndex; }
    int32_t PreviewCommandPanel::getPressedButtonIndex() const { return m_PressedButtonIndex; }
    bool PreviewCommandPanel::isDimensionSliderDragging() const { return m_WidthSlider.Dragging || m_HeightSlider.Dragging || m_AnimationTimelineSlider.Dragging || std::any_of(m_CalibrationSliders.begin(), m_CalibrationSliders.end(), [](const PreviewCommandPanelSlider& slider) { return slider.Dragging; }); }

    const PreviewCommandData* PreviewCommandPanel::getHoveredCommandData() const
    {
        if (m_HoveredButtonIndex < 0) { return nullptr; }
        return &getPreviewCommandData(m_Buttons[static_cast<std::size_t>(m_HoveredButtonIndex)].Command.Type);
    }

    void PreviewCommandPanel::addButton(const PreviewCommand& command, float x, float y, float width, float height, std::string label)
    {
        PreviewCommandPanelButton button;
        button.Command = command;
        button.Bounds = sf::FloatRect(x, y, width, height);
        button.Label = std::move(label);
        m_Buttons.push_back(std::move(button));
    }

    void PreviewCommandPanel::addFullButton(const PreviewCommand& command, float& y)
    {
        addButton(command, static_cast<float>(PreviewCommandPanelX) + PanelPadding, y, static_cast<float>(PreviewCommandPanelWidth) - PanelPadding * 2.0f, ButtonHeight);
        y += ButtonHeight + RowSpacing;
    }

    void PreviewCommandPanel::addGroupHeader(const char* label, float& y)
    {
        if (!m_GroupHeaders.empty()) { y += GroupSpacing; }
        m_GroupHeaders.push_back({ label, sf::Vector2f(static_cast<float>(PreviewCommandPanelX) + PanelPadding, y) });
        y += GroupHeaderHeight;
    }

    void PreviewCommandPanel::addPairButtons(const PreviewCommand& leftCommand, const PreviewCommand& rightCommand, float& y)
    {
        const float x = static_cast<float>(PreviewCommandPanelX) + PanelPadding;
        const float totalWidth = static_cast<float>(PreviewCommandPanelWidth) - PanelPadding * 2.0f;
        const float buttonWidth = (totalWidth - PairSpacing) * 0.5f;
        addButton(leftCommand, x, y, buttonWidth, ButtonHeight);
        addButton(rightCommand, x + buttonWidth + PairSpacing, y, buttonWidth, ButtonHeight);
        y += ButtonHeight + RowSpacing;
    }

    void PreviewCommandPanel::addQuadButtons(const PreviewCommand& first, const PreviewCommand& second, const PreviewCommand& third, const PreviewCommand& fourth, float& y)
    {
        const float x = static_cast<float>(PreviewCommandPanelX) + PanelPadding;
        const float totalWidth = static_cast<float>(PreviewCommandPanelWidth) - PanelPadding * 2.0f;
        const float buttonWidth = (totalWidth - PairSpacing * 3.0f) * 0.25f;
        addButton(first, x, y, buttonWidth, ButtonHeight);
        addButton(second, x + (buttonWidth + PairSpacing), y, buttonWidth, ButtonHeight);
        addButton(third, x + (buttonWidth + PairSpacing) * 2.0f, y, buttonWidth, ButtonHeight);
        addButton(fourth, x + (buttonWidth + PairSpacing) * 3.0f, y, buttonWidth, ButtonHeight);
        y += ButtonHeight + RowSpacing;
    }

    void PreviewCommandPanel::addBookmarkButtons(float& y)
    {
        const float x = static_cast<float>(PreviewCommandPanelX) + PanelPadding;
        const float totalWidth = static_cast<float>(PreviewCommandPanelWidth) - PanelPadding * 2.0f;
        const float buttonWidth = (totalWidth - PairSpacing * static_cast<float>(MaximumResolutionBookmarks - 1u)) / static_cast<float>(MaximumResolutionBookmarks);

        for (uint32_t slot = 0u; slot < MaximumResolutionBookmarks; ++slot)
        {
            addButton({ PreviewCommandType::SELECT_RESOLUTION_BOOKMARK, slot }, x + (buttonWidth + PairSpacing) * static_cast<float>(slot), y, buttonWidth, BookmarkButtonHeight, "-");
        }
        y += BookmarkButtonHeight + RowSpacing;
    }

    void PreviewCommandPanel::addSelector(const char* label, const PreviewCommand& previousCommand, const PreviewCommand& nextCommand, float& y, bool compact)
    {
        const float x = static_cast<float>(PreviewCommandPanelX) + PanelPadding;
        const float totalWidth = static_cast<float>(PreviewCommandPanelWidth) - PanelPadding * 2.0f;
        const float valueX = x + SelectorLabelWidth + SelectorButtonWidth + PairSpacing;
        const float valueWidth = totalWidth - SelectorLabelWidth - SelectorButtonWidth * 2.0f - PairSpacing * 2.0f;
        const float selectorHeight = compact ? 18.0f : ButtonHeight;
        PreviewCommandPanelSelector selector;
        selector.Label = label;
        selector.ValueBounds = sf::FloatRect(valueX, y, valueWidth, selectorHeight);
        selector.PreviousCommand = previousCommand;
        selector.NextCommand = nextCommand;
        m_Selectors.push_back(selector);
        addButton(previousCommand, x + SelectorLabelWidth, y, SelectorButtonWidth, selectorHeight);
        addButton(nextCommand, valueX + valueWidth + PairSpacing, y, SelectorButtonWidth, selectorHeight);
        y += selectorHeight + SelectorSpacing;
    }

    void PreviewCommandPanel::addDimensionSliders(float& y)
    {
        const float x = static_cast<float>(PreviewCommandPanelX) + PanelPadding;
        const float totalWidth = static_cast<float>(PreviewCommandPanelWidth) - PanelPadding * 2.0f;
        const float sliderWidth = (totalWidth - PairSpacing) * 0.5f;

        const auto configure = [&](PreviewCommandPanelSlider& slider, const char* label, PreviewCommandType applyCommand, float sliderX)
            {
                slider.Label = label;
                slider.ApplyCommand = applyCommand;
                slider.ValueBounds = sf::FloatRect(sliderX, y, sliderWidth, ButtonHeight);
                slider.TrackBounds = sf::FloatRect(sliderX + 62.0f, y + 14.0f, std::max(1.0f, sliderWidth - 70.0f), 4.0f);
            };

        configure(m_WidthSlider, "WIDTH", PreviewCommandType::SET_WIDTH, x);
        configure(m_HeightSlider, "HEIGHT", PreviewCommandType::SET_HEIGHT, x + sliderWidth + PairSpacing);
        y += ButtonHeight + RowSpacing;
    }

    void PreviewCommandPanel::addDimensionControlButtons(float& y)
    {
        const float x = static_cast<float>(PreviewCommandPanelX) + PanelPadding;
        const float totalWidth = static_cast<float>(PreviewCommandPanelWidth) - PanelPadding * 2.0f;
        constexpr uint32_t ButtonCount = 5u;
        const float buttonWidth = (totalWidth - PairSpacing * static_cast<float>(ButtonCount - 1u)) / static_cast<float>(ButtonCount);
        addButton({ PreviewCommandType::PREVIOUS_RESOLUTION, 0u }, x, y, buttonWidth, ButtonHeight, "<");
        addButton({ PreviewCommandType::NEXT_RESOLUTION, 0u }, x + (buttonWidth + PairSpacing), y, buttonWidth, ButtonHeight, ">");
        addButton({ PreviewCommandType::TOGGLE_ASPECT_RATIO_LOCK, 0u }, x + (buttonWidth + PairSpacing) * 2.0f, y, buttonWidth, ButtonHeight, "1:1");
        addButton({ PreviewCommandType::ADD_RESOLUTION_BOOKMARK, 0u }, x + (buttonWidth + PairSpacing) * 3.0f, y, buttonWidth, ButtonHeight, "+BM");
        addButton({ PreviewCommandType::REMOVE_RESOLUTION_BOOKMARK, 0u }, x + (buttonWidth + PairSpacing) * 4.0f, y, buttonWidth, ButtonHeight, "-BM");
        y += ButtonHeight + RowSpacing;
    }

    void PreviewCommandPanel::addAnimationTimelineSlider(float& y)
    {
        const float x = static_cast<float>(PreviewCommandPanelX) + PanelPadding;
        const float width = static_cast<float>(PreviewCommandPanelWidth) - PanelPadding * 2.0f;
        m_AnimationTimelineSlider.Label = "TIME";
        m_AnimationTimelineSlider.ApplyCommand = PreviewCommandType::SET_ANIMATION_NORMALIZED_TIME;
        m_AnimationTimelineSlider.Minimum = 0u;
        m_AnimationTimelineSlider.Maximum = 1000u;
        m_AnimationTimelineSlider.Step = 1u;
        m_AnimationTimelineSlider.ValueBounds = sf::FloatRect(x, y, width, ButtonHeight);
        m_AnimationTimelineSlider.TrackBounds = sf::FloatRect(x + 52.0f, y + 14.0f, std::max(1.0f, width - 112.0f), 4.0f);
        y += ButtonHeight + RowSpacing;
    }

    void PreviewCommandPanel::addCalibrationWeightSliders(float& y)
    {
        const float x = static_cast<float>(PreviewCommandPanelX) + PanelPadding;
        const float width = static_cast<float>(PreviewCommandPanelWidth) - PanelPadding * 2.0f;
        m_CalibrationSliders.clear();
        m_CalibrationSliders.resize(6u);
        for (uint32_t index = 0u; index < m_CalibrationSliders.size(); ++index)
        {
            PreviewCommandPanelSlider& slider = m_CalibrationSliders[index];
            slider.ApplyCommand = PreviewCommandType::CALIBRATION_SET_WEIGHT;
            slider.Minimum = 0u;
            slider.Maximum = 300u;
            slider.Step = 1u;
            slider.ValueTag = index;
            slider.ValueBounds = sf::FloatRect(x, y, width, 34.0f);
            slider.TrackBounds = sf::FloatRect(x + 8.0f, y + 25.0f, width - 16.0f, 4.0f);
            y += 37.0f;
        }
    }

    void PreviewCommandPanel::addRerollStudioDomainButton(PixelShipGenerator::GenerationDomain domain, float x, float y, float width, float height)
    {
        addButton({ PreviewCommandType::REROLL_STUDIO_TOGGLE_DOMAIN, static_cast<uint32_t>(domain) }, x, y, width, height, std::string("[ ] ") + getRerollStudioDomainLabel(domain));
    }

    void PreviewCommandPanel::buildLayout(PreviewCommandPanelMode mode)
    {
        m_Mode = mode;
        m_Buttons.clear();
        m_Selectors.clear();
        m_GroupHeaders.clear();
        float y = static_cast<float>(PreviewWorkspaceNavigationHeight) + 30.0f;

        if (mode == PreviewCommandPanelMode::CALIBRATION)
        {
            addGroupHeader("CALIBRATION LAB", y);
            addSelector("GROUP", { PreviewCommandType::CALIBRATION_PREVIOUS_GROUP, 0u }, { PreviewCommandType::CALIBRATION_NEXT_GROUP, 0u }, y);
            addCalibrationWeightSliders(y);
            addFullButton({ PreviewCommandType::CALIBRATION_GENERATE_PAIR, 0u }, y);
            addQuadButtons({ PreviewCommandType::CALIBRATION_PREFER_LEFT, 0u }, { PreviewCommandType::CALIBRATION_NO_PREFERENCE, 0u }, { PreviewCommandType::CALIBRATION_PREFER_RIGHT, 0u }, { PreviewCommandType::CALIBRATION_SKIP, 0u }, y);
            addPairButtons({ PreviewCommandType::CALIBRATION_RESET_GROUP, 0u }, { PreviewCommandType::CALIBRATION_RESET_ALL, 0u }, y);
            addPairButtons({ PreviewCommandType::CALIBRATION_APPLY_SUGGESTED, 0u }, { PreviewCommandType::CALIBRATION_TOGGLE_SHOW_VALUES, 0u }, y);
            addPairButtons({ PreviewCommandType::CALIBRATION_TOGGLE_CONTEXT_FILTER, 0u }, { PreviewCommandType::CALIBRATION_RUN_OBJECTIVE_BATCH, 0u }, y);
            addPairButtons({ PreviewCommandType::CALIBRATION_SAVE_SESSION, 0u }, { PreviewCommandType::CALIBRATION_LOAD_SESSION, 0u }, y);
            addPairButtons({ PreviewCommandType::CALIBRATION_EXPORT_REPORT, 0u }, { PreviewCommandType::CALIBRATION_EXPORT_TUNING_PROFILE, 0u }, y);
            addFullButton({ PreviewCommandType::CALIBRATION_EXIT, 0u }, y);
            return;
        }

        if (mode == PreviewCommandPanelMode::REROLL_STUDIO)
        {
            m_CalibrationSliders.clear();
            const float x = static_cast<float>(PreviewCommandPanelX) + PanelPadding;
            const float totalWidth = static_cast<float>(PreviewCommandPanelWidth) - PanelPadding * 2.0f;
            const float pairWidth = (totalWidth - PairSpacing) * 0.5f;

            addGroupHeader("ATTRIBUTE REROLL STUDIO", y);
            addGroupHeader("STRUCTURE", y);
            addRerollStudioDomainButton(PixelShipGenerator::GenerationDomain::HULL, x, y, pairWidth, ButtonHeight);
            addRerollStudioDomainButton(PixelShipGenerator::GenerationDomain::WINGS, x + pairWidth + PairSpacing, y, pairWidth, ButtonHeight);
            y += ButtonHeight + RowSpacing;
            addRerollStudioDomainButton(PixelShipGenerator::GenerationDomain::COCKPIT, x, y, pairWidth, ButtonHeight);
            addRerollStudioDomainButton(PixelShipGenerator::GenerationDomain::ENGINES, x + pairWidth + PairSpacing, y, pairWidth, ButtonHeight);
            y += ButtonHeight + RowSpacing;
            addRerollStudioDomainButton(PixelShipGenerator::GenerationDomain::HULL_LAYERS, x, y, pairWidth, ButtonHeight);
            addRerollStudioDomainButton(PixelShipGenerator::GenerationDomain::MAJOR_FEATURES, x + pairWidth + PairSpacing, y, pairWidth, ButtonHeight);
            y += ButtonHeight + RowSpacing;
            addRerollStudioDomainButton(PixelShipGenerator::GenerationDomain::MACRO_ASYMMETRY, x, y, totalWidth, ButtonHeight);
            y += ButtonHeight + RowSpacing;

            addGroupHeader("EQUIPMENT", y);
            addRerollStudioDomainButton(PixelShipGenerator::GenerationDomain::WEAPONS, x, y, pairWidth, ButtonHeight);
            addRerollStudioDomainButton(PixelShipGenerator::GenerationDomain::ATTACHMENTS, x + pairWidth + PairSpacing, y, pairWidth, ButtonHeight);
            y += ButtonHeight + RowSpacing;

            addGroupHeader("APPEARANCE", y);
            addRerollStudioDomainButton(PixelShipGenerator::GenerationDomain::PALETTE, x, y, pairWidth, ButtonHeight);
            addRerollStudioDomainButton(PixelShipGenerator::GenerationDomain::DETAILS, x + pairWidth + PairSpacing, y, pairWidth, ButtonHeight);
            y += ButtonHeight + RowSpacing;

            addGroupHeader("SELECTION", y);
            addPairButtons({ PreviewCommandType::REROLL_STUDIO_SELECT_ALL, 0u }, { PreviewCommandType::REROLL_STUDIO_CLEAR, 0u }, y);
            addPairButtons({ PreviewCommandType::REROLL_STUDIO_SELECT_STRUCTURE, 0u }, { PreviewCommandType::REROLL_STUDIO_SELECT_APPEARANCE, 0u }, y);

            addGroupHeader("CHANNEL LOCKS", y);
            addPairButtons({ PreviewCommandType::TOGGLE_STRUCTURE_LOCK, 0u }, { PreviewCommandType::TOGGLE_PALETTE_LOCK, 0u }, y);
            addPairButtons({ PreviewCommandType::TOGGLE_DETAILS_LOCK, 0u }, { PreviewCommandType::TOGGLE_ATTACHMENTS_LOCK, 0u }, y);

            addGroupHeader("ACTIONS", y);
            addFullButton({ PreviewCommandType::REROLL_STUDIO_GENERATE_CANDIDATE, 0u }, y);
            addPairButtons({ PreviewCommandType::REROLL_STUDIO_ACCEPT, 0u }, { PreviewCommandType::REROLL_STUDIO_CANCEL, 0u }, y);
            return;
        }

        m_CalibrationSliders.clear();

        if (mode == PreviewCommandPanelMode::PROFILES)
        {
            addGroupHeader("PROFILE LIBRARY", y);
            addSelector("TYPE", { PreviewCommandType::PROFILES_PREVIOUS_SECTION, 0u }, { PreviewCommandType::PROFILES_NEXT_SECTION, 0u }, y);
            addSelector("SELECTED", { PreviewCommandType::PROFILES_PREVIOUS_ITEM, 0u }, { PreviewCommandType::PROFILES_NEXT_ITEM, 0u }, y);
            addGroupHeader("ACTIONS", y);
            addPairButtons({ PreviewCommandType::PROFILES_NEW_DEFAULT, 0u }, { PreviewCommandType::PROFILES_EDIT_SELECTED, 0u }, y);
            addPairButtons({ PreviewCommandType::PROFILES_DUPLICATE_SELECTED, 0u }, { PreviewCommandType::PROFILES_DELETE_SELECTED, 0u }, y);
            addPairButtons({ PreviewCommandType::PROFILES_IMPORT_SELECTED, 0u }, { PreviewCommandType::PROFILES_EXPORT_SELECTED, 0u }, y);
            addFullButton({ PreviewCommandType::PROFILES_USE_SELECTED, 0u }, y);
            return;
        }

        if (mode == PreviewCommandPanelMode::INSPECT)
        {
            addGroupHeader("SEMANTIC VIEW", y);
            addSelector("GROUP", { PreviewCommandType::INSPECTION_PREVIOUS_GROUP, 0u }, { PreviewCommandType::INSPECTION_NEXT_GROUP, 0u }, y);
            addSelector("VIEW", { PreviewCommandType::INSPECTION_PREVIOUS_VIEW, 0u }, { PreviewCommandType::INSPECTION_NEXT_VIEW, 0u }, y);
            addFullButton({ PreviewCommandType::TOGGLE_INSPECTION_PRESENTATION, 0u }, y);

            addGroupHeader("GENERATION", y);
            addFullButton({ PreviewCommandType::TOGGLE_GENERATION_STAGE_VIEW, 0u }, y);
            addPairButtons({ PreviewCommandType::PREVIOUS_GENERATION_STAGE, 0u }, { PreviewCommandType::NEXT_GENERATION_STAGE, 0u }, y);
            addPairButtons({ PreviewCommandType::TOGGLE_GENERATION_INSPECTOR, 0u }, { PreviewCommandType::TOGGLE_PALETTE_INSPECTOR, 0u }, y);

            addGroupHeader("REFERENCE", y);
            addPairButtons({ PreviewCommandType::PIN_CURRENT, 0u }, { PreviewCommandType::CLEAR_PIN, 0u }, y);
            addFullButton({ PreviewCommandType::TOGGLE_COMPARISON, 0u }, y);

            addGroupHeader("WORKFLOW", y);
            addPairButtons({ PreviewCommandType::OPEN_GENERATE_WORKSPACE, 0u }, { PreviewCommandType::OPEN_REROLL_STUDIO, 0u }, y);
            addPairButtons({ PreviewCommandType::OPEN_ANIMATION_WORKSPACE, 0u }, { PreviewCommandType::ADD_CURRENT_TO_FAVORITES, 0u }, y);

            addGroupHeader("FILES", y);
            addFullButton({ PreviewCommandType::SAVE_CURRENT, 0u }, y);
            return;
        }

        if (mode == PreviewCommandPanelMode::FAVORITES)
        {
            addGroupHeader("COLLECTION", y);
            addQuadButtons({ PreviewCommandType::FAVORITES_LEFT, 0u }, { PreviewCommandType::FAVORITES_UP, 0u }, { PreviewCommandType::FAVORITES_DOWN, 0u }, { PreviewCommandType::FAVORITES_RIGHT, 0u }, y);
            addFullButton({ PreviewCommandType::SELECT_FAVORITE, 0u }, y);
            addPairButtons({ PreviewCommandType::OPEN_FAVORITE_INSPECT, 0u }, { PreviewCommandType::OPEN_FAVORITE_ANIMATION, 0u }, y);
            addFullButton({ PreviewCommandType::OPEN_FAVORITE_REROLL, 0u }, y);
            addGroupHeader("EXPORT", y);
            addPairButtons({ PreviewCommandType::EXPORT_FAVORITE_IMAGE, 0u }, { PreviewCommandType::EXPORT_RECIPE, 0u }, y);
            addGroupHeader("MANAGE", y);
            addFullButton({ PreviewCommandType::REMOVE_SELECTED_FAVORITE, 0u }, y);
            return;
        }

        if (mode == PreviewCommandPanelMode::ANIMATION)
        {
            addGroupHeader("ANIMATION LAB", y);
            addPairButtons({ PreviewCommandType::CYCLE_ANIMATION_TYPE, 0u }, { PreviewCommandType::CYCLE_MOVEMENT_PHASE, 0u }, y);
            addFullButton({ PreviewCommandType::CYCLE_FIRING_TARGET, 0u }, y);
            addGroupHeader("STATE COMPOSITION", y);
            addFullButton({ PreviewCommandType::CYCLE_ANIMATION_BASE_STATE, 0u }, y);
            addPairButtons({ PreviewCommandType::TRIGGER_ANIMATION_FIRE, 0u }, { PreviewCommandType::RETURN_ANIMATION_TO_IDLE, 0u }, y);
            addGroupHeader("PLAYBACK", y);
            addPairButtons({ PreviewCommandType::TOGGLE_ANIMATION, 0u }, { PreviewCommandType::CYCLE_ANIMATION_PLAYBACK_SPEED, 0u }, y);
            addAnimationTimelineSlider(y);
            addPairButtons({ PreviewCommandType::PREVIOUS_FRAME, 0u }, { PreviewCommandType::NEXT_FRAME, 0u }, y);
            addGroupHeader("FILES", y);
            addPairButtons({ PreviewCommandType::SAVE_CURRENT, 0u }, { PreviewCommandType::SAVE_SPRITESHEET, 0u }, y);
            addFullButton({ PreviewCommandType::OPEN_GENERATE_WORKSPACE, 0u }, y);
            return;
        }

        addGroupHeader("GENERATION", y);
        addPairButtons({ PreviewCommandType::GENERATE_NEW, 0u }, { PreviewCommandType::GENERATE_FROM_MASTER_SEED, 0u }, y);
        addPairButtons({ PreviewCommandType::TOGGLE_ATTACHMENTS_ENABLED, 0u }, { PreviewCommandType::OPEN_CALIBRATION_LAB, 0u }, y);

        addGroupHeader("IDLE PREVIEW", y);
        addFullButton({ PreviewCommandType::TOGGLE_ANIMATION, 0u }, y);

        addGroupHeader("HISTORY / GALLERY", y);
        addPairButtons({ PreviewCommandType::PREVIOUS_HISTORY, 0u }, { PreviewCommandType::NEXT_HISTORY, 0u }, y);
        addPairButtons({ PreviewCommandType::OPEN_GALLERY, 0u }, { PreviewCommandType::OPEN_GALLERY_FROM_SEED, 0u }, y);
        addQuadButtons({ PreviewCommandType::GALLERY_LEFT, 0u }, { PreviewCommandType::GALLERY_UP, 0u }, { PreviewCommandType::GALLERY_DOWN, 0u }, { PreviewCommandType::GALLERY_RIGHT, 0u }, y);
        addFullButton({ PreviewCommandType::SELECT_GALLERY_CANDIDATE, 0u }, y);

        addGroupHeader("CONFIGURATION", y);
        addSelector("PROFILE", { PreviewCommandType::PREVIOUS_STYLE, 0u }, { PreviewCommandType::NEXT_STYLE, 0u }, y, true);
        addSelector("FACTION", { PreviewCommandType::PREVIOUS_FACTION, 0u }, { PreviewCommandType::NEXT_FACTION, 0u }, y, true);
        addSelector("PALETTE", { PreviewCommandType::PREVIOUS_PALETTE, 0u }, { PreviewCommandType::NEXT_PALETTE, 0u }, y, true);
        addSelector("FULL CFG", { PreviewCommandType::PREVIOUS_CONFIGURATION_BUNDLE, 0u }, { PreviewCommandType::NEXT_CONFIGURATION_BUNDLE, 0u }, y, true);
        addDimensionSliders(y);
        addDimensionControlButtons(y);
        addBookmarkButtons(y);

        addGroupHeader("SAVE OUTPUT", y);
        addPairButtons({ PreviewCommandType::SAVE_CURRENT, 0u }, { PreviewCommandType::SAVE_SPRITESHEET, 0u }, y);

        addGroupHeader("RECIPE", y);
        addPairButtons({ PreviewCommandType::IMPORT_RECIPE, 0u }, { PreviewCommandType::EXPORT_RECIPE, 0u }, y);

        addGroupHeader("COLLECTION", y);
        addPairButtons({ PreviewCommandType::ADD_CURRENT_TO_FAVORITES, 0u }, { PreviewCommandType::REMOVE_CURRENT_FROM_FAVORITES, 0u }, y);
    }

    int32_t PreviewCommandPanel::findButtonIndex(sf::Vector2f position) const
    {
        for (std::size_t index = 0u; index < m_Buttons.size(); ++index)
        {
            if (m_Buttons[index].Bounds.contains(position)) { return static_cast<int32_t>(index); }
        }
        return -1;
    }

    PreviewCommandPanelSlider* PreviewCommandPanel::findSlider(sf::Vector2f position)
    {
        if (m_Mode == PreviewCommandPanelMode::CALIBRATION)
        {
            for (PreviewCommandPanelSlider& slider : m_CalibrationSliders) { if (slider.ValueBounds.contains(position)) { return &slider; } }
            return nullptr;
        }
        if (m_Mode == PreviewCommandPanelMode::ANIMATION) { return m_AnimationTimelineSlider.ValueBounds.contains(position) ? &m_AnimationTimelineSlider : nullptr; }
        if (m_Mode != PreviewCommandPanelMode::GENERATE) { return nullptr; }
        if (m_WidthSlider.ValueBounds.contains(position)) { return &m_WidthSlider; }
        if (m_HeightSlider.ValueBounds.contains(position)) { return &m_HeightSlider; }
        return nullptr;
    }

    const PreviewCommandPanelSlider* PreviewCommandPanel::findSlider(sf::Vector2f position) const
    {
        if (m_Mode == PreviewCommandPanelMode::CALIBRATION)
        {
            for (const PreviewCommandPanelSlider& slider : m_CalibrationSliders) { if (slider.ValueBounds.contains(position)) { return &slider; } }
            return nullptr;
        }
        if (m_Mode == PreviewCommandPanelMode::ANIMATION) { return m_AnimationTimelineSlider.ValueBounds.contains(position) ? &m_AnimationTimelineSlider : nullptr; }
        if (m_Mode != PreviewCommandPanelMode::GENERATE) { return nullptr; }
        if (m_WidthSlider.ValueBounds.contains(position)) { return &m_WidthSlider; }
        if (m_HeightSlider.ValueBounds.contains(position)) { return &m_HeightSlider; }
        return nullptr;
    }

    uint32_t PreviewCommandPanel::getSliderValueForPosition(const PreviewCommandPanelSlider& slider, float x) const
    {
        const float left = slider.TrackBounds.left;
        const float width = std::max(1.0f, slider.TrackBounds.width);
        const float normalized = std::clamp((x - left) / width, 0.0f, 1.0f);
        const float range = static_cast<float>(slider.Maximum - slider.Minimum);
        const uint32_t raw = slider.Minimum + static_cast<uint32_t>(std::lround(normalized * range));
        const uint32_t step = std::max(1u, slider.Step);
        const uint32_t snapped = slider.Minimum + ((raw - slider.Minimum + step / 2u) / step) * step;
        if (slider.ApplyCommand == PreviewCommandType::SET_WIDTH || slider.ApplyCommand == PreviewCommandType::SET_HEIGHT)
        {
            return std::clamp(clampPreviewDimensionValue(snapped), slider.Minimum, slider.Maximum);
        }
        return std::clamp(snapped, slider.Minimum, slider.Maximum);
    }
}
