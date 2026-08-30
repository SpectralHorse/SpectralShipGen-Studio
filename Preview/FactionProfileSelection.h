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
    enum class FactionProfileSelectionKind : uint32_t
    {
        BUILT_IN = 0u,
        RUNTIME_CUSTOM,
        ADD_FACTION
    };

    struct FactionProfileSelectionEntry
    {
        FactionProfileSelectionKind Kind = FactionProfileSelectionKind::BUILT_IN;
        std::string Label;
        PixelShipGenerator::ShipFactionType Faction = PixelShipGenerator::ShipFactionType::FRONTIER;
        RuntimeCustomPresetId CustomPresetId = 0u;
    };

    std::vector<FactionProfileSelectionEntry> buildFactionProfileSelection(const RuntimeCustomPresetWorkspace& workspace);
    std::size_t findFactionProfileSelectionIndex(const std::vector<FactionProfileSelectionEntry>& entries, const PixelShipGenerator::ShipGenerationRecipe& recipe, std::optional<RuntimeCustomPresetId> activeCustomPresetId);
}
