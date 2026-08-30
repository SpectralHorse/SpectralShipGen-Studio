#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "ShipFactionType.h"
#include "ShipGenerationRecipe.h"

#include "RuntimeCustomPresetWorkspace.h"

namespace PixelShipGeneratorPreview
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
        PixelShipGenerator::ShipFactionType PalettePreset = PixelShipGenerator::ShipFactionType::SHIP_FACTION_TYPE_END;
        RuntimeCustomPresetId CustomPresetId = 0u;
    };

    std::vector<PaletteProfileSelectionEntry> buildPaletteProfileSelection(const RuntimeCustomPresetWorkspace& workspace);
    std::size_t findPaletteProfileSelectionIndex(const std::vector<PaletteProfileSelectionEntry>& entries,
        const PixelShipGenerator::ShipGenerationRecipe& recipe,
        std::optional<PixelShipGenerator::ShipFactionType> activeBuiltInPalette,
        std::optional<RuntimeCustomPresetId> activeCustomPresetId);
}
