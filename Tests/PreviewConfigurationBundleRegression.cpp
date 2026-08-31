#include "PreviewRegressionSuites.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>

#include "ConfigurationBundle.h"
#include "PreviewConfigurationEditor.h"
#include "PreviewFavoritesPersistence.h"
#include "PreviewGenerationRecipe.h"
#include "PreviewWorkspace.h"
#include "RuntimeCustomPresetWorkspace.h"
#include "ShipGenerationRecipeSerializer.h"
#include <PixelShipGenerator/ShipGenerationSeeds.h>
#include <PixelShipGenerator/ShipGenerator.h>
#include "UserPresetPersistence.h"

namespace
{
    using namespace PixelShipGenerator;
    using namespace PixelShipGeneratorPreview;

    bool imagesEqual(const Image& first, const Image& second)
    {
        return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
    }

    ShipPaletteConfiguration makeBundlePalette()
    {
        ShipPaletteConfiguration configuration;
        configuration.Mode = ShipPaletteSourceMode::FIXED;
        configuration.Fixed = ShipPalette{};
        configuration.Fixed.HullBase = Color(38u, 67u, 104u, 255u);
        configuration.Fixed.HullAccent = Color(168u, 192u, 218u, 255u);
        configuration.Fixed.EngineHotCore = Color(255u, 224u, 132u, 255u);
        return configuration;
    }

    PreviewGenerationRecipe makeMixedRecipe()
    {
        PreviewGenerationRecipe recipe;
        recipe.Seeds = deriveShipGenerationSeeds(0x9600000000000096ull);
        recipe.Dimensions = { 96u, 64u };
        recipe.StructuralSource = ShipGenerationRecipeProfileSource::BUILT_IN_PRESET;
        recipe.Style = ShipStyle::INDUSTRIAL;
        recipe.FactionSource = ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        recipe.Faction = ShipFactionType::SHIP_FACTION_TYPE_END;
        recipe.FactionProfile = getShipFactionProfile(ShipFactionType::MILITARY);
        recipe.FactionProfile.Weapons.EmissiveChance = 61u;
        recipe.PaletteConfiguration = makeBundlePalette();
        recipe.DetailDensity = 53u;
        recipe.AsymmetricDetailChance = 17u;
        recipe.AttachmentsEnabled = true;
        return recipe;
    }
}

int PixelShipGeneratorTests::runPreviewConfigurationBundleRegression()
{
    using namespace PixelShipGenerator;
    using namespace PixelShipGeneratorPreview;

    bool success = true;
    const std::filesystem::path directory = std::filesystem::temp_directory_path() / "pixel_ship_generator_configuration_bundle_regression";
    const std::filesystem::path libraryPath = directory / "user_presets.json";
    const std::filesystem::path bundlePath = directory / "heavy_naval_test.shipgenbundle.json";
    const std::filesystem::path favoritePath = directory / "favorites.json";
    const std::filesystem::path recipePath = directory / "bundle_ship.shipgen.json";
    std::error_code filesystemError;
    std::filesystem::remove_all(directory, filesystemError);
    std::filesystem::create_directories(directory, filesystemError);

    // Secondary Profiles navigation remains one four-section cyclic model.
    if (std::string(getProfilesSectionName(ProfilesSection::STRUCTURAL)) != "Structural" ||
        std::string(getProfilesSectionName(ProfilesSection::FACTION)) != "Faction" ||
        std::string(getProfilesSectionName(ProfilesSection::PALETTE)) != "Palette" ||
        std::string(getProfilesSectionName(ProfilesSection::FULL_CONFIGURATION)) != "Full Configuration" ||
        getWrappedProfilesSection(ProfilesSection::STRUCTURAL, -1) != ProfilesSection::FULL_CONFIGURATION ||
        getWrappedProfilesSection(ProfilesSection::FULL_CONFIGURATION, 1) != ProfilesSection::STRUCTURAL)
    {
        success = false;
        std::cerr << "Profiles secondary section navigation failed.\n";
    }

    const PreviewGenerationRecipe sourceRecipe = makeMixedRecipe();
    const ConfigurationBundle sourceBundle = makeConfigurationBundle(sourceRecipe, "INDUSTRIAL", "Military Tech Variant", "Naval Steel");
    if (!validateConfigurationBundle(sourceBundle).isValid())
    {
        success = false;
        std::cerr << "Valid mixed built-in/custom Full Configuration bundle failed Core validation.\n";
    }

    // Full Configuration editor is a shallow component editor, not three nested field editors.
    PreviewConfigurationEditor editor;
    editor.openConfigurationBundle("Heavy Naval Test", sourceBundle);
    if (editor.getProfileKind() != ConfigurationEditorProfileKind::FULL_CONFIGURATION || editor.getBoundValueCount() != 3u ||
        editor.getBundleComponentControls()[0u].Value != "INDUSTRIAL" || editor.getBundleComponentControls()[1u].Value != "Military Tech Variant" ||
        editor.getBundleComponentControls()[2u].Value != "Naval Steel")
    {
        success = false;
        std::cerr << "Full Configuration editor did not expose the three reusable components.\n";
    }

    RuntimeCustomPresetWorkspace workspace;
    const RuntimeCustomPresetId structuralId = workspace.addStructural("Local Structural", sourceBundle.StructuralProfile);
    const RuntimeCustomPresetId factionId = workspace.addFaction("Local Faction", sourceBundle.FactionProfile);
    const RuntimeCustomPresetId paletteId = workspace.addPalette("Local Palette", sourceBundle.PaletteConfiguration);
    const RuntimeCustomPresetId bundleId = workspace.addConfigurationBundle("Heavy Naval Test", sourceBundle);
    if (structuralId == factionId || factionId == paletteId || paletteId == bundleId || workspace.findConfigurationBundle(bundleId) == nullptr)
    {
        success = false;
        std::cerr << "Configuration bundle did not participate in stable shared local identity.\n";
    }

    // CRUD semantics: duplicate, rename/save, delete.
    const std::optional<RuntimeCustomPresetId> duplicateId = workspace.duplicateConfigurationBundle(bundleId);
    if (!duplicateId.has_value() || workspace.findConfigurationBundle(*duplicateId) == nullptr ||
        !workspace.updateConfigurationBundle(*duplicateId, "Heavy Naval Test Renamed", sourceBundle) ||
        workspace.findConfigurationBundle(*duplicateId)->Name != "Heavy Naval Test Renamed" ||
        !workspace.removeConfigurationBundle(*duplicateId))
    {
        success = false;
        std::cerr << "Configuration bundle duplicate/rename/delete semantics failed.\n";
    }

    std::string error;
    if (!saveUserPresetLibrary(workspace, libraryPath, error))
    {
        success = false;
        std::cerr << error << '\n';
    }
    const UserPresetLibraryLoadResult reloaded = loadUserPresetLibrary(libraryPath);
    const RuntimeConfigurationBundle* reloadedBundle = reloaded.Success ? reloaded.Workspace.findConfigurationBundle(bundleId) : nullptr;
    if (!reloaded.Success || reloaded.SkippedEntryCount != 0u || reloadedBundle == nullptr || reloadedBundle->Name != "Heavy Naval Test")
    {
        success = false;
        std::cerr << "Configuration bundle persistence/restart failed.\n";
    }

    // Task-93 v1 libraries remain valid and simply have no Full Configuration entries.
    std::string versionOneLibrary = serializeUserPresetLibrary(RuntimeCustomPresetWorkspace{});
    const std::string versionTwoToken = "\"format_version\": 2";
    const std::size_t versionPosition = versionOneLibrary.find(versionTwoToken);
    const std::size_t bundlesPosition = versionOneLibrary.find("\"configuration_bundles\": []");
    if (versionPosition == std::string::npos || bundlesPosition == std::string::npos)
    {
        success = false;
        std::cerr << "Could not construct Task-93 compatibility fixture.\n";
    }
    else
    {
        versionOneLibrary.replace(versionPosition, versionTwoToken.size(), "\"format_version\": 1");
        std::size_t eraseStart = bundlesPosition;
        if (eraseStart >= 2u && versionOneLibrary.substr(eraseStart - 2u, 2u) == ",\n") { eraseStart -= 2u; }
        else
        {
            const std::size_t trailing = bundlesPosition + std::string("\"configuration_bundles\": []").size();
            if (trailing + 2u <= versionOneLibrary.size() && versionOneLibrary.substr(trailing, 2u) == ",\n") { versionOneLibrary.erase(trailing, 2u); }
        }
        versionOneLibrary.erase(eraseStart, bundlesPosition + std::string("\"configuration_bundles\": []").size() - eraseStart);
        const UserPresetLibraryLoadResult oldLibrary = deserializeUserPresetLibrary(versionOneLibrary);
        if (!oldLibrary.Success || !oldLibrary.Workspace.getConfigurationBundles().empty())
        {
            success = false;
            std::cerr << "Task-93 v1 user preset library compatibility failed.\n";
        }
    }

    if (!exportUserPreset(workspace, UserPresetCategory::FULL_CONFIGURATION, bundleId, bundlePath, error))
    {
        success = false;
        std::cerr << (error.empty() ? "Configuration bundle export failed." : error) << '\n';
    }

    // Empty-install portability: import without any matching component presets.
    RuntimeCustomPresetWorkspace emptyInstall;
    const UserPresetImportResult imported = importUserPreset(emptyInstall, UserPresetCategory::FULL_CONFIGURATION, bundlePath);
    const RuntimeConfigurationBundle* importedBundle = imported.Success ? emptyInstall.findConfigurationBundle(imported.ImportedId) : nullptr;
    if (!imported.Success || importedBundle == nullptr || !emptyInstall.getStructuralPresets().empty() || !emptyInstall.getFactionPresets().empty() || !emptyInstall.getPalettePresets().empty())
    {
        success = false;
        std::cerr << "Empty-install Full Configuration import depended on local component presets.\n";
    }
    if (importUserPreset(emptyInstall, UserPresetCategory::STRUCTURAL, bundlePath).Success)
    {
        success = false;
        std::cerr << "Full Configuration file silently imported as another preset category.\n";
    }

    if (importedBundle != nullptr)
    {
        PreviewGenerationRecipe appliedRecipe = sourceRecipe;
        appliedRecipe.StructuralSource = ShipGenerationRecipeProfileSource::BUILT_IN_PRESET;
        appliedRecipe.Style = ShipStyle::SLEEK;
        appliedRecipe.FactionSource = ShipGenerationRecipeProfileSource::BUILT_IN_PRESET;
        appliedRecipe.Faction = ShipFactionType::XENO;
        appliedRecipe.PaletteConfiguration = {};
        const ShipGenerationSeeds seedsBefore = appliedRecipe.Seeds;
        const ShipDimensions dimensionsBefore = appliedRecipe.Dimensions;
        const uint32_t detailDensityBefore = appliedRecipe.DetailDensity;
        const uint32_t asymmetricBefore = appliedRecipe.AsymmetricDetailChance;
        const bool attachmentsBefore = appliedRecipe.AttachmentsEnabled;
        applyConfigurationBundle(importedBundle->Bundle, appliedRecipe);
        if (!(appliedRecipe.Seeds == seedsBefore) || !(appliedRecipe.Dimensions == dimensionsBefore) || appliedRecipe.DetailDensity != detailDensityBefore ||
            appliedRecipe.AsymmetricDetailChance != asymmetricBefore || appliedRecipe.AttachmentsEnabled != attachmentsBefore ||
            appliedRecipe.StructuralSource != ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM || appliedRecipe.FactionSource != ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM)
        {
            success = false;
            std::cerr << "Applying a bundle modified generation-specific recipe state or fabricated built-in identity.\n";
        }

        const Image expected = ShipGenerator{}.generate(sourceRecipe).FinalImage;
        const Image applied = ShipGenerator{}.generate(appliedRecipe).FinalImage;
        if (!imagesEqual(expected, applied))
        {
            success = false;
            std::cerr << "Imported Full Configuration changed deterministic generation output.\n";
        }

        const std::string bundleBeforeIndependentEdit = serializeUserPresetFile(*importedBundle);
        appliedRecipe.StructuralProfile.LargeWeaponChance = std::min<uint32_t>(100u, appliedRecipe.StructuralProfile.LargeWeaponChance + 1u);
        if (serializeUserPresetFile(*importedBundle) != bundleBeforeIndependentEdit)
        {
            success = false;
            std::cerr << "Changing an active recipe component silently mutated the saved bundle.\n";
        }
        applyConfigurationBundle(importedBundle->Bundle, appliedRecipe);

        ShipGenerationRecipeDocument document;
        document.Recipe = appliedRecipe;
        if (!saveShipGenerationRecipe(document, recipePath, error) || !savePreviewFavorites({ appliedRecipe }, favoritePath, error))
        {
            success = false;
            std::cerr << (error.empty() ? "Recipe/Favorite bundle-independence setup failed." : error) << '\n';
        }
        emptyInstall.removeConfigurationBundle(imported.ImportedId);
        const ShipGenerationRecipeLoadResult loadedRecipe = loadShipGenerationRecipe(recipePath);
        const PreviewFavoritesLoadResult loadedFavorites = loadPreviewFavorites(favoritePath);
        if (!loadedRecipe.Success || !loadedFavorites.Success || loadedFavorites.Favorites.size() != 1u ||
            !imagesEqual(expected, ShipGenerator{}.generate(loadedRecipe.Document.Recipe).FinalImage) ||
            !imagesEqual(expected, ShipGenerator{}.generate(loadedFavorites.Favorites.front()).FinalImage))
        {
            success = false;
            std::cerr << "Recipe/Favorite became dependent on deleted local Full Configuration bundle.\n";
        }
    }

    if (std::filesystem::exists(bundlePath.string() + ".tmp") || std::filesystem::exists(bundlePath.string() + ".bak") ||
        std::filesystem::exists(libraryPath.string() + ".tmp") || std::filesystem::exists(libraryPath.string() + ".bak"))
    {
        success = false;
        std::cerr << "Configuration bundle safe write left temporary/backup files behind.\n";
    }

    std::filesystem::remove_all(directory, filesystemError);
    return success ? 0 : 1;
}
