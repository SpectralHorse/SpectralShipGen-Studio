#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace SpectralShipGenStudioPreview
{
    enum class PreviewCommandGroup : uint32_t
    {
        GENERATION = 0u,
        NAVIGATION,
        APPEARANCE,
        LOCKS,
        VIEW,
        ANIMATION,
        FAVORITES,
        COMPARISON,
        FILES,
        REROLL_STUDIO,
        CALIBRATION
    };

    enum class PreviewCommandType : uint32_t
    {
        GENERATE_NEW = 0u,
        REROLL,
        GENERATE_FROM_MASTER_SEED,
        PREVIOUS_HISTORY,
        NEXT_HISTORY,
        OPEN_GALLERY,
        OPEN_GALLERY_FROM_SEED,
        GALLERY_LEFT,
        GALLERY_RIGHT,
        GALLERY_UP,
        GALLERY_DOWN,
        SELECT_GALLERY_CANDIDATE,
        ADD_CURRENT_TO_FAVORITES,
        REMOVE_CURRENT_FROM_FAVORITES,
        OPEN_FAVORITES,
        CLOSE_FAVORITES,
        FAVORITES_LEFT,
        FAVORITES_RIGHT,
        FAVORITES_UP,
        FAVORITES_DOWN,
        FAVORITES_PREVIOUS_PAGE,
        FAVORITES_NEXT_PAGE,
        SELECT_FAVORITE,
        OPEN_FAVORITE_INSPECT,
        OPEN_FAVORITE_ANIMATION,
        OPEN_FAVORITE_REROLL,
        REMOVE_SELECTED_FAVORITE,
        EXPORT_FAVORITE_IMAGE,
        SELECT_STYLE,
        PREVIOUS_STYLE,
        NEXT_STYLE,
        SELECT_FACTION,
        PREVIOUS_FACTION,
        NEXT_FACTION,
        PREVIOUS_PALETTE,
        NEXT_PALETTE,
        PREVIOUS_CONFIGURATION_BUNDLE,
        NEXT_CONFIGURATION_BUNDLE,
        PROFILES_PREVIOUS_SECTION,
        PROFILES_NEXT_SECTION,
        PROFILES_PREVIOUS_ITEM,
        PROFILES_NEXT_ITEM,
        PROFILES_NEW_DEFAULT,
        PROFILES_EDIT_SELECTED,
        PROFILES_DUPLICATE_SELECTED,
        PROFILES_DELETE_SELECTED,
        PROFILES_IMPORT_SELECTED,
        PROFILES_EXPORT_SELECTED,
        PROFILES_USE_SELECTED,
        OPEN_STRUCTURAL_EDITOR,
        OPEN_FACTION_EDITOR,
        OPEN_PALETTE_EDITOR,
        SELECT_RESOLUTION,
        PREVIOUS_RESOLUTION,
        NEXT_RESOLUTION,
        SET_WIDTH,
        SET_HEIGHT,
        TOGGLE_ASPECT_RATIO_LOCK,
        ADD_RESOLUTION_BOOKMARK,
        REMOVE_RESOLUTION_BOOKMARK,
        SELECT_RESOLUTION_BOOKMARK,
        TOGGLE_ATTACHMENTS_ENABLED,
        TOGGLE_STRUCTURE_LOCK,
        TOGGLE_PALETTE_LOCK,
        TOGGLE_DETAILS_LOCK,
        TOGGLE_ATTACHMENTS_LOCK,
        TOGGLE_HELP,
        INSPECTION_PREVIOUS_GROUP,
        INSPECTION_NEXT_GROUP,
        INSPECTION_PREVIOUS_VIEW,
        INSPECTION_NEXT_VIEW,
        TOGGLE_INSPECTION_PRESENTATION,
        OPEN_GENERATE_WORKSPACE,
        OPEN_ANIMATION_WORKSPACE,
        TOGGLE_GENERATION_INSPECTOR,
        TOGGLE_PALETTE_INSPECTOR,
        CYCLE_DIAGNOSTIC_VIEW,
        TOGGLE_GENERATION_STAGE_VIEW,
        PREVIOUS_GENERATION_STAGE,
        NEXT_GENERATION_STAGE,
        CYCLE_ANIMATION_TYPE,
        CYCLE_ANIMATION_BASE_STATE,
        CYCLE_MOVEMENT_PHASE,
        CYCLE_FIRING_TARGET,
        TRIGGER_ANIMATION_FIRE,
        CYCLE_ANIMATION_PLAYBACK_SPEED,
        SET_ANIMATION_NORMALIZED_TIME,
        APPLY_ANIMATION_STATE,
        RETURN_ANIMATION_TO_IDLE,
        TOGGLE_ANIMATION,
        TOGGLE_FRAME_INSPECTION,
        PREVIOUS_FRAME,
        NEXT_FRAME,
        PIN_CURRENT,
        CLEAR_PIN,
        TOGGLE_COMPARISON,
        SAVE_CURRENT,
        EXPORT_RECIPE,
        IMPORT_RECIPE,
        SAVE_SPRITESHEET,
        OPEN_REROLL_STUDIO,
        REROLL_STUDIO_TOGGLE_DOMAIN,
        REROLL_STUDIO_SELECT_ALL,
        REROLL_STUDIO_CLEAR,
        REROLL_STUDIO_SELECT_STRUCTURE,
        REROLL_STUDIO_SELECT_APPEARANCE,
        REROLL_STUDIO_GENERATE_CANDIDATE,
        REROLL_STUDIO_ACCEPT,
        REROLL_STUDIO_CANCEL,
        OPEN_CALIBRATION_LAB,
        CALIBRATION_PREVIOUS_GROUP,
        CALIBRATION_NEXT_GROUP,
        CALIBRATION_SET_WEIGHT,
        CALIBRATION_GENERATE_PAIR,
        CALIBRATION_PREFER_LEFT,
        CALIBRATION_NO_PREFERENCE,
        CALIBRATION_PREFER_RIGHT,
        CALIBRATION_SKIP,
        CALIBRATION_RESET_GROUP,
        CALIBRATION_RESET_ALL,
        CALIBRATION_APPLY_SUGGESTED,
        CALIBRATION_TOGGLE_SHOW_VALUES,
        CALIBRATION_TOGGLE_CONTEXT_FILTER,
        CALIBRATION_RUN_OBJECTIVE_BATCH,
        CALIBRATION_SAVE_SESSION,
        CALIBRATION_LOAD_SESSION,
        CALIBRATION_EXPORT_REPORT,
        CALIBRATION_EXPORT_TUNING_PROFILE,
        CALIBRATION_EXIT,
        BACK_OR_EXIT,
        PREVIEW_COMMAND_TYPE_END
    };

    struct PreviewCommand
    {
        PreviewCommandType Type = PreviewCommandType::PREVIEW_COMMAND_TYPE_END;
        uint32_t Value = 0u;

        bool operator==(const PreviewCommand& other) const { return Type == other.Type && Value == other.Value; }
        bool operator!=(const PreviewCommand& other) const { return !(*this == other); }
    };

    struct PreviewCommandData
    {
        PreviewCommandType Type = PreviewCommandType::PREVIEW_COMMAND_TYPE_END;
        const char* Label = "";
        const char* Shortcut = "";
        const char* Description = "";
        PreviewCommandGroup Group = PreviewCommandGroup::VIEW;
    };

    std::size_t getWrappedPreviewSelectorIndex(std::size_t currentIndex, int32_t delta, std::size_t valueCount);
    const PreviewCommandData& getPreviewCommandData(PreviewCommandType type);
    const char* getPreviewCommandCompactLabel(PreviewCommandType type);
    bool isGalleryGenerationConfigurationCommand(PreviewCommandType type);
    const std::array<PreviewCommandData, static_cast<std::size_t>(PreviewCommandType::PREVIEW_COMMAND_TYPE_END)>& getPreviewCommandDataTable();
}
