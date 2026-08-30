#include "RegressionSuites.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "BuiltInPresetCatalog.h"
#include "FactionProfileSelection.h"
#include "PaletteProfileSelection.h"
#include "PreviewConfigurationEditor.h"
#include "PreviewFavoritesPersistence.h"
#include "PreviewGenerationRecipe.h"
#include "RuntimeCustomPresetWorkspace.h"
#include "ShipFactionProfile.h"
#include "ShipGenerationProfile.h"
#include "ShipGenerationRecipeSerializer.h"
#include "ShipGenerationSeeds.h"
#include "ShipGenerator.h"
#include "ShipPaletteGenerationProfile.h"
#include "StructuralProfileSelection.h"
#include "UserPresetPersistence.h"

namespace
{
    using namespace PixelShipGenerator;
    using namespace PixelShipGeneratorPreview;

    bool imagesEqual(const Image& first, const Image& second)
    {
        return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
    }

    bool replaceOnce(std::string& text, const std::string& from, const std::string& to)
    {
        const std::size_t position = text.find(from);
        if (position == std::string::npos) { return false; }
        text.replace(position, from.size(), to);
        return true;
    }

    ShipPaletteConfiguration makeCustomPalette()
    {
        ShipPaletteConfiguration configuration;
        configuration.Mode = ShipPaletteSourceMode::FIXED;
        configuration.Generated = getBuiltInPalettePresetProfile(ShipFactionType::CORPORATE);
        configuration.Fixed = ShipPalette{};
        configuration.Fixed.HullBase = Color(43u, 92u, 137u, 255u);
        configuration.Fixed.HullAccent = Color(211u, 79u, 148u, 255u);
        configuration.Fixed.EngineHotCore = Color(255u, 218u, 122u, 255u);
        return configuration;
    }

    PreviewGenerationRecipe makeCustomRecipe(const RuntimeStructuralPreset& structural, const RuntimeFactionPreset& faction, const RuntimePalettePreset& palette)
    {
        PreviewGenerationRecipe recipe;
        recipe.Seeds = deriveShipGenerationSeeds(0x9300000000000020ull);
        recipe.Dimensions = { 96u, 64u };
        recipe.StructuralSource = ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        recipe.Style = ShipStyle::SHIP_STYLE_END;
        recipe.StructuralProfile = structural.Profile;
        recipe.FactionSource = ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        recipe.Faction = ShipFactionType::SHIP_FACTION_TYPE_END;
        recipe.FactionProfile = faction.Profile;
        recipe.PaletteConfiguration = palette.Configuration;
        recipe.DetailDensity = 57u;
        recipe.AsymmetricDetailChance = 13u;
        recipe.AttachmentsEnabled = true;
        return recipe;
    }

    bool noSafeWriteJunk(const std::filesystem::path& path)
    {
        return !std::filesystem::exists(path.string() + ".tmp") && !std::filesystem::exists(path.string() + ".bak");
    }
}

int PixelShipGeneratorTests::runPreviewUserPresetPersistenceRegression()
{
    using namespace PixelShipGenerator;
    using namespace PixelShipGeneratorPreview;

    bool success = true;
    const std::filesystem::path directory = std::filesystem::temp_directory_path() / "pixel_ship_generator_user_preset_regression";
    const std::filesystem::path libraryPath = directory / "user_presets.json";
    const std::filesystem::path structuralExportPath = directory / "industrial.shipgenpreset.json";
    const std::filesystem::path factionExportPath = directory / "faction.shipgenpreset.json";
    const std::filesystem::path paletteExportPath = directory / "palette.shipgenpreset.json";
    const std::filesystem::path favoritePath = directory / "favorites.json";
    const std::filesystem::path recipePath = directory / "favorite.shipgen.json";
    std::error_code filesystemError;
    std::filesystem::remove_all(directory, filesystemError);
    std::filesystem::create_directories(directory, filesystemError);

    const UserPresetLibraryLoadResult missing = loadUserPresetLibrary(libraryPath);
    if (!missing.Success || !missing.Workspace.getStructuralPresets().empty() || !missing.Workspace.getFactionPresets().empty() || !missing.Workspace.getPalettePresets().empty())
    {
        success = false;
        std::cerr << "Missing user preset library did not load as an empty first-run library.\n";
    }

    const ShipGenerationProfile industrialBefore = getShipGenerationProfile(ShipStyle::INDUSTRIAL);
    const ShipFactionProfile factionBefore = getShipFactionProfile(ShipFactionType::RELIC);
    const ShipPaletteGenerationProfile paletteBefore = getBuiltInPalettePresetProfile(ShipFactionType::CORPORATE);

    RuntimeCustomPresetWorkspace workspace;
    ShipGenerationProfile structural = industrialBefore;
    structural.LargeWeaponChance = 78u;
    structural.SweptWingWeight += 11u;
    structural.LargeWeaponScalePercent = 145u;
    const RuntimeCustomPresetId structuralId = workspace.addStructural("My Heavy Freighter", structural);

    ShipFactionProfile faction = factionBefore;
    faction.MajorFeatures.WeightMultipliersPercent.TechCore = 180u;
    faction.Weapons.EmissiveChance = 64u;
    faction.Animation.Idle.TechPulseStrength = 7u;
    const RuntimeCustomPresetId factionId = workspace.addFaction("Relic Tech Variant", faction);

    const ShipPaletteConfiguration palette = makeCustomPalette();
    const RuntimeCustomPresetId paletteId = workspace.addPalette("Freighter Neon", palette);
    const RuntimeCustomPresetId nextIdBeforeSave = workspace.getNextId();

    std::string error;
    if (!saveUserPresetLibrary(workspace, libraryPath, error))
    {
        success = false;
        std::cerr << error << '\n';
    }
    const UserPresetLibraryLoadResult reloaded = loadUserPresetLibrary(libraryPath);
    if (!reloaded.Success || reloaded.SkippedEntryCount != 0u || reloaded.Workspace.getNextId() != nextIdBeforeSave ||
        reloaded.Workspace.findStructural(structuralId) == nullptr || reloaded.Workspace.findFaction(factionId) == nullptr || reloaded.Workspace.findPalette(paletteId) == nullptr ||
        serializeUserPresetLibrary(reloaded.Workspace) != serializeUserPresetLibrary(workspace))
    {
        success = false;
        std::cerr << "All-category user preset save/restart/load did not preserve identity and configuration.\n";
    }
    if (!noSafeWriteJunk(libraryPath))
    {
        success = false;
        std::cerr << "User preset library safe write left temporary/backup files behind.\n";
    }

    const auto structuralEntries = buildStructuralProfileSelection(reloaded.Workspace);
    const auto factionEntries = buildFactionProfileSelection(reloaded.Workspace);
    const auto paletteEntries = buildPaletteProfileSelection(reloaded.Workspace);
    const bool structuralVisible = std::any_of(structuralEntries.begin(), structuralEntries.end(), [structuralId](const StructuralProfileSelectionEntry& entry)
        { return entry.Kind == StructuralProfileSelectionKind::RUNTIME_CUSTOM && entry.CustomPresetId == structuralId && entry.Label == "My Heavy Freighter"; });
    const bool factionVisible = std::any_of(factionEntries.begin(), factionEntries.end(), [factionId](const FactionProfileSelectionEntry& entry)
        { return entry.Kind == FactionProfileSelectionKind::RUNTIME_CUSTOM && entry.CustomPresetId == factionId && entry.Label == "Relic Tech Variant"; });
    const bool paletteVisible = std::any_of(paletteEntries.begin(), paletteEntries.end(), [paletteId](const PaletteProfileSelectionEntry& entry)
        { return entry.Kind == PaletteProfileSelectionKind::RUNTIME_CUSTOM && entry.CustomPresetId == paletteId && entry.Label == "Freighter Neon"; });
    if (!structuralVisible || !factionVisible || !paletteVisible)
    {
        success = false;
        std::cerr << "Reloaded user presets were not restored into all three existing selector models.\n";
    }

    PreviewConfigurationEditor editor;
    editor.openStructuralProfile("INDUSTRIAL", industrialBefore);
    const auto findAction = [&](ConfigurationEditorAction action) -> const ConfigurationEditorActionButton*
        {
            const auto& buttons = editor.getActionButtons();
            const auto iterator = std::find_if(buttons.begin(), buttons.end(), [action](const ConfigurationEditorActionButton& button) { return button.Action == action; });
            return iterator == buttons.end() ? nullptr : &*iterator;
        };
    const ConfigurationEditorActionButton* deleteBuiltIn = findAction(ConfigurationEditorAction::DELETE_PRESET);
    const ConfigurationEditorActionButton* exportBuiltIn = findAction(ConfigurationEditorAction::EXPORT_PRESET);
    const ConfigurationEditorActionButton* importBuiltIn = findAction(ConfigurationEditorAction::IMPORT_PRESET);
    if (deleteBuiltIn == nullptr || exportBuiltIn == nullptr || importBuiltIn == nullptr || deleteBuiltIn->Enabled || exportBuiltIn->Enabled || !importBuiltIn->Enabled)
    {
        success = false;
        std::cerr << "Configuration editor did not preserve built-in immutability for Task-93 preset actions.\n";
    }
    editor.setExistingCustomPreset(true);
    const ConfigurationEditorActionButton* deleteCustom = findAction(ConfigurationEditorAction::DELETE_PRESET);
    const ConfigurationEditorActionButton* exportCustom = findAction(ConfigurationEditorAction::EXPORT_PRESET);
    if (deleteCustom == nullptr || exportCustom == nullptr || !deleteCustom->Enabled || !exportCustom->Enabled)
    {
        success = false;
        std::cerr << "Saved user preset actions were not enabled for an existing custom preset.\n";
    }

    RuntimeCustomPresetWorkspace mutationWorkspace = reloaded.Workspace;
    const RuntimeCustomPresetId stableRenameId = structuralId;
    if (!mutationWorkspace.updateStructural(stableRenameId, "Capital Freighter", structural) || mutationWorkspace.findStructural(stableRenameId) == nullptr || mutationWorkspace.findStructural(stableRenameId)->Name != "Capital Freighter")
    {
        success = false;
        std::cerr << "User preset rename did not retain stable local identity.\n";
    }
    const std::optional<RuntimeCustomPresetId> duplicateId = mutationWorkspace.duplicateFaction(factionId);
    if (!duplicateId.has_value() || *duplicateId == factionId || mutationWorkspace.findFaction(*duplicateId) == nullptr || mutationWorkspace.findFaction(*duplicateId)->Name == mutationWorkspace.findFaction(factionId)->Name)
    {
        success = false;
        std::cerr << "User preset duplicate did not allocate independent identity/name.\n";
    }
    if (!mutationWorkspace.removePalette(paletteId) || mutationWorkspace.findPalette(paletteId) != nullptr)
    {
        success = false;
        std::cerr << "User preset delete failed.\n";
    }
    if (!saveUserPresetLibrary(mutationWorkspace, libraryPath, error) || !loadUserPresetLibrary(libraryPath).Success)
    {
        success = false;
        std::cerr << (error.empty() ? "Committed user preset mutation persistence failed." : error) << '\n';
    }

    if (getShipGenerationProfile(ShipStyle::INDUSTRIAL).LargeWeaponChance != industrialBefore.LargeWeaponChance ||
        getShipFactionProfile(ShipFactionType::RELIC).Weapons.EmissiveChance != factionBefore.Weapons.EmissiveChance ||
        getBuiltInPalettePresetProfile(ShipFactionType::CORPORATE).Ranges.HullHue.Min != paletteBefore.Ranges.HullHue.Min)
    {
        success = false;
        std::cerr << "Built-in preset data changed while editing user copies.\n";
    }

    if (!exportUserPreset(workspace, UserPresetCategory::STRUCTURAL, structuralId, structuralExportPath, error) ||
        !exportUserPreset(workspace, UserPresetCategory::FACTION, factionId, factionExportPath, error) ||
        !exportUserPreset(workspace, UserPresetCategory::PALETTE, paletteId, paletteExportPath, error))
    {
        success = false;
        std::cerr << (error.empty() ? "Per-preset export failed." : error) << '\n';
    }

    RuntimeCustomPresetWorkspace conflictWorkspace;
    conflictWorkspace.addStructural("My Heavy Freighter", getShipGenerationProfile(ShipStyle::FIGHTER));
    const UserPresetImportResult conflictImport = importUserPreset(conflictWorkspace, UserPresetCategory::STRUCTURAL, structuralExportPath);
    if (!conflictImport.Success || !conflictImport.DisplayNameDisambiguated || conflictImport.DisplayName == "My Heavy Freighter" || conflictWorkspace.findStructural(conflictImport.ImportedId) == nullptr)
    {
        success = false;
        std::cerr << "Import name conflict was not predictably disambiguated.\n";
    }
    const UserPresetImportResult wrongCategory = importUserPreset(conflictWorkspace, UserPresetCategory::STRUCTURAL, factionExportPath);
    if (wrongCategory.Success)
    {
        success = false;
        std::cerr << "Faction preset silently imported as a structural preset.\n";
    }

    std::ifstream structuralExportStream(structuralExportPath, std::ios::binary);
    std::string structuralExportText((std::istreambuf_iterator<char>(structuralExportStream)), std::istreambuf_iterator<char>());
    std::string invalidImport = structuralExportText;
    if (!replaceOnce(invalidImport, "\"LargeWeaponChance\": 78", "\"LargeWeaponChance\": 999") || importUserPreset(conflictWorkspace, UserPresetCategory::STRUCTURAL, invalidImport).Success)
    {
        success = false;
        std::cerr << "Core structural validation was not enforced during import.\n";
    }

    std::string partialLibrary = serializeUserPresetLibrary(workspace);
    if (!replaceOnce(partialLibrary, "\"LargeWeaponChance\": 78", "\"LargeWeaponChance\": 999"))
    {
        success = false;
        std::cerr << "Regression fixture could not create an invalid structural library entry.\n";
    }
    else
    {
        const UserPresetLibraryLoadResult partial = deserializeUserPresetLibrary(partialLibrary);
        if (!partial.Success || partial.SkippedEntryCount != 1u || !partial.Workspace.getStructuralPresets().empty() || partial.Workspace.getFactionPresets().size() != 1u || partial.Workspace.getPalettePresets().size() != 1u)
        {
            success = false;
            std::cerr << "Partially invalid user preset library did not preserve valid entries.\n";
        }
    }

    std::string unsupportedLibrary = serializeUserPresetLibrary(workspace);
    replaceOnce(unsupportedLibrary, "\"format_version\": 1", "\"format_version\": 99");
    if (deserializeUserPresetLibrary(unsupportedLibrary).Success || deserializeUserPresetLibrary("{ this is not JSON").Success)
    {
        success = false;
        std::cerr << "Malformed/schema-version user preset library handling failed.\n";
    }
    std::string unsupportedImport = structuralExportText;
    replaceOnce(unsupportedImport, "\"format_version\": 1", "\"format_version\": 99");
    if (importUserPreset(conflictWorkspace, UserPresetCategory::STRUCTURAL, unsupportedImport).Success)
    {
        success = false;
        std::cerr << "Unsupported individual preset schema version was accepted.\n";
    }

    // Task-93 end-to-end workflow: author three custom presets, generate/favorite/export,
    // restart, delete local presets, verify self-contained recipe/Favorite, then import again.
    RuntimeCustomPresetWorkspace acceptanceWorkspace = workspace;
    const RuntimeStructuralPreset* acceptanceStructural = acceptanceWorkspace.findStructural(structuralId);
    const RuntimeFactionPreset* acceptanceFaction = acceptanceWorkspace.findFaction(factionId);
    const RuntimePalettePreset* acceptancePalette = acceptanceWorkspace.findPalette(paletteId);
    if (acceptanceStructural == nullptr || acceptanceFaction == nullptr || acceptancePalette == nullptr)
    {
        success = false;
        std::cerr << "Acceptance preset setup failed.\n";
    }
    else
    {
        const PreviewGenerationRecipe recipe = makeCustomRecipe(*acceptanceStructural, *acceptanceFaction, *acceptancePalette);
        const Image beforeDeletion = ShipGenerator{}.generate(recipe).FinalImage;
        ShipGenerationRecipeDocument document;
        document.Recipe = recipe;
        if (!saveShipGenerationRecipe(document, recipePath, error) || !savePreviewFavorites({ recipe }, favoritePath, error) || !saveUserPresetLibrary(acceptanceWorkspace, libraryPath, error))
        {
            success = false;
            std::cerr << (error.empty() ? "Acceptance export/persistence setup failed." : error) << '\n';
        }

        UserPresetLibraryLoadResult restarted = loadUserPresetLibrary(libraryPath);
        if (!restarted.Success || restarted.Workspace.findStructural(structuralId) == nullptr || restarted.Workspace.findFaction(factionId) == nullptr || restarted.Workspace.findPalette(paletteId) == nullptr)
        {
            success = false;
            std::cerr << "Acceptance restart did not restore all custom presets.\n";
        }
        else
        {
            restarted.Workspace.removeStructural(structuralId);
            restarted.Workspace.removeFaction(factionId);
            restarted.Workspace.removePalette(paletteId);
            if (!saveUserPresetLibrary(restarted.Workspace, libraryPath, error))
            {
                success = false;
                std::cerr << error << '\n';
            }

            const ShipGenerationRecipeLoadResult loadedRecipe = loadShipGenerationRecipe(recipePath);
            const PreviewFavoritesLoadResult loadedFavorites = loadPreviewFavorites(favoritePath);
            if (!loadedRecipe.Success || !loadedFavorites.Success || loadedFavorites.Favorites.size() != 1u ||
                !imagesEqual(beforeDeletion, ShipGenerator{}.generate(loadedRecipe.Document.Recipe).FinalImage) ||
                !imagesEqual(beforeDeletion, ShipGenerator{}.generate(loadedFavorites.Favorites.front()).FinalImage))
            {
                success = false;
                std::cerr << "Recipe/Favorite stopped reproducing after originating local presets were deleted.\n";
            }

            UserPresetLibraryLoadResult emptyRestart = loadUserPresetLibrary(libraryPath);
            if (!emptyRestart.Success || !emptyRestart.Workspace.getStructuralPresets().empty() || !emptyRestart.Workspace.getFactionPresets().empty() || !emptyRestart.Workspace.getPalettePresets().empty())
            {
                success = false;
                std::cerr << "Deleted local presets unexpectedly reappeared after restart.\n";
            }
            else
            {
                const UserPresetImportResult importedStructural = importUserPreset(emptyRestart.Workspace, UserPresetCategory::STRUCTURAL, structuralExportPath);
                const UserPresetImportResult importedFaction = importUserPreset(emptyRestart.Workspace, UserPresetCategory::FACTION, factionExportPath);
                const UserPresetImportResult importedPalette = importUserPreset(emptyRestart.Workspace, UserPresetCategory::PALETTE, paletteExportPath);
                const RuntimeStructuralPreset* restoredStructural = importedStructural.Success ? emptyRestart.Workspace.findStructural(importedStructural.ImportedId) : nullptr;
                const RuntimeFactionPreset* restoredFaction = importedFaction.Success ? emptyRestart.Workspace.findFaction(importedFaction.ImportedId) : nullptr;
                const RuntimePalettePreset* restoredPalette = importedPalette.Success ? emptyRestart.Workspace.findPalette(importedPalette.ImportedId) : nullptr;
                if (restoredStructural == nullptr || restoredFaction == nullptr || restoredPalette == nullptr)
                {
                    success = false;
                    std::cerr << "Acceptance re-import did not restore all three preset categories.\n";
                }
                else
                {
                    const PreviewGenerationRecipe restoredRecipe = makeCustomRecipe(*restoredStructural, *restoredFaction, *restoredPalette);
                    if (!imagesEqual(beforeDeletion, ShipGenerator{}.generate(restoredRecipe).FinalImage))
                    {
                        success = false;
                        std::cerr << "Re-imported presets changed deterministic generation output.\n";
                    }
                }
            }
        }
    }

    if (!noSafeWriteJunk(libraryPath) || !noSafeWriteJunk(structuralExportPath) || !noSafeWriteJunk(factionExportPath) || !noSafeWriteJunk(paletteExportPath))
    {
        success = false;
        std::cerr << "Task-93 persistence/export left temporary or backup junk.\n";
    }

    std::filesystem::remove_all(directory, filesystemError);
    return success ? 0 : 1;
}
