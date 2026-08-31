#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <SpectralShipGen/ShipGenerationRecipe.h>

#include "RuntimeCustomPresetWorkspace.h"

namespace SpectralShipGenStudioPreview
{
    enum class StructuralProfileSelectionKind : uint32_t
    {
        BUILT_IN = 0u,
        RUNTIME_CUSTOM,
        ADD_PROFILE
    };

    struct StructuralProfileSelectionEntry
    {
        StructuralProfileSelectionKind Kind = StructuralProfileSelectionKind::BUILT_IN;
        std::string Label;
        std::optional<SpectralShipGen::ShipStyle> Style = std::nullopt;
        RuntimeCustomPresetId CustomPresetId = 0u;
    };

    std::vector<StructuralProfileSelectionEntry> buildStructuralProfileSelection(const RuntimeCustomPresetWorkspace& workspace);
    std::size_t findStructuralProfileSelectionIndex(const std::vector<StructuralProfileSelectionEntry>& entries, const SpectralShipGen::ShipGenerationRecipe& recipe, std::optional<RuntimeCustomPresetId> activeCustomPresetId);
}
