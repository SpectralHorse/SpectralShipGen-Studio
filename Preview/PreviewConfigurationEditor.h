#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "ShipFactionProfile.h"
#include "ShipGenerationProfile.h"
#include "ShipPaletteConfiguration.h"
#include "Validation.h"

#include "ConfigurationBundle.h"
#include "ConfigurationEditorControls.h"
#include "ShipFactionProfileEditorBindings.h"
#include "ShipGenerationProfileEditorBindings.h"
#include "ShipPaletteConfigurationEditorBindings.h"

namespace PixelShipGeneratorPreview
{
    enum class ConfigurationEditorProfileKind : uint32_t
    {
        STRUCTURAL = 0u,
        FACTION,
        PALETTE,
        FULL_CONFIGURATION,
        CONFIGURATION_EDITOR_PROFILE_KIND_END
    };

    enum class ConfigurationEditorAction : uint32_t
    {
        APPLY = 0u,
        CANCEL,
        RESET,
        DUPLICATE,
        DELETE_PRESET,
        EXPORT_PRESET,
        IMPORT_PRESET,
        REPLACE_BUNDLE_STRUCTURAL,
        REPLACE_BUNDLE_FACTION,
        REPLACE_BUNDLE_PALETTE,
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


    struct ConfigurationBundleComponentControl
    {
        std::string Label;
        std::string Value;
        ConfigurationEditorRect RowBounds;
        ConfigurationEditorRect ReplaceBounds;
        ConfigurationEditorAction Action = ConfigurationEditorAction::CONFIGURATION_EDITOR_ACTION_END;
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
        void openFactionProfile(std::string name, const PixelShipGenerator::ShipFactionProfile& profile);
        void openPaletteConfiguration(std::string name, const PixelShipGenerator::ShipPaletteConfiguration& configuration);
        void openConfigurationBundle(std::string name, const ConfigurationBundle& bundle);
        void close();
        bool isOpen() const;
        ConfigurationEditorProfileKind getProfileKind() const;

        void setPanelBounds(const ConfigurationEditorRect& bounds);
        void onMouseMove(float x, float y);
        void onMousePress(float x, float y);
        std::optional<ConfigurationEditorEvent> onMouseRelease(float x, float y);
        void onMouseWheelScrolled(float delta);
        bool onTextEntered(uint32_t unicode);
        bool hasKeyboardFocus() const;
        void releaseKeyboardFocus();
        ConfigurationEditorEvent createCancelEvent() const;

        void setValidationResult(const PixelShipGenerator::ValidationResult& result);
        void setExistingCustomPreset(bool existingCustomPreset);
        const PixelShipGenerator::ValidationResult& getValidationResult() const;

        const std::string& getName() const;
        const PixelShipGenerator::ShipGenerationProfile& getDraftProfile() const;
        const PixelShipGenerator::ShipGenerationProfile& getInitialProfile() const;
        const PixelShipGenerator::ShipFactionProfile& getDraftFactionProfile() const;
        const PixelShipGenerator::ShipFactionProfile& getInitialFactionProfile() const;
        const PixelShipGenerator::ShipPaletteConfiguration& getDraftPaletteConfiguration() const;
        const PixelShipGenerator::ShipPaletteConfiguration& getInitialPaletteConfiguration() const;
        const ConfigurationBundle& getDraftConfigurationBundle() const;
        const ConfigurationBundle& getInitialConfigurationBundle() const;
        void replaceBundleStructural(std::string displayName, const PixelShipGenerator::ShipGenerationProfile& profile);
        void replaceBundleFaction(std::string displayName, const PixelShipGenerator::ShipFactionProfile& profile);
        void replaceBundlePalette(std::string displayName, const PixelShipGenerator::ShipPaletteConfiguration& configuration);
        bool hasUnsavedChanges() const;

        float getScrollOffset() const;
        float getMaximumScrollOffset() const;
        ConfigurationEditorRect getPanelBounds() const;
        ConfigurationEditorRect getContentViewport() const;
        const ConfigurationTextField& getNameField() const;
        const std::vector<StructuralProfileEditorSection>& getProfileSections() const;
        const std::vector<FactionProfileEditorSection>& getFactionProfileSections() const;
        const std::vector<PaletteProfileEditorSection>& getPaletteProfileSections() const;
        bool isPaletteSectionVisible(const PaletteProfileEditorSection& section) const;
        const ConfigurationEditorSectionState& getValidationSection() const;
        const std::array<ConfigurationEditorActionButton, 7u>& getActionButtons() const;
        const std::array<ConfigurationBundleComponentControl, 3u>& getBundleComponentControls() const;
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

        const FactionIntegerFieldBinding* findFactionIntegerField(std::string_view path) const;
        FactionIntegerFieldBinding* findFactionIntegerField(std::string_view path);
        const FactionRangeFieldBinding* findFactionRangeField(std::string_view path) const;
        FactionRangeFieldBinding* findFactionRangeField(std::string_view path);
        const FactionToggleFieldBinding* findFactionToggleField(std::string_view path) const;
        FactionToggleFieldBinding* findFactionToggleField(std::string_view path);
        const FactionChoiceFieldBinding* findFactionChoiceField(std::string_view path) const;
        FactionChoiceFieldBinding* findFactionChoiceField(std::string_view path);
        const FactionWeightGroupBinding* findFactionWeightGroup(std::string_view path) const;
        FactionWeightGroupBinding* findFactionWeightGroup(std::string_view path);

        const PaletteIntegerFieldBinding* findPaletteIntegerField(std::string_view path) const;
        PaletteIntegerFieldBinding* findPaletteIntegerField(std::string_view path);
        const PaletteRangeFieldBinding* findPaletteRangeField(std::string_view path) const;
        PaletteRangeFieldBinding* findPaletteRangeField(std::string_view path);
        const PaletteChoiceFieldBinding* findPaletteChoiceField(std::string_view path) const;
        PaletteChoiceFieldBinding* findPaletteChoiceField(std::string_view path);
        const PaletteColorFieldBinding* findPaletteColorField(std::string_view path) const;
        PaletteColorFieldBinding* findPaletteColorField(std::string_view path);

        void setSectionExpanded(std::size_t sectionIndex, bool expanded);

    private:
        void configureControlsFromDraft();
        void syncDraftFromControls();
        void refreshValidation();
        void rebuildLayout();
        bool isWithinContentViewport(float x, float y) const;
        bool activateSectionHeader(float x, float y);
        std::optional<ConfigurationEditorEvent> activateAction(float x, float y);
        std::optional<ConfigurationEditorEvent> activateBundleComponentAction(float x, float y);
        void resetDraft();
        void cancelDragging();
        void collapseAllProfileSections();

    private:
        bool m_Open = false;
        ConfigurationEditorProfileKind m_ProfileKind = ConfigurationEditorProfileKind::STRUCTURAL;
        std::string m_InitialName;
        PixelShipGenerator::ShipGenerationProfile m_InitialProfile;
        PixelShipGenerator::ShipGenerationProfile m_DraftProfile;
        PixelShipGenerator::ShipFactionProfile m_InitialFactionProfile;
        PixelShipGenerator::ShipFactionProfile m_DraftFactionProfile;
        PixelShipGenerator::ShipPaletteConfiguration m_InitialPaletteConfiguration;
        PixelShipGenerator::ShipPaletteConfiguration m_DraftPaletteConfiguration;
        ConfigurationBundle m_InitialConfigurationBundle;
        ConfigurationBundle m_DraftConfigurationBundle;
        PixelShipGenerator::ValidationResult m_ValidationResult;
        ConfigurationEditorRect m_PanelBounds = { 880.0f, 0.0f, 760.0f, 1000.0f };
        ConfigurationEditorRect m_ContentViewport;
        float m_ScrollOffset = 0.0f;
        float m_MaximumScrollOffset = 0.0f;
        float m_ContentHeight = 0.0f;

        ConfigurationTextField m_NameField;
        ShipGenerationProfileEditorBindings m_ProfileBindings;
        ShipFactionProfileEditorBindings m_FactionProfileBindings;
        ShipPaletteConfigurationEditorBindings m_PaletteBindings;
        ConfigurationEditorSectionState m_ValidationSection = { "VALIDATION", {}, true };
        std::array<ConfigurationEditorActionButton, 7u> m_ActionButtons = {};
        std::array<ConfigurationBundleComponentControl, 3u> m_BundleComponentControls = {};
        bool m_ExistingCustomPreset = false;
    };
}
