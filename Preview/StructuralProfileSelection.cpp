#include "StructuralProfileSelection.h"

#include <algorithm>

#include "BuiltInPresetCatalog.h"

namespace PixelShipGeneratorPreview
{
    std::vector<StructuralProfileSelectionEntry> buildStructuralProfileSelection(const RuntimeCustomPresetWorkspace& workspace)
    {
        std::vector<StructuralProfileSelectionEntry> entries;
        entries.reserve(PixelShipGenerator::getBuiltInStructuralPresetCatalog().size() + workspace.getStructuralPresets().size() + 1u);
        for (const PixelShipGenerator::BuiltInStructuralPreset& preset : PixelShipGenerator::getBuiltInStructuralPresetCatalog())
        {
            entries.push_back({ StructuralProfileSelectionKind::BUILT_IN, preset.StableId, preset.Preset, 0u });
        }
        for (const RuntimeStructuralPreset& preset : workspace.getStructuralPresets())
        {
            entries.push_back({ StructuralProfileSelectionKind::RUNTIME_CUSTOM, preset.Name, PixelShipGenerator::ShipStyle::SHIP_STYLE_END, preset.Id });
        }
        entries.push_back({ StructuralProfileSelectionKind::ADD_PROFILE, "+ ADD PROFILE", PixelShipGenerator::ShipStyle::SHIP_STYLE_END, 0u });
        return entries;
    }

    std::size_t findStructuralProfileSelectionIndex(const std::vector<StructuralProfileSelectionEntry>& entries, const PixelShipGenerator::ShipGenerationRecipe& recipe, std::optional<RuntimeCustomPresetId> activeCustomPresetId)
    {
        if (recipe.StructuralSource == PixelShipGenerator::ShipGenerationRecipeProfileSource::BUILT_IN_PRESET)
        {
            const auto iterator = std::find_if(entries.begin(), entries.end(), [&](const StructuralProfileSelectionEntry& entry)
            {
                return entry.Kind == StructuralProfileSelectionKind::BUILT_IN && entry.Style == recipe.Style;
            });
            if (iterator != entries.end()) { return static_cast<std::size_t>(std::distance(entries.begin(), iterator)); }
        }
        else if (activeCustomPresetId.has_value())
        {
            const auto iterator = std::find_if(entries.begin(), entries.end(), [&](const StructuralProfileSelectionEntry& entry)
            {
                return entry.Kind == StructuralProfileSelectionKind::RUNTIME_CUSTOM && entry.CustomPresetId == *activeCustomPresetId;
            });
            if (iterator != entries.end()) { return static_cast<std::size_t>(std::distance(entries.begin(), iterator)); }
        }

        const auto customIterator = std::find_if(entries.begin(), entries.end(), [](const StructuralProfileSelectionEntry& entry)
        {
            return entry.Kind == StructuralProfileSelectionKind::RUNTIME_CUSTOM;
        });
        if (recipe.StructuralSource == PixelShipGenerator::ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM && customIterator != entries.end())
        {
            return static_cast<std::size_t>(std::distance(entries.begin(), customIterator));
        }
        return 0u;
    }
}
