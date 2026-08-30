#pragma once

#include <algorithm>
#include <cstddef>

namespace PixelShipGeneratorPreview
{
    inline std::size_t getPreviewPageCount(std::size_t itemCount, std::size_t pageSize)
    {
        const std::size_t safePageSize = std::max<std::size_t>(1u, pageSize);
        return itemCount == 0u ? 0u : (itemCount + safePageSize - 1u) / safePageSize;
    }

    inline std::size_t getPreviewPageForItem(std::size_t itemIndex, std::size_t pageSize)
    {
        return itemIndex / std::max<std::size_t>(1u, pageSize);
    }

    inline std::size_t clampPreviewPageIndex(std::size_t pageIndex, std::size_t itemCount, std::size_t pageSize)
    {
        const std::size_t pageCount = getPreviewPageCount(itemCount, pageSize);
        return pageCount == 0u ? 0u : std::min(pageIndex, pageCount - 1u);
    }

    inline std::size_t getPreviewPageStart(std::size_t pageIndex, std::size_t itemCount, std::size_t pageSize)
    {
        const std::size_t safePageSize = std::max<std::size_t>(1u, pageSize);
        return clampPreviewPageIndex(pageIndex, itemCount, safePageSize) * safePageSize;
    }
}
