#include "FactionProfileSelection.h"

#include <algorithm>

#include "BuiltInPresetCatalog.h"

namespace PixelShipGeneratorPreview
{
    std::vector<FactionProfileSelectionEntry> buildFactionProfileSelection(const RuntimeCustomPresetWorkspace& workspace)
    {
        std::vector<FactionProfileSelectionEntry> entries;
        entries.reserve(PixelShipGenerator::getBuiltInFactionPresetCatalog().size() + workspace.getFactionPresets().size() + 1u);
        for (const PixelShipGenerator::BuiltInFactionPreset& preset : PixelShipGenerator::getBuiltInFactionPresetCatalog())
        {
            entries.push_back({ FactionProfileSelectionKind::BUILT_IN, preset.StableId, preset.Preset, 0u });
        }
        for (const RuntimeFactionPreset& preset : workspace.getFactionPresets())
        {
            entries.push_back({ FactionProfileSelectionKind::RUNTIME_CUSTOM, preset.Name, PixelShipGenerator::ShipFactionType::SHIP_FACTION_TYPE_END, preset.Id });
        }
        entries.push_back({ FactionProfileSelectionKind::ADD_FACTION, "+ ADD FACTION", PixelShipGenerator::ShipFactionType::SHIP_FACTION_TYPE_END, 0u });
        return entries;
    }

    std::size_t findFactionProfileSelectionIndex(const std::vector<FactionProfileSelectionEntry>& entries, const PixelShipGenerator::ShipGenerationRecipe& recipe, std::optional<RuntimeCustomPresetId> activeCustomPresetId)
    {
        if (recipe.FactionSource == PixelShipGenerator::ShipGenerationRecipeProfileSource::BUILT_IN_PRESET)
        {
            const auto iterator = std::find_if(entries.begin(), entries.end(), [&](const FactionProfileSelectionEntry& entry)
            {
                return entry.Kind == FactionProfileSelectionKind::BUILT_IN && entry.Faction == recipe.Faction;
            });
            if (iterator != entries.end()) { return static_cast<std::size_t>(std::distance(entries.begin(), iterator)); }
        }
        else if (activeCustomPresetId.has_value())
        {
            const auto iterator = std::find_if(entries.begin(), entries.end(), [&](const FactionProfileSelectionEntry& entry)
            {
                return entry.Kind == FactionProfileSelectionKind::RUNTIME_CUSTOM && entry.CustomPresetId == *activeCustomPresetId;
            });
            if (iterator != entries.end()) { return static_cast<std::size_t>(std::distance(entries.begin(), iterator)); }
        }

        const auto customIterator = std::find_if(entries.begin(), entries.end(), [](const FactionProfileSelectionEntry& entry)
        {
            return entry.Kind == FactionProfileSelectionKind::RUNTIME_CUSTOM;
        });
        if (recipe.FactionSource == PixelShipGenerator::ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM && customIterator != entries.end())
        {
            return static_cast<std::size_t>(std::distance(entries.begin(), customIterator));
        }
        return 0u;
    }
}
