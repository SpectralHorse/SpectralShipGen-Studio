#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "ShipFactionProfile.h"
#include "ShipGenerationProfile.h"
#include "ShipPaletteConfiguration.h"

namespace PixelShipGeneratorPreview
{
    using RuntimeCustomPresetId = uint32_t;

    struct RuntimeStructuralPreset
    {
        RuntimeCustomPresetId Id = 0u;
        std::string Name;
        PixelShipGenerator::ShipGenerationProfile Profile;
    };

    struct RuntimeFactionPreset
    {
        RuntimeCustomPresetId Id = 0u;
        std::string Name;
        PixelShipGenerator::ShipFactionProfile Profile;
    };

    struct RuntimePalettePreset
    {
        RuntimeCustomPresetId Id = 0u;
        std::string Name;
        PixelShipGenerator::ShipPaletteConfiguration Configuration;
    };

    // Session-only authoring workspace used by the Preview editors. Persistence,
    // import and export of reusable user presets belong to the later preset-library task.
    class RuntimeCustomPresetWorkspace
    {
    public:
        RuntimeCustomPresetId addStructural(std::string name, const PixelShipGenerator::ShipGenerationProfile& profile);
        RuntimeCustomPresetId addFaction(std::string name, const PixelShipGenerator::ShipFactionProfile& profile);
        RuntimeCustomPresetId addPalette(std::string name, const PixelShipGenerator::ShipPaletteConfiguration& configuration);

        bool updateStructural(RuntimeCustomPresetId id, std::string name, const PixelShipGenerator::ShipGenerationProfile& profile);
        bool updateFaction(RuntimeCustomPresetId id, std::string name, const PixelShipGenerator::ShipFactionProfile& profile);
        bool updatePalette(RuntimeCustomPresetId id, std::string name, const PixelShipGenerator::ShipPaletteConfiguration& configuration);
        bool removeStructural(RuntimeCustomPresetId id);
        bool removeFaction(RuntimeCustomPresetId id);
        bool removePalette(RuntimeCustomPresetId id);

        std::optional<RuntimeCustomPresetId> duplicateStructural(RuntimeCustomPresetId id);
        std::optional<RuntimeCustomPresetId> duplicateFaction(RuntimeCustomPresetId id);
        std::optional<RuntimeCustomPresetId> duplicatePalette(RuntimeCustomPresetId id);

        RuntimeStructuralPreset* findStructural(RuntimeCustomPresetId id);
        RuntimeFactionPreset* findFaction(RuntimeCustomPresetId id);
        RuntimePalettePreset* findPalette(RuntimeCustomPresetId id);
        const RuntimeStructuralPreset* findStructural(RuntimeCustomPresetId id) const;
        const RuntimeFactionPreset* findFaction(RuntimeCustomPresetId id) const;
        const RuntimePalettePreset* findPalette(RuntimeCustomPresetId id) const;

        const std::vector<RuntimeStructuralPreset>& getStructuralPresets() const;
        const std::vector<RuntimeFactionPreset>& getFactionPresets() const;
        const std::vector<RuntimePalettePreset>& getPalettePresets() const;

    private:
        std::string makeUniqueStructuralName(const std::string& base) const;
        std::string makeUniqueFactionName(const std::string& base) const;
        std::string makeUniquePaletteName(const std::string& base) const;
        RuntimeCustomPresetId allocateId();

    private:
        RuntimeCustomPresetId m_NextId = 1u;
        std::vector<RuntimeStructuralPreset> m_StructuralPresets;
        std::vector<RuntimeFactionPreset> m_FactionPresets;
        std::vector<RuntimePalettePreset> m_PalettePresets;
    };
}
