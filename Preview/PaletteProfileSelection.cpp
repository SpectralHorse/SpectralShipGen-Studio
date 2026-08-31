#include "PaletteProfileSelection.h"

#include <algorithm>

#include <SpectralShipGen/BuiltInPresetCatalog.h>

namespace SpectralShipGenStudioPreview
{
    std::vector<PaletteProfileSelectionEntry> buildPaletteProfileSelection(const RuntimeCustomPresetWorkspace& workspace)
    {
        std::vector<PaletteProfileSelectionEntry> entries;
        entries.reserve(1u + SpectralShipGen::getBuiltInPalettePresetCatalog().size() + workspace.getPalettePresets().size() + 1u);
        entries.push_back({ PaletteProfileSelectionKind::FACTION_DEFAULT, "FACTION DEFAULT", std::nullopt, 0u });
        for (const SpectralShipGen::BuiltInPalettePreset& preset : SpectralShipGen::getBuiltInPalettePresetCatalog())
        {
            entries.push_back({ PaletteProfileSelectionKind::BUILT_IN_GENERATED, preset.StableId, preset.FactionPreset, 0u });
        }
        for (const RuntimePalettePreset& preset : workspace.getPalettePresets())
        {
            entries.push_back({ PaletteProfileSelectionKind::RUNTIME_CUSTOM, preset.Name, std::nullopt, preset.Id });
        }
        entries.push_back({ PaletteProfileSelectionKind::ADD_PALETTE, "+ ADD PALETTE", std::nullopt, 0u });
        return entries;
    }

    std::size_t findPaletteProfileSelectionIndex(const std::vector<PaletteProfileSelectionEntry>& entries,
        const SpectralShipGen::ShipGenerationRecipe& recipe,
        std::optional<SpectralShipGen::ShipFactionType> activeBuiltInPalette,
        std::optional<RuntimeCustomPresetId> activeCustomPresetId)
    {
        if (recipe.PaletteConfiguration.Mode == SpectralShipGen::ShipPaletteSourceMode::FACTION_PROFILE_GENERATED)
        {
            const auto iterator = std::find_if(entries.begin(), entries.end(), [](const PaletteProfileSelectionEntry& entry)
                {
                    return entry.Kind == PaletteProfileSelectionKind::FACTION_DEFAULT;
                });
            if (iterator != entries.end()) { return static_cast<std::size_t>(std::distance(entries.begin(), iterator)); }
        }
        if (activeBuiltInPalette.has_value())
        {
            const auto iterator = std::find_if(entries.begin(), entries.end(), [&](const PaletteProfileSelectionEntry& entry)
                {
                    return entry.Kind == PaletteProfileSelectionKind::BUILT_IN_GENERATED && entry.PalettePreset == activeBuiltInPalette;
                });
            if (iterator != entries.end()) { return static_cast<std::size_t>(std::distance(entries.begin(), iterator)); }
        }
        if (activeCustomPresetId.has_value())
        {
            const auto iterator = std::find_if(entries.begin(), entries.end(), [&](const PaletteProfileSelectionEntry& entry)
                {
                    return entry.Kind == PaletteProfileSelectionKind::RUNTIME_CUSTOM && entry.CustomPresetId == *activeCustomPresetId;
                });
            if (iterator != entries.end()) { return static_cast<std::size_t>(std::distance(entries.begin(), iterator)); }
        }
        return 0u;
    }
}
