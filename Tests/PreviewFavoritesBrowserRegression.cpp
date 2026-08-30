#include "RegressionSuites.h"

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "PreviewCollectionSession.h"
#include "PreviewCommand.h"
#include "PreviewFavoritesPersistence.h"
#include "PreviewWorkspace.h"
#include "ShipGenerationSeeds.h"

namespace
{
    using namespace PixelShipGeneratorPreview;

    PreviewGenerationRecipe makeRecipe(uint64_t seed, PixelShipGenerator::ShipStyle style, PixelShipGenerator::ShipFactionType faction)
    {
        PreviewGenerationRecipe recipe;
        recipe.Seeds = PixelShipGenerator::deriveShipGenerationSeeds(seed);
        recipe.Dimensions = { 64u, 64u };
        recipe.Style = style;
        recipe.Faction = faction;
        return recipe;
    }

    int fail(const char* message)
    {
        std::cerr << "Favorites browser regression: " << message << '\n';
        return 1;
    }
}

int PixelShipGeneratorTests::runPreviewFavoritesBrowserRegression()
{
    using namespace PixelShipGeneratorPreview;

    const PreviewGenerationRecipe initial = makeRecipe(0x9800000000000001ull, PixelShipGenerator::ShipStyle::FIGHTER, PixelShipGenerator::ShipFactionType::MILITARY);
    const PreviewGenerationRecipe favoriteA = makeRecipe(0x9800000000000002ull, PixelShipGenerator::ShipStyle::SPEARHEAD, PixelShipGenerator::ShipFactionType::ASCENDANT);
    const PreviewGenerationRecipe favoriteB = makeRecipe(0x9800000000000003ull, PixelShipGenerator::ShipStyle::DELTA, PixelShipGenerator::ShipFactionType::CORPORATE);

    PreviewCollectionSession collection(initial);
    if (!collection.addFavorite(favoriteA) || !collection.addFavorite(favoriteB) || collection.addFavorite(favoriteA))
    {
        return fail("Favorite insertion order/duplicate prevention changed");
    }
    if (collection.getFavorites() != std::vector<PreviewGenerationRecipe>{ favoriteA, favoriteB })
    {
        return fail("Favorite order is not stable insertion order");
    }

    // Browser selection is deliberately separate from loading: reading a selected recipe must not change current state.
    const PreviewGenerationRecipe* selected = collection.getFavorite(1u);
    if (selected == nullptr || *selected != favoriteB || collection.getCurrentRecipe() != initial)
    {
        return fail("selecting/browsing a Favorite changed the shared current recipe");
    }

    collection.appendHistoryEntry(*selected);
    const std::vector<PreviewGenerationRecipe> favoritesBeforeRouting = collection.getFavorites();
    PreviewWorkspaceSession workspaces;
    workspaces.switchTo(PreviewWorkspace::FAVORITES, PreviewMode::STATIC);
    workspaces.switchTo(PreviewWorkspace::GENERATE, PreviewMode::FAVORITES);
    workspaces.switchTo(PreviewWorkspace::INSPECT, PreviewMode::STATIC);
    workspaces.switchTo(PreviewWorkspace::ANIMATION, PreviewMode::STATIC);
    workspaces.switchTo(PreviewWorkspace::REROLL, PreviewMode::FRAME_INSPECTION);
    if (collection.getCurrentRecipe() != favoriteB || collection.getFavorites() != favoritesBeforeRouting)
    {
        return fail("cross-workspace routing changed the opened recipe or stored Favorites");
    }

    const PreviewCommandData& open = getPreviewCommandData(PreviewCommandType::SELECT_FAVORITE);
    const PreviewCommandData& remove = getPreviewCommandData(PreviewCommandType::REMOVE_SELECTED_FAVORITE);
    const PreviewCommandData& inspect = getPreviewCommandData(PreviewCommandType::OPEN_FAVORITE_INSPECT);
    const PreviewCommandData& animate = getPreviewCommandData(PreviewCommandType::OPEN_FAVORITE_ANIMATION);
    const PreviewCommandData& rerollCommand = getPreviewCommandData(PreviewCommandType::OPEN_FAVORITE_REROLL);
    const PreviewCommandData& exportImage = getPreviewCommandData(PreviewCommandType::EXPORT_FAVORITE_IMAGE);
    if (std::string(open.Shortcut) != "ENTER" || std::string(remove.Shortcut) != "DELETE" ||
        std::string(inspect.Label) != "Inspect" || std::string(animate.Label) != "Animate" ||
        std::string(rerollCommand.Label) != "Reroll" || std::string(rerollCommand.Description).find("without generating a candidate") == std::string::npos ||
        std::string(exportImage.Label) != "Export Image")
    {
        return fail("Favorites browser command routing metadata is incomplete");
    }

    collection.beginGallery(0x9800000000000010ull, initial);
    const PreviewGenerationRecipe galleryFavorite = makeRecipe(0x9800000000000004ull, PixelShipGenerator::ShipStyle::INDUSTRIAL, PixelShipGenerator::ShipFactionType::FRONTIER);
    collection.addGalleryRecipe(galleryFavorite);
    const std::optional<bool> galleryToggle = collection.toggleGalleryFavorite(0u);
    if (!galleryToggle.has_value() || !*galleryToggle || !collection.isFavorite(galleryFavorite))
    {
        return fail("Gallery Favorite did not become immediately visible in the canonical collection");
    }

    if (!collection.removeFavorite(favoriteB) || collection.getCurrentRecipe() != favoriteB || collection.isFavorite(favoriteB))
    {
        return fail("removing a Favorite mutated the already-open shared current ship");
    }

    const std::filesystem::path directory = std::filesystem::temp_directory_path() / "pixel_ship_generator_favorites_browser_regression";
    const std::filesystem::path path = directory / "favorites.json";
    std::error_code ec;
    std::filesystem::remove_all(directory, ec);
    std::filesystem::create_directories(directory, ec);
    std::string error;
    if (!savePreviewFavorites(collection.getFavorites(), path, error))
    {
        return fail("Favorite browser persistence save failed");
    }
    const PreviewFavoritesLoadResult reload = loadPreviewFavorites(path);
    if (!reload.Success || reload.Favorites != collection.getFavorites())
    {
        return fail("Favorite browser restart persistence/order changed");
    }
    std::filesystem::remove_all(directory, ec);

    const PreviewHelpSection& favoritesHelp = getPreviewWorkspaceHelpSection(PreviewWorkspace::FAVORITES);
    bool hasEnter = false;
    bool hasDelete = false;
    for (std::size_t index = 0u; index < favoritesHelp.Count; ++index)
    {
        hasEnter = hasEnter || std::string(favoritesHelp.Entries[index].Shortcut) == "ENTER";
        hasDelete = hasDelete || std::string(favoritesHelp.Entries[index].Shortcut) == "DELETE";
    }
    if (!hasEnter || !hasDelete)
    {
        return fail("Favorites contextual Help is missing Enter/Delete behavior");
    }

    std::cout << "Favorites collection browser regression passed.\n";
    return 0;
}
