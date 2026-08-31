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
        std::optional<SpectralShipGen::ShipFactionType> Faction = std::nullopt;
        RuntimeCustomPresetId CustomPresetId = 0u;
    };

    std::vector<FactionProfileSelectionEntry> buildFactionProfileSelection(const RuntimeCustomPresetWorkspace& workspace);
    std::size_t findFactionProfileSelectionIndex(const std::vector<FactionProfileSelectionEntry>& entries, const SpectralShipGen::ShipGenerationRecipe& recipe, std::optional<RuntimeCustomPresetId> activeCustomPresetId);
}
