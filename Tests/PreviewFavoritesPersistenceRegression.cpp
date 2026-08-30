#include "RegressionSuites.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "PreviewCollectionSession.h"
#include "PreviewFavoritesPersistence.h"
#include "ShipGenerationRecipeSerializer.h"
#include "ShipGenerationSeeds.h"
#include "ShipPaletteGenerationProfile.h"
#include "ShipGenerationSettings.h"
#include "ShipGenerator.h"

namespace
{
    using namespace PixelShipGeneratorPreview;

    PreviewGenerationRecipe makeRecipe(uint64_t seed, uint32_t width, uint32_t height, PixelShipGenerator::ShipStyle style, PixelShipGenerator::ShipFactionType faction)
    {
        PreviewGenerationRecipe recipe;
        recipe.Seeds = PixelShipGenerator::deriveShipGenerationSeeds(seed);
        recipe.Dimensions = { width, height };
        recipe.Style = style;
        recipe.Faction = faction;
        recipe.DetailDensity = 57u;
        recipe.AsymmetricDetailChance = 13u;
        recipe.AttachmentsEnabled = true;
        return recipe;
    }


    PreviewGenerationRecipe makeCustomRecipe()
    {
        using namespace PixelShipGenerator;
        PreviewGenerationRecipe recipe = makeRecipe(0x7800000000000004ull, 96u, 64u, ShipStyle::FIGHTER, ShipFactionType::FRONTIER);
        recipe.StructuralSource = ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        recipe.Style = ShipStyle::SHIP_STYLE_END;
        recipe.StructuralProfile = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
        recipe.StructuralProfile.LargeWeaponChance = 81u;
        recipe.FactionSource = ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        recipe.Faction = ShipFactionType::SHIP_FACTION_TYPE_END;
        recipe.FactionProfile = getShipFactionProfile(ShipFactionType::CORPORATE);
        recipe.FactionProfile.SurfaceDetails.DetailDensityPercent = 87u;
        recipe.PaletteConfiguration.Mode = ShipPaletteSourceMode::FIXED;
        recipe.PaletteConfiguration.Fixed.HullBase = Color(41u, 73u, 109u, 255u);
        recipe.PaletteConfiguration.Fixed.HullAccent = Color(216u, 84u, 143u, 255u);
        return recipe;
    }

    PixelShipGenerator::Image generateImage(const PreviewGenerationRecipe& recipe)
    {
        return PixelShipGenerator::ShipGenerator{}.generate(recipe).FinalImage;
    }

    std::string recipeJson(const PreviewGenerationRecipe& recipe)
    {
        ShipGenerationRecipeDocument document;
        document.Recipe = recipe;
        return serializeShipGenerationRecipe(document);
    }

    std::string makeFavoritesJson(const std::vector<std::string>& entries)
    {
        std::ostringstream stream;
        stream << "{\n  \"format_version\": 1,\n  \"favorites\": [\n";
        for (std::size_t index = 0u; index < entries.size(); ++index)
        {
            stream << entries[index];
            if (index + 1u < entries.size()) { stream << ','; }
            stream << '\n';
        }
        stream << "  ]\n}\n";
        return stream.str();
    }

    std::string favoriteEntry(const PreviewGenerationRecipe& recipe)
    {
        return "    { \"recipe\": " + recipeJson(recipe) + "    }";
    }

    bool writeText(const std::filesystem::path& path, const std::string& text)
    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        stream << text;
        return static_cast<bool>(stream);
    }
}

int PixelShipGeneratorTests::runPreviewFavoritesPersistenceRegression()
{
    using namespace PixelShipGeneratorPreview;

    bool success = true;
    const PreviewGenerationRecipe recipeA = makeRecipe(0x7800000000000001ull, 64u, 64u, PixelShipGenerator::ShipStyle::FIGHTER, PixelShipGenerator::ShipFactionType::MILITARY);
    const PreviewGenerationRecipe recipeB = makeRecipe(0x7800000000000002ull, 96u, 64u, PixelShipGenerator::ShipStyle::DELTA, PixelShipGenerator::ShipFactionType::CORPORATE);
    const PreviewGenerationRecipe recipeC = makeRecipe(0x7800000000000003ull, 128u, 128u, PixelShipGenerator::ShipStyle::INDUSTRIAL, PixelShipGenerator::ShipFactionType::FRONTIER);
    const PreviewGenerationRecipe customRecipe = makeCustomRecipe();

    const std::filesystem::path directory = std::filesystem::temp_directory_path() / "pixel_ship_generator_favorites_regression";
    const std::filesystem::path path = directory / "favorites.json";
    const std::filesystem::path temporaryPath = path.string() + ".tmp";
    const std::filesystem::path backupPath = path.string() + ".bak";
    std::error_code filesystemError;
    std::filesystem::remove_all(directory, filesystemError);
    std::filesystem::create_directories(directory, filesystemError);

    const PreviewFavoritesLoadResult missing = loadPreviewFavorites(path);
    if (!missing.Success || !missing.Favorites.empty())
    {
        success = false;
        std::cerr << "Missing Favorites file did not load as an empty collection.\n";
    }

    PreviewCollectionSession firstSession(recipeA);
    if (!firstSession.addFavorite(recipeA) || !firstSession.addFavorite(recipeB) || firstSession.addFavorite(recipeA))
    {
        success = false;
        std::cerr << "Favorite add/duplicate behavior changed.\n";
    }

    std::string error;
    if (!savePreviewFavorites(firstSession.getFavorites(), path, error))
    {
        success = false;
        std::cerr << error << '\n';
    }

    const PreviewFavoritesLoadResult firstReload = loadPreviewFavorites(path);
    if (!firstReload.Success || firstReload.Favorites != std::vector<PreviewGenerationRecipe>{ recipeA, recipeB })
    {
        success = false;
        std::cerr << "Favorite ordering did not survive save/reload.\n";
    }

    if (!firstSession.addFavorite(customRecipe) || !savePreviewFavorites(firstSession.getFavorites(), path, error))
    {
        success = false;
        std::cerr << (error.empty() ? "Custom Favorite save failed." : error) << '\n';
    }
    const PreviewFavoritesLoadResult customReload = loadPreviewFavorites(path);
    if (!customReload.Success || customReload.Favorites.size() != 3u || customReload.Favorites[2] != customRecipe || generateImage(customReload.Favorites[2]).getPixels() != generateImage(customRecipe).getPixels())
    {
        success = false;
        std::cerr << "Self-contained custom Favorite did not survive persistence/regeneration.\n";
    }

    PreviewCollectionSession secondSession(recipeC);
    secondSession.setFavorites(firstReload.Favorites);
    if (secondSession.getFavorites() != std::vector<PreviewGenerationRecipe>{ recipeA, recipeB })
    {
        success = false;
        std::cerr << "PreviewCollectionSession did not restore Favorite ordering.\n";
    }

    const PixelShipGenerator::Image originalImage = generateImage(recipeA);
    const PixelShipGenerator::Image recreatedImage = generateImage(secondSession.getFavorites().front());
    if (originalImage.getWidth() != recreatedImage.getWidth() || originalImage.getHeight() != recreatedImage.getHeight() || originalImage.getPixels() != recreatedImage.getPixels())
    {
        success = false;
        std::cerr << "Loaded Favorite recipe did not recreate the deterministic ship image.\n";
    }

    if (!secondSession.removeFavorite(recipeA) || !savePreviewFavorites(secondSession.getFavorites(), path, error))
    {
        success = false;
        std::cerr << (error.empty() ? "Favorite remove/save failed." : error) << '\n';
    }
    const PreviewFavoritesLoadResult removeReload = loadPreviewFavorites(path);
    if (!removeReload.Success || removeReload.Favorites != std::vector<PreviewGenerationRecipe>{ recipeB })
    {
        success = false;
        std::cerr << "Favorite removal did not survive restart/reload.\n";
    }

    if (std::filesystem::exists(temporaryPath) || std::filesystem::exists(backupPath))
    {
        success = false;
        std::cerr << "Favorites safe-write left a temporary or backup file behind.\n";
    }

    std::string invalidEntry = favoriteEntry(recipeB);
    const std::string validDimensions = "\"width\": 96";
    const std::size_t dimensionsPosition = invalidEntry.find(validDimensions);
    if (dimensionsPosition != std::string::npos) { invalidEntry.replace(dimensionsPosition, validDimensions.size(), "\"width\": 0"); }
    const std::string partialJson = makeFavoritesJson({ favoriteEntry(recipeA), invalidEntry, favoriteEntry(recipeC) });
    const PreviewFavoritesLoadResult partial = deserializePreviewFavorites(partialJson);
    if (!partial.Success || partial.Favorites != std::vector<PreviewGenerationRecipe>{ recipeA, recipeC } || partial.SkippedEntryCount != 1u)
    {
        success = false;
        std::cerr << "Partially invalid Favorites collection did not retain valid entries.\n";
    }

    const std::string duplicateJson = makeFavoritesJson({ favoriteEntry(recipeA), favoriteEntry(recipeB), favoriteEntry(recipeA) });
    const PreviewFavoritesLoadResult duplicate = deserializePreviewFavorites(duplicateJson);
    if (!duplicate.Success || duplicate.Favorites != std::vector<PreviewGenerationRecipe>{ recipeA, recipeB } || duplicate.DuplicateEntryCount != 1u)
    {
        success = false;
        std::cerr << "Persisted Favorite duplicate filtering/order failed.\n";
    }

    const PreviewFavoritesLoadResult corrupt = deserializePreviewFavorites("{ this is not valid JSON");
    if (corrupt.Success)
    {
        success = false;
        std::cerr << "Corrupt Favorites JSON was not rejected.\n";
    }

    const PreviewFavoritesLoadResult unsupported = deserializePreviewFavorites("{\"format_version\":99,\"favorites\":[]}");
    if (unsupported.Success)
    {
        success = false;
        std::cerr << "Unsupported Favorites format version was not rejected.\n";
    }

    if (!writeText(path, "{ this is not valid JSON") || loadPreviewFavorites(path).Success)
    {
        success = false;
        std::cerr << "Corrupt Favorites file handling failed.\n";
    }

    std::filesystem::remove_all(directory, filesystemError);
    return success ? 0 : 1;
}
