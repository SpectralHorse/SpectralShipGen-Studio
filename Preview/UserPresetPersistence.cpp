#include "UserPresetPersistence.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>

#include "ShipFactionProfileValidation.h"
#include "ShipGenerationProfileValidation.h"
#include "ShipGenerationRecipeJson.h"
#include "ShipGenerationRecipeProfileSerialization.h"
#include "ShipPaletteGenerationProfileValidation.h"

namespace
{
    using namespace PixelShipGenerator;
    using namespace PixelShipGeneratorPreview;
    using RecipeJson::Type;
    using RecipeJson::Value;

    const char* paletteModeId(ShipPaletteSourceMode mode)
    {
        switch (mode)
        {
        case ShipPaletteSourceMode::FACTION_PROFILE_GENERATED: return "FACTION_PROFILE_GENERATED";
        case ShipPaletteSourceMode::EXPLICIT_GENERATED: return "EXPLICIT_GENERATED";
        case ShipPaletteSourceMode::FIXED: return "FIXED";
        default: return "INVALID";
        }
    }

    bool paletteModeFromId(const std::string& id, ShipPaletteSourceMode& mode)
    {
        if (id == "FACTION_PROFILE_GENERATED") { mode = ShipPaletteSourceMode::FACTION_PROFILE_GENERATED; return true; }
        if (id == "EXPLICIT_GENERATED") { mode = ShipPaletteSourceMode::EXPLICIT_GENERATED; return true; }
        if (id == "FIXED") { mode = ShipPaletteSourceMode::FIXED; return true; }
        return false;
    }

    bool categoryFromId(const std::string& id, UserPresetCategory& category)
    {
        if (id == "STRUCTURAL") { category = UserPresetCategory::STRUCTURAL; return true; }
        if (id == "FACTION") { category = UserPresetCategory::FACTION; return true; }
        if (id == "PALETTE") { category = UserPresetCategory::PALETTE; return true; }
        if (id == "FULL_CONFIGURATION") { category = UserPresetCategory::FULL_CONFIGURATION; return true; }
        return false;
    }

    Value serializePaletteConfiguration(const ShipPaletteConfiguration& configuration)
    {
        Value result = Value::object();
        result.Object["mode"] = Value::string(paletteModeId(configuration.Mode));
        result.Object["generated"] = RecipeProfileSerialization::serialize(configuration.Generated);
        result.Object["fixed"] = RecipeProfileSerialization::serialize(configuration.Fixed);
        return result;
    }

    bool deserializePaletteConfiguration(const Value& value, ShipPaletteConfiguration& configuration, std::string& error, const std::string& path)
    {
        if (value.ValueType != Type::Object) { error = path + " must be a JSON object."; return false; }
        std::string mode;
        if (!RecipeJson::getString(value, "mode", mode, error, path) || !paletteModeFromId(mode, configuration.Mode))
        {
            if (error.empty()) { error = "Unknown " + path + ".mode: " + mode + "."; }
            return false;
        }

        const Value* generated = value.find("generated");
        if (generated == nullptr) { error = "Missing required field: " + path + ".generated."; return false; }
        if (!RecipeProfileSerialization::deserialize(*generated, configuration.Generated, error, path + ".generated")) { return false; }

        const Value* fixed = value.find("fixed");
        if (fixed == nullptr) { error = "Missing required field: " + path + ".fixed."; return false; }
        return RecipeProfileSerialization::deserialize(*fixed, configuration.Fixed, error, path + ".fixed");
    }

    Value serializeConfigurationBundle(const ConfigurationBundle& bundle)
    {
        Value result = Value::object();
        Value metadata = Value::object();
        metadata.Object["structural_display_name"] = Value::string(bundle.StructuralDisplayName);
        metadata.Object["faction_display_name"] = Value::string(bundle.FactionDisplayName);
        metadata.Object["palette_display_name"] = Value::string(bundle.PaletteDisplayName);
        result.Object["component_metadata"] = std::move(metadata);
        result.Object["structural"] = RecipeProfileSerialization::serialize(bundle.StructuralProfile);
        result.Object["faction"] = RecipeProfileSerialization::serialize(bundle.FactionProfile);
        result.Object["palette"] = serializePaletteConfiguration(bundle.PaletteConfiguration);
        return result;
    }

    bool deserializeConfigurationBundle(const Value& value, ConfigurationBundle& bundle, std::string& error, const std::string& path)
    {
        if (value.ValueType != Type::Object) { error = path + " must be a JSON object."; return false; }
        const Value* metadata = value.find("component_metadata");
        if (metadata != nullptr && metadata->ValueType == Type::Object)
        {
            if (!RecipeJson::getString(*metadata, "structural_display_name", bundle.StructuralDisplayName, error, path + ".component_metadata") ||
                !RecipeJson::getString(*metadata, "faction_display_name", bundle.FactionDisplayName, error, path + ".component_metadata") ||
                !RecipeJson::getString(*metadata, "palette_display_name", bundle.PaletteDisplayName, error, path + ".component_metadata")) { return false; }
        }
        else
        {
            bundle.StructuralDisplayName = "Embedded Structural";
            bundle.FactionDisplayName = "Embedded Faction";
            bundle.PaletteDisplayName = "Embedded Palette";
        }

        const Value* structural = value.find("structural");
        const Value* faction = value.find("faction");
        const Value* palette = value.find("palette");
        if (structural == nullptr) { error = "Missing required field: " + path + ".structural."; return false; }
        if (faction == nullptr) { error = "Missing required field: " + path + ".faction."; return false; }
        if (palette == nullptr) { error = "Missing required field: " + path + ".palette."; return false; }
        if (!RecipeProfileSerialization::deserialize(*structural, bundle.StructuralProfile, error, path + ".structural")) { return false; }
        if (!RecipeProfileSerialization::deserialize(*faction, bundle.FactionProfile, error, path + ".faction")) { return false; }
        return deserializePaletteConfiguration(*palette, bundle.PaletteConfiguration, error, path + ".palette");
    }

    std::string validationError(const ValidationResult& validation, const std::string& prefix)
    {
        if (validation.isValid()) { return {}; }
        return prefix + validation.Errors.front().Field + " - " + validation.Errors.front().Message;
    }

    bool validateStructural(const ShipGenerationProfile& profile, std::string& error)
    {
        error = validationError(validateShipGenerationProfile(profile), "Invalid structural profile: ");
        return error.empty();
    }

    bool validateFaction(const ShipFactionProfile& profile, std::string& error)
    {
        error = validationError(validateShipFactionProfile(profile), "Invalid faction profile: ");
        return error.empty();
    }

    bool validatePalette(const ShipPaletteConfiguration& configuration, std::string& error)
    {
        if (configuration.Mode >= ShipPaletteSourceMode::SHIP_PALETTE_SOURCE_MODE_END)
        {
            error = "Invalid palette configuration: Mode is outside the supported range.";
            return false;
        }
        if (configuration.Mode != ShipPaletteSourceMode::EXPLICIT_GENERATED)
        {
            error.clear();
            return true;
        }
        error = validationError(validateShipPaletteGenerationProfile(configuration.Generated), "Invalid palette generation profile: ");
        return error.empty();
    }

    bool validateBundle(const ConfigurationBundle& bundle, std::string& error)
    {
        error = validationError(validateConfigurationBundle(bundle), "Invalid full configuration bundle: ");
        return error.empty();
    }

    Value serializeStructuralEntry(const RuntimeStructuralPreset& preset)
    {
        Value entry = Value::object();
        entry.Object["id"] = Value::number(static_cast<uint64_t>(preset.Id));
        entry.Object["display_name"] = Value::string(preset.Name);
        entry.Object["configuration"] = RecipeProfileSerialization::serialize(preset.Profile);
        return entry;
    }

    Value serializeFactionEntry(const RuntimeFactionPreset& preset)
    {
        Value entry = Value::object();
        entry.Object["id"] = Value::number(static_cast<uint64_t>(preset.Id));
        entry.Object["display_name"] = Value::string(preset.Name);
        entry.Object["configuration"] = RecipeProfileSerialization::serialize(preset.Profile);
        return entry;
    }

    Value serializePaletteEntry(const RuntimePalettePreset& preset)
    {
        Value entry = Value::object();
        entry.Object["id"] = Value::number(static_cast<uint64_t>(preset.Id));
        entry.Object["display_name"] = Value::string(preset.Name);
        entry.Object["configuration"] = serializePaletteConfiguration(preset.Configuration);
        return entry;
    }

    Value serializeConfigurationBundleEntry(const RuntimeConfigurationBundle& preset)
    {
        Value entry = Value::object();
        entry.Object["id"] = Value::number(static_cast<uint64_t>(preset.Id));
        entry.Object["display_name"] = Value::string(preset.Name);
        entry.Object["configuration"] = serializeConfigurationBundle(preset.Bundle);
        return entry;
    }

    bool parseCommonEntry(const Value& entry, RuntimeCustomPresetId& id, std::string& name, const Value*& configuration, std::string& error, const std::string& path)
    {
        if (entry.ValueType != Type::Object) { error = path + " must be a JSON object."; return false; }
        uint32_t parsedId = 0u;
        if (!RecipeJson::getUInt32(entry, "id", parsedId, error, path) || parsedId == 0u)
        {
            if (error.empty()) { error = path + ".id must be non-zero."; }
            return false;
        }
        if (!RecipeJson::getString(entry, "display_name", name, error, path) || name.empty())
        {
            if (error.empty()) { error = path + ".display_name must not be empty."; }
            return false;
        }
        configuration = entry.find("configuration");
        if (configuration == nullptr) { error = "Missing required field: " + path + ".configuration."; return false; }
        id = parsedId;
        return true;
    }

    template<typename Preset, typename Serializer>
    Value serializePresetArray(const std::vector<Preset>& presets, Serializer serializer)
    {
        Value array = Value::array();
        array.Array.reserve(presets.size());
        for (const Preset& preset : presets) { array.Array.push_back(serializer(preset)); }
        return array;
    }

    bool requireArray(const Value& root, const char* name, const Value*& array, std::string& error)
    {
        array = root.find(name);
        if (array == nullptr || array->ValueType != Type::Array)
        {
            error = std::string("Missing or invalid user preset library field: ") + name + ".";
            return false;
        }
        return true;
    }

    bool writeTextFile(const std::filesystem::path& path, const std::string& text, const char* label, std::string& error)
    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        if (!stream) { error = std::string("Failed to open ") + label + " file for writing: " + path.string(); return false; }
        stream << text;
        stream.flush();
        if (!stream) { error = std::string("Failed while writing ") + label + " file: " + path.string(); return false; }
        stream.close();
        if (!stream) { error = std::string("Failed while closing ") + label + " file: " + path.string(); return false; }
        return true;
    }

    void removeIfExists(const std::filesystem::path& path)
    {
        std::error_code error;
        std::filesystem::remove(path, error);
    }

    bool safeWriteTextFile(const std::filesystem::path& path, const std::string& text, const char* label, std::string& error)
    {
        error.clear();
        const std::filesystem::path temporaryPath = path.string() + ".tmp";
        const std::filesystem::path backupPath = path.string() + ".bak";
        removeIfExists(temporaryPath);
        removeIfExists(backupPath);

        if (!writeTextFile(temporaryPath, text, label, error))
        {
            removeIfExists(temporaryPath);
            return false;
        }

        std::error_code filesystemError;
        const bool destinationExists = std::filesystem::exists(path, filesystemError);
        if (filesystemError)
        {
            error = std::string("Failed to inspect existing ") + label + " file: " + path.string() + ".";
            removeIfExists(temporaryPath);
            return false;
        }

        if (destinationExists)
        {
            std::filesystem::rename(path, backupPath, filesystemError);
            if (filesystemError)
            {
                error = std::string("Failed to prepare existing ") + label + " file for replacement: " + path.string() + ".";
                removeIfExists(temporaryPath);
                removeIfExists(backupPath);
                return false;
            }
        }

        filesystemError.clear();
        std::filesystem::rename(temporaryPath, path, filesystemError);
        if (filesystemError)
        {
            error = std::string("Failed to replace ") + label + " file: " + path.string() + ".";
            removeIfExists(temporaryPath);
            if (destinationExists)
            {
                std::error_code restoreError;
                std::filesystem::rename(backupPath, path, restoreError);
                if (restoreError) { error += std::string(" Previous ") + label + " file could not be restored."; }
            }
            return false;
        }

        removeIfExists(backupPath);
        return true;
    }

    bool readTextFile(const std::filesystem::path& path, const char* label, std::string& text, std::string& error)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream) { error = std::string("Failed to open ") + label + " file: " + path.string(); return false; }
        std::ostringstream buffer;
        buffer << stream.rdbuf();
        if (!stream.good() && !stream.eof()) { error = std::string("Failed while reading ") + label + " file: " + path.string(); return false; }
        text = buffer.str();
        return true;
    }

    Value createExportRoot(UserPresetCategory category, RuntimeCustomPresetId id, const std::string& name, Value configuration)
    {
        Value root = Value::object();
        root.Object["format_version"] = Value::number(static_cast<uint64_t>(UserPresetFileFormatVersion));
        root.Object["preset_category"] = Value::string(getUserPresetCategoryId(category));
        Value metadata = Value::object();
        metadata.Object["display_name"] = Value::string(name);
        metadata.Object["source_local_id"] = Value::number(static_cast<uint64_t>(id));
        root.Object["metadata"] = std::move(metadata);
        root.Object["configuration"] = std::move(configuration);
        return root;
    }

    UserPresetImportResult importParsedPreset(RuntimeCustomPresetWorkspace& workspace, UserPresetCategory expectedCategory, const Value& root)
    {
        UserPresetImportResult result;
        if (root.ValueType != Type::Object) { result.Error = "User preset file root must be a JSON object."; return result; }

        uint32_t version = 0u;
        if (!RecipeJson::getUInt32(root, "format_version", version, result.Error, "preset")) { return result; }
        if (version != 1u && version != UserPresetFileFormatVersion)
        {
            result.Error = "Unsupported user preset file format version: " + std::to_string(version) + ".";
            return result;
        }

        std::string categoryId;
        if (!RecipeJson::getString(root, "preset_category", categoryId, result.Error, "preset") || !categoryFromId(categoryId, result.Category))
        {
            if (result.Error.empty()) { result.Error = "Unknown user preset category: " + categoryId + "."; }
            return result;
        }
        if (result.Category != expectedCategory)
        {
            result.Error = "Preset category mismatch: expected " + std::string(getUserPresetCategoryId(expectedCategory)) + ", got " + categoryId + ".";
            return result;
        }

        const Value* metadata = nullptr;
        if (!RecipeJson::getObject(root, "metadata", metadata, result.Error, "preset")) { return result; }
        std::string displayName;
        if (!RecipeJson::getString(*metadata, "display_name", displayName, result.Error, "preset.metadata") || displayName.empty())
        {
            if (result.Error.empty()) { result.Error = "preset.metadata.display_name must not be empty."; }
            return result;
        }
        uint32_t sourceLocalId = 0u;
        if (!RecipeJson::getUInt32(*metadata, "source_local_id", sourceLocalId, result.Error, "preset.metadata") || sourceLocalId == 0u)
        {
            if (result.Error.empty()) { result.Error = "preset.metadata.source_local_id must be non-zero."; }
            return result;
        }
        // The source ID is descriptive export metadata only. Imports always receive a
        // fresh stable ID owned by the destination installation.
        (void)sourceLocalId;

        const Value* configuration = root.find("configuration");
        if (configuration == nullptr) { result.Error = "Missing required field: preset.configuration."; return result; }

        const std::string requestedName = displayName;
        if (result.Category == UserPresetCategory::STRUCTURAL)
        {
            ShipGenerationProfile profile;
            if (!RecipeProfileSerialization::deserialize(*configuration, profile, result.Error, "preset.configuration") || !validateStructural(profile, result.Error)) { return result; }
            result.ImportedId = workspace.addStructural(displayName, profile);
            const RuntimeStructuralPreset* preset = workspace.findStructural(result.ImportedId);
            result.DisplayName = preset == nullptr ? displayName : preset->Name;
        }
        else if (result.Category == UserPresetCategory::FACTION)
        {
            ShipFactionProfile profile;
            if (!RecipeProfileSerialization::deserialize(*configuration, profile, result.Error, "preset.configuration") || !validateFaction(profile, result.Error)) { return result; }
            result.ImportedId = workspace.addFaction(displayName, profile);
            const RuntimeFactionPreset* preset = workspace.findFaction(result.ImportedId);
            result.DisplayName = preset == nullptr ? displayName : preset->Name;
        }
        else if (result.Category == UserPresetCategory::PALETTE)
        {
            ShipPaletteConfiguration palette;
            if (!deserializePaletteConfiguration(*configuration, palette, result.Error, "preset.configuration") || !validatePalette(palette, result.Error)) { return result; }
            result.ImportedId = workspace.addPalette(displayName, palette);
            const RuntimePalettePreset* preset = workspace.findPalette(result.ImportedId);
            result.DisplayName = preset == nullptr ? displayName : preset->Name;
        }
        else if (result.Category == UserPresetCategory::FULL_CONFIGURATION)
        {
            if (version < 2u) { result.Error = "Full configuration bundles require user preset file format version 2."; return result; }
            ConfigurationBundle bundle;
            if (!deserializeConfigurationBundle(*configuration, bundle, result.Error, "preset.configuration") || !validateBundle(bundle, result.Error)) { return result; }
            result.ImportedId = workspace.addConfigurationBundle(displayName, bundle);
            const RuntimeConfigurationBundle* preset = workspace.findConfigurationBundle(result.ImportedId);
            result.DisplayName = preset == nullptr ? displayName : preset->Name;
        }
        else
        {
            result.Error = "Unsupported user preset category.";
            return result;
        }

        result.DisplayNameDisambiguated = result.DisplayName != requestedName;
        result.Success = true;
        return result;
    }
}

namespace PixelShipGeneratorPreview
{
    const char* getUserPresetCategoryId(UserPresetCategory category)
    {
        switch (category)
        {
        case UserPresetCategory::STRUCTURAL: return "STRUCTURAL";
        case UserPresetCategory::FACTION: return "FACTION";
        case UserPresetCategory::PALETTE: return "PALETTE";
        case UserPresetCategory::FULL_CONFIGURATION: return "FULL_CONFIGURATION";
        default: return "INVALID";
        }
    }

    std::string serializeUserPresetLibrary(const RuntimeCustomPresetWorkspace& workspace)
    {
        Value root = Value::object();
        root.Object["format_version"] = Value::number(static_cast<uint64_t>(UserPresetLibraryFormatVersion));
        root.Object["next_id"] = Value::number(static_cast<uint64_t>(workspace.getNextId()));
        root.Object["structural_presets"] = serializePresetArray(workspace.getStructuralPresets(), serializeStructuralEntry);
        root.Object["faction_presets"] = serializePresetArray(workspace.getFactionPresets(), serializeFactionEntry);
        root.Object["palette_presets"] = serializePresetArray(workspace.getPalettePresets(), serializePaletteEntry);
        root.Object["configuration_bundles"] = serializePresetArray(workspace.getConfigurationBundles(), serializeConfigurationBundleEntry);
        return RecipeJson::stringify(root);
    }

    UserPresetLibraryLoadResult deserializeUserPresetLibrary(const std::string& jsonText)
    {
        UserPresetLibraryLoadResult result;
        const RecipeJson::ParseResult parsed = RecipeJson::parse(jsonText);
        if (!parsed.Success) { result.Error = "Failed to parse user preset library JSON: " + parsed.Error; return result; }
        if (parsed.Root.ValueType != Type::Object) { result.Error = "User preset library root must be a JSON object."; return result; }

        uint32_t version = 0u;
        if (!RecipeJson::getUInt32(parsed.Root, "format_version", version, result.Error, "library")) { return result; }
        if (version != 1u && version != UserPresetLibraryFormatVersion)
        {
            result.Error = "Unsupported user preset library format version: " + std::to_string(version) + ".";
            return result;
        }

        uint32_t nextId = 0u;
        if (!RecipeJson::getUInt32(parsed.Root, "next_id", nextId, result.Error, "library") || nextId == 0u)
        {
            if (result.Error.empty()) { result.Error = "library.next_id must be non-zero."; }
            return result;
        }

        const Value* structural = nullptr;
        const Value* factions = nullptr;
        const Value* palettes = nullptr;
        const Value* bundles = nullptr;
        if (!requireArray(parsed.Root, "structural_presets", structural, result.Error) ||
            !requireArray(parsed.Root, "faction_presets", factions, result.Error) ||
            !requireArray(parsed.Root, "palette_presets", palettes, result.Error)) { return result; }
        if (version >= 2u && !requireArray(parsed.Root, "configuration_bundles", bundles, result.Error)) { return result; }

        for (std::size_t index = 0u; index < structural->Array.size(); ++index)
        {
            RuntimeCustomPresetId id = 0u;
            std::string name;
            const Value* configuration = nullptr;
            std::string entryError;
            ShipGenerationProfile profile;
            const std::string path = "library.structural_presets[" + std::to_string(index) + "]";
            if (!parseCommonEntry(structural->Array[index], id, name, configuration, entryError, path) ||
                !RecipeProfileSerialization::deserialize(*configuration, profile, entryError, path + ".configuration") ||
                !validateStructural(profile, entryError) ||
                !result.Workspace.restoreStructural(id, name, profile))
            {
                ++result.SkippedEntryCount;
            }
        }

        for (std::size_t index = 0u; index < factions->Array.size(); ++index)
        {
            RuntimeCustomPresetId id = 0u;
            std::string name;
            const Value* configuration = nullptr;
            std::string entryError;
            ShipFactionProfile profile;
            const std::string path = "library.faction_presets[" + std::to_string(index) + "]";
            if (!parseCommonEntry(factions->Array[index], id, name, configuration, entryError, path) ||
                !RecipeProfileSerialization::deserialize(*configuration, profile, entryError, path + ".configuration") ||
                !validateFaction(profile, entryError) ||
                !result.Workspace.restoreFaction(id, name, profile))
            {
                ++result.SkippedEntryCount;
            }
        }

        for (std::size_t index = 0u; index < palettes->Array.size(); ++index)
        {
            RuntimeCustomPresetId id = 0u;
            std::string name;
            const Value* configuration = nullptr;
            std::string entryError;
            ShipPaletteConfiguration palette;
            const std::string path = "library.palette_presets[" + std::to_string(index) + "]";
            if (!parseCommonEntry(palettes->Array[index], id, name, configuration, entryError, path) ||
                !deserializePaletteConfiguration(*configuration, palette, entryError, path + ".configuration") ||
                !validatePalette(palette, entryError) ||
                !result.Workspace.restorePalette(id, name, palette))
            {
                ++result.SkippedEntryCount;
            }
        }

        if (bundles != nullptr)
        {
            for (std::size_t index = 0u; index < bundles->Array.size(); ++index)
            {
                RuntimeCustomPresetId id = 0u;
                std::string name;
                const Value* configuration = nullptr;
                std::string entryError;
                ConfigurationBundle bundle;
                const std::string path = "library.configuration_bundles[" + std::to_string(index) + "]";
                if (!parseCommonEntry(bundles->Array[index], id, name, configuration, entryError, path) ||
                    !deserializeConfigurationBundle(*configuration, bundle, entryError, path + ".configuration") ||
                    !validateBundle(bundle, entryError) ||
                    !result.Workspace.restoreConfigurationBundle(id, name, bundle))
                {
                    ++result.SkippedEntryCount;
                }
            }
        }

        result.Workspace.ensureNextIdAtLeast(nextId);
        result.Success = true;
        return result;
    }

    bool saveUserPresetLibrary(const RuntimeCustomPresetWorkspace& workspace, const std::filesystem::path& path, std::string& error)
    {
        try
        {
            return safeWriteTextFile(path, serializeUserPresetLibrary(workspace), "user preset library", error);
        }
        catch (const std::exception& exception)
        {
            error = std::string("Failed to serialize user preset library: ") + exception.what();
            return false;
        }
    }

    UserPresetLibraryLoadResult loadUserPresetLibrary(const std::filesystem::path& path)
    {
        std::error_code filesystemError;
        if (!std::filesystem::exists(path, filesystemError))
        {
            UserPresetLibraryLoadResult result;
            if (filesystemError) { result.Error = "Failed to inspect user preset library file: " + path.string() + "."; return result; }
            result.Success = true;
            return result;
        }

        std::string text;
        std::string error;
        if (!readTextFile(path, "user preset library", text, error))
        {
            UserPresetLibraryLoadResult result;
            result.Error = std::move(error);
            return result;
        }
        return deserializeUserPresetLibrary(text);
    }

    std::string serializeUserPresetFile(const RuntimeStructuralPreset& preset)
    {
        return RecipeJson::stringify(createExportRoot(UserPresetCategory::STRUCTURAL, preset.Id, preset.Name, RecipeProfileSerialization::serialize(preset.Profile)));
    }

    std::string serializeUserPresetFile(const RuntimeFactionPreset& preset)
    {
        return RecipeJson::stringify(createExportRoot(UserPresetCategory::FACTION, preset.Id, preset.Name, RecipeProfileSerialization::serialize(preset.Profile)));
    }

    std::string serializeUserPresetFile(const RuntimePalettePreset& preset)
    {
        return RecipeJson::stringify(createExportRoot(UserPresetCategory::PALETTE, preset.Id, preset.Name, serializePaletteConfiguration(preset.Configuration)));
    }

    std::string serializeUserPresetFile(const RuntimeConfigurationBundle& preset)
    {
        return RecipeJson::stringify(createExportRoot(UserPresetCategory::FULL_CONFIGURATION, preset.Id, preset.Name, serializeConfigurationBundle(preset.Bundle)));
    }

    bool exportUserPreset(const RuntimeCustomPresetWorkspace& workspace, UserPresetCategory category, RuntimeCustomPresetId id, const std::filesystem::path& path, std::string& error)
    {
        try
        {
            std::string serialized;
            if (category == UserPresetCategory::STRUCTURAL)
            {
                const RuntimeStructuralPreset* preset = workspace.findStructural(id);
                if (preset == nullptr) { error = "Structural user preset was not found."; return false; }
                serialized = serializeUserPresetFile(*preset);
            }
            else if (category == UserPresetCategory::FACTION)
            {
                const RuntimeFactionPreset* preset = workspace.findFaction(id);
                if (preset == nullptr) { error = "Faction user preset was not found."; return false; }
                serialized = serializeUserPresetFile(*preset);
            }
            else if (category == UserPresetCategory::PALETTE)
            {
                const RuntimePalettePreset* preset = workspace.findPalette(id);
                if (preset == nullptr) { error = "Palette user preset was not found."; return false; }
                serialized = serializeUserPresetFile(*preset);
            }
            else if (category == UserPresetCategory::FULL_CONFIGURATION)
            {
                const RuntimeConfigurationBundle* preset = workspace.findConfigurationBundle(id);
                if (preset == nullptr) { error = "Full configuration bundle was not found."; return false; }
                serialized = serializeUserPresetFile(*preset);
            }
            else
            {
                error = "Unsupported user preset category.";
                return false;
            }
            return safeWriteTextFile(path, serialized, "user preset export", error);
        }
        catch (const std::exception& exception)
        {
            error = std::string("Failed to serialize user preset export: ") + exception.what();
            return false;
        }
    }

    UserPresetImportResult importUserPreset(RuntimeCustomPresetWorkspace& workspace, UserPresetCategory expectedCategory, const std::string& jsonText)
    {
        const RecipeJson::ParseResult parsed = RecipeJson::parse(jsonText);
        if (!parsed.Success)
        {
            UserPresetImportResult result;
            result.Error = "Failed to parse user preset JSON: " + parsed.Error;
            return result;
        }
        try
        {
            return importParsedPreset(workspace, expectedCategory, parsed.Root);
        }
        catch (const std::exception& exception)
        {
            UserPresetImportResult result;
            result.Error = std::string("Failed to import user preset: ") + exception.what();
            return result;
        }
    }

    UserPresetImportResult importUserPreset(RuntimeCustomPresetWorkspace& workspace, UserPresetCategory expectedCategory, const std::filesystem::path& path)
    {
        std::string text;
        UserPresetImportResult result;
        if (!readTextFile(path, "user preset import", text, result.Error)) { return result; }
        return importUserPreset(workspace, expectedCategory, text);
    }
}
