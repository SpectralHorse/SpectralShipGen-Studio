#pragma once

#include <algorithm>
#include <cstdint>

namespace SpectralShipGenStudioPreview
{
    struct PreviewPhysicalSize
    {
        uint32_t Width = 1u;
        uint32_t Height = 1u;
    };

    struct PreviewNormalizedViewport
    {
        float Left = 0.0f;
        float Top = 0.0f;
        float Width = 1.0f;
        float Height = 1.0f;
    };

    inline PreviewPhysicalSize fitPreviewWindowToAvailableClientArea(
        uint32_t availableWidth,
        uint32_t availableHeight,
        uint32_t logicalWidth,
        uint32_t logicalHeight)
    {
        if (availableWidth == 0u || availableHeight == 0u || logicalWidth == 0u || logicalHeight == 0u)
        {
            return {};
        }

        if (availableWidth >= logicalWidth && availableHeight >= logicalHeight)
        {
            return { logicalWidth, logicalHeight };
        }

        const double horizontalScale = static_cast<double>(availableWidth) / static_cast<double>(logicalWidth);
        const double verticalScale = static_cast<double>(availableHeight) / static_cast<double>(logicalHeight);
        const double scale = std::min(horizontalScale, verticalScale);

        return {
            std::max(1u, static_cast<uint32_t>(static_cast<double>(logicalWidth) * scale)),
            std::max(1u, static_cast<uint32_t>(static_cast<double>(logicalHeight) * scale))
        };
    }

    inline PreviewNormalizedViewport calculatePreviewLogicalViewport(
        uint32_t physicalWidth,
        uint32_t physicalHeight,
        uint32_t logicalWidth,
        uint32_t logicalHeight)
    {
        if (physicalWidth == 0u || physicalHeight == 0u || logicalWidth == 0u || logicalHeight == 0u)
        {
            return {};
        }

        const double physicalAspect = static_cast<double>(physicalWidth) / static_cast<double>(physicalHeight);
        const double logicalAspect = static_cast<double>(logicalWidth) / static_cast<double>(logicalHeight);

        PreviewNormalizedViewport viewport;
        if (physicalAspect > logicalAspect)
        {
            viewport.Width = static_cast<float>(logicalAspect / physicalAspect);
            viewport.Left = (1.0f - viewport.Width) * 0.5f;
        }
        else if (physicalAspect < logicalAspect)
        {
            viewport.Height = static_cast<float>(physicalAspect / logicalAspect);
            viewport.Top = (1.0f - viewport.Height) * 0.5f;
        }

        return viewport;
    }

    inline bool isPreviewPixelInsideLogicalViewport(
        int32_t pixelX,
        int32_t pixelY,
        uint32_t physicalWidth,
        uint32_t physicalHeight,
        const PreviewNormalizedViewport& viewport)
    {
        if (physicalWidth == 0u || physicalHeight == 0u || pixelX < 0 || pixelY < 0 ||
            static_cast<uint32_t>(pixelX) >= physicalWidth || static_cast<uint32_t>(pixelY) >= physicalHeight)
        {
            return false;
        }

        const float normalizedX = static_cast<float>(pixelX) / static_cast<float>(physicalWidth);
        const float normalizedY = static_cast<float>(pixelY) / static_cast<float>(physicalHeight);
        return normalizedX >= viewport.Left &&
            normalizedX <= viewport.Left + viewport.Width &&
            normalizedY >= viewport.Top &&
            normalizedY <= viewport.Top + viewport.Height;
    }
}
