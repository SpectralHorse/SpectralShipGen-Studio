#include "PreviewCommand.h"

#include <algorithm>

namespace PixelShipGeneratorPreview
{
    namespace
    {
        constexpr std::array<PreviewCommandData, static_cast<std::size_t>(PreviewCommandType::PREVIEW_COMMAND_TYPE_END)> CommandData = { {
            { PreviewCommandType::GENERATE_NEW, "Generate New", "SPACE", "Generate a completely new ship. In Gallery, generate a new gallery batch.", PreviewCommandGroup::GENERATION },
            { PreviewCommandType::REROLL, "Reroll", "", "Regenerate only currently unlocked generation channels.", PreviewCommandGroup::GENERATION },
            { PreviewCommandType::GENERATE_FROM_MASTER_SEED, "Known Seed", "", "Generate the current recipe from a known master seed.", PreviewCommandGroup::GENERATION },
            { PreviewCommandType::PREVIOUS_HISTORY, "Previous", "LEFT", "Move to the previous history entry in Generate.", PreviewCommandGroup::NAVIGATION },
            { PreviewCommandType::NEXT_HISTORY, "Next", "RIGHT", "Move to the next history entry in Generate.", PreviewCommandGroup::NAVIGATION },
            { PreviewCommandType::OPEN_GALLERY, "Gallery", "SHIFT+SPACE", "Enter Gallery with a new batch seed.", PreviewCommandGroup::NAVIGATION },
            { PreviewCommandType::OPEN_GALLERY_FROM_SEED, "Gallery Seed", "", "Enter or rebuild Gallery from a known batch seed.", PreviewCommandGroup::NAVIGATION },
            { PreviewCommandType::GALLERY_LEFT, "Gallery Left", "ARROWS", "Move the gallery selection with the arrow keys.", PreviewCommandGroup::NAVIGATION },
            { PreviewCommandType::GALLERY_RIGHT, "Gallery Right", "", "Move the gallery selection right.", PreviewCommandGroup::NAVIGATION },
            { PreviewCommandType::GALLERY_UP, "Gallery Up", "", "Move the gallery selection up.", PreviewCommandGroup::NAVIGATION },
            { PreviewCommandType::GALLERY_DOWN, "Gallery Down", "", "Move the gallery selection down.", PreviewCommandGroup::NAVIGATION },
            { PreviewCommandType::SELECT_GALLERY_CANDIDATE, "Accept Primary", "ENTER", "Accept the highlighted primary Gallery candidate as the current ship.", PreviewCommandGroup::NAVIGATION },
            { PreviewCommandType::ADD_CURRENT_TO_FAVORITES, "Add Favorite", "B", "Bookmark the exact current self-contained generation recipe.", PreviewCommandGroup::FAVORITES },
            { PreviewCommandType::REMOVE_CURRENT_FROM_FAVORITES, "Remove Favorite", "", "Remove the exact current generation recipe from Favorites.", PreviewCommandGroup::FAVORITES },
            { PreviewCommandType::OPEN_FAVORITES, "Favorites", "", "Browse the persistent collection of explicitly bookmarked recipe-backed ships.", PreviewCommandGroup::FAVORITES },
            { PreviewCommandType::CLOSE_FAVORITES, "Back", "", "Return from the Favorites browser to the current ship.", PreviewCommandGroup::FAVORITES },
            { PreviewCommandType::FAVORITES_LEFT, "Favorites Left", "FAV ARROWS", "Move the Favorites selection left.", PreviewCommandGroup::FAVORITES },
            { PreviewCommandType::FAVORITES_RIGHT, "Favorites Right", "", "Move the Favorites selection right.", PreviewCommandGroup::FAVORITES },
            { PreviewCommandType::FAVORITES_UP, "Favorites Up", "", "Move the Favorites selection up.", PreviewCommandGroup::FAVORITES },
            { PreviewCommandType::FAVORITES_DOWN, "Favorites Down", "", "Move the Favorites selection down.", PreviewCommandGroup::FAVORITES },
            { PreviewCommandType::SELECT_FAVORITE, "Open", "ENTER", "Open the selected Favorite as the exact current ship in Generate.", PreviewCommandGroup::FAVORITES },
            { PreviewCommandType::OPEN_FAVORITE_INSPECT, "Inspect", "", "Open the selected Favorite as the exact current ship in Inspect.", PreviewCommandGroup::FAVORITES },
            { PreviewCommandType::OPEN_FAVORITE_ANIMATION, "Animate", "", "Open the selected Favorite as the exact current ship in Animation.", PreviewCommandGroup::FAVORITES },
            { PreviewCommandType::OPEN_FAVORITE_REROLL, "Reroll", "", "Open the selected Favorite as the unchanged Reroll Studio base ship without generating a candidate.", PreviewCommandGroup::FAVORITES },
            { PreviewCommandType::REMOVE_SELECTED_FAVORITE, "Remove", "DELETE", "Remove the selected Favorite after a second explicit confirmation action.", PreviewCommandGroup::FAVORITES },
            { PreviewCommandType::EXPORT_FAVORITE_IMAGE, "Export Image", "", "Export the selected Favorite's exact generated image through the existing image save path.", PreviewCommandGroup::FILES },
            { PreviewCommandType::SELECT_STYLE, "Structural Profile", "", "Select a structural profile through the Profiles/Generate UI.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::PREVIOUS_STYLE, "Previous Profile", "", "Select the previous built-in/runtime structural profile.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::NEXT_STYLE, "Next Profile", "", "Select the next built-in/runtime structural profile, wrapping at the end.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::SELECT_FACTION, "Faction", "", "Select a faction profile through the Profiles/Generate UI.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::PREVIOUS_FACTION, "Previous Faction", "", "Select the previous faction.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::NEXT_FACTION, "Next Faction", "", "Select the next faction.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::PREVIOUS_PALETTE, "Previous Palette", "", "Select the previous faction-default, built-in, or runtime custom palette source.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::NEXT_PALETTE, "Next Palette", "", "Select the next palette source, wrapping at the end.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::PREVIOUS_CONFIGURATION_BUNDLE, "Previous Full Configuration", "", "Select the previous saved Full Configuration bundle.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::NEXT_CONFIGURATION_BUNDLE, "Next Full Configuration", "", "Select the next saved Full Configuration bundle and copy its three embedded components into the current recipe.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::PROFILES_PREVIOUS_SECTION, "Previous Profile Type", "", "Move to the previous Profiles authoring section.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::PROFILES_NEXT_SECTION, "Next Profile Type", "", "Move to the next Profiles authoring section.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::PROFILES_PREVIOUS_ITEM, "Previous Library Item", "", "Select the previous item in the active Profiles section.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::PROFILES_NEXT_ITEM, "Next Library Item", "", "Select the next item in the active Profiles section.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::PROFILES_NEW_DEFAULT, "New / Current", "", "Create a new editable item. Full Configuration starts from the current Structural, Faction and Palette components.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::PROFILES_EDIT_SELECTED, "Edit / Inspect", "", "Open the selected item in the existing editor shell. Built-ins open as duplicate-to-edit drafts.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::PROFILES_DUPLICATE_SELECTED, "Duplicate", "CTRL+D", "Duplicate the selected Profiles item into an editable user-owned entry.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::PROFILES_DELETE_SELECTED, "Delete", "", "Delete the selected user-owned item. Built-ins remain immutable.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::PROFILES_IMPORT_SELECTED, "Import", "CTRL+O", "Import a portable preset or Full Configuration matching the active Profiles section.", PreviewCommandGroup::FILES },
            { PreviewCommandType::PROFILES_EXPORT_SELECTED, "Export", "CTRL+E", "Export the selected saved user preset or Full Configuration.", PreviewCommandGroup::FILES },
            { PreviewCommandType::PROFILES_USE_SELECTED, "Use in Generate", "", "Apply the selected Full Configuration to the shared generation session without mutating the saved bundle.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::OPEN_STRUCTURAL_EDITOR, "Structural Editor", "", "Open the selected structural profile in the Profiles editor.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::OPEN_FACTION_EDITOR, "Faction Editor", "", "Open the selected faction profile in the Profiles editor.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::OPEN_PALETTE_EDITOR, "Palette Editor", "", "Open the selected palette configuration in the Profiles editor.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::SELECT_RESOLUTION, "Resolution", "", "Select an established resolution preset through the Generate UI.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::PREVIOUS_RESOLUTION, "Previous Preset", "", "Select the previous established resolution preset.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::NEXT_RESOLUTION, "Next Preset", "", "Select the next established resolution preset.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::SET_WIDTH, "Set Width", "", "Apply the width selected by the Width slider.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::SET_HEIGHT, "Set Height", "", "Apply the height selected by the Height slider.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::TOGGLE_ASPECT_RATIO_LOCK, "1:1 Lock", "", "Lock Width and Height together as a square. Enabled by default.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::ADD_RESOLUTION_BOOKMARK, "Add Bookmark", "", "Bookmark the current Width x Height dimensions. Up to six values are stored as PreviewApp preferences.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::REMOVE_RESOLUTION_BOOKMARK, "Remove Bookmark", "", "Remove the current Width x Height dimensions from the bookmark list.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::SELECT_RESOLUTION_BOOKMARK, "Resolution Bookmark", "", "Select one of the saved dimension bookmarks. Slots are sorted by width, then height.", PreviewCommandGroup::APPEARANCE },
            { PreviewCommandType::TOGGLE_ATTACHMENTS_ENABLED, "Attach Gen", "", "Enable or disable attachment generation for the current recipe.", PreviewCommandGroup::GENERATION },
            { PreviewCommandType::TOGGLE_STRUCTURE_LOCK, "Struct Lock", "", "Preserve structural geometry while rerolling.", PreviewCommandGroup::LOCKS },
            { PreviewCommandType::TOGGLE_PALETTE_LOCK, "Palette Lock", "", "Preserve the palette while rerolling.", PreviewCommandGroup::LOCKS },
            { PreviewCommandType::TOGGLE_DETAILS_LOCK, "Details Lock", "", "Preserve surface-detail generation while rerolling.", PreviewCommandGroup::LOCKS },
            { PreviewCommandType::TOGGLE_ATTACHMENTS_LOCK, "Attach Lock", "", "Preserve the attachment RNG channel while rerolling.", PreviewCommandGroup::LOCKS },
            { PreviewCommandType::TOGGLE_HELP, "Help", "F1", "Show or hide contextual Help for the active workspace.", PreviewCommandGroup::VIEW },
            { PreviewCommandType::INSPECTION_PREVIOUS_GROUP, "Previous Group", "", "Select the previous semantic inspection group.", PreviewCommandGroup::VIEW },
            { PreviewCommandType::INSPECTION_NEXT_GROUP, "Next Group", "", "Select the next semantic inspection group.", PreviewCommandGroup::VIEW },
            { PreviewCommandType::INSPECTION_PREVIOUS_VIEW, "Previous View", "", "Select the previous semantic view in the active inspection group.", PreviewCommandGroup::VIEW },
            { PreviewCommandType::INSPECTION_NEXT_VIEW, "Next View", "", "Select the next semantic view in the active inspection group.", PreviewCommandGroup::VIEW },
            { PreviewCommandType::TOGGLE_INSPECTION_PRESENTATION, "Overlay / Isolate", "", "Toggle between semantic overlay on the painted ship and isolated semantic pixels.", PreviewCommandGroup::VIEW },
            { PreviewCommandType::OPEN_GENERATE_WORKSPACE, "Return Generate", "", "Return to Generate without changing the current ship or recipe.", PreviewCommandGroup::NAVIGATION },
            { PreviewCommandType::OPEN_ANIMATION_WORKSPACE, "Open Animation", "", "Open the current ship in Animation without regenerating it.", PreviewCommandGroup::NAVIGATION },
            { PreviewCommandType::TOGGLE_GENERATION_INSPECTOR, "Decision Details", "", "Show or hide additional one-ship generation decision details.", PreviewCommandGroup::VIEW },
            { PreviewCommandType::TOGGLE_PALETTE_INSPECTOR, "Palette", "", "Show or hide the resolved palette inspector.", PreviewCommandGroup::VIEW },
            { PreviewCommandType::CYCLE_DIAGNOSTIC_VIEW, "Mask View", "", "Cycle FINAL, individual mask, and combined diagnostic views.", PreviewCommandGroup::VIEW },
            { PreviewCommandType::TOGGLE_GENERATION_STAGE_VIEW, "Gen Stages", "", "Toggle captured hull-generation stage visualization.", PreviewCommandGroup::VIEW },
            { PreviewCommandType::PREVIOUS_GENERATION_STAGE, "Previous Stage", "", "Show the previous captured generation stage.", PreviewCommandGroup::VIEW },
            { PreviewCommandType::NEXT_GENERATION_STAGE, "Next Stage", "", "Show the next captured generation stage.", PreviewCommandGroup::VIEW },
            { PreviewCommandType::CYCLE_ANIMATION_TYPE, "Animation Type", "", "Cycle IDLE, MOVE LEFT, MOVE RIGHT, MOVE UP, MOVE DOWN, and FIRE animation playback.", PreviewCommandGroup::ANIMATION },
            { PreviewCommandType::CYCLE_MOVEMENT_PHASE, "Movement Phase", "", "Cycle ENTER, SUSTAIN, and EXIT for the selected movement animation.", PreviewCommandGroup::ANIMATION },
            { PreviewCommandType::CYCLE_FIRING_TARGET, "Firing Target", "", "Cycle the semantic weapon component/group targeted by FIRE.", PreviewCommandGroup::ANIMATION },
            { PreviewCommandType::APPLY_ANIMATION_STATE, "Apply State", "", "Apply the selected IDLE/MOVE/FIRE state using Task-71 transition and pose-composition semantics.", PreviewCommandGroup::ANIMATION },
            { PreviewCommandType::RETURN_ANIMATION_TO_IDLE, "Return Idle", "", "Exit the current sustained movement through its existing Exit clip and resume IDLE from neutral.", PreviewCommandGroup::ANIMATION },
            { PreviewCommandType::TOGGLE_ANIMATION, "Play / Pause", "SPACE", "Play or pause the normal IDLE preview in Generate, or the selected animation in Animation.", PreviewCommandGroup::ANIMATION },
            { PreviewCommandType::TOGGLE_FRAME_INSPECTION, "Frame Inspect", "", "Pause on the selected animation frame for inspection.", PreviewCommandGroup::ANIMATION },
            { PreviewCommandType::PREVIOUS_FRAME, "Previous Frame", "LEFT", "Show the previous animation frame while inspecting frames.", PreviewCommandGroup::ANIMATION },
            { PreviewCommandType::NEXT_FRAME, "Next Frame", "RIGHT", "Show the next animation frame while inspecting frames.", PreviewCommandGroup::ANIMATION },
            { PreviewCommandType::PIN_CURRENT, "Pin Current", "", "Keep this ship visible for side-by-side comparison.", PreviewCommandGroup::COMPARISON },
            { PreviewCommandType::CLEAR_PIN, "Clear Pin", "", "Clear the pinned comparison reference.", PreviewCommandGroup::COMPARISON },
            { PreviewCommandType::TOGGLE_COMPARISON, "Comparison", "", "Toggle pinned side-by-side comparison view.", PreviewCommandGroup::COMPARISON },
            { PreviewCommandType::SAVE_CURRENT, "Save PNG", "", "Save the current static sprite or inspected animation frame as PNG.", PreviewCommandGroup::FILES },
            { PreviewCommandType::EXPORT_RECIPE, "Export Recipe", "CTRL+E", "Export the current recipe, or the selected Favorite recipe while browsing Favorites, as .shipgen.json.", PreviewCommandGroup::FILES },
            { PreviewCommandType::IMPORT_RECIPE, "Import Recipe", "CTRL+O", "Load a .shipgen.json path entered in the console and regenerate the exact ship.", PreviewCommandGroup::FILES },
            { PreviewCommandType::SAVE_SPRITESHEET, "Save Spritesheet", "", "Save the current IDLE spritesheet from Generate, or the selected advanced animation spritesheet from Animation.", PreviewCommandGroup::FILES },
            { PreviewCommandType::OPEN_REROLL_STUDIO, "Reroll Studio", "", "Open the non-destructive fine-grained attribute reroll workflow.", PreviewCommandGroup::REROLL_STUDIO },
            { PreviewCommandType::REROLL_STUDIO_TOGGLE_DOMAIN, "Toggle Attribute", "", "Selected attributes receive new Task-50 domain seeds; unselected domain seeds are preserved.", PreviewCommandGroup::REROLL_STUDIO },
            { PreviewCommandType::REROLL_STUDIO_SELECT_ALL, "Select All", "", "Select every fine-grained reroll domain.", PreviewCommandGroup::REROLL_STUDIO },
            { PreviewCommandType::REROLL_STUDIO_CLEAR, "Clear", "", "Clear the attribute reroll selection.", PreviewCommandGroup::REROLL_STUDIO },
            { PreviewCommandType::REROLL_STUDIO_SELECT_STRUCTURE, "Structural", "", "Select the Structure-owned Task-50 reroll domains.", PreviewCommandGroup::REROLL_STUDIO },
            { PreviewCommandType::REROLL_STUDIO_SELECT_APPEARANCE, "Appearance", "", "Select Palette and Details for appearance-only rerolling.", PreviewCommandGroup::REROLL_STUDIO },
            { PreviewCommandType::REROLL_STUDIO_GENERATE_CANDIDATE, "Reroll Candidate", "SPACE", "Generate a new candidate from the unchanged BaseRecipe using only the selected domains.", PreviewCommandGroup::REROLL_STUDIO },
            { PreviewCommandType::REROLL_STUDIO_ACCEPT, "Accept", "ENTER", "Make the current candidate the authoritative recipe/ship and add it to History.", PreviewCommandGroup::REROLL_STUDIO },
            { PreviewCommandType::REROLL_STUDIO_CANCEL, "Discard Candidate", "ESCAPE", "Discard the candidate while remaining in the Reroll workspace.", PreviewCommandGroup::REROLL_STUDIO },
            { PreviewCommandType::OPEN_CALIBRATION_LAB, "Calibration Lab", "", "Open the developer-only generation weight calibration workflow.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_PREVIOUS_GROUP, "Previous Group", "", "Select the previous tunable calibration weight group.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_NEXT_GROUP, "Next Group", "", "Select the next tunable calibration weight group.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_SET_WEIGHT, "Set Weight", "", "Change one temporary relative weight. Production defaults are not modified.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_GENERATE_PAIR, "New A/B Pair", "N", "Generate the next balanced controlled comparison pair for the selected group.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_PREFER_LEFT, "Prefer Left", "LEFT / A", "Record a preference for the ship currently shown on the left.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_NO_PREFERENCE, "No Preference", "DOWN / S", "Record a tie/no-preference result for the current pair.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_PREFER_RIGHT, "Prefer Right", "RIGHT / D", "Record a preference for the ship currently shown on the right.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_SKIP, "Skip Pair", "SPACE", "Skip the pair without affecting preference scores.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_RESET_GROUP, "Reset Group", "", "Restore the selected style/group to its production-default weights.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_RESET_ALL, "Reset All", "", "Restore every temporary tuning value to its production-default snapshot.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_APPLY_SUGGESTED, "Apply Suggested", "", "Explicitly apply suggested weights to the temporary tuning profile only.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_TOGGLE_SHOW_VALUES, "Show Test Values", "", "Show or hide the underlying option names during A/B comparisons.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_TOGGLE_CONTEXT_FILTER, "Context Filter", "", "Toggle statistics between all records and the current style, faction, and dimension bucket.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_RUN_OBJECTIVE_BATCH, "Objective Batch", "", "Run a small deterministic Task-33 statistics batch for production defaults versus the temporary tuning profile.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_SAVE_SESSION, "Save Session", "", "Save the calibration profile, pair schedule and comparison records to JSON.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_LOAD_SESSION, "Load Session", "", "Resume the saved calibration session JSON.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_EXPORT_REPORT, "Export CSV", "", "Export raw calibration comparison records as CSV for external analysis.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_EXPORT_TUNING_PROFILE, "Export Tuning", "", "Export the current temporary tuning profile without modifying C++ defaults.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::CALIBRATION_EXIT, "Exit Calibration", "ESCAPE", "Return to the normal PreviewApp without applying tuning to production defaults.", PreviewCommandGroup::CALIBRATION },
            { PreviewCommandType::BACK_OR_EXIT, "Back / Cancel", "ESC", "Dismiss the active overlay or nested operation. Never closes Preview.", PreviewCommandGroup::VIEW }
        } };
    }

    std::size_t getWrappedPreviewSelectorIndex(std::size_t currentIndex, int32_t delta, std::size_t valueCount)
    {
        if (valueCount == 0u) { return 0u; }

        const int32_t count = static_cast<int32_t>(valueCount);
        int32_t index = static_cast<int32_t>(std::min(currentIndex, valueCount - 1u)) + delta;
        while (index < 0) { index += count; }
        while (index >= count) { index -= count; }
        return static_cast<std::size_t>(index);
    }

    const PreviewCommandData& getPreviewCommandData(PreviewCommandType type)
    {
        const std::size_t index = static_cast<std::size_t>(type);
        return index < CommandData.size() ? CommandData[index] : CommandData.back();
    }

    const std::array<PreviewCommandData, static_cast<std::size_t>(PreviewCommandType::PREVIEW_COMMAND_TYPE_END)>& getPreviewCommandDataTable()
    {
        return CommandData;
    }
}
