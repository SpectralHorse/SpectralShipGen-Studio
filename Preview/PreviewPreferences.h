#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <SpectralShipGen/ShipDimensions.h>

namespace SpectralShipGenStudioPreview
{
    inline constexpr uint32_t PreviewPreferencesFormatVersion = 2u;

    struct PreviewPreferences
    {
        std::vector<SpectralShipGen::ShipDimensions> ResolutionBookmarks;
    };

    struct PreviewPreferencesLoadResult
    {
        bool Success = false;
        PreviewPreferences Preferences;
        std::string Error;
    };

    std::string serializePreviewPreferences(const PreviewPreferences& preferences);
    PreviewPreferencesLoadResult deserializePreviewPreferences(const std::string& jsonText);
    bool savePreviewPreferences(const PreviewPreferences& preferences, const std::filesystem::path& path, std::string& error);
    PreviewPreferencesLoadResult loadPreviewPreferences(const std::filesystem::path& path);
}
