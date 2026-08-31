#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipPaletteConfiguration.h>

#include "ConfigurationBundle.h"

namespace SpectralShipGenStudioPreview
{
    using RuntimeCustomPresetId = uint32_t;

    struct RuntimeStructuralPreset
    {
        RuntimeCustomPresetId Id = 0u;
        std::string Name;
        SpectralShipGen::ShipGenerationProfile Profile;
    };

    struct RuntimeFactionPreset
    {
        RuntimeCustomPresetId Id = 0u;
        std::string Name;
        SpectralShipGen::ShipFactionProfile Profile;
    };

    struct RuntimePalettePreset
    {
        RuntimeCustomPresetId Id = 0u;
        std::string Name;
        SpectralShipGen::ShipPaletteConfiguration Configuration;
    };

    struct RuntimeConfigurationBundle
    {
        RuntimeCustomPresetId Id = 0u;
        std::string Name;
        ConfigurationBundle Bundle;
    };

    // Preview-side owner for editable user presets. Built-ins remain in Core; this
    // workspace owns only application-local custom entries and their stable local IDs.
    class RuntimeCustomPresetWorkspace
    {
    public:
        RuntimeCustomPresetId addStructural(std::string name, const SpectralShipGen::ShipGenerationProfile& profile);
        RuntimeCustomPresetId addFaction(std::string name, const SpectralShipGen::ShipFactionProfile& profile);
        RuntimeCustomPresetId addPalette(std::string name, const SpectralShipGen::ShipPaletteConfiguration& configuration);
        RuntimeCustomPresetId addConfigurationBundle(std::string name, const ConfigurationBundle& bundle);

        // Persistence restore path. IDs are retained across restarts and remain globally
        // unique across all application-side user preset categories.
        bool restoreStructural(RuntimeCustomPresetId id, std::string name, const SpectralShipGen::ShipGenerationProfile& profile);
        bool restoreFaction(RuntimeCustomPresetId id, std::string name, const SpectralShipGen::ShipFactionProfile& profile);
        bool restorePalette(RuntimeCustomPresetId id, std::string name, const SpectralShipGen::ShipPaletteConfiguration& configuration);
        bool restoreConfigurationBundle(RuntimeCustomPresetId id, std::string name, const ConfigurationBundle& bundle);

        bool updateStructural(RuntimeCustomPresetId id, std::string name, const SpectralShipGen::ShipGenerationProfile& profile);
        bool updateFaction(RuntimeCustomPresetId id, std::string name, const SpectralShipGen::ShipFactionProfile& profile);
        bool updatePalette(RuntimeCustomPresetId id, std::string name, const SpectralShipGen::ShipPaletteConfiguration& configuration);
        bool updateConfigurationBundle(RuntimeCustomPresetId id, std::string name, const ConfigurationBundle& bundle);
        bool removeStructural(RuntimeCustomPresetId id);
        bool removeFaction(RuntimeCustomPresetId id);
        bool removePalette(RuntimeCustomPresetId id);
        bool removeConfigurationBundle(RuntimeCustomPresetId id);

        std::optional<RuntimeCustomPresetId> duplicateStructural(RuntimeCustomPresetId id);
        std::optional<RuntimeCustomPresetId> duplicateFaction(RuntimeCustomPresetId id);
        std::optional<RuntimeCustomPresetId> duplicatePalette(RuntimeCustomPresetId id);
        std::optional<RuntimeCustomPresetId> duplicateConfigurationBundle(RuntimeCustomPresetId id);

        RuntimeStructuralPreset* findStructural(RuntimeCustomPresetId id);
        RuntimeFactionPreset* findFaction(RuntimeCustomPresetId id);
        RuntimePalettePreset* findPalette(RuntimeCustomPresetId id);
        RuntimeConfigurationBundle* findConfigurationBundle(RuntimeCustomPresetId id);
        const RuntimeStructuralPreset* findStructural(RuntimeCustomPresetId id) const;
        const RuntimeFactionPreset* findFaction(RuntimeCustomPresetId id) const;
        const RuntimePalettePreset* findPalette(RuntimeCustomPresetId id) const;
        const RuntimeConfigurationBundle* findConfigurationBundle(RuntimeCustomPresetId id) const;

        const std::vector<RuntimeStructuralPreset>& getStructuralPresets() const;
        const std::vector<RuntimeFactionPreset>& getFactionPresets() const;
        const std::vector<RuntimePalettePreset>& getPalettePresets() const;
        const std::vector<RuntimeConfigurationBundle>& getConfigurationBundles() const;

        RuntimeCustomPresetId getNextId() const;
        void ensureNextIdAtLeast(RuntimeCustomPresetId nextId);

    private:
        std::string makeUniqueStructuralName(const std::string& base) const;
        std::string makeUniqueFactionName(const std::string& base) const;
        std::string makeUniquePaletteName(const std::string& base) const;
        std::string makeUniqueConfigurationBundleName(const std::string& base) const;
        bool containsId(RuntimeCustomPresetId id) const;
        RuntimeCustomPresetId allocateId();

    private:
        RuntimeCustomPresetId m_NextId = 1u;
        std::vector<RuntimeStructuralPreset> m_StructuralPresets;
        std::vector<RuntimeFactionPreset> m_FactionPresets;
        std::vector<RuntimePalettePreset> m_PalettePresets;
        std::vector<RuntimeConfigurationBundle> m_ConfigurationBundles;
    };
}
