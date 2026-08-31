#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

#include <PixelShipGenerator/ShipDimensions.h>

namespace PixelShipGeneratorPreview
{
    inline constexpr uint32_t MinimumPreviewResolution = 24u;
    inline constexpr uint32_t MaximumPreviewResolution = 256u;
    inline constexpr uint32_t PreviewResolutionStep = 2u;
    inline constexpr uint32_t MaximumResolutionBookmarks = 6u;
    inline constexpr std::array<uint32_t, 7u> DirectPreviewResolutions = { 24u, 32u, 44u, 64u, 96u, 128u, 160u };

    inline constexpr bool isSelectablePreviewDimensionValue(uint32_t value)
    {
        return value >= MinimumPreviewResolution && value <= MaximumPreviewResolution && (value - MinimumPreviewResolution) % PreviewResolutionStep == 0u;
    }

    inline constexpr uint32_t clampPreviewDimensionValue(uint32_t value)
    {
        if (value <= MinimumPreviewResolution) { return MinimumPreviewResolution; }
        if (value >= MaximumPreviewResolution) { return MaximumPreviewResolution; }
        const uint32_t offset = value - MinimumPreviewResolution;
        const uint32_t snappedOffset = ((offset + PreviewResolutionStep / 2u) / PreviewResolutionStep) * PreviewResolutionStep;
        return MinimumPreviewResolution + snappedOffset;
    }

    inline constexpr bool hasSupportedPreviewAspectRatio(const PixelShipGenerator::ShipDimensions& dimensions)
    {
        return static_cast<uint64_t>(dimensions.Width) <= static_cast<uint64_t>(dimensions.Height) * 2u && static_cast<uint64_t>(dimensions.Height) <= static_cast<uint64_t>(dimensions.Width) * 2u;
    }

    inline constexpr bool isSelectablePreviewDimensions(const PixelShipGenerator::ShipDimensions& dimensions)
    {
        return isSelectablePreviewDimensionValue(dimensions.Width) && isSelectablePreviewDimensionValue(dimensions.Height) && hasSupportedPreviewAspectRatio(dimensions);
    }

    inline constexpr PixelShipGenerator::ShipDimensions makeSquarePreviewDimensions(uint32_t value)
    {
        const uint32_t snapped = clampPreviewDimensionValue(value);
        return { snapped, snapped };
    }

    inline uint32_t getMinimumPreviewWidthForHeight(uint32_t height)
    {
        return clampPreviewDimensionValue(std::max(MinimumPreviewResolution, (height + 1u) / 2u));
    }

    inline uint32_t getMaximumPreviewWidthForHeight(uint32_t height)
    {
        return clampPreviewDimensionValue(std::min(MaximumPreviewResolution, height * 2u));
    }

    inline uint32_t getMinimumPreviewHeightForWidth(uint32_t width)
    {
        return clampPreviewDimensionValue(std::max(MinimumPreviewResolution, (width + 1u) / 2u));
    }

    inline uint32_t getMaximumPreviewHeightForWidth(uint32_t width)
    {
        return clampPreviewDimensionValue(std::min(MaximumPreviewResolution, width * 2u));
    }

    // Kept for direct square-preset call sites.
    inline constexpr bool isSelectablePreviewResolution(uint32_t resolution)
    {
        return isSelectablePreviewDimensionValue(resolution);
    }

    inline constexpr uint32_t clampPreviewResolution(uint32_t resolution)
    {
        return clampPreviewDimensionValue(resolution);
    }
}
