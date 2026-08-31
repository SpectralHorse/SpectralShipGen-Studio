#include "PreviewWorkspaceNavigation.h"

#include "PreviewState.h"

#include <utility>

namespace SpectralShipGenStudioPreview
{
    PreviewWorkspaceNavigation::PreviewWorkspaceNavigation()
    {
        buildLayout();
        setActiveWorkspace(PreviewWorkspace::GENERATE);
    }

    void PreviewWorkspaceNavigation::setActiveWorkspace(PreviewWorkspace workspace)
    {
        for (PreviewWorkspaceNavigationButton& button : m_Buttons)
        {
            button.Active = button.Workspace == workspace;
        }
    }

    void PreviewWorkspaceNavigation::onMouseMove(sf::Vector2f position)
    {
        m_HoveredButtonIndex = findButtonIndex(position);
    }

    bool PreviewWorkspaceNavigation::onMousePress(sf::Vector2f position)
    {
        m_PressedButtonIndex = findButtonIndex(position);
        return m_PressedButtonIndex >= 0;
    }

    std::optional<PreviewWorkspace> PreviewWorkspaceNavigation::onMouseRelease(sf::Vector2f position)
    {
        const int32_t releasedIndex = findButtonIndex(position);
        const int32_t pressedIndex = m_PressedButtonIndex;
        m_PressedButtonIndex = -1;
        if (releasedIndex < 0 || releasedIndex != pressedIndex) { return std::nullopt; }
        return m_Buttons[static_cast<std::size_t>(releasedIndex)].Workspace;
    }

    void PreviewWorkspaceNavigation::cancelPress()
    {
        m_PressedButtonIndex = -1;
    }

    const std::vector<PreviewWorkspaceNavigationButton>& PreviewWorkspaceNavigation::getButtons() const { return m_Buttons; }
    int32_t PreviewWorkspaceNavigation::getHoveredButtonIndex() const { return m_HoveredButtonIndex; }
    int32_t PreviewWorkspaceNavigation::getPressedButtonIndex() const { return m_PressedButtonIndex; }

    void PreviewWorkspaceNavigation::buildLayout()
    {
        constexpr float Margin = 6.0f;
        constexpr float Spacing = 4.0f;
        constexpr float Height = static_cast<float>(PreviewWorkspaceNavigationHeight) - Margin * 2.0f;
        const float totalWidth = static_cast<float>(PreviewWindowWidth) - Margin * 2.0f;
        const float buttonWidth = (totalWidth - Spacing * static_cast<float>(PreviewWorkspaceCount - 1u)) / static_cast<float>(PreviewWorkspaceCount);

        m_Buttons.clear();
        m_Buttons.reserve(PreviewWorkspaceCount);
        for (const PreviewWorkspaceData& data : getPreviewWorkspaceDataTable())
        {
            const std::size_t index = static_cast<std::size_t>(data.Workspace);
            PreviewWorkspaceNavigationButton button;
            button.Workspace = data.Workspace;
            button.Bounds = sf::FloatRect(Margin + static_cast<float>(index) * (buttonWidth + Spacing), Margin, buttonWidth, Height);
            button.Label = std::to_string(data.ShortcutNumber) + " " + data.Label;
            m_Buttons.push_back(std::move(button));
        }
    }

    int32_t PreviewWorkspaceNavigation::findButtonIndex(sf::Vector2f position) const
    {
        for (std::size_t index = 0u; index < m_Buttons.size(); ++index)
        {
            if (m_Buttons[index].Bounds.contains(position)) { return static_cast<int32_t>(index); }
        }
        return -1;
    }
}
