#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "ShipGenerationProfile.h"
#include "Validation.h"

#include "ConfigurationEditorControls.h"
#include "ShipGenerationProfileEditorBindings.h"

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
        const ConfigurationTextField& getNameField() const;
        const std::vector<StructuralProfileEditorSection>& getProfileSections() const;
        const ConfigurationEditorSectionState& getValidationSection() const;
        const std::array<ConfigurationEditorActionButton, 4u>& getActionButtons() const;
        std::size_t getBoundValueCount() const;

        const StructuralIntegerFieldBinding* findIntegerField(std::string_view path) const;
        StructuralIntegerFieldBinding* findIntegerField(std::string_view path);
        const StructuralRangeFieldBinding* findRangeField(std::string_view path) const;
        StructuralRangeFieldBinding* findRangeField(std::string_view path);
        const StructuralToggleFieldBinding* findToggleField(std::string_view path) const;
        StructuralToggleFieldBinding* findToggleField(std::string_view path);
        const StructuralChoiceFieldBinding* findChoiceField(std::string_view path) const;
        StructuralChoiceFieldBinding* findChoiceField(std::string_view path);
        const StructuralWeightGroupBinding* findWeightGroup(std::string_view path) const;
        StructuralWeightGroupBinding* findWeightGroup(std::string_view path);

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
        void cancelDragging();

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
        ShipGenerationProfileEditorBindings m_ProfileBindings;
        ConfigurationEditorSectionState m_ValidationSection = { "VALIDATION", {}, true };
        std::array<ConfigurationEditorActionButton, 4u> m_ActionButtons = {};
    };
}
