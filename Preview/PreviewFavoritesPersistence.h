#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "PreviewGenerationRecipe.h"

namespace SpectralShipGenStudioPreview
{
    inline constexpr uint32_t PreviewFavoritesFormatVersion = 1u;

    struct PreviewFavoritesLoadResult
    {
        bool Success = false;
        std::vector<PreviewGenerationRecipe> Favorites;
        uint32_t SkippedEntryCount = 0u;
        uint32_t DuplicateEntryCount = 0u;
        std::string Error;
    };

    std::string serializePreviewFavorites(const std::vector<PreviewGenerationRecipe>& favorites);
    PreviewFavoritesLoadResult deserializePreviewFavorites(const std::string& jsonText);
    bool savePreviewFavorites(const std::vector<PreviewGenerationRecipe>& favorites, const std::filesystem::path& path, std::string& error);
    PreviewFavoritesLoadResult loadPreviewFavorites(const std::filesystem::path& path);
}
