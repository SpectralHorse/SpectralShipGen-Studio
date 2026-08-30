#include "PreviewWorkspace.h"

#include <algorithm>

namespace PixelShipGeneratorPreview
{
    namespace
    {
        constexpr std::array<PreviewWorkspaceData, PreviewWorkspaceCount> WorkspaceData = { {
            { PreviewWorkspace::GENERATE, 1u, "Generate" },
            { PreviewWorkspace::PROFILES, 2u, "Profiles" },
            { PreviewWorkspace::REROLL, 3u, "Reroll" },
            { PreviewWorkspace::INSPECT, 4u, "Inspect" },
            { PreviewWorkspace::FAVORITES, 5u, "Favorites" },
            { PreviewWorkspace::ANIMATION, 6u, "Animation" }
        } };

        constexpr PreviewHelpSection GlobalHelp = { {
            PreviewHelpEntry{ "1-6", "Switch workspaces" },
            PreviewHelpEntry{ "F1", "Contextual Help" },
            PreviewHelpEntry{ "ESC", "Back / Cancel / Close overlay" },
            PreviewHelpEntry{ "B", "Bookmark current ship when available" }
        }, 4u };

        constexpr std::array<PreviewHelpSection, PreviewWorkspaceCount> WorkspaceHelp = { {
            PreviewHelpSection{ {
                PreviewHelpEntry{ "SPACE", "Generate one ship" },
                PreviewHelpEntry{ "SHIFT+SPACE", "Generate / open Gallery" },
                PreviewHelpEntry{ "CTRL+O", "Import generation recipe" },
                PreviewHelpEntry{ "CTRL+E", "Export current generation recipe" },
                PreviewHelpEntry{ "LEFT / RIGHT", "Move through generation History" }
            }, 5u },
            PreviewHelpSection{ {
                PreviewHelpEntry{ "CTRL+D", "Duplicate the open profile / palette" },
                PreviewHelpEntry{ "CTRL+O", "Import the open preset category" },
                PreviewHelpEntry{ "CTRL+E", "Export the open saved user preset" }
            }, 3u },
            PreviewHelpSection{ {
                PreviewHelpEntry{ "SPACE", "Generate configured reroll candidate" },
                PreviewHelpEntry{ "ENTER", "Accept current reroll candidate" }
            }, 2u },
            PreviewHelpSection{ {
                PreviewHelpEntry{ "M", "Cycle diagnostic mask view" },
                PreviewHelpEntry{ "F8", "Toggle generation-stage view" },
                PreviewHelpEntry{ "[ / ]", "Previous / next generation stage" }
            }, 3u },
            PreviewHelpSection{ {
                PreviewHelpEntry{ "ARROWS", "Move Favorite selection" },
                PreviewHelpEntry{ "ENTER", "Load selected Favorite" },
                PreviewHelpEntry{ "CTRL+E", "Export selected Favorite recipe" }
            }, 3u },
            PreviewHelpSection{ {
                PreviewHelpEntry{ "SPACE", "Play / Pause animation" },
                PreviewHelpEntry{ "LEFT / RIGHT", "Previous / next frame while paused" }
            }, 2u }
        } };
    }

    const std::array<PreviewWorkspaceData, PreviewWorkspaceCount>& getPreviewWorkspaceDataTable()
    {
        return WorkspaceData;
    }

    const PreviewWorkspaceData& getPreviewWorkspaceData(PreviewWorkspace workspace)
    {
        const std::size_t index = static_cast<std::size_t>(workspace);
        return index < WorkspaceData.size() ? WorkspaceData[index] : WorkspaceData.front();
    }

    const char* getPreviewWorkspaceName(PreviewWorkspace workspace)
    {
        return getPreviewWorkspaceData(workspace).Label;
    }

    std::optional<PreviewWorkspace> getPreviewWorkspaceForShortcut(uint32_t shortcutNumber, bool keyboardInputFocused)
    {
        if (keyboardInputFocused) { return std::nullopt; }
        const auto iterator = std::find_if(WorkspaceData.begin(), WorkspaceData.end(), [shortcutNumber](const PreviewWorkspaceData& data) { return data.ShortcutNumber == shortcutNumber; });
        return iterator == WorkspaceData.end() ? std::nullopt : std::optional<PreviewWorkspace>(iterator->Workspace);
    }

    PreviewMode getDefaultPreviewMode(PreviewWorkspace workspace)
    {
        switch (workspace)
        {
        case PreviewWorkspace::GENERATE: return PreviewMode::STATIC;
        case PreviewWorkspace::PROFILES: return PreviewMode::STATIC;
        case PreviewWorkspace::REROLL: return PreviewMode::REROLL_STUDIO;
        case PreviewWorkspace::INSPECT: return PreviewMode::STATIC;
        case PreviewWorkspace::FAVORITES: return PreviewMode::FAVORITES;
        case PreviewWorkspace::ANIMATION: return PreviewMode::FRAME_INSPECTION;
        case PreviewWorkspace::PREVIEW_WORKSPACE_END:
        default: return PreviewMode::STATIC;
        }
    }

    bool isPreviewModeOwnedByWorkspace(PreviewWorkspace workspace, PreviewMode mode)
    {
        switch (workspace)
        {
        case PreviewWorkspace::GENERATE:
            return mode == PreviewMode::STATIC || mode == PreviewMode::ANIMATION || mode == PreviewMode::GALLERY || mode == PreviewMode::CALIBRATION;
        case PreviewWorkspace::PROFILES:
            return mode == PreviewMode::STATIC || mode == PreviewMode::CONFIGURATION_EDITOR;
        case PreviewWorkspace::REROLL:
            return mode == PreviewMode::REROLL_STUDIO;
        case PreviewWorkspace::INSPECT:
            return mode == PreviewMode::STATIC;
        case PreviewWorkspace::FAVORITES:
            return mode == PreviewMode::FAVORITES;
        case PreviewWorkspace::ANIMATION:
            return mode == PreviewMode::ANIMATION || mode == PreviewMode::FRAME_INSPECTION;
        case PreviewWorkspace::PREVIEW_WORKSPACE_END:
        default:
            return false;
        }
    }

    const PreviewHelpSection& getPreviewGlobalHelpSection()
    {
        return GlobalHelp;
    }

    const PreviewHelpSection& getPreviewWorkspaceHelpSection(PreviewWorkspace workspace)
    {
        const std::size_t index = static_cast<std::size_t>(workspace);
        return index < WorkspaceHelp.size() ? WorkspaceHelp[index] : WorkspaceHelp.front();
    }

    PreviewWorkspaceSession::PreviewWorkspaceSession()
    {
        for (const PreviewWorkspaceData& data : WorkspaceData)
        {
            m_LocalModes[static_cast<std::size_t>(data.Workspace)] = getDefaultPreviewMode(data.Workspace);
        }
    }

    PreviewWorkspace PreviewWorkspaceSession::getActiveWorkspace() const
    {
        return m_ActiveWorkspace;
    }

    PreviewMode PreviewWorkspaceSession::getLocalMode(PreviewWorkspace workspace) const
    {
        const std::size_t index = static_cast<std::size_t>(workspace);
        return index < m_LocalModes.size() ? m_LocalModes[index] : PreviewMode::STATIC;
    }

    void PreviewWorkspaceSession::rememberActiveMode(PreviewMode mode)
    {
        if (!isPreviewModeOwnedByWorkspace(m_ActiveWorkspace, mode)) { return; }
        m_LocalModes[static_cast<std::size_t>(m_ActiveWorkspace)] = mode;
    }

    PreviewMode PreviewWorkspaceSession::switchTo(PreviewWorkspace workspace, PreviewMode currentMode)
    {
        rememberActiveMode(currentMode);
        if (static_cast<std::size_t>(workspace) >= PreviewWorkspaceCount) { workspace = PreviewWorkspace::GENERATE; }
        m_ActiveWorkspace = workspace;
        return m_LocalModes[static_cast<std::size_t>(workspace)];
    }

    PreviewBackAction resolvePreviewBackAction(const PreviewBackContext& context)
    {
        if (context.KeyboardInputFocused) { return PreviewBackAction::RELEASE_KEYBOARD_FOCUS; }
        if (context.HelpVisible) { return PreviewBackAction::CLOSE_HELP; }
        if (context.GenerationInspectorVisible || context.PaletteInspectorVisible) { return PreviewBackAction::CLOSE_CONTEXT_OVERLAY; }
        if (context.Mode == PreviewMode::CONFIGURATION_EDITOR) { return PreviewBackAction::CANCEL_CONFIGURATION_EDITOR; }
        if (context.Mode == PreviewMode::GALLERY) { return PreviewBackAction::CLOSE_GALLERY; }
        if (context.Mode == PreviewMode::CALIBRATION) { return PreviewBackAction::EXIT_CALIBRATION; }
        if (context.Workspace == PreviewWorkspace::REROLL && context.RerollCandidateValid) { return PreviewBackAction::DISCARD_REROLL_CANDIDATE; }
        return PreviewBackAction::NONE;
    }
}
