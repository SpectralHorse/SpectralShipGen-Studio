#include "PreviewRegressionSuites.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>

#include "PreviewCollectionSession.h"
#include "PreviewCommand.h"
#if PIXEL_SHIP_GENERATOR_PREVIEW_HAS_SFML
#include "PreviewCommandPanel.h"
#endif
#include "FactionProfileSelection.h"
#include "PaletteProfileSelection.h"
#include "PreviewConfigurationEditor.h"
#include "PreviewWorkspace.h"
#include "RuntimeCustomPresetWorkspace.h"
#include <PixelShipGenerator/ShipGenerationProfile.h>
#include "StructuralProfileSelection.h"

namespace PixelShipGeneratorTests
{
    namespace
    {
        int fail(const std::string& message)
        {
            std::cerr << "Preview workspace regression failed: " << message << '\n';
            return 1;
        }

        PixelShipGeneratorPreview::PreviewGenerationRecipe makeRecipe(uint64_t seed)
        {
            PixelShipGeneratorPreview::PreviewGenerationRecipe recipe;
            recipe.Seeds = PixelShipGenerator::deriveShipGenerationSeeds(seed);
            recipe.Dimensions = { 96u, 64u };
            recipe.Style = PixelShipGenerator::ShipStyle::INDUSTRIAL;
            recipe.Faction = PixelShipGenerator::ShipFactionType::CORPORATE;
            return recipe;
        }

        bool containsShortcut(const PixelShipGeneratorPreview::PreviewHelpSection& section, const std::string& shortcut)
        {
            for (std::size_t index = 0u; index < section.Count; ++index)
            {
                if (section.Entries[index].Shortcut == shortcut) { return true; }
            }
            return false;
        }
    }

    int runPreviewWorkspaceRegression()
    {
        using namespace PixelShipGeneratorPreview;

        PreviewWorkspaceSession session;
        if (session.getActiveWorkspace() != PreviewWorkspace::GENERATE || session.getLocalMode(PreviewWorkspace::GENERATE) != PreviewMode::STATIC)
        {
            return fail("Generate is not the default workspace/mode");
        }

        const std::array<PreviewWorkspace, PreviewWorkspaceCount> workspaces = {
            PreviewWorkspace::GENERATE,
            PreviewWorkspace::PROFILES,
            PreviewWorkspace::REROLL,
            PreviewWorkspace::INSPECT,
            PreviewWorkspace::FAVORITES,
            PreviewWorkspace::ANIMATION
        };
        for (std::size_t index = 0u; index < workspaces.size(); ++index)
        {
            const uint32_t shortcut = static_cast<uint32_t>(index + 1u);
            const auto resolved = getPreviewWorkspaceForShortcut(shortcut);
            if (!resolved.has_value() || *resolved != workspaces[index]) { return fail("1-6 workspace shortcut mapping is incorrect"); }
            if (getPreviewWorkspaceForShortcut(shortcut, true).has_value()) { return fail("focused input did not suppress a global workspace shortcut"); }
        }
        if (getPreviewWorkspaceForShortcut(0u).has_value() || getPreviewWorkspaceForShortcut(7u).has_value()) { return fail("invalid workspace shortcut resolved unexpectedly"); }

        session.rememberActiveMode(PreviewMode::GALLERY);
        if (session.switchTo(PreviewWorkspace::PROFILES, PreviewMode::GALLERY) != PreviewMode::STATIC) { return fail("Profiles did not open in its default local mode"); }
        session.rememberActiveMode(PreviewMode::CONFIGURATION_EDITOR);
        if (session.switchTo(PreviewWorkspace::INSPECT, PreviewMode::CONFIGURATION_EDITOR) != PreviewMode::STATIC) { return fail("Inspect did not use its own local mode"); }
        if (session.switchTo(PreviewWorkspace::GENERATE, PreviewMode::STATIC) != PreviewMode::GALLERY) { return fail("Generate Gallery state was not retained across workspace switches"); }
        if (session.switchTo(PreviewWorkspace::PROFILES, PreviewMode::GALLERY) != PreviewMode::CONFIGURATION_EDITOR) { return fail("Profiles editor state was not retained across workspace switches"); }
        if (session.switchTo(PreviewWorkspace::REROLL, PreviewMode::CONFIGURATION_EDITOR) != PreviewMode::REROLL_STUDIO) { return fail("Reroll default mode is not Reroll Studio"); }
        if (session.switchTo(PreviewWorkspace::FAVORITES, PreviewMode::REROLL_STUDIO) != PreviewMode::FAVORITES) { return fail("Favorites default mode is incorrect"); }
        if (session.switchTo(PreviewWorkspace::ANIMATION, PreviewMode::FAVORITES) != PreviewMode::FRAME_INSPECTION) { return fail("Animation default mode is not paused frame inspection"); }
        if (!isPreviewModeOwnedByWorkspace(PreviewWorkspace::GENERATE, PreviewMode::ANIMATION)) { return fail("Generate no longer owns normal IDLE playback mode"); }
        session.switchTo(PreviewWorkspace::GENERATE, PreviewMode::FRAME_INSPECTION);
        session.rememberActiveMode(PreviewMode::ANIMATION);
        session.switchTo(PreviewWorkspace::ANIMATION, PreviewMode::ANIMATION);
        if (session.switchTo(PreviewWorkspace::GENERATE, PreviewMode::FRAME_INSPECTION) != PreviewMode::ANIMATION) { return fail("Generate IDLE playback state was not retained across Animation workspace switching"); }

        RuntimeCustomPresetWorkspace presetWorkspace;
        const auto structuralEntries = buildStructuralProfileSelection(presetWorkspace);
        const auto factionEntries = buildFactionProfileSelection(presetWorkspace);
        const auto paletteEntries = buildPaletteProfileSelection(presetWorkspace);
        const std::size_t structuralCount = structuralEntries.size() - 1u;
        const std::size_t factionCount = factionEntries.size() - 1u;
        const std::size_t paletteCount = paletteEntries.size() - 1u;
        if (structuralEntries.back().Kind != StructuralProfileSelectionKind::ADD_PROFILE ||
            factionEntries.back().Kind != FactionProfileSelectionKind::ADD_FACTION ||
            paletteEntries.back().Kind != PaletteProfileSelectionKind::ADD_PALETTE)
        {
            return fail("profile selector action entry is no longer a trailing explicit editor action");
        }
        if (getWrappedPreviewSelectorIndex(0u, -1, structuralCount) != structuralCount - 1u || getWrappedPreviewSelectorIndex(structuralCount - 1u, 1, structuralCount) != 0u)
        {
            return fail("structural profile selector does not wrap bidirectionally");
        }
        if (getWrappedPreviewSelectorIndex(0u, -1, factionCount) != factionCount - 1u || getWrappedPreviewSelectorIndex(factionCount - 1u, 1, factionCount) != 0u)
        {
            return fail("faction profile selector does not wrap bidirectionally");
        }
        if (getWrappedPreviewSelectorIndex(0u, -1, paletteCount) != paletteCount - 1u || getWrappedPreviewSelectorIndex(paletteCount - 1u, 1, paletteCount) != 0u)
        {
            return fail("palette selector does not wrap bidirectionally");
        }
        if (structuralEntries[getWrappedPreviewSelectorIndex(structuralCount - 1u, 1, structuralCount)].Kind == StructuralProfileSelectionKind::ADD_PROFILE ||
            factionEntries[getWrappedPreviewSelectorIndex(factionCount - 1u, 1, factionCount)].Kind == FactionProfileSelectionKind::ADD_FACTION ||
            paletteEntries[getWrappedPreviewSelectorIndex(paletteCount - 1u, 1, paletteCount)].Kind == PaletteProfileSelectionKind::ADD_PALETTE)
        {
            return fail("selector cycling reached an explicit editor/add action");
        }
        if (getPreviewCommandData(PreviewCommandType::OPEN_STRUCTURAL_EDITOR).Label[0] == '\0' ||
            getPreviewCommandData(PreviewCommandType::OPEN_FACTION_EDITOR).Label[0] == '\0' ||
            getPreviewCommandData(PreviewCommandType::OPEN_PALETTE_EDITOR).Label[0] == '\0')
        {
            return fail("explicit profile editor actions are unavailable");
        }

        const PreviewGenerationRecipe recipe = makeRecipe(940094u);
        PreviewCollectionSession collection(recipe);
        const PreviewGenerationRecipe recipeBeforeSwitching = collection.getCurrentRecipe();
        session.switchTo(PreviewWorkspace::GENERATE, PreviewMode::FRAME_INSPECTION);
        session.switchTo(PreviewWorkspace::INSPECT, PreviewMode::STATIC);
        session.switchTo(PreviewWorkspace::ANIMATION, PreviewMode::STATIC);
        if (collection.getCurrentRecipe() != recipeBeforeSwitching || collection.getHistoryCount() != 1u)
        {
            return fail("workspace switching changed the shared generation session");
        }

        PreviewConfigurationEditor editor;
        editor.setPanelBounds({ 0.0f, 40.0f, 760.0f, 900.0f });
        editor.openStructuralProfile("Focus 2 Space", PixelShipGenerator::ShipGenerationProfile{});
        const ConfigurationTextField& nameField = editor.getNameField();
        editor.onMousePress(nameField.Bounds.Left + nameField.Bounds.Width * 0.5f, nameField.Bounds.Top + nameField.Bounds.Height * 0.5f);
        if (!editor.hasKeyboardFocus()) { return fail("configuration editor did not report name-field keyboard focus"); }
        if (getPreviewWorkspaceForShortcut(2u, editor.hasKeyboardFocus()).has_value()) { return fail("numeric input focus did not suppress workspace navigation"); }
        editor.releaseKeyboardFocus();
        if (editor.hasKeyboardFocus() || !getPreviewWorkspaceForShortcut(2u, editor.hasKeyboardFocus()).has_value()) { return fail("releasing editor focus did not restore workspace routing"); }

        PreviewBackContext back;
        back.KeyboardInputFocused = true;
        if (resolvePreviewBackAction(back) != PreviewBackAction::RELEASE_KEYBOARD_FOCUS) { return fail("Esc does not prioritize focused input"); }
        back = {};
        back.HelpVisible = true;
        if (resolvePreviewBackAction(back) != PreviewBackAction::CLOSE_HELP) { return fail("Esc does not close Help"); }
        back = {};
        back.GenerationInspectorVisible = true;
        if (resolvePreviewBackAction(back) != PreviewBackAction::CLOSE_CONTEXT_OVERLAY) { return fail("Esc does not close Inspect overlays"); }
        back = {};
        back.Mode = PreviewMode::CONFIGURATION_EDITOR;
        if (resolvePreviewBackAction(back) != PreviewBackAction::CANCEL_CONFIGURATION_EDITOR) { return fail("Esc does not cancel the configuration editor"); }
        back.Mode = PreviewMode::GALLERY;
        if (resolvePreviewBackAction(back) != PreviewBackAction::CLOSE_GALLERY) { return fail("Esc does not close Gallery"); }
        back.Mode = PreviewMode::CALIBRATION;
        if (resolvePreviewBackAction(back) != PreviewBackAction::EXIT_CALIBRATION) { return fail("Esc does not exit Calibration"); }
        back = {};
        back.Workspace = PreviewWorkspace::REROLL;
        back.Mode = PreviewMode::REROLL_STUDIO;
        back.RerollCandidateValid = true;
        if (resolvePreviewBackAction(back) != PreviewBackAction::DISCARD_REROLL_CANDIDATE) { return fail("Esc does not discard a transient reroll candidate"); }
        back.RerollCandidateValid = false;
        if (resolvePreviewBackAction(back) != PreviewBackAction::NONE || resolvePreviewBackAction(back) != PreviewBackAction::NONE) { return fail("repeated Esc with no contextual target is not a no-op"); }

        const PreviewHelpSection& globalHelp = getPreviewGlobalHelpSection();
        if (!containsShortcut(globalHelp, "1-6") || !containsShortcut(globalHelp, "F1") || !containsShortcut(globalHelp, "ESC")) { return fail("global contextual Help is incomplete"); }
        const PreviewHelpSection& generateHelp = getPreviewWorkspaceHelpSection(PreviewWorkspace::GENERATE);
        if (!containsShortcut(generateHelp, "SPACE") || !containsShortcut(generateHelp, "ENTER") || !containsShortcut(generateHelp, "GALLERY RMB") || !containsShortcut(getPreviewWorkspaceHelpSection(PreviewWorkspace::ANIMATION), "SPACE"))
        {
            return fail("workspace Help does not expose Generate/Gallery contextual actions");
        }

        const PreviewCommandData& spritesheetCommand = getPreviewCommandData(PreviewCommandType::SAVE_SPRITESHEET);
        if (std::string(spritesheetCommand.Label) != "Save Spritesheet" || std::string(spritesheetCommand.Description).find("IDLE") == std::string::npos)
        {
            return fail("Generate IDLE spritesheet export command metadata is missing");
        }
#if PIXEL_SHIP_GENERATOR_PREVIEW_HAS_SFML
        PreviewCommandPanel generatePanel;
        bool generateHasSpritesheet = false;
        for (const PreviewCommandPanelButton& button : generatePanel.getButtons())
        {
            if (button.Command.Type == PreviewCommandType::SAVE_SPRITESHEET) { generateHasSpritesheet = true; break; }
        }
        if (!generateHasSpritesheet) { return fail("Generate command panel does not expose Save Spritesheet"); }
#endif

        const PreviewCommandData& helpCommand = getPreviewCommandData(PreviewCommandType::TOGGLE_HELP);
        const PreviewCommandData& backCommand = getPreviewCommandData(PreviewCommandType::BACK_OR_EXIT);
        if (std::string(helpCommand.Shortcut) != "F1") { return fail("F1 was not assigned exclusively to Help metadata"); }
        if (std::string(backCommand.Shortcut) != "ESC") { return fail("Esc metadata no longer routes through Back/Cancel"); }

        const std::array<PreviewCommandType, 3u> directPresetCommands = {
            PreviewCommandType::SELECT_STYLE,
            PreviewCommandType::SELECT_FACTION,
            PreviewCommandType::SELECT_RESOLUTION
        };
        for (const PreviewCommandType command : directPresetCommands)
        {
            if (getPreviewCommandData(command).Shortcut[0] != '\0') { return fail("obsolete direct preset/resolution shortcut remains advertised"); }
        }

        std::cout << "Preview workspace/navigation regression passed.\n";
        return 0;
    }
}
