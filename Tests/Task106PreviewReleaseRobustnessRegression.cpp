#include "PreviewRegressionSuites.h"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>

#include "ConfigurationBundle.h"
#include "PreviewAnimationSession.h"
#include "PreviewCollectionSession.h"
#include "PreviewConfigurationEditor.h"
#include "PreviewFavoritesPersistence.h"
#include "PreviewGenerationRecipe.h"
#include "PreviewInspection.h"
#include "PreviewWorkspace.h"
#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipGenerationSeeds.h>
#include <SpectralShipGen/ShipGenerator.h>

namespace
{
    using namespace SpectralShipGen;
    using namespace SpectralShipGenStudioPreview;

    int fail(const char* message)
    {
        std::cerr << "Task-106 Preview release robustness regression failed: " << message << '\n';
        return 1;
    }

    PreviewGenerationRecipe makeRecipe(uint64_t seed, ShipDimensions dimensions = { 96u, 64u })
    {
        PreviewGenerationRecipe recipe;
        recipe.Seeds = deriveShipGenerationSeeds(seed);
        recipe.Dimensions = dimensions;
        recipe.Style = ShipStyle::INDUSTRIAL;
        recipe.Faction = ShipFactionType::CORPORATE;
        recipe.DetailDensity = 67u;
        recipe.AsymmetricDetailChance = 31u;
        return recipe;
    }
}

int SpectralShipGenStudioTests::runTask106PreviewReleaseRobustnessRegression()
{
    using namespace SpectralShipGen;
    using namespace SpectralShipGenStudioPreview;

    const PreviewGenerationRecipe initial = makeRecipe(0x1065000000000001ull);
    PreviewCollectionSession collection(initial, 64u);

    for (uint64_t index = 0u; index < 512u; ++index)
    {
        const PreviewGenerationRecipe recipe = makeRecipe(0x1065000000010000ull + index,
            index % 2u == 0u ? ShipDimensions{ 160u, 96u } : ShipDimensions{ 96u, 160u });
        if (!collection.addFavorite(recipe)) { return fail("large Favorites population unexpectedly rejected a unique recipe"); }
    }
    if (collection.getFavorites().size() != 512u) { return fail("large Favorites collection size is incorrect"); }

    collection.beginGallery(0x1066000000000001ull, initial);
    for (uint64_t index = 0u; index < 256u; ++index)
    {
        collection.addGalleryRecipe(makeRecipe(0x1066000000010000ull + index));
    }
    if (collection.getGalleryRecipes().size() != 256u) { return fail("large Gallery batch size is incorrect"); }
    for (std::size_t index = 0u; index < collection.getGalleryRecipes().size(); index += 17u)
    {
        const std::optional<bool> favorite = collection.toggleGalleryFavorite(index);
        if (!favorite.has_value() || !*favorite) { return fail("Gallery favorite toggle failed under a large collection"); }
    }

    const std::filesystem::path directory = std::filesystem::temp_directory_path() / "spectral_ship_gen_task106_preview_robustness";
    const std::filesystem::path favoritesPath = directory / "favorites.json";
    std::error_code filesystemError;
    std::filesystem::remove_all(directory, filesystemError);
    std::filesystem::create_directories(directory, filesystemError);
    std::string saveError;
    if (!savePreviewFavorites(collection.getFavorites(), favoritesPath, saveError)) { return fail("large Favorites save failed"); }
    const PreviewFavoritesLoadResult loadedFavorites = loadPreviewFavorites(favoritesPath);
    if (!loadedFavorites.Success || loadedFavorites.Favorites != collection.getFavorites()) { return fail("large Favorites persistence round-trip changed recipe semantics"); }
    std::filesystem::remove_all(directory, filesystemError);

    PreviewWorkspaceSession workspaces;
    PreviewMode mode = PreviewMode::STATIC;
    for (uint32_t iteration = 0u; iteration < 10000u; ++iteration)
    {
        const PreviewWorkspace workspace = static_cast<PreviewWorkspace>(iteration % PreviewWorkspaceCount);
        mode = workspaces.switchTo(workspace, mode);
        workspaces.rememberActiveMode(mode);
    }
    if (workspaces.getActiveWorkspace() >= PreviewWorkspace::PREVIEW_WORKSPACE_END) { return fail("repeated workspace switching corrupted active workspace state"); }

    ShipGenerationSettings settings;
    settings.Seed = 0x1067000000000001ull;
    settings.Dimensions = { 160u, 96u };
    settings.Style = ShipStyle::INDUSTRIAL;
    settings.Faction = ShipFactionType::MILITARY;
    settings.DetailDensity = 75u;
    settings.AsymmetricDetailChance = 40u;
    ShipGenerationDebugInfo debugInfo;
    const GeneratedShip ship = ShipGenerator{}.generate(settings, &debugInfo);
    if (ship.FinalImage.empty()) { return fail("stress ship generation failed"); }

    for (uint32_t pass = 0u; pass < 12u; ++pass)
    {
        for (uint32_t view = 0u; view < static_cast<uint32_t>(DiagnosticViewMode::DIAGNOSTIC_VIEW_MODE_END); ++view)
        {
            for (uint32_t presentation = 0u; presentation < static_cast<uint32_t>(PreviewInspectionPresentation::PREVIEW_INSPECTION_PRESENTATION_END); ++presentation)
            {
                const Image image = createPreviewInspectionImage(ship, debugInfo,
                    static_cast<DiagnosticViewMode>(view),
                    static_cast<PreviewInspectionPresentation>(presentation));
                if (image.getWidth() != ship.FinalImage.getWidth() || image.getHeight() != ship.FinalImage.getHeight())
                {
                    return fail("repeated Inspect view generation changed output dimensions");
                }
            }
        }
    }

    PreviewAnimationSession animation;
    if (!animation.resetForGeneratedShip(ship).Success || animation.getActiveFrames().empty()) { return fail("Animation session reset failed"); }
    for (uint32_t index = 0u; index < 1000u; ++index)
    {
        const double normalized = static_cast<double>(index % 997u) / 997.0;
        if (!animation.setNormalizedTime(normalized)) { return fail("Animation scrubbing failed"); }
    }
    for (uint32_t cycle = 0u; cycle < 2u; ++cycle)
    {
        for (uint32_t type = 0u; type < 6u; ++type)
        {
            animation.cycleAnimationType(ship);
            if (animation.getActiveFrames().empty() && animation.getSelectedAnimationType() != ShipAnimationType::FIRE)
            {
                return fail("Animation family switch produced an empty non-FIRE state");
            }
        }
    }
    animation.returnToIdle(ship);
    animation.cycleBaseMovementState(ship);
    animation.advancePlayback(ship, 2000000.0);
    const PreviewAnimationActionResult fireResult = animation.triggerFiringEvent(ship);
    if (fireResult.Success)
    {
        animation.advancePlayback(ship, 4000000.0);
        if (animation.isTransientStatePreviewActive()) { return fail("movement+FIRE transient did not complete under repeated-session use"); }
    }

    PreviewConfigurationEditor editor;
    editor.setPanelBounds({ 880.0f, 0.0f, 760.0f, 1000.0f });
    const ShipGenerationProfile structural = getShipGenerationProfile(ShipStyle::DELTA);
    const ShipFactionProfile faction = getShipFactionProfile(ShipFactionType::RELIC);
    ShipPaletteConfiguration palette;
    palette.Mode = ShipPaletteSourceMode::FIXED;
    palette.Fixed.HullBase = Color(210u, 40u, 110u, 255u);
    const ConfigurationBundle bundle = makeConfigurationBundle(initial, "DELTA", "RELIC", "Fixed");
    for (uint32_t iteration = 0u; iteration < 100u; ++iteration)
    {
        editor.openStructuralProfile("Task106 Structural", structural);
        editor.openFactionProfile("Task106 Faction", faction);
        editor.openPaletteConfiguration("Task106 Palette", palette);
        editor.openConfigurationBundle("Task106 Full Configuration", bundle);
    }

    std::cout << "Task-106 Preview release robustness regression passed.\n";
    return 0;
}
