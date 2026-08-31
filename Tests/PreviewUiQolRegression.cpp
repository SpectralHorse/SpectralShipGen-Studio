#include "PreviewRegressionSuites.h"

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>

#include "ConfigurationBundle.h"
#include "PreviewCollectionSession.h"
#include "PreviewCommand.h"
#include "PreviewConfigurationEditor.h"
#include "PreviewFavoritesPersistence.h"
#include "PreviewGenerationRecipe.h"
#include "PreviewPagination.h"
#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipGenerationSeeds.h>

namespace
{
    using namespace SpectralShipGen;
    using namespace SpectralShipGenStudioPreview;

    int fail(const char* message)
    {
        std::cerr << "Preview UI/QoL regression: " << message << '\n';
        return 1;
    }

    PreviewGenerationRecipe makeRecipe(uint64_t seed)
    {
        PreviewGenerationRecipe recipe;
        recipe.Seeds = deriveShipGenerationSeeds(seed);
        recipe.Dimensions = { 64u, 64u };
        recipe.StructuralPreset = ShipStyle::INDUSTRIAL;
        recipe.FactionPreset = ShipFactionType::MILITARY;
        return recipe;
    }
}

int SpectralShipGenStudioTests::runPreviewUiQolRegression()
{
    using namespace SpectralShipGen;
    using namespace SpectralShipGenStudioPreview;

    if (std::string(getPreviewCommandCompactLabel(PreviewCommandType::INSPECTION_PREVIOUS_GROUP)) != "<" ||
        std::string(getPreviewCommandCompactLabel(PreviewCommandType::INSPECTION_NEXT_GROUP)) != ">" ||
        std::string(getPreviewCommandCompactLabel(PreviewCommandType::INSPECTION_PREVIOUS_VIEW)) != "<" ||
        std::string(getPreviewCommandCompactLabel(PreviewCommandType::INSPECTION_NEXT_VIEW)) != ">")
    {
        return fail("Inspect GROUP/VIEW controls lost compact selector labels");
    }

    struct PageCase { std::size_t Count; std::size_t Pages; };
    constexpr PageCase cases[] = { { 0u, 0u }, { 1u, 1u }, { 24u, 1u }, { 25u, 1u }, { 26u, 2u }, { 49u, 2u }, { 50u, 2u }, { 51u, 3u } };
    for (const PageCase& pageCase : cases)
    {
        if (getPreviewPageCount(pageCase.Count, 25u) != pageCase.Pages) { return fail("Favorites page count boundary is incorrect"); }
    }
    if (getPreviewPageStart(0u, 51u, 25u) != 0u || getPreviewPageStart(1u, 51u, 25u) != 25u || getPreviewPageStart(2u, 51u, 25u) != 50u)
    {
        return fail("Favorites visible page ranges are incorrect");
    }
    if (clampPreviewPageIndex(2u, 50u, 25u) != 1u || clampPreviewPageIndex(99u, 0u, 25u) != 0u)
    {
        return fail("Favorites page clamp after deletion/empty collection is incorrect");
    }
    if (getPreviewPageForItem(25u, 25u) != 1u || getPreviewPageForItem(50u, 25u) != 2u)
    {
        return fail("Favorites global index to page mapping is incorrect");
    }

    PreviewCollectionSession largeFavorites(makeRecipe(0x1000000000000100ull));
    for (uint64_t index = 0u; index < 51u; ++index)
    {
        if (!largeFavorites.addFavorite(makeRecipe(0x1000000000000200ull + index))) { return fail("could not populate a Favorites collection beyond 25 entries"); }
    }
    if (largeFavorites.getFavorites().size() != 51u || largeFavorites.getFavorite(25u) == nullptr || largeFavorites.getFavorite(50u) == nullptr ||
        largeFavorites.getFavorite(25u)->Seeds.Master != 0x1000000000000219ull || largeFavorites.getFavorite(50u)->Seeds.Master != 0x1000000000000232ull)
    {
        return fail("Favorites beyond the first page are not reachable through stable global indices");
    }
    const PreviewGenerationRecipe lastFavorite = *largeFavorites.getFavorite(50u);
    if (!largeFavorites.removeFavorite(lastFavorite) || largeFavorites.getFavorites().size() != 50u || getPreviewPageCount(largeFavorites.getFavorites().size(), 25u) != 2u)
    {
        return fail("deleting the final item on the final Favorites page did not collapse page count correctly");
    }
    const std::filesystem::path favoritesDirectory = std::filesystem::temp_directory_path() / "spectral_ship_gen_task100_favorites";
    const std::filesystem::path favoritesPath = favoritesDirectory / "favorites.json";
    std::error_code filesystemError;
    std::filesystem::remove_all(favoritesDirectory, filesystemError);
    std::filesystem::create_directories(favoritesDirectory, filesystemError);
    std::string favoritesError;
    if (!savePreviewFavorites(largeFavorites.getFavorites(), favoritesPath, favoritesError)) { return fail("large Favorites collection failed persistence save"); }
    const PreviewFavoritesLoadResult largeReload = loadPreviewFavorites(favoritesPath);
    if (!largeReload.Success || largeReload.Favorites != largeFavorites.getFavorites()) { return fail("large Favorites collection lost order/reproducibility across restart persistence"); }
    std::filesystem::remove_all(favoritesDirectory, filesystemError);

    constexpr PreviewCommandType galleryConfigCommands[] = {
        PreviewCommandType::PREVIOUS_STYLE, PreviewCommandType::NEXT_STYLE,
        PreviewCommandType::PREVIOUS_FACTION, PreviewCommandType::NEXT_FACTION,
        PreviewCommandType::PREVIOUS_PALETTE, PreviewCommandType::NEXT_PALETTE,
        PreviewCommandType::PREVIOUS_CONFIGURATION_BUNDLE, PreviewCommandType::NEXT_CONFIGURATION_BUNDLE,
        PreviewCommandType::PREVIOUS_RESOLUTION, PreviewCommandType::NEXT_RESOLUTION,
        PreviewCommandType::SET_WIDTH, PreviewCommandType::SET_HEIGHT,
        PreviewCommandType::TOGGLE_ASPECT_RATIO_LOCK, PreviewCommandType::TOGGLE_ATTACHMENTS_ENABLED
    };
    for (PreviewCommandType command : galleryConfigCommands)
    {
        if (!isGalleryGenerationConfigurationCommand(command)) { return fail("normal generation configuration is not Gallery-editable"); }
    }
    constexpr PreviewCommandType galleryBlockedCommands[] = {
        PreviewCommandType::SAVE_CURRENT, PreviewCommandType::SAVE_SPRITESHEET, PreviewCommandType::EXPORT_RECIPE,
        PreviewCommandType::PREVIOUS_HISTORY, PreviewCommandType::NEXT_HISTORY
    };
    for (PreviewCommandType command : galleryBlockedCommands)
    {
        if (isGalleryGenerationConfigurationCommand(command)) { return fail("single-current-ship action was classified as Gallery configuration"); }
    }

    const PreviewGenerationRecipe initial = makeRecipe(0x1000000000000001ull);
    PreviewCollectionSession collection(initial);
    collection.beginGallery(0x1000000000000010ull, initial);
    PreviewGenerationRecipe candidate = makeRecipe(0x1000000000000002ull);
    candidate.Dimensions = { 44u, 44u };
    collection.addGalleryRecipe(candidate);
    const PreviewGenerationRecipe templateBefore = collection.getGalleryTemplateRecipe();
    const PreviewGenerationRecipe candidateBefore = *collection.getGalleryRecipe(0u);
    collection.getCurrentRecipe().Dimensions = { 128u, 96u };
    collection.getCurrentRecipe().StructuralPreset = ShipStyle::SPEARHEAD;
    collection.getCurrentRecipe().FactionPreset = ShipFactionType::ASCENDANT;
    collection.getCurrentRecipe().AttachmentsEnabled = !collection.getCurrentRecipe().AttachmentsEnabled;
    if (collection.getGalleryTemplateRecipe() != templateBefore || collection.getGalleryRecipe(0u) == nullptr || *collection.getGalleryRecipe(0u) != candidateBefore)
    {
        return fail("Gallery configuration editing mutated existing candidate snapshots");
    }

    PreviewConfigurationEditor editor;
    editor.setPanelBounds({ 880.0f, 0.0f, 760.0f, 1000.0f });
    editor.openStructuralProfile("Readable Structural", getShipGenerationProfile(ShipStyle::INDUSTRIAL));
    if (editor.getNameField().Bounds.Height < 36.0f || editor.getProfileSections().empty() || editor.getProfileSections().front().HeaderBounds.Height < 32.0f)
    {
        return fail("Structural editor geometry does not accommodate enlarged ordinary text");
    }
    for (std::size_t index = 0u; index < editor.getProfileSections().size(); ++index) { editor.setSectionExpanded(index, index == 10u); }
    if (editor.getMaximumScrollOffset() <= 0.0f) { return fail("dense Structural section no longer exposes scroll range"); }
    editor.onMouseWheelScrolled(-100000.0f);
    if (editor.getScrollOffset() != editor.getMaximumScrollOffset()) { return fail("Structural editor cannot reach maximum scroll after readability layout changes"); }

    editor.openFactionProfile("Readable Faction", getShipFactionProfile(ShipFactionType::MILITARY));
    if (editor.getNameField().Bounds.Height < 36.0f || editor.getFactionProfileSections().empty() || editor.getFactionProfileSections().front().HeaderBounds.Height < 32.0f)
    {
        return fail("Faction editor readability geometry is inconsistent");
    }

    ShipPaletteConfiguration palette;
    palette.Mode = ShipPaletteSourceMode::FACTION_PROFILE_GENERATED;
    editor.openPaletteConfiguration("Readable Palette", palette);
    if (editor.getNameField().Bounds.Height < 36.0f || editor.getPaletteProfileSections().empty() || editor.getPaletteProfileSections().front().HeaderBounds.Height < 32.0f)
    {
        return fail("Palette editor readability geometry is inconsistent");
    }

    ConfigurationBundle bundle = makeConfigurationBundle(initial, "INDUSTRIAL", "MILITARY", "Faction Derived");
    editor.openConfigurationBundle("Readable Full Configuration", bundle);
    if (editor.getNameField().Bounds.Height < 36.0f)
    {
        return fail("Full Configuration editor name field did not inherit readability geometry");
    }
    for (const ConfigurationBundleComponentControl& component : editor.getBundleComponentControls())
    {
        if (component.RowBounds.Height < 40.0f || component.ReplaceBounds.Height < 34.0f) { return fail("Full Configuration component rows do not fit enlarged text"); }
    }

    std::cout << "Preview UI readability/Favorites scalability/Gallery QoL regression passed.\n";
    return 0;
}
