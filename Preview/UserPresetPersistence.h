#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "RuntimeCustomPresetWorkspace.h"

namespace PixelShipGeneratorPreview
{
    inline constexpr uint32_t UserPresetLibraryFormatVersion = 1u;
    inline constexpr uint32_t UserPresetFileFormatVersion = 1u;

    enum class UserPresetCategory : uint32_t
    {
        STRUCTURAL = 0u,
        FACTION,
        PALETTE,
        USER_PRESET_CATEGORY_END
    };

    struct UserPresetLibraryLoadResult
    {
        bool Success = false;
        RuntimeCustomPresetWorkspace Workspace;
        uint32_t SkippedEntryCount = 0u;
        std::string Error;
    };

    struct UserPresetImportResult
    {
        bool Success = false;
        UserPresetCategory Category = UserPresetCategory::USER_PRESET_CATEGORY_END;
        RuntimeCustomPresetId ImportedId = 0u;
        std::string DisplayName;
        bool DisplayNameDisambiguated = false;
        std::string Error;
    };

    const char* getUserPresetCategoryId(UserPresetCategory category);

    std::string serializeUserPresetLibrary(const RuntimeCustomPresetWorkspace& workspace);
    UserPresetLibraryLoadResult deserializeUserPresetLibrary(const std::string& jsonText);
    bool saveUserPresetLibrary(const RuntimeCustomPresetWorkspace& workspace, const std::filesystem::path& path, std::string& error);
    UserPresetLibraryLoadResult loadUserPresetLibrary(const std::filesystem::path& path);

    std::string serializeUserPresetFile(const RuntimeStructuralPreset& preset);
    std::string serializeUserPresetFile(const RuntimeFactionPreset& preset);
    std::string serializeUserPresetFile(const RuntimePalettePreset& preset);
    bool exportUserPreset(const RuntimeCustomPresetWorkspace& workspace, UserPresetCategory category, RuntimeCustomPresetId id, const std::filesystem::path& path, std::string& error);

    UserPresetImportResult importUserPreset(RuntimeCustomPresetWorkspace& workspace, UserPresetCategory expectedCategory, const std::string& jsonText);
    UserPresetImportResult importUserPreset(RuntimeCustomPresetWorkspace& workspace, UserPresetCategory expectedCategory, const std::filesystem::path& path);
}
