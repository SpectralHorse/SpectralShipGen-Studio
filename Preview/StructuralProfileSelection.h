#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <PixelShipGenerator/ShipGenerationRecipe.h>

#include "RuntimeCustomPresetWorkspace.h"

namespace PixelShipGeneratorPreview
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
        PixelShipGenerator::ShipStyle Style = PixelShipGenerator::ShipStyle::SHIP_STYLE_END;
        RuntimeCustomPresetId CustomPresetId = 0u;
    };

    std::vector<StructuralProfileSelectionEntry> buildStructuralProfileSelection(const RuntimeCustomPresetWorkspace& workspace);
    std::size_t findStructuralProfileSelectionIndex(const std::vector<StructuralProfileSelectionEntry>& entries, const PixelShipGenerator::ShipGenerationRecipe& recipe, std::optional<RuntimeCustomPresetId> activeCustomPresetId);
}
