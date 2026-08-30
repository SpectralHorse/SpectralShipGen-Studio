#include "PreviewConfigurationEditor.h"

#include <algorithm>
#include <utility>

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

        auto& sections = m_ProfileBindings.getSections();
        for (StructuralProfileEditorSection& section : sections) { section.Expanded = false; }
        if (!sections.empty()) { sections.front().Expanded = true; }
        rebuildLayout();
    }

    void PreviewConfigurationEditor::openStructuralProfile(std::string name, const PixelShipGenerator::ShipGenerationProfile& profile)
    {
        m_Open = true;
        m_InitialName = std::move(name);
        m_InitialProfile = profile;
        m_DraftProfile = profile;
        m_NameField.Label = "DISPLAY NAME";
        m_NameField.Value = m_InitialName;
        m_NameField.Focused = false;
        m_ScrollOffset = 0.0f;
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

    void PreviewConfigurationEditor::setPanelBounds(const ConfigurationEditorRect& bounds)
    {
        m_PanelBounds = bounds;
        rebuildLayout();
    }

    void PreviewConfigurationEditor::onMouseMove(float x, float)
    {
        if (!m_Open) { return; }
        bool changed = false;
        for (StructuralProfileEditorSection& section : m_ProfileBindings.getSections())
        {
            if (!section.Expanded) { continue; }
            for (StructuralIntegerFieldBinding& field : section.Integers) { changed = field.Control.updatePointer(x) || changed; }
            for (StructuralWeightGroupBinding& field : section.WeightGroups) { changed = field.Control.updatePointer(x) || changed; }
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

        for (StructuralProfileEditorSection& section : m_ProfileBindings.getSections())
        {
            if (!section.Expanded) { continue; }
            for (StructuralIntegerFieldBinding& field : section.Integers)
            {
                if (field.Control.beginPointer(x, y)) { return; }
            }
            for (StructuralWeightGroupBinding& field : section.WeightGroups)
            {
                if (field.Control.beginPointer(x, y)) { return; }
            }
        }
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
        for (StructuralProfileEditorSection& section : m_ProfileBindings.getSections())
        {
            if (!section.Expanded) { continue; }
            for (StructuralIntegerFieldBinding& field : section.Integers) { changed = field.Control.endPointer(x, y) || changed; }
            for (StructuralRangeFieldBinding& field : section.Ranges) { changed = field.Control.activate(x, y) || changed; }
            for (StructuralToggleFieldBinding& field : section.Toggles) { changed = field.Control.activate(x, y) || changed; }
            for (StructuralChoiceFieldBinding& field : section.Choices) { changed = field.Control.activate(x, y) || changed; }
            for (StructuralWeightGroupBinding& field : section.WeightGroups) { changed = field.Control.endPointer(x, y) || changed; }
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

    ConfigurationEditorEvent PreviewConfigurationEditor::createCancelEvent() const { return { ConfigurationEditorAction::CANCEL }; }

    void PreviewConfigurationEditor::setValidationResult(const PixelShipGenerator::ValidationResult& result)
    {
        m_ValidationResult = result;
    }

    const PixelShipGenerator::ValidationResult& PreviewConfigurationEditor::getValidationResult() const { return m_ValidationResult; }
    const std::string& PreviewConfigurationEditor::getName() const { return m_NameField.Value; }
    const PixelShipGenerator::ShipGenerationProfile& PreviewConfigurationEditor::getDraftProfile() const { return m_DraftProfile; }
    const PixelShipGenerator::ShipGenerationProfile& PreviewConfigurationEditor::getInitialProfile() const { return m_InitialProfile; }

    bool PreviewConfigurationEditor::hasUnsavedChanges() const
    {
        return m_NameField.Value != m_InitialName || !m_ProfileBindings.equivalent(m_DraftProfile, m_InitialProfile);
    }

    float PreviewConfigurationEditor::getScrollOffset() const { return m_ScrollOffset; }
    float PreviewConfigurationEditor::getMaximumScrollOffset() const { return m_MaximumScrollOffset; }
    ConfigurationEditorRect PreviewConfigurationEditor::getPanelBounds() const { return m_PanelBounds; }
    ConfigurationEditorRect PreviewConfigurationEditor::getContentViewport() const { return m_ContentViewport; }
    const ConfigurationTextField& PreviewConfigurationEditor::getNameField() const { return m_NameField; }
    const std::vector<StructuralProfileEditorSection>& PreviewConfigurationEditor::getProfileSections() const { return m_ProfileBindings.getSections(); }
    const ConfigurationEditorSectionState& PreviewConfigurationEditor::getValidationSection() const { return m_ValidationSection; }
    const std::array<ConfigurationEditorActionButton, 4u>& PreviewConfigurationEditor::getActionButtons() const { return m_ActionButtons; }
    std::size_t PreviewConfigurationEditor::getBoundValueCount() const { return m_ProfileBindings.getBoundValueCount(); }

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

    void PreviewConfigurationEditor::setSectionExpanded(std::size_t sectionIndex, bool expanded)
    {
        auto& sections = m_ProfileBindings.getSections();
        if (sectionIndex >= sections.size()) { return; }
        sections[sectionIndex].Expanded = expanded;
        rebuildLayout();
    }

    void PreviewConfigurationEditor::configureControlsFromDraft()
    {
        m_ProfileBindings.load(m_DraftProfile);
    }

    void PreviewConfigurationEditor::syncDraftFromControls()
    {
        m_ProfileBindings.write(m_DraftProfile);
    }

    void PreviewConfigurationEditor::refreshValidation()
    {
        setValidationResult(PixelShipGenerator::validateShipGenerationProfile(m_DraftProfile));
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

        for (StructuralProfileEditorSection& section : m_ProfileBindings.getSections())
        {
            section.HeaderBounds = { contentLeft, y, contentWidth, SectionHeaderHeight };
            y += SectionHeaderHeight;
            if (section.Expanded)
            {
                for (StructuralIntegerFieldBinding& field : section.Integers) { field.Control.setRowBounds({ contentLeft, y, contentWidth, RowHeight - 4.0f }); y += RowHeight; }
                for (StructuralRangeFieldBinding& field : section.Ranges) { field.Control.setRowBounds({ contentLeft, y, contentWidth, RowHeight - 4.0f }); y += RowHeight; }
                for (StructuralToggleFieldBinding& field : section.Toggles) { field.Control.setRowBounds({ contentLeft, y, contentWidth, RowHeight - 4.0f }); y += RowHeight; }
                for (StructuralChoiceFieldBinding& field : section.Choices) { field.Control.setRowBounds({ contentLeft, y, contentWidth, RowHeight - 4.0f }); y += RowHeight; }
                for (StructuralWeightGroupBinding& field : section.WeightGroups)
                {
                    const float groupHeight = WeightHeaderHeight + static_cast<float>(field.Control.getRowCount()) * WeightRowHeight;
                    field.Control.setBounds({ contentLeft, y, contentWidth, groupHeight });
                    y += groupHeight + 4.0f;
                }
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

    bool PreviewConfigurationEditor::isWithinContentViewport(float x, float y) const
    {
        return m_ContentViewport.contains(x, y);
    }

    bool PreviewConfigurationEditor::activateSectionHeader(float x, float y)
    {
        for (StructuralProfileEditorSection& section : m_ProfileBindings.getSections())
        {
            if (!section.HeaderBounds.contains(x, y)) { continue; }
            section.Expanded = !section.Expanded;
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
        m_DraftProfile = m_InitialProfile;
        m_NameField.Value = m_InitialName;
        configureControlsFromDraft();
        refreshValidation();
        rebuildLayout();
    }

    void PreviewConfigurationEditor::cancelDragging()
    {
        for (StructuralProfileEditorSection& section : m_ProfileBindings.getSections())
        {
            for (StructuralIntegerFieldBinding& field : section.Integers) { field.Control.Dragging = false; }
            for (StructuralWeightGroupBinding& field : section.WeightGroups)
            {
                for (ConfigurationWeightRow& row : field.Control.getRows()) { row.Control.Dragging = false; }
            }
        }
    }
}
