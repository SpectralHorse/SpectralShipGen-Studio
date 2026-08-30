#include "RuntimeCustomPresetWorkspace.h"

#include <algorithm>
#include <utility>

namespace PixelShipGeneratorPreview
{
    namespace
    {
        template<typename Preset>
        Preset* findPreset(std::vector<Preset>& presets, RuntimeCustomPresetId id)
        {
            const auto iterator = std::find_if(presets.begin(), presets.end(), [id](const Preset& preset) { return preset.Id == id; });
            return iterator == presets.end() ? nullptr : &*iterator;
        }

        template<typename Preset>
        const Preset* findPreset(const std::vector<Preset>& presets, RuntimeCustomPresetId id)
        {
            const auto iterator = std::find_if(presets.begin(), presets.end(), [id](const Preset& preset) { return preset.Id == id; });
            return iterator == presets.end() ? nullptr : &*iterator;
        }

        template<typename Preset>
        bool removePreset(std::vector<Preset>& presets, RuntimeCustomPresetId id)
        {
            const auto iterator = std::remove_if(presets.begin(), presets.end(), [id](const Preset& preset) { return preset.Id == id; });
            if (iterator == presets.end()) { return false; }
            presets.erase(iterator, presets.end());
            return true;
        }

        template<typename Preset>
        std::string makeUniqueName(const std::vector<Preset>& presets, const std::string& base)
        {
            const std::string root = base.empty() ? "Custom Preset" : base;
            const auto exists = [&](const std::string& name)
            {
                return std::any_of(presets.begin(), presets.end(), [&](const Preset& preset) { return preset.Name == name; });
            };
            if (!exists(root)) { return root; }
            const std::string firstCopy = root + " Copy";
            if (!exists(firstCopy)) { return firstCopy; }
            for (uint32_t suffix = 2u;; ++suffix)
            {
                const std::string candidate = root + " Copy " + std::to_string(suffix);
                if (!exists(candidate)) { return candidate; }
            }
        }
    }

    RuntimeCustomPresetId RuntimeCustomPresetWorkspace::addStructural(std::string name, const PixelShipGenerator::ShipGenerationProfile& profile)
    {
        const RuntimeCustomPresetId id = allocateId();
        m_StructuralPresets.push_back({ id, makeUniqueStructuralName(name), profile });
        return id;
    }

    RuntimeCustomPresetId RuntimeCustomPresetWorkspace::addFaction(std::string name, const PixelShipGenerator::ShipFactionProfile& profile)
    {
        const RuntimeCustomPresetId id = allocateId();
        m_FactionPresets.push_back({ id, makeUniqueFactionName(name), profile });
        return id;
    }

    RuntimeCustomPresetId RuntimeCustomPresetWorkspace::addPalette(std::string name, const PixelShipGenerator::ShipPaletteConfiguration& configuration)
    {
        const RuntimeCustomPresetId id = allocateId();
        m_PalettePresets.push_back({ id, makeUniquePaletteName(name), configuration });
        return id;
    }

    bool RuntimeCustomPresetWorkspace::updateStructural(RuntimeCustomPresetId id, std::string name, const PixelShipGenerator::ShipGenerationProfile& profile)
    {
        RuntimeStructuralPreset* preset = findStructural(id);
        if (preset == nullptr) { return false; }
        const std::string requested = name.empty() ? preset->Name : std::move(name);
        std::string unique = requested;
        if (unique != preset->Name)
        {
            const std::string original = preset->Name;
            preset->Name.clear();
            unique = makeUniqueStructuralName(requested);
            preset->Name = original;
        }
        preset->Name = std::move(unique);
        preset->Profile = profile;
        return true;
    }

    bool RuntimeCustomPresetWorkspace::updateFaction(RuntimeCustomPresetId id, std::string name, const PixelShipGenerator::ShipFactionProfile& profile)
    {
        RuntimeFactionPreset* preset = findFaction(id);
        if (preset == nullptr) { return false; }
        const std::string requested = name.empty() ? preset->Name : std::move(name);
        std::string unique = requested;
        if (unique != preset->Name)
        {
            const std::string original = preset->Name;
            preset->Name.clear();
            unique = makeUniqueFactionName(requested);
            preset->Name = original;
        }
        preset->Name = std::move(unique);
        preset->Profile = profile;
        return true;
    }

    bool RuntimeCustomPresetWorkspace::removeStructural(RuntimeCustomPresetId id) { return removePreset(m_StructuralPresets, id); }
    bool RuntimeCustomPresetWorkspace::removeFaction(RuntimeCustomPresetId id) { return removePreset(m_FactionPresets, id); }
    bool RuntimeCustomPresetWorkspace::removePalette(RuntimeCustomPresetId id) { return removePreset(m_PalettePresets, id); }

    std::optional<RuntimeCustomPresetId> RuntimeCustomPresetWorkspace::duplicateStructural(RuntimeCustomPresetId id)
    {
        const RuntimeStructuralPreset* preset = findStructural(id);
        if (preset == nullptr) { return std::nullopt; }
        const RuntimeStructuralPreset copy = *preset;
        return addStructural(copy.Name, copy.Profile);
    }

    std::optional<RuntimeCustomPresetId> RuntimeCustomPresetWorkspace::duplicateFaction(RuntimeCustomPresetId id)
    {
        const RuntimeFactionPreset* preset = findFaction(id);
        if (preset == nullptr) { return std::nullopt; }
        const RuntimeFactionPreset copy = *preset;
        return addFaction(copy.Name, copy.Profile);
    }

    std::optional<RuntimeCustomPresetId> RuntimeCustomPresetWorkspace::duplicatePalette(RuntimeCustomPresetId id)
    {
        const RuntimePalettePreset* preset = findPalette(id);
        if (preset == nullptr) { return std::nullopt; }
        const RuntimePalettePreset copy = *preset;
        return addPalette(copy.Name, copy.Configuration);
    }

    RuntimeStructuralPreset* RuntimeCustomPresetWorkspace::findStructural(RuntimeCustomPresetId id) { return findPreset(m_StructuralPresets, id); }
    RuntimeFactionPreset* RuntimeCustomPresetWorkspace::findFaction(RuntimeCustomPresetId id) { return findPreset(m_FactionPresets, id); }
    RuntimePalettePreset* RuntimeCustomPresetWorkspace::findPalette(RuntimeCustomPresetId id) { return findPreset(m_PalettePresets, id); }
    const RuntimeStructuralPreset* RuntimeCustomPresetWorkspace::findStructural(RuntimeCustomPresetId id) const { return findPreset(m_StructuralPresets, id); }
    const RuntimeFactionPreset* RuntimeCustomPresetWorkspace::findFaction(RuntimeCustomPresetId id) const { return findPreset(m_FactionPresets, id); }
    const RuntimePalettePreset* RuntimeCustomPresetWorkspace::findPalette(RuntimeCustomPresetId id) const { return findPreset(m_PalettePresets, id); }

    const std::vector<RuntimeStructuralPreset>& RuntimeCustomPresetWorkspace::getStructuralPresets() const { return m_StructuralPresets; }
    const std::vector<RuntimeFactionPreset>& RuntimeCustomPresetWorkspace::getFactionPresets() const { return m_FactionPresets; }
    const std::vector<RuntimePalettePreset>& RuntimeCustomPresetWorkspace::getPalettePresets() const { return m_PalettePresets; }

    std::string RuntimeCustomPresetWorkspace::makeUniqueStructuralName(const std::string& base) const { return makeUniqueName(m_StructuralPresets, base); }
    std::string RuntimeCustomPresetWorkspace::makeUniqueFactionName(const std::string& base) const { return makeUniqueName(m_FactionPresets, base); }
    std::string RuntimeCustomPresetWorkspace::makeUniquePaletteName(const std::string& base) const { return makeUniqueName(m_PalettePresets, base); }

    RuntimeCustomPresetId RuntimeCustomPresetWorkspace::allocateId()
    {
        return m_NextId++;
    }
}
