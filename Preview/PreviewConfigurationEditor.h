#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "ShipGenerationProfile.h"
#include "Validation.h"

#include "ConfigurationEditorControls.h"

namespace PixelShipGeneratorPreview
{
    enum class ConfigurationEditorAction : uint32_t
    {
        APPLY = 0u,
        CANCEL,
        RESET,
        DUPLICATE,
        CONFIGURATION_EDITOR_ACTION_END
    };

    struct ConfigurationEditorEvent
    {
        ConfigurationEditorAction Action = ConfigurationEditorAction::CONFIGURATION_EDITOR_ACTION_END;
    };

    struct ConfigurationEditorActionButton
    {
        ConfigurationEditorAction Action = ConfigurationEditorAction::CONFIGURATION_EDITOR_ACTION_END;
        std::string Label;
        ConfigurationEditorRect Bounds;
        bool Enabled = true;
    };

    struct ConfigurationEditorSectionState
    {
        std::string Label;
        ConfigurationEditorRect HeaderBounds;
        bool Expanded = true;
    };

    class PreviewConfigurationEditor
    {
    public:
        static constexpr std::size_t SectionCount = 6u;

        PreviewConfigurationEditor();

        void openStructuralProfile(std::string name, const PixelShipGenerator::ShipGenerationProfile& profile);
        void close();
        bool isOpen() const;

        void setPanelBounds(const ConfigurationEditorRect& bounds);
        void onMouseMove(float x, float y);
        void onMousePress(float x, float y);
        std::optional<ConfigurationEditorEvent> onMouseRelease(float x, float y);
        void onMouseWheelScrolled(float delta);
        bool onTextEntered(uint32_t unicode);
        ConfigurationEditorEvent createCancelEvent() const;

        void setValidationResult(const PixelShipGenerator::ValidationResult& result);
        const PixelShipGenerator::ValidationResult& getValidationResult() const;

        const std::string& getName() const;
        const PixelShipGenerator::ShipGenerationProfile& getDraftProfile() const;
        const PixelShipGenerator::ShipGenerationProfile& getInitialProfile() const;
        bool hasUnsavedChanges() const;

        float getScrollOffset() const;
        float getMaximumScrollOffset() const;
        ConfigurationEditorRect getPanelBounds() const;
        ConfigurationEditorRect getContentViewport() const;
        const std::array<ConfigurationEditorSectionState, SectionCount>& getSections() const;
        const std::array<ConfigurationEditorActionButton, 4u>& getActionButtons() const;
        const ConfigurationTextField& getNameField() const;
        const ConfigurationIntegerControl& getWeaponChanceControl() const;
        const ConfigurationIntegerControl& getWeaponScaleControl() const;
        const ConfigurationIntegerControl& getWeaponGroupCountControl() const;
        const ConfigurationIntegerControl& getPaletteValueOffsetControl() const;
        const ConfigurationRangeControl& getNoseWidthRangeControl() const;
        const ConfigurationWeightGroupControl& getVisualAnchorWeightsControl() const;

        void setSectionExpanded(std::size_t sectionIndex, bool expanded);

    private:
        void configureControlsFromDraft();
        void syncDraftFromControls();
        void refreshValidation();
        void rebuildLayout();
        bool isWithinContentViewport(float x, float y) const;
        bool activateSectionHeader(float x, float y);
        std::optional<ConfigurationEditorEvent> activateAction(float x, float y);
        void resetDraft();

    private:
        bool m_Open = false;
        std::string m_InitialName;
        PixelShipGenerator::ShipGenerationProfile m_InitialProfile;
        PixelShipGenerator::ShipGenerationProfile m_DraftProfile;
        PixelShipGenerator::ValidationResult m_ValidationResult;
        ConfigurationEditorRect m_PanelBounds = { 880.0f, 0.0f, 760.0f, 1000.0f };
        ConfigurationEditorRect m_ContentViewport;
        float m_ScrollOffset = 0.0f;
        float m_MaximumScrollOffset = 0.0f;
        float m_ContentHeight = 0.0f;

        ConfigurationTextField m_NameField;
        ConfigurationIntegerControl m_WeaponChance;
        ConfigurationIntegerControl m_WeaponScale;
        ConfigurationIntegerControl m_WeaponGroupCount;
        ConfigurationIntegerControl m_PaletteValueOffset;
        ConfigurationRangeControl m_NoseWidthRange;
        ConfigurationWeightGroupControl m_VisualAnchorWeights;
        std::array<ConfigurationEditorSectionState, SectionCount> m_Sections = {};
        std::array<ConfigurationEditorActionButton, 4u> m_ActionButtons = {};
    };
}
