#include "StructuralProfileSelection.h"

#include <algorithm>

#include <SpectralShipGen/BuiltInPresetCatalog.h>

namespace SpectralShipGenStudioPreview
{
    std::vector<StructuralProfileSelectionEntry> buildStructuralProfileSelection(const RuntimeCustomPresetWorkspace& workspace)
    {
        std::vector<StructuralProfileSelectionEntry> entries;
        entries.reserve(SpectralShipGen::getBuiltInStructuralPresetCatalog().size() + workspace.getStructuralPresets().size() + 1u);
        for (const SpectralShipGen::BuiltInStructuralPreset& preset : SpectralShipGen::getBuiltInStructuralPresetCatalog())
        {
            entries.push_back({ StructuralProfileSelectionKind::BUILT_IN, preset.StableId, preset.Preset, 0u });
        }
        for (const RuntimeStructuralPreset& preset : workspace.getStructuralPresets())
        {
            entries.push_back({ StructuralProfileSelectionKind::RUNTIME_CUSTOM, preset.Name, SpectralShipGen::ShipStyle::SHIP_STYLE_END, preset.Id });
        }
        entries.push_back({ StructuralProfileSelectionKind::ADD_PROFILE, "+ ADD PROFILE", SpectralShipGen::ShipStyle::SHIP_STYLE_END, 0u });
        return entries;
    }

    std::size_t findStructuralProfileSelectionIndex(const std::vector<StructuralProfileSelectionEntry>& entries, const SpectralShipGen::ShipGenerationRecipe& recipe, std::optional<RuntimeCustomPresetId> activeCustomPresetId)
    {
        if (recipe.StructuralSource == SpectralShipGen::ShipGenerationRecipeProfileSource::BUILT_IN_PRESET)
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
        if (recipe.StructuralSource == SpectralShipGen::ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM && customIterator != entries.end())
        {
            return static_cast<std::size_t>(std::distance(entries.begin(), customIterator));
        }
        return 0u;
    }
}
