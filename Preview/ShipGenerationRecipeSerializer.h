#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "ShipIdleAnimation.h"
#include "PreviewGenerationRecipe.h"

namespace PixelShipGeneratorPreview
{
    inline constexpr uint32_t ShipGenerationRecipeFormatVersion = 4u;

    struct ShipGenerationRecipeDocument
    {
        PreviewGenerationRecipe Recipe;
        std::optional<PixelShipGenerator::ShipIdleAnimationSettings> AnimationSettings;
    };

    struct ShipGenerationRecipeLoadResult
    {
        bool Success = false;
        ShipGenerationRecipeDocument Document;
        std::string Error;
    };

    std::string shipStyleToRecipeString(PixelShipGenerator::ShipStyle style);
    std::string shipFactionToRecipeString(PixelShipGenerator::ShipFactionType faction);
    bool shipStyleFromRecipeString(const std::string& value, PixelShipGenerator::ShipStyle& style);
    bool shipFactionFromRecipeString(const std::string& value, PixelShipGenerator::ShipFactionType& faction);
    std::string serializeShipGenerationRecipe(const ShipGenerationRecipeDocument& document);
    ShipGenerationRecipeLoadResult deserializeShipGenerationRecipe(const std::string& jsonText);
    bool saveShipGenerationRecipe(const ShipGenerationRecipeDocument& document, const std::filesystem::path& path, std::string& error);
    ShipGenerationRecipeLoadResult loadShipGenerationRecipe(const std::filesystem::path& path);
}
