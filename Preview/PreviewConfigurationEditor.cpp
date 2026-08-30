#include "PreviewConfigurationEditor.h"

#include <algorithm>
#include <utility>

#include "ShipFactionProfileValidation.h"
#include "ShipGenerationProfileValidation.h"

namespace PixelShipGeneratorPreview
{
    namespace
    {
        constexpr float PanelPadding = 18.0f;
        constexpr float HeaderHeight = 62.0f;
        constexpr float ActionAreaHeight = 150.0f;
        constexpr float SectionHeaderHeight = 26.0f;
        constexpr float RowHeight = 34.0f;
        constexpr float WeightRowHeight = 30.0f;
        constexpr float WeightHeaderHeight = 22.0f;
        constexpr float SectionGap = 10.0f;
        constexpr float ScrollStep = 52.0f;
    }

    PreviewConfigurationEditor::PreviewConfigurationEditor()
    {
        m_ActionButtons = { {
            { ConfigurationEditorAction::APPLY, "APPLY", {}, true },
            { ConfigurationEditorAction::CANCEL, "CANCEL", {}, true },
            { ConfigurationEditorAction::RESET, "RESET", {}, true },
            { ConfigurationEditorAction::DUPLICATE, "DUPLICATE", {}, true }
        } };

        collapseAllProfileSections();
        rebuildLayout();
    }

    void PreviewConfigurationEditor::openStructuralProfile(std::string name, const PixelShipGenerator::ShipGenerationProfile& profile)
    {
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
        else { updateSections(m_FactionProfileBindings.getSections()); }
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
        else { pressSections(m_FactionProfileBindings.getSections()); }
    }

    std::optional<ConfigurationEditorEvent> PreviewConfigurationEditor::onMouseRelease(float x, float y)
    {
        if (!m_Open) { return std::nullopt; }
        if (const auto action = activateAction(x, y); action.has_value()) { return action; }
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
        else { releaseSections(m_FactionProfileBindings.getSections()); }
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

    ConfigurationEditorEvent PreviewConfigurationEditor::createCancelEvent() const { return { ConfigurationEditorAction::CANCEL }; }

    void PreviewConfigurationEditor::setValidationResult(const PixelShipGenerator::ValidationResult& result) { m_ValidationResult = result; }
    const PixelShipGenerator::ValidationResult& PreviewConfigurationEditor::getValidationResult() const { return m_ValidationResult; }
    const std::string& PreviewConfigurationEditor::getName() const { return m_NameField.Value; }
    const PixelShipGenerator::ShipGenerationProfile& PreviewConfigurationEditor::getDraftProfile() const { return m_DraftProfile; }
    const PixelShipGenerator::ShipGenerationProfile& PreviewConfigurationEditor::getInitialProfile() const { return m_InitialProfile; }
    const PixelShipGenerator::ShipFactionProfile& PreviewConfigurationEditor::getDraftFactionProfile() const { return m_DraftFactionProfile; }
    const PixelShipGenerator::ShipFactionProfile& PreviewConfigurationEditor::getInitialFactionProfile() const { return m_InitialFactionProfile; }

    bool PreviewConfigurationEditor::hasUnsavedChanges() const
    {
        if (m_NameField.Value != m_InitialName) { return true; }
        return m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL
            ? !m_ProfileBindings.equivalent(m_DraftProfile, m_InitialProfile)
            : !m_FactionProfileBindings.equivalent(m_DraftFactionProfile, m_InitialFactionProfile);
    }

    float PreviewConfigurationEditor::getScrollOffset() const { return m_ScrollOffset; }
    float PreviewConfigurationEditor::getMaximumScrollOffset() const { return m_MaximumScrollOffset; }
    ConfigurationEditorRect PreviewConfigurationEditor::getPanelBounds() const { return m_PanelBounds; }
    ConfigurationEditorRect PreviewConfigurationEditor::getContentViewport() const { return m_ContentViewport; }
    const ConfigurationTextField& PreviewConfigurationEditor::getNameField() const { return m_NameField; }
    const std::vector<StructuralProfileEditorSection>& PreviewConfigurationEditor::getProfileSections() const { return m_ProfileBindings.getSections(); }
    const std::vector<FactionProfileEditorSection>& PreviewConfigurationEditor::getFactionProfileSections() const { return m_FactionProfileBindings.getSections(); }
    const ConfigurationEditorSectionState& PreviewConfigurationEditor::getValidationSection() const { return m_ValidationSection; }
    const std::array<ConfigurationEditorActionButton, 4u>& PreviewConfigurationEditor::getActionButtons() const { return m_ActionButtons; }
    std::size_t PreviewConfigurationEditor::getBoundValueCount() const { return m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL ? m_ProfileBindings.getBoundValueCount() : m_FactionProfileBindings.getBoundValueCount(); }

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

    void PreviewConfigurationEditor::setSectionExpanded(std::size_t sectionIndex, bool expanded)
    {
        if (m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL)
        {
            auto& sections = m_ProfileBindings.getSections();
            if (sectionIndex >= sections.size()) { return; }
            sections[sectionIndex].Expanded = expanded;
        }
        else
        {
            auto& sections = m_FactionProfileBindings.getSections();
            if (sectionIndex >= sections.size()) { return; }
            sections[sectionIndex].Expanded = expanded;
        }
        rebuildLayout();
    }

    void PreviewConfigurationEditor::configureControlsFromDraft()
    {
        if (m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL) { m_ProfileBindings.load(m_DraftProfile); }
        else { m_FactionProfileBindings.load(m_DraftFactionProfile); }
    }

    void PreviewConfigurationEditor::syncDraftFromControls()
    {
        if (m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL) { m_ProfileBindings.write(m_DraftProfile); }
        else { m_FactionProfileBindings.write(m_DraftFactionProfile); }
    }

    void PreviewConfigurationEditor::refreshValidation()
    {
        setValidationResult(m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL
            ? PixelShipGenerator::validateShipGenerationProfile(m_DraftProfile)
            : PixelShipGenerator::validateShipFactionProfile(m_DraftFactionProfile));
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
        else { layoutSections(m_FactionProfileBindings.getSections()); }

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
        const float actionTop = m_PanelBounds.Top + m_PanelBounds.Height - ActionAreaHeight + 18.0f;
        const float actionWidth = (m_PanelBounds.Width - PanelPadding * 2.0f - ActionGap * 3.0f) / 4.0f;
        for (std::size_t index = 0u; index < m_ActionButtons.size(); ++index)
        {
            m_ActionButtons[index].Bounds = { m_PanelBounds.Left + PanelPadding + static_cast<float>(index) * (actionWidth + ActionGap), actionTop, actionWidth, 32.0f };
        }
        const bool validForCommit = m_ValidationResult.isValid() && !m_NameField.Value.empty();
        m_ActionButtons[0u].Enabled = validForCommit;
        m_ActionButtons[1u].Enabled = true;
        m_ActionButtons[2u].Enabled = hasUnsavedChanges();
        m_ActionButtons[3u].Enabled = validForCommit;
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
        else { activate(m_FactionProfileBindings.getSections()); }
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

    void PreviewConfigurationEditor::resetDraft()
    {
        if (m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL) { m_DraftProfile = m_InitialProfile; }
        else { m_DraftFactionProfile = m_InitialFactionProfile; }
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
    }

    void PreviewConfigurationEditor::collapseAllProfileSections()
    {
        for (StructuralProfileEditorSection& section : m_ProfileBindings.getSections()) { section.Expanded = false; }
        for (FactionProfileEditorSection& section : m_FactionProfileBindings.getSections()) { section.Expanded = false; }
        if (m_ProfileKind == ConfigurationEditorProfileKind::STRUCTURAL && !m_ProfileBindings.getSections().empty()) { m_ProfileBindings.getSections().front().Expanded = true; }
        if (m_ProfileKind == ConfigurationEditorProfileKind::FACTION && !m_FactionProfileBindings.getSections().empty()) { m_FactionProfileBindings.getSections().front().Expanded = true; }
    }
}
