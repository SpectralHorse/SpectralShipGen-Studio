#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace PixelShipGeneratorPreview
{
    enum class PreviewWorkspace : uint32_t
    {
        GENERATE = 0u,
        PROFILES,
        REROLL,
        INSPECT,
        FAVORITES,
        ANIMATION,
        PREVIEW_WORKSPACE_END
    };

    inline constexpr std::size_t PreviewWorkspaceCount = static_cast<std::size_t>(PreviewWorkspace::PREVIEW_WORKSPACE_END);

    enum class PreviewMode : uint32_t
    {
        STATIC = 0u,
        ANIMATION,
        FRAME_INSPECTION,
        GALLERY,
        FAVORITES,
        REROLL_STUDIO,
        CALIBRATION,
        CONFIGURATION_EDITOR
    };

    struct PreviewWorkspaceData
    {
        PreviewWorkspace Workspace = PreviewWorkspace::GENERATE;
        uint32_t ShortcutNumber = 1u;
        const char* Label = "Generate";
    };

    struct PreviewHelpEntry
    {
        const char* Shortcut = "";
        const char* Description = "";
    };

    struct PreviewHelpSection
    {
        std::array<PreviewHelpEntry, 8u> Entries = {};
        std::size_t Count = 0u;
    };

    const std::array<PreviewWorkspaceData, PreviewWorkspaceCount>& getPreviewWorkspaceDataTable();
    const PreviewWorkspaceData& getPreviewWorkspaceData(PreviewWorkspace workspace);
    const char* getPreviewWorkspaceName(PreviewWorkspace workspace);
    std::optional<PreviewWorkspace> getPreviewWorkspaceForShortcut(uint32_t shortcutNumber, bool keyboardInputFocused = false);
    PreviewMode getDefaultPreviewMode(PreviewWorkspace workspace);
    bool isPreviewModeOwnedByWorkspace(PreviewWorkspace workspace, PreviewMode mode);
    const PreviewHelpSection& getPreviewGlobalHelpSection();
    const PreviewHelpSection& getPreviewWorkspaceHelpSection(PreviewWorkspace workspace);

    class PreviewWorkspaceSession
    {
    public:
        PreviewWorkspaceSession();

        PreviewWorkspace getActiveWorkspace() const;
        PreviewMode getLocalMode(PreviewWorkspace workspace) const;
        void rememberActiveMode(PreviewMode mode);
        PreviewMode switchTo(PreviewWorkspace workspace, PreviewMode currentMode);

    private:
        PreviewWorkspace m_ActiveWorkspace = PreviewWorkspace::GENERATE;
        std::array<PreviewMode, PreviewWorkspaceCount> m_LocalModes = {};
    };

    enum class PreviewBackAction : uint32_t
    {
        NONE = 0u,
        RELEASE_KEYBOARD_FOCUS,
        CLOSE_HELP,
        CLOSE_CONTEXT_OVERLAY,
        CANCEL_CONFIGURATION_EDITOR,
        CLOSE_GALLERY,
        EXIT_CALIBRATION,
        DISCARD_REROLL_CANDIDATE
    };

    struct PreviewBackContext
    {
        PreviewWorkspace Workspace = PreviewWorkspace::GENERATE;
        PreviewMode Mode = PreviewMode::STATIC;
        bool KeyboardInputFocused = false;
        bool HelpVisible = false;
        bool GenerationInspectorVisible = false;
        bool PaletteInspectorVisible = false;
        bool RerollCandidateValid = false;
    };

    PreviewBackAction resolvePreviewBackAction(const PreviewBackContext& context);
}
