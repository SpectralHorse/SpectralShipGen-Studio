#pragma once

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "PreviewWorkspace.h"

namespace PixelShipGeneratorPreview
{
    struct PreviewWorkspaceNavigationButton
    {
        PreviewWorkspace Workspace = PreviewWorkspace::GENERATE;
        sf::FloatRect Bounds;
        std::string Label;
        bool Active = false;
    };

    class PreviewWorkspaceNavigation
    {
    public:
        PreviewWorkspaceNavigation();

        void setActiveWorkspace(PreviewWorkspace workspace);
        void onMouseMove(sf::Vector2f position);
        bool onMousePress(sf::Vector2f position);
        std::optional<PreviewWorkspace> onMouseRelease(sf::Vector2f position);
        void cancelPress();

        const std::vector<PreviewWorkspaceNavigationButton>& getButtons() const;
        int32_t getHoveredButtonIndex() const;
        int32_t getPressedButtonIndex() const;

    private:
        void buildLayout();
        int32_t findButtonIndex(sf::Vector2f position) const;

    private:
        std::vector<PreviewWorkspaceNavigationButton> m_Buttons;
        int32_t m_HoveredButtonIndex = -1;
        int32_t m_PressedButtonIndex = -1;
    };
}
