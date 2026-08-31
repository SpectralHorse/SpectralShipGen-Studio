#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <SpectralShipGen/ShipFactionType.h>
#include <SpectralShipGen/ShipGenerationRecipe.h>

#include "RuntimeCustomPresetWorkspace.h"

namespace SpectralShipGenStudioPreview
{
    enum class PaletteProfileSelectionKind : uint32_t
    {
        FACTION_DEFAULT = 0u,
        BUILT_IN_GENERATED,
        RUNTIME_CUSTOM,
        ADD_PALETTE
    };

    struct PaletteProfileSelectionEntry
    {
        PaletteProfileSelectionKind Kind = PaletteProfileSelectionKind::FACTION_DEFAULT;
        std::string Label;
        std::optional<SpectralShipGen::ShipFactionType> PalettePreset = std::nullopt;
        RuntimeCustomPresetId CustomPresetId = 0u;
    };

    std::vector<PaletteProfileSelectionEntry> buildPaletteProfileSelection(const RuntimeCustomPresetWorkspace& workspace);
    std::size_t findPaletteProfileSelectionIndex(const std::vector<PaletteProfileSelectionEntry>& entries,
        const SpectralShipGen::ShipGenerationRecipe& recipe,
        std::optional<SpectralShipGen::ShipFactionType> activeBuiltInPalette,
        std::optional<RuntimeCustomPresetId> activeCustomPresetId);
}
