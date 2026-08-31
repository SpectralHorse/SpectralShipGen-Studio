#include "PreviewConfigurationEditor.h"

#include <algorithm>
#include <utility>

#include <PixelShipGenerator/ShipFactionProfileValidation.h>
#include <PixelShipGenerator/ShipGenerationProfileValidation.h>
#include <PixelShipGenerator/ShipPaletteGenerationProfileValidation.h>

namespace PixelShipGeneratorPreview
{
    namespace
    {
        constexpr float PanelPadding = 18.0f;
        constexpr float HeaderHeight = 62.0f;
        constexpr float ActionAreaHeight = 150.0f;
        constexpr float SectionHeaderHeight = 32.0f;
        constexpr float RowHeight = 40.0f;
        constexpr float WeightRowHeight = 38.0f;
        constexpr float WeightHeaderHeight = 28.0f;
        constexpr float SectionGap = 12.0f;
        constexpr float ScrollStep = 64.0f;
    }

    PreviewConfigurationEditor::PreviewConfigurationEditor()
    {
        m_ActionButtons = { {
            { ConfigurationEditorAction::APPLY, "APPLY", {}, true },
            { ConfigurationEditorAction::CANCEL, "CANCEL", {}, true },
            { ConfigurationEditorAction::RESET, "RESET", {}, true },
            { ConfigurationEditorAction::DUPLICATE, "DUPLICATE", {}, true },
            { ConfigurationEditorAction::DELETE_PRESET, "DELETE", {}, false },
            { ConfigurationEditorAction::EXPORT_PRESET, "EXPORT", {}, false },
            { ConfigurationEditorAction::IMPORT_PRESET, "IMPORT", {}, true }
        } };
        m_BundleComponentControls = { {
            { "STRUCTURAL", "", {}, {}, ConfigurationEditorAction::REPLACE_BUNDLE_STRUCTURAL },
            { "FACTION", "", {}, {}, ConfigurationEditorAction::REPLACE_BUNDLE_FACTION },
            { "PALETTE", "", {}, {}, ConfigurationEditorAction::REPLACE_BUNDLE_PALETTE }
        } };

        collapseAllProfileSections();
        rebuildLayout();
    }

    void PreviewConfigurationEditor::openStructuralProfile(std::string name, const PixelShipGenerator::ShipGenerationProfile& profile)
    {
        m_ExistingCustomPreset = false;
        m_Open = true;
        m_ProfileKind = ConfigurationEditorProfileKind::STRUCTURAL;
        m_InitialName = std::move(name);
        m_InitialProfile = profile;
        m_DraftProfile = profile;
        m_NameField.Label = "DISPLAY NAME";
        m_NameField.Value = m_InitialName;
        m_NameField.Focused = false;
        m_ScrollOffset = 0.0f;
        collapseAllProfileSections();
        configureControlsFromDraft();
        refreshValidation();
        rebuildLayout();
    }

    void PreviewConfigurationEditor::openFactionProfile(std::string name, const PixelShipGenerator::ShipFactionProfile& profile)
    {
        m_ExistingCustomPreset = false;
        m_Open = true;
        m_ProfileKind = ConfigurationEditorProfileKind::FACTION;
        m_InitialName = std::move(name);
        m_InitialFactionProfile = profile;
        m_DraftFactionProfile = profile;
        m_NameField.Label = "DISPLAY NAME";
        m_NameField.Value = m_InitialName;
        m_NameField.Focused = false;
        m_ScrollOffset = 0.0f;
        collapseAllProfileSections();
        configureControlsFromDraft();
        refreshValidation();
        rebuildLayout();
    }

    void PreviewConfigurationEditor::openPaletteConfiguration(std::string name, const PixelShipGenerator::ShipPaletteConfiguration& configuration)
    {
        m_ExistingCustomPreset = false;
        m_Open = true;
        m_ProfileKind = ConfigurationEditorProfileKind::PALETTE;
        m_InitialName = std::move(name);
        m_InitialPaletteConfiguration = configuration;
        m_DraftPaletteConfiguration = configuration;
        m_NameField.Label = "DISPLAY NAME";
        m_NameField.Value = m_InitialName;
        m_NameField.Focused = false;
        m_ScrollOffset = 0.0f;
        collapseAllProfileSections();
        configureControlsFromDraft();
        refreshValidation();
        rebuildLayout();
    }

    void PreviewConfigurationEditor::openConfigurationBundle(std::string name, const ConfigurationBundle& bundle)
    {
        m_ExistingCustomPreset = false;
        m_Open = true;
        m_ProfileKind = ConfigurationEditorProfileKind::FULL_CONFIGURATION;
        m_InitialName = std::move(name);
        m_InitialConfigurationBundle = bundle;
        m_DraftConfigurationBundle = bundle;
        m_NameField.Label = "DISPLAY NAME";
        m_NameField.Value = m_InitialName;
        m_NameField.Focused = false;
        m_ScrollOffset = 0.0f;
        refreshValidation();
        rebuildLayout();
    }

    void PreviewConfigurationEditor::close()
    {
        m_Open = false;
        m_NameField.Focused = false;
        cancelDragging();
    }

    bool PreviewConfigurationEditor::isOpen() const { return m_Open; }
    ConfigurationEditorProfileKind PreviewConfigurationEditor::getProfileKind() const { return m_ProfileKind; }

    void PreviewConfigurationEditor::setPanelBounds(const ConfigurationEditorRect& bounds)
    {
        m_PanelBounds = bounds;
        rebuildLayout();
    }

    void PreviewConfigurationEditor::onMouseMove(float x, float)
    {
        if (!m_Open) { return; }
        bool changed = false;
        const auto updateSections = [&](auto& sections)
            {
                for (auto& section : sections)
                {
                    if (!section.Expanded) { continue; }
                    for (auto& field : section.Integers) { changed = field.Control.updatePointer(x) || changed; }
                    for (auto& field : section.WeightGroups) { changed = field.Control.updatePointer(x) || changed; }
                }
            };
        if (m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL) { updateSections(m_ProfileBindings.getSections()); }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::FACTION) { updateSections(m_FactionProfileBindings.getSections()); }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::PALETTE)
        {
            for (auto& section : m_PaletteBindings.getSections())
            {
                if (!section.Expanded || !m_PaletteBindings.isSectionVisible(section)) { continue; }
                for (auto& field : section.Integers) { changed = field.Control.updatePointer(x) || changed; }
                for (auto& field : section.Colors) { changed = field.Control.updatePointer(x) || changed; }
            }
        }
        if (changed)
        {
            syncDraftFromControls();
            refreshValidation();
        }
    }

    void PreviewConfigurationEditor::onMousePress(float x, float y)
    {
        if (!m_Open) { return; }
        if (!isWithinContentViewport(x, y))
        {
            m_NameField.Focused = false;
            return;
        }
        if (activateSectionHeader(x, y))
        {
            m_NameField.Focused = false;
            return;
        }
        if (m_NameField.activate(x, y)) { return; }
        m_NameField.Focused = false;

        bool consumed = false;
        const auto pressSections = [&](auto& sections)
            {
                for (auto& section : sections)
                {
                    if (!section.Expanded || consumed) { continue; }
                    for (auto& field : section.Integers) { if (field.Control.beginPointer(x, y)) { consumed = true; break; } }
                    if (consumed) { continue; }
                    for (auto& field : section.WeightGroups) { if (field.Control.beginPointer(x, y)) { consumed = true; break; } }
                }
            };
        if (m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL) { pressSections(m_ProfileBindings.getSections()); }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::FACTION) { pressSections(m_FactionProfileBindings.getSections()); }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::PALETTE)
        {
            for (auto& section : m_PaletteBindings.getSections())
            {
                if (!section.Expanded || !m_PaletteBindings.isSectionVisible(section) || consumed) { continue; }
                for (auto& field : section.Integers) { if (field.Control.beginPointer(x, y)) { consumed = true; break; } }
                if (consumed) { continue; }
                for (auto& field : section.Colors) { if (field.Control.beginPointer(x, y)) { consumed = true; break; } }
            }
        }
    }

    std::optional<ConfigurationEditorEvent> PreviewConfigurationEditor::onMouseRelease(float x, float y)
    {
        if (!m_Open) { return std::nullopt; }
        if (const auto action = activateAction(x, y); action.has_value()) { return action; }
        if (const auto action = activateBundleComponentAction(x, y); action.has_value()) { return action; }
        if (!isWithinContentViewport(x, y))
        {
            cancelDragging();
            return std::nullopt;
        }

        bool changed = false;
        const auto releaseSections = [&](auto& sections)
            {
                for (auto& section : sections)
                {
                    if (!section.Expanded) { continue; }
                    for (auto& field : section.Integers) { changed = field.Control.endPointer(x, y) || changed; }
                    for (auto& field : section.Ranges) { changed = field.Control.activate(x, y) || changed; }
                    for (auto& field : section.Toggles) { changed = field.Control.activate(x, y) || changed; }
                    for (auto& field : section.Choices) { changed = field.Control.activate(x, y) || changed; }
                    for (auto& field : section.WeightGroups) { changed = field.Control.endPointer(x, y) || changed; }
                }
            };
        if (m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL) { releaseSections(m_ProfileBindings.getSections()); }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::FACTION) { releaseSections(m_FactionProfileBindings.getSections()); }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::PALETTE)
        {
            for (auto& section : m_PaletteBindings.getSections())
            {
                if (!section.Expanded || !m_PaletteBindings.isSectionVisible(section)) { continue; }
                for (auto& field : section.Integers) { changed = field.Control.endPointer(x, y) || changed; }
                for (auto& field : section.Ranges) { changed = field.Control.activate(x, y) || changed; }
                for (auto& field : section.Choices) { changed = field.Control.activate(x, y) || changed; }
                for (auto& field : section.Colors) { changed = field.Control.endPointer(x, y) || changed; }
            }
        }
        if (changed)
        {
            syncDraftFromControls();
            refreshValidation();
            rebuildLayout();
        }
        return std::nullopt;
    }

    void PreviewConfigurationEditor::onMouseWheelScrolled(float delta)
    {
        if (!m_Open || m_MaximumScrollOffset <= 0.0f) { return; }
        m_ScrollOffset = std::clamp(m_ScrollOffset - delta * ScrollStep, 0.0f, m_MaximumScrollOffset);
        rebuildLayout();
    }

    bool PreviewConfigurationEditor::onTextEntered(uint32_t unicode)
    {
        if (!m_Open || !m_NameField.onTextEntered(unicode)) { return false; }
        rebuildLayout();
        return true;
    }

    bool PreviewConfigurationEditor::hasKeyboardFocus() const
    {
        return m_Open && m_NameField.Focused;
    }

    void PreviewConfigurationEditor::releaseKeyboardFocus()
    {
        m_NameField.Focused = false;
    }

    ConfigurationEditorEvent PreviewConfigurationEditor::createCancelEvent() const { return { ConfigurationEditorAction::CANCEL }; }

    void PreviewConfigurationEditor::setValidationResult(const PixelShipGenerator::ValidationResult& result) { m_ValidationResult = result; }
    void PreviewConfigurationEditor::setExistingCustomPreset(bool existingCustomPreset) { m_ExistingCustomPreset = existingCustomPreset; rebuildLayout(); }
    const PixelShipGenerator::ValidationResult& PreviewConfigurationEditor::getValidationResult() const { return m_ValidationResult; }
    const std::string& PreviewConfigurationEditor::getName() const { return m_NameField.Value; }
    const PixelShipGenerator::ShipGenerationProfile& PreviewConfigurationEditor::getDraftProfile() const { return m_DraftProfile; }
    const PixelShipGenerator::ShipGenerationProfile& PreviewConfigurationEditor::getInitialProfile() const { return m_InitialProfile; }
    const PixelShipGenerator::ShipFactionProfile& PreviewConfigurationEditor::getDraftFactionProfile() const { return m_DraftFactionProfile; }
    const PixelShipGenerator::ShipFactionProfile& PreviewConfigurationEditor::getInitialFactionProfile() const { return m_InitialFactionProfile; }
    const PixelShipGenerator::ShipPaletteConfiguration& PreviewConfigurationEditor::getDraftPaletteConfiguration() const { return m_DraftPaletteConfiguration; }
    const PixelShipGenerator::ShipPaletteConfiguration& PreviewConfigurationEditor::getInitialPaletteConfiguration() const { return m_InitialPaletteConfiguration; }
    const ConfigurationBundle& PreviewConfigurationEditor::getDraftConfigurationBundle() const { return m_DraftConfigurationBundle; }
    const ConfigurationBundle& PreviewConfigurationEditor::getInitialConfigurationBundle() const { return m_InitialConfigurationBundle; }

    void PreviewConfigurationEditor::replaceBundleStructural(std::string displayName, const PixelShipGenerator::ShipGenerationProfile& profile)
    {
        m_DraftConfigurationBundle.StructuralDisplayName = std::move(displayName);
        m_DraftConfigurationBundle.StructuralProfile = profile;
        refreshValidation();
        rebuildLayout();
    }

    void PreviewConfigurationEditor::replaceBundleFaction(std::string displayName, const PixelShipGenerator::ShipFactionProfile& profile)
    {
        m_DraftConfigurationBundle.FactionDisplayName = std::move(displayName);
        m_DraftConfigurationBundle.FactionProfile = profile;
        refreshValidation();
        rebuildLayout();
    }

    void PreviewConfigurationEditor::replaceBundlePalette(std::string displayName, const PixelShipGenerator::ShipPaletteConfiguration& configuration)
    {
        m_DraftConfigurationBundle.PaletteDisplayName = std::move(displayName);
        m_DraftConfigurationBundle.PaletteConfiguration = configuration;
        refreshValidation();
        rebuildLayout();
    }

    bool PreviewConfigurationEditor::hasUnsavedChanges() const
    {
        if (m_NameField.Value != m_InitialName) { return true; }
        if (m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL) { return !m_ProfileBindings.equivalent(m_DraftProfile, m_InitialProfile); }
        if (m_ProfileKind == ConfigurationEditorProfileKind::FACTION) { return !m_FactionProfileBindings.equivalent(m_DraftFactionProfile, m_InitialFactionProfile); }
        if (m_ProfileKind == ConfigurationEditorProfileKind::PALETTE) { return !m_PaletteBindings.equivalent(m_DraftPaletteConfiguration, m_InitialPaletteConfiguration); }
        return m_DraftConfigurationBundle.StructuralDisplayName != m_InitialConfigurationBundle.StructuralDisplayName ||
            m_DraftConfigurationBundle.FactionDisplayName != m_InitialConfigurationBundle.FactionDisplayName ||
            m_DraftConfigurationBundle.PaletteDisplayName != m_InitialConfigurationBundle.PaletteDisplayName ||
            !m_ProfileBindings.equivalent(m_DraftConfigurationBundle.StructuralProfile, m_InitialConfigurationBundle.StructuralProfile) ||
            !m_FactionProfileBindings.equivalent(m_DraftConfigurationBundle.FactionProfile, m_InitialConfigurationBundle.FactionProfile) ||
            !m_PaletteBindings.equivalent(m_DraftConfigurationBundle.PaletteConfiguration, m_InitialConfigurationBundle.PaletteConfiguration);
    }

    float PreviewConfigurationEditor::getScrollOffset() const { return m_ScrollOffset; }
    float PreviewConfigurationEditor::getMaximumScrollOffset() const { return m_MaximumScrollOffset; }
    ConfigurationEditorRect PreviewConfigurationEditor::getPanelBounds() const { return m_PanelBounds; }
    ConfigurationEditorRect PreviewConfigurationEditor::getContentViewport() const { return m_ContentViewport; }
    const ConfigurationTextField& PreviewConfigurationEditor::getNameField() const { return m_NameField; }
    const std::vector<StructuralProfileEditorSection>& PreviewConfigurationEditor::getProfileSections() const { return m_ProfileBindings.getSections(); }
    const std::vector<FactionProfileEditorSection>& PreviewConfigurationEditor::getFactionProfileSections() const { return m_FactionProfileBindings.getSections(); }
    const std::vector<PaletteProfileEditorSection>& PreviewConfigurationEditor::getPaletteProfileSections() const { return m_PaletteBindings.getSections(); }
    bool PreviewConfigurationEditor::isPaletteSectionVisible(const PaletteProfileEditorSection& section) const { return m_PaletteBindings.isSectionVisible(section); }
    const ConfigurationEditorSectionState& PreviewConfigurationEditor::getValidationSection() const { return m_ValidationSection; }
    const std::array<ConfigurationEditorActionButton, 7u>& PreviewConfigurationEditor::getActionButtons() const { return m_ActionButtons; }
    const std::array<ConfigurationBundleComponentControl, 3u>& PreviewConfigurationEditor::getBundleComponentControls() const { return m_BundleComponentControls; }
    std::size_t PreviewConfigurationEditor::getBoundValueCount() const
    {
        if (m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL) { return m_ProfileBindings.getBoundValueCount(); }
        if (m_ProfileKind == ConfigurationEditorProfileKind::FACTION) { return m_FactionProfileBindings.getBoundValueCount(); }
        if (m_ProfileKind == ConfigurationEditorProfileKind::PALETTE) { return m_PaletteBindings.getBoundValueCount(); }
        return 3u;
    }

    const StructuralIntegerFieldBinding* PreviewConfigurationEditor::findIntegerField(std::string_view path) const { return m_ProfileBindings.findInteger(path); }
    StructuralIntegerFieldBinding* PreviewConfigurationEditor::findIntegerField(std::string_view path) { return m_ProfileBindings.findInteger(path); }
    const StructuralRangeFieldBinding* PreviewConfigurationEditor::findRangeField(std::string_view path) const { return m_ProfileBindings.findRange(path); }
    StructuralRangeFieldBinding* PreviewConfigurationEditor::findRangeField(std::string_view path) { return m_ProfileBindings.findRange(path); }
    const StructuralToggleFieldBinding* PreviewConfigurationEditor::findToggleField(std::string_view path) const { return m_ProfileBindings.findToggle(path); }
    StructuralToggleFieldBinding* PreviewConfigurationEditor::findToggleField(std::string_view path) { return m_ProfileBindings.findToggle(path); }
    const StructuralChoiceFieldBinding* PreviewConfigurationEditor::findChoiceField(std::string_view path) const { return m_ProfileBindings.findChoice(path); }
    StructuralChoiceFieldBinding* PreviewConfigurationEditor::findChoiceField(std::string_view path) { return m_ProfileBindings.findChoice(path); }
    const StructuralWeightGroupBinding* PreviewConfigurationEditor::findWeightGroup(std::string_view path) const { return m_ProfileBindings.findWeightGroup(path); }
    StructuralWeightGroupBinding* PreviewConfigurationEditor::findWeightGroup(std::string_view path) { return m_ProfileBindings.findWeightGroup(path); }

    const FactionIntegerFieldBinding* PreviewConfigurationEditor::findFactionIntegerField(std::string_view path) const { return m_FactionProfileBindings.findInteger(path); }
    FactionIntegerFieldBinding* PreviewConfigurationEditor::findFactionIntegerField(std::string_view path) { return m_FactionProfileBindings.findInteger(path); }
    const FactionRangeFieldBinding* PreviewConfigurationEditor::findFactionRangeField(std::string_view path) const { return m_FactionProfileBindings.findRange(path); }
    FactionRangeFieldBinding* PreviewConfigurationEditor::findFactionRangeField(std::string_view path) { return m_FactionProfileBindings.findRange(path); }
    const FactionToggleFieldBinding* PreviewConfigurationEditor::findFactionToggleField(std::string_view path) const { return m_FactionProfileBindings.findToggle(path); }
    FactionToggleFieldBinding* PreviewConfigurationEditor::findFactionToggleField(std::string_view path) { return m_FactionProfileBindings.findToggle(path); }
    const FactionChoiceFieldBinding* PreviewConfigurationEditor::findFactionChoiceField(std::string_view path) const { return m_FactionProfileBindings.findChoice(path); }
    FactionChoiceFieldBinding* PreviewConfigurationEditor::findFactionChoiceField(std::string_view path) { return m_FactionProfileBindings.findChoice(path); }
    const FactionWeightGroupBinding* PreviewConfigurationEditor::findFactionWeightGroup(std::string_view path) const { return m_FactionProfileBindings.findWeightGroup(path); }
    FactionWeightGroupBinding* PreviewConfigurationEditor::findFactionWeightGroup(std::string_view path) { return m_FactionProfileBindings.findWeightGroup(path); }

    const PaletteIntegerFieldBinding* PreviewConfigurationEditor::findPaletteIntegerField(std::string_view path) const { return m_PaletteBindings.findInteger(path); }
    PaletteIntegerFieldBinding* PreviewConfigurationEditor::findPaletteIntegerField(std::string_view path) { return m_PaletteBindings.findInteger(path); }
    const PaletteRangeFieldBinding* PreviewConfigurationEditor::findPaletteRangeField(std::string_view path) const { return m_PaletteBindings.findRange(path); }
    PaletteRangeFieldBinding* PreviewConfigurationEditor::findPaletteRangeField(std::string_view path) { return m_PaletteBindings.findRange(path); }
    const PaletteChoiceFieldBinding* PreviewConfigurationEditor::findPaletteChoiceField(std::string_view path) const { return m_PaletteBindings.findChoice(path); }
    PaletteChoiceFieldBinding* PreviewConfigurationEditor::findPaletteChoiceField(std::string_view path) { return m_PaletteBindings.findChoice(path); }
    const PaletteColorFieldBinding* PreviewConfigurationEditor::findPaletteColorField(std::string_view path) const { return m_PaletteBindings.findColor(path); }
    PaletteColorFieldBinding* PreviewConfigurationEditor::findPaletteColorField(std::string_view path) { return m_PaletteBindings.findColor(path); }

    void PreviewConfigurationEditor::setSectionExpanded(std::size_t sectionIndex, bool expanded)
    {
        if (m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL)
        {
            auto& sections = m_ProfileBindings.getSections();
            if (sectionIndex >= sections.size()) { return; }
            sections[sectionIndex].Expanded = expanded;
        }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::FACTION)
        {
            auto& sections = m_FactionProfileBindings.getSections();
            if (sectionIndex >= sections.size()) { return; }
            sections[sectionIndex].Expanded = expanded;
        }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::PALETTE)
        {
            auto& sections = m_PaletteBindings.getSections();
            if (sectionIndex >= sections.size()) { return; }
            sections[sectionIndex].Expanded = expanded;
        }
        else { return; }
        rebuildLayout();
    }

    void PreviewConfigurationEditor::configureControlsFromDraft()
    {
        if (m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL) { m_ProfileBindings.load(m_DraftProfile); }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::FACTION) { m_FactionProfileBindings.load(m_DraftFactionProfile); }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::PALETTE) { m_PaletteBindings.load(m_DraftPaletteConfiguration); }
    }

    void PreviewConfigurationEditor::syncDraftFromControls()
    {
        if (m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL) { m_ProfileBindings.write(m_DraftProfile); }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::FACTION) { m_FactionProfileBindings.write(m_DraftFactionProfile); }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::PALETTE) { m_PaletteBindings.write(m_DraftPaletteConfiguration); }
    }

    void PreviewConfigurationEditor::refreshValidation()
    {
        if (m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL)
        {
            setValidationResult(PixelShipGenerator::validateShipGenerationProfile(m_DraftProfile));
        }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::FACTION)
        {
            setValidationResult(PixelShipGenerator::validateShipFactionProfile(m_DraftFactionProfile));
        }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::PALETTE && m_DraftPaletteConfiguration.Mode == PixelShipGenerator::ShipPaletteSourceMode::EXPLICIT_GENERATED)
        {
            setValidationResult(PixelShipGenerator::validateShipPaletteGenerationProfile(m_DraftPaletteConfiguration.Generated));
        }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::FULL_CONFIGURATION)
        {
            setValidationResult(validateConfigurationBundle(m_DraftConfigurationBundle));
        }
        else
        {
            setValidationResult({});
        }
    }

    void PreviewConfigurationEditor::rebuildLayout()
    {
        m_ContentViewport = {
            m_PanelBounds.Left + PanelPadding,
            m_PanelBounds.Top + HeaderHeight,
            std::max(1.0f, m_PanelBounds.Width - PanelPadding * 2.0f),
            std::max(1.0f, m_PanelBounds.Height - HeaderHeight - ActionAreaHeight - PanelPadding)
        };

        const float contentLeft = m_ContentViewport.Left;
        const float contentWidth = m_ContentViewport.Width - 8.0f;
        float y = m_ContentViewport.Top - m_ScrollOffset;

        m_NameField.Bounds = { contentLeft, y + 2.0f, contentWidth, RowHeight - 4.0f };
        y += RowHeight + SectionGap;

        const auto layoutSections = [&](auto& sections)
            {
                for (auto& section : sections)
                {
                    section.HeaderBounds = { contentLeft, y, contentWidth, SectionHeaderHeight };
                    y += SectionHeaderHeight;
                    if (section.Expanded)
                    {
                        for (auto& field : section.Integers) { field.Control.setRowBounds({ contentLeft, y, contentWidth, RowHeight - 4.0f }); y += RowHeight; }
                        for (auto& field : section.Ranges) { field.Control.setRowBounds({ contentLeft, y, contentWidth, RowHeight - 4.0f }); y += RowHeight; }
                        for (auto& field : section.Toggles) { field.Control.setRowBounds({ contentLeft, y, contentWidth, RowHeight - 4.0f }); y += RowHeight; }
                        for (auto& field : section.Choices) { field.Control.setRowBounds({ contentLeft, y, contentWidth, RowHeight - 4.0f }); y += RowHeight; }
                        for (auto& field : section.WeightGroups)
                        {
                            const float groupHeight = WeightHeaderHeight + static_cast<float>(field.Control.getRowCount()) * WeightRowHeight;
                            field.Control.setBounds({ contentLeft, y, contentWidth, groupHeight });
                            y += groupHeight + 4.0f;
                        }
                    }
                    y += SectionGap;
                }
            };
        if (m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL) { layoutSections(m_ProfileBindings.getSections()); }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::FACTION) { layoutSections(m_FactionProfileBindings.getSections()); }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::PALETTE)
        {
            constexpr float ColorRowHeight = 72.0f;
            for (auto& section : m_PaletteBindings.getSections())
            {
                if (!m_PaletteBindings.isSectionVisible(section)) { section.HeaderBounds = {}; continue; }
                section.HeaderBounds = { contentLeft, y, contentWidth, SectionHeaderHeight };
                y += SectionHeaderHeight;
                if (section.Expanded)
                {
                    for (auto& field : section.Integers) { field.Control.setRowBounds({ contentLeft, y, contentWidth, RowHeight - 4.0f }); y += RowHeight; }
                    for (auto& field : section.Ranges) { field.Control.setRowBounds({ contentLeft, y, contentWidth, RowHeight - 4.0f }); y += RowHeight; }
                    for (auto& field : section.Choices) { field.Control.setRowBounds({ contentLeft, y, contentWidth, RowHeight - 4.0f }); y += RowHeight; }
                    for (auto& field : section.Colors) { field.Control.setRowBounds({ contentLeft, y, contentWidth, ColorRowHeight - 4.0f }); y += ColorRowHeight; }
                }
                y += SectionGap;
            }
        }

        else
        {
            m_BundleComponentControls[0u].Value = m_DraftConfigurationBundle.StructuralDisplayName;
            m_BundleComponentControls[1u].Value = m_DraftConfigurationBundle.FactionDisplayName;
            m_BundleComponentControls[2u].Value = m_DraftConfigurationBundle.PaletteDisplayName;
            for (ConfigurationBundleComponentControl& component : m_BundleComponentControls)
            {
                component.RowBounds = { contentLeft, y, contentWidth, RowHeight };
                component.ReplaceBounds = { contentLeft + contentWidth - 130.0f, y + 3.0f, 124.0f, RowHeight - 6.0f };
                y += RowHeight + 4.0f;
            }
            y += SectionGap;
        }

        m_ValidationSection.HeaderBounds = { contentLeft, y, contentWidth, SectionHeaderHeight };
        y += SectionHeaderHeight;
        if (m_ValidationSection.Expanded)
        {
            const std::size_t visibleMessages = std::min<std::size_t>(8u, m_ValidationResult.Errors.size() + m_ValidationResult.Warnings.size());
            y += std::max(48.0f, 22.0f + static_cast<float>(visibleMessages) * 30.0f);
        }
        y += SectionGap;

        m_ContentHeight = std::max(0.0f, y + m_ScrollOffset - m_ContentViewport.Top);
        m_MaximumScrollOffset = std::max(0.0f, m_ContentHeight - m_ContentViewport.Height);
        const float clampedScrollOffset = std::clamp(m_ScrollOffset, 0.0f, m_MaximumScrollOffset);
        if (clampedScrollOffset != m_ScrollOffset)
        {
            m_ScrollOffset = clampedScrollOffset;
            rebuildLayout();
            return;
        }

        constexpr float ActionGap = 8.0f;
        constexpr std::size_t ActionColumns = 4u;
        constexpr float ActionRowGap = 8.0f;
        const float actionTop = m_PanelBounds.Top + m_PanelBounds.Height - ActionAreaHeight + 18.0f;
        const float actionWidth = (m_PanelBounds.Width - PanelPadding * 2.0f - ActionGap * static_cast<float>(ActionColumns - 1u)) / static_cast<float>(ActionColumns);
        for (std::size_t index = 0u; index < m_ActionButtons.size(); ++index)
        {
            const std::size_t column = index % ActionColumns;
            const std::size_t row = index / ActionColumns;
            m_ActionButtons[index].Bounds = { m_PanelBounds.Left + PanelPadding + static_cast<float>(column) * (actionWidth + ActionGap), actionTop + static_cast<float>(row) * (32.0f + ActionRowGap), actionWidth, 32.0f };
        }
        const bool validForCommit = m_ValidationResult.isValid() && !m_NameField.Value.empty();
        for (ConfigurationEditorActionButton& button : m_ActionButtons)
        {
            switch (button.Action)
            {
            case ConfigurationEditorAction::APPLY: button.Enabled = validForCommit; break;
            case ConfigurationEditorAction::CANCEL: button.Enabled = true; break;
            case ConfigurationEditorAction::RESET: button.Enabled = hasUnsavedChanges(); break;
            case ConfigurationEditorAction::DUPLICATE: button.Enabled = validForCommit; break;
            case ConfigurationEditorAction::DELETE_PRESET: button.Enabled = m_ExistingCustomPreset; break;
            case ConfigurationEditorAction::EXPORT_PRESET: button.Enabled = m_ExistingCustomPreset; break;
            case ConfigurationEditorAction::IMPORT_PRESET: button.Enabled = true; break;
            default: button.Enabled = false; break;
            }
        }
    }

    bool PreviewConfigurationEditor::isWithinContentViewport(float x, float y) const { return m_ContentViewport.contains(x, y); }

    bool PreviewConfigurationEditor::activateSectionHeader(float x, float y)
    {
        bool activated = false;
        const auto activate = [&](auto& sections)
            {
                for (auto& section : sections)
                {
                    if (!section.HeaderBounds.contains(x, y)) { continue; }
                    section.Expanded = !section.Expanded;
                    activated = true;
                    break;
                }
            };
        if (m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL) { activate(m_ProfileBindings.getSections()); }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::FACTION) { activate(m_FactionProfileBindings.getSections()); }
        else
        {
            for (auto& section : m_PaletteBindings.getSections())
            {
                if (!m_PaletteBindings.isSectionVisible(section) || !section.HeaderBounds.contains(x, y)) { continue; }
                section.Expanded = !section.Expanded;
                activated = true;
                break;
            }
        }
        if (activated)
        {
            rebuildLayout();
            return true;
        }
        if (m_ValidationSection.HeaderBounds.contains(x, y))
        {
            m_ValidationSection.Expanded = !m_ValidationSection.Expanded;
            rebuildLayout();
            return true;
        }
        return false;
    }

    std::optional<ConfigurationEditorEvent> PreviewConfigurationEditor::activateAction(float x, float y)
    {
        for (ConfigurationEditorActionButton& button : m_ActionButtons)
        {
            if (!button.Bounds.contains(x, y) || !button.Enabled) { continue; }
            if (button.Action == ConfigurationEditorAction::RESET)
            {
                resetDraft();
                return std::nullopt;
            }
            return ConfigurationEditorEvent{ button.Action };
        }
        return std::nullopt;
    }

    std::optional<ConfigurationEditorEvent> PreviewConfigurationEditor::activateBundleComponentAction(float x, float y)
    {
        if (m_ProfileKind != ConfigurationEditorProfileKind::FULL_CONFIGURATION) { return std::nullopt; }
        for (const ConfigurationBundleComponentControl& component : m_BundleComponentControls)
        {
            if (component.ReplaceBounds.contains(x, y)) { return ConfigurationEditorEvent{ component.Action }; }
        }
        return std::nullopt;
    }

    void PreviewConfigurationEditor::resetDraft()
    {
        if (m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL) { m_DraftProfile = m_InitialProfile; }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::FACTION) { m_DraftFactionProfile = m_InitialFactionProfile; }
        else if (m_ProfileKind == ConfigurationEditorProfileKind::PALETTE) { m_DraftPaletteConfiguration = m_InitialPaletteConfiguration; }
        else { m_DraftConfigurationBundle = m_InitialConfigurationBundle; }
        m_NameField.Value = m_InitialName;
        configureControlsFromDraft();
        refreshValidation();
        rebuildLayout();
    }

    void PreviewConfigurationEditor::cancelDragging()
    {
        const auto cancel = [](auto& sections)
            {
                for (auto& section : sections)
                {
                    for (auto& field : section.Integers) { field.Control.Dragging = false; }
                    for (auto& field : section.WeightGroups)
                    {
                        for (ConfigurationWeightRow& row : field.Control.getRows()) { row.Control.Dragging = false; }
                    }
                }
            };
        cancel(m_ProfileBindings.getSections());
        cancel(m_FactionProfileBindings.getSections());
        for (auto& section : m_PaletteBindings.getSections())
        {
            for (auto& field : section.Integers) { field.Control.Dragging = false; }
            for (auto& field : section.Colors) { field.Control.DraggingChannel = -1; }
        }
    }

    void PreviewConfigurationEditor::collapseAllProfileSections()
    {
        for (StructuralProfileEditorSection& section : m_ProfileBindings.getSections()) { section.Expanded = false; }
        for (FactionProfileEditorSection& section : m_FactionProfileBindings.getSections()) { section.Expanded = false; }
        for (PaletteProfileEditorSection& section : m_PaletteBindings.getSections()) { section.Expanded = false; }
        if (m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL && !m_ProfileBindings.getSections().empty()) { m_ProfileBindings.getSections().front().Expanded = true; }
        if (m_ProfileKind == ConfigurationEditorProfileKind::FACTION && !m_FactionProfileBindings.getSections().empty()) { m_FactionProfileBindings.getSections().front().Expanded = true; }
        if (m_ProfileKind == ConfigurationEditorProfileKind::PALETTE && !m_PaletteBindings.getSections().empty()) { m_PaletteBindings.getSections().front().Expanded = true; }
    }
}
