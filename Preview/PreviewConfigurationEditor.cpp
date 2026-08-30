#include "PreviewConfigurationEditor.h"

#include <algorithm>
#include <array>
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
        constexpr float SectionGap = 10.0f;
        constexpr float ScrollStep = 52.0f;

        const std::array<std::string, ConfigurationWeightGroupControl::MaximumRows> EmptyWeightLabels = {};
        const std::array<uint32_t, ConfigurationWeightGroupControl::MaximumRows> EmptyWeights = {};

        bool profilesEqualForTask89Draft(const PixelShipGenerator::ShipGenerationProfile& first, const PixelShipGenerator::ShipGenerationProfile& second)
        {
            return first.LargeWeaponChance == second.LargeWeaponChance &&
                first.LargeWeaponScalePercent == second.LargeWeaponScalePercent &&
                first.MaximumLargeWeaponGroups == second.MaximumLargeWeaponGroups &&
                first.PaletteHullValueOffset == second.PaletteHullValueOffset &&
                first.NoseWidthPercent.Min == second.NoseWidthPercent.Min &&
                first.NoseWidthPercent.Max == second.NoseWidthPercent.Max &&
                first.VisualAnchorWeights.Silhouette == second.VisualAnchorWeights.Silhouette &&
                first.VisualAnchorWeights.Cockpit == second.VisualAnchorWeights.Cockpit &&
                first.VisualAnchorWeights.Wings == second.VisualAnchorWeights.Wings &&
                first.VisualAnchorWeights.Engines == second.VisualAnchorWeights.Engines &&
                first.VisualAnchorWeights.Weapons == second.VisualAnchorWeights.Weapons &&
                first.VisualAnchorWeights.MajorFeature == second.VisualAnchorWeights.MajorFeature &&
                first.VisualAnchorWeights.HullLayers == second.VisualAnchorWeights.HullLayers &&
                first.VisualAnchorWeights.CentralCore == second.VisualAnchorWeights.CentralCore &&
                first.VisualAnchorWeights.MacroAsymmetry == second.VisualAnchorWeights.MacroAsymmetry &&
                first.VisualAnchorWeights.NegativeSpace == second.VisualAnchorWeights.NegativeSpace;
        }
    }

    PreviewConfigurationEditor::PreviewConfigurationEditor()
    {
        m_Sections = { {
            { "PRESET", {}, true },
            { "WEAPONS", {}, true },
            { "STRUCTURE", {}, true },
            { "PALETTE MODIFIERS", {}, true },
            { "VISUAL ANCHOR WEIGHTS", {}, true },
            { "VALIDATION", {}, true }
        } };
        m_ActionButtons = { {
            { ConfigurationEditorAction::APPLY, "APPLY", {}, true },
            { ConfigurationEditorAction::CANCEL, "CANCEL", {}, true },
            { ConfigurationEditorAction::RESET, "RESET", {}, true },
            { ConfigurationEditorAction::DUPLICATE, "DUPLICATE", {}, true }
        } };
        m_VisualAnchorWeights.configure("VISUAL ANCHOR WEIGHTS", EmptyWeightLabels, EmptyWeights, 0u);
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
        m_WeaponChance.Dragging = false;
        m_WeaponScale.Dragging = false;
        m_WeaponGroupCount.Dragging = false;
        m_PaletteValueOffset.Dragging = false;
        for (ConfigurationWeightRow& row : m_VisualAnchorWeights.getRows()) { row.Control.Dragging = false; }
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
        changed = m_WeaponChance.updatePointer(x) || changed;
        changed = m_WeaponScale.updatePointer(x) || changed;
        changed = m_WeaponGroupCount.updatePointer(x) || changed;
        changed = m_PaletteValueOffset.updatePointer(x) || changed;
        changed = m_VisualAnchorWeights.updatePointer(x) || changed;
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

        const bool nameFocused = m_Sections[0u].Expanded && m_NameField.activate(x, y);
        if (!nameFocused && !m_NameField.Bounds.contains(x, y)) { m_NameField.Focused = false; }
        if (m_Sections[1u].Expanded)
        {
            m_WeaponChance.beginPointer(x, y);
            m_WeaponScale.beginPointer(x, y);
            m_WeaponGroupCount.beginPointer(x, y);
        }
        if (m_Sections[3u].Expanded) { m_PaletteValueOffset.beginPointer(x, y); }
        if (m_Sections[4u].Expanded) { m_VisualAnchorWeights.beginPointer(x, y); }
    }

    std::optional<ConfigurationEditorEvent> PreviewConfigurationEditor::onMouseRelease(float x, float y)
    {
        if (!m_Open) { return std::nullopt; }
        if (const std::optional<ConfigurationEditorEvent> action = activateAction(x, y); action.has_value()) { return action; }
        if (isWithinContentViewport(x, y) && activateSectionHeader(x, y)) { return std::nullopt; }

        bool changed = false;
        if (isWithinContentViewport(x, y))
        {
            if (m_Sections[1u].Expanded)
            {
                changed = m_WeaponChance.endPointer(x, y) || changed;
                changed = m_WeaponScale.endPointer(x, y) || changed;
                changed = m_WeaponGroupCount.endPointer(x, y) || changed;
            }
            if (m_Sections[2u].Expanded) { changed = m_NoseWidthRange.activate(x, y) || changed; }
            if (m_Sections[3u].Expanded) { changed = m_PaletteValueOffset.endPointer(x, y) || changed; }
            if (m_Sections[4u].Expanded) { changed = m_VisualAnchorWeights.endPointer(x, y) || changed; }
        }
        else
        {
            m_WeaponChance.Dragging = false;
            m_WeaponScale.Dragging = false;
            m_WeaponGroupCount.Dragging = false;
            m_PaletteValueOffset.Dragging = false;
            for (ConfigurationWeightRow& row : m_VisualAnchorWeights.getRows()) { row.Control.Dragging = false; }
        }

        if (changed)
        {
            syncDraftFromControls();
            refreshValidation();
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
        if (!m_Open) { return false; }
        const bool changed = m_NameField.onTextEntered(unicode);
        if (changed) { rebuildLayout(); }
        return changed;
    }

    ConfigurationEditorEvent PreviewConfigurationEditor::createCancelEvent() const
    {
        return { ConfigurationEditorAction::CANCEL };
    }

    void PreviewConfigurationEditor::setValidationResult(const PixelShipGenerator::ValidationResult& result)
    {
        m_ValidationResult = result;
        rebuildLayout();
    }

    const PixelShipGenerator::ValidationResult& PreviewConfigurationEditor::getValidationResult() const { return m_ValidationResult; }
    const std::string& PreviewConfigurationEditor::getName() const { return m_NameField.Value; }
    const PixelShipGenerator::ShipGenerationProfile& PreviewConfigurationEditor::getDraftProfile() const { return m_DraftProfile; }
    const PixelShipGenerator::ShipGenerationProfile& PreviewConfigurationEditor::getInitialProfile() const { return m_InitialProfile; }

    bool PreviewConfigurationEditor::hasUnsavedChanges() const
    {
        return m_NameField.Value != m_InitialName || !profilesEqualForTask89Draft(m_DraftProfile, m_InitialProfile);
    }

    float PreviewConfigurationEditor::getScrollOffset() const { return m_ScrollOffset; }
    float PreviewConfigurationEditor::getMaximumScrollOffset() const { return m_MaximumScrollOffset; }
    ConfigurationEditorRect PreviewConfigurationEditor::getPanelBounds() const { return m_PanelBounds; }
    ConfigurationEditorRect PreviewConfigurationEditor::getContentViewport() const { return m_ContentViewport; }
    const std::array<ConfigurationEditorSectionState, PreviewConfigurationEditor::SectionCount>& PreviewConfigurationEditor::getSections() const { return m_Sections; }
    const std::array<ConfigurationEditorActionButton, 4u>& PreviewConfigurationEditor::getActionButtons() const { return m_ActionButtons; }
    const ConfigurationTextField& PreviewConfigurationEditor::getNameField() const { return m_NameField; }
    const ConfigurationIntegerControl& PreviewConfigurationEditor::getWeaponChanceControl() const { return m_WeaponChance; }
    const ConfigurationIntegerControl& PreviewConfigurationEditor::getWeaponScaleControl() const { return m_WeaponScale; }
    const ConfigurationIntegerControl& PreviewConfigurationEditor::getWeaponGroupCountControl() const { return m_WeaponGroupCount; }
    const ConfigurationIntegerControl& PreviewConfigurationEditor::getPaletteValueOffsetControl() const { return m_PaletteValueOffset; }
    const ConfigurationRangeControl& PreviewConfigurationEditor::getNoseWidthRangeControl() const { return m_NoseWidthRange; }
    const ConfigurationWeightGroupControl& PreviewConfigurationEditor::getVisualAnchorWeightsControl() const { return m_VisualAnchorWeights; }

    void PreviewConfigurationEditor::setSectionExpanded(std::size_t sectionIndex, bool expanded)
    {
        if (sectionIndex >= m_Sections.size()) { return; }
        m_Sections[sectionIndex].Expanded = expanded;
        rebuildLayout();
    }

    void PreviewConfigurationEditor::configureControlsFromDraft()
    {
        m_WeaponChance.configure("LARGE WEAPON CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1, static_cast<int32_t>(m_DraftProfile.LargeWeaponChance));
        m_WeaponScale.configure("LARGE WEAPON SCALE", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 25, 300, 5, static_cast<int32_t>(m_DraftProfile.LargeWeaponScalePercent));
        m_WeaponGroupCount.configure("MAX WEAPON GROUPS", ConfigurationNumericSemantic::COUNT, 0, 6, 1, static_cast<int32_t>(m_DraftProfile.MaximumLargeWeaponGroups));
        m_PaletteValueOffset.configure("HULL VALUE OFFSET", ConfigurationNumericSemantic::SIGNED_OFFSET, -50, 50, 1, m_DraftProfile.PaletteHullValueOffset);
        m_NoseWidthRange.configure("NOSE WIDTH RANGE", 0, 100, 1, static_cast<int32_t>(m_DraftProfile.NoseWidthPercent.Min), static_cast<int32_t>(m_DraftProfile.NoseWidthPercent.Max));

        const std::array<std::string, ConfigurationWeightGroupControl::MaximumRows> labels = {
            "SILHOUETTE", "COCKPIT", "WINGS", "ENGINES", "WEAPONS", "MAJOR FEATURE", "HULL LAYERS", "CENTRAL CORE", "MACRO ASYMMETRY", "NEGATIVE SPACE"
        };
        const std::array<uint32_t, ConfigurationWeightGroupControl::MaximumRows> weights = {
            m_DraftProfile.VisualAnchorWeights.Silhouette,
            m_DraftProfile.VisualAnchorWeights.Cockpit,
            m_DraftProfile.VisualAnchorWeights.Wings,
            m_DraftProfile.VisualAnchorWeights.Engines,
            m_DraftProfile.VisualAnchorWeights.Weapons,
            m_DraftProfile.VisualAnchorWeights.MajorFeature,
            m_DraftProfile.VisualAnchorWeights.HullLayers,
            m_DraftProfile.VisualAnchorWeights.CentralCore,
            m_DraftProfile.VisualAnchorWeights.MacroAsymmetry,
            m_DraftProfile.VisualAnchorWeights.NegativeSpace
        };
        m_VisualAnchorWeights.configure("VISUAL ANCHOR WEIGHTS", labels, weights, 10u, 500u);
    }

    void PreviewConfigurationEditor::syncDraftFromControls()
    {
        m_DraftProfile.LargeWeaponChance = static_cast<uint32_t>(m_WeaponChance.Value);
        m_DraftProfile.LargeWeaponScalePercent = static_cast<uint32_t>(m_WeaponScale.Value);
        m_DraftProfile.MaximumLargeWeaponGroups = static_cast<uint32_t>(m_WeaponGroupCount.Value);
        m_DraftProfile.PaletteHullValueOffset = m_PaletteValueOffset.Value;
        m_DraftProfile.NoseWidthPercent.Min = static_cast<uint32_t>(m_NoseWidthRange.MinimumValue);
        m_DraftProfile.NoseWidthPercent.Max = static_cast<uint32_t>(m_NoseWidthRange.MaximumValue);
        const auto& rows = m_VisualAnchorWeights.getRows();
        m_DraftProfile.VisualAnchorWeights.Silhouette = static_cast<uint32_t>(rows[0u].Control.Value);
        m_DraftProfile.VisualAnchorWeights.Cockpit = static_cast<uint32_t>(rows[1u].Control.Value);
        m_DraftProfile.VisualAnchorWeights.Wings = static_cast<uint32_t>(rows[2u].Control.Value);
        m_DraftProfile.VisualAnchorWeights.Engines = static_cast<uint32_t>(rows[3u].Control.Value);
        m_DraftProfile.VisualAnchorWeights.Weapons = static_cast<uint32_t>(rows[4u].Control.Value);
        m_DraftProfile.VisualAnchorWeights.MajorFeature = static_cast<uint32_t>(rows[5u].Control.Value);
        m_DraftProfile.VisualAnchorWeights.HullLayers = static_cast<uint32_t>(rows[6u].Control.Value);
        m_DraftProfile.VisualAnchorWeights.CentralCore = static_cast<uint32_t>(rows[7u].Control.Value);
        m_DraftProfile.VisualAnchorWeights.MacroAsymmetry = static_cast<uint32_t>(rows[8u].Control.Value);
        m_DraftProfile.VisualAnchorWeights.NegativeSpace = static_cast<uint32_t>(rows[9u].Control.Value);
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

        const auto beginSection = [&](std::size_t index)
        {
            m_Sections[index].HeaderBounds = { contentLeft, y, contentWidth, SectionHeaderHeight };
            y += SectionHeaderHeight;
            return m_Sections[index].Expanded;
        };
        const auto endSection = [&]() { y += SectionGap; };

        if (beginSection(0u))
        {
            m_NameField.Bounds = { contentLeft, y + 2.0f, contentWidth, RowHeight - 4.0f };
            y += RowHeight;
        }
        endSection();

        if (beginSection(1u))
        {
            m_WeaponChance.setRowBounds({ contentLeft, y, contentWidth, RowHeight - 4.0f }); y += RowHeight;
            m_WeaponScale.setRowBounds({ contentLeft, y, contentWidth, RowHeight - 4.0f }); y += RowHeight;
            m_WeaponGroupCount.setRowBounds({ contentLeft, y, contentWidth, RowHeight - 4.0f }); y += RowHeight;
        }
        endSection();

        if (beginSection(2u))
        {
            m_NoseWidthRange.setRowBounds({ contentLeft, y, contentWidth, RowHeight - 4.0f }); y += RowHeight;
        }
        endSection();

        if (beginSection(3u))
        {
            m_PaletteValueOffset.setRowBounds({ contentLeft, y, contentWidth, RowHeight - 4.0f }); y += RowHeight;
        }
        endSection();

        if (beginSection(4u))
        {
            const float groupHeight = 22.0f + static_cast<float>(m_VisualAnchorWeights.getRowCount()) * 30.0f;
            m_VisualAnchorWeights.setBounds({ contentLeft, y, contentWidth, groupHeight });
            y += groupHeight;
        }
        endSection();

        if (beginSection(5u))
        {
            const std::size_t visibleMessages = std::min<std::size_t>(5u, m_ValidationResult.Errors.size() + m_ValidationResult.Warnings.size());
            y += std::max(48.0f, 22.0f + static_cast<float>(visibleMessages) * 30.0f);
        }
        endSection();

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
        for (ConfigurationEditorSectionState& section : m_Sections)
        {
            if (!section.HeaderBounds.contains(x, y)) { continue; }
            section.Expanded = !section.Expanded;
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
}
