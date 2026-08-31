#pragma once

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cstdint>
#include <cmath>

#include "PreviewPagination.h"
#include "PreviewState.h"

namespace SpectralShipGenStudioPreview
{
    inline uint32_t getPreviewThumbnailPageCapacity(const PreviewThumbnailGridState& grid)
    {
        return std::max(1u, grid.Columns * grid.Rows);
    }

    inline uint32_t getPreviewThumbnailPageStart(const PreviewThumbnailGridState& grid)
    {
        const uint32_t pageCapacity = getPreviewThumbnailPageCapacity(grid);
        const std::size_t currentPage = getPreviewPageForItem(grid.SelectedIndex, pageCapacity);
        return static_cast<uint32_t>(getPreviewPageStart(currentPage, grid.Items.size(), pageCapacity));
    }

    inline uint32_t getPreviewThumbnailPageCount(const PreviewThumbnailGridState& grid)
    {
        return static_cast<uint32_t>(getPreviewPageCount(grid.Items.size(), getPreviewThumbnailPageCapacity(grid)));
    }

    inline uint32_t getPreviewThumbnailCurrentPage(const PreviewThumbnailGridState& grid)
    {
        if (grid.Items.empty()) { return 0u; }
        const std::size_t page = getPreviewPageForItem(std::min<std::size_t>(grid.SelectedIndex, grid.Items.size() - 1u), getPreviewThumbnailPageCapacity(grid));
        return static_cast<uint32_t>(clampPreviewPageIndex(page, grid.Items.size(), getPreviewThumbnailPageCapacity(grid)));
    }

    inline sf::FloatRect getPreviewThumbnailCellBounds(const PreviewThumbnailGridState& grid, uint32_t itemIndex)
    {
        const uint32_t pageStart = getPreviewThumbnailPageStart(grid);
        const uint32_t visibleIndex = itemIndex >= pageStart ? itemIndex - pageStart : 0u;
        const uint32_t column = grid.Columns == 0u ? 0u : visibleIndex % grid.Columns;
        const uint32_t row = grid.Columns == 0u ? 0u : visibleIndex / grid.Columns;
        const float cellWidth = grid.Columns == 0u ? static_cast<float>(PreviewContentWidth) : static_cast<float>(PreviewContentWidth) / static_cast<float>(grid.Columns);
        const float availableHeight = static_cast<float>(PreviewWindowHeight - PreviewWorkspaceNavigationHeight);
        const float cellHeight = grid.Rows == 0u ? availableHeight : availableHeight / static_cast<float>(grid.Rows);
        return sf::FloatRect(static_cast<float>(column) * cellWidth, static_cast<float>(PreviewWorkspaceNavigationHeight) + static_cast<float>(row) * cellHeight, cellWidth, cellHeight);
    }

    inline float calculatePreviewThumbnailScale(uint32_t imageWidth, uint32_t imageHeight, const sf::FloatRect& cellBounds)
    {
        if (imageWidth == 0u || imageHeight == 0u) { return 1.0f; }
        const float availableWidth = std::max(1.0f, cellBounds.width - PreviewThumbnailCellPadding * 2.0f);
        const float availableHeight = std::max(1.0f, cellBounds.height - PreviewThumbnailCellPadding * 2.0f);
        const float fitScale = std::min(availableWidth / static_cast<float>(imageWidth), availableHeight / static_cast<float>(imageHeight));
        return fitScale >= 1.0f ? std::max(1.0f, std::floor(fitScale)) : fitScale;
    }

    inline sf::Vector2f calculatePreviewThumbnailPosition(uint32_t imageWidth, uint32_t imageHeight, float scale, const sf::FloatRect& cellBounds)
    {
        const float renderedWidth = static_cast<float>(imageWidth) * scale;
        const float renderedHeight = static_cast<float>(imageHeight) * scale;
        return sf::Vector2f(cellBounds.left + (cellBounds.width - renderedWidth) * 0.5f, cellBounds.top + (cellBounds.height - renderedHeight) * 0.5f);
    }

    inline int32_t findPreviewThumbnailItemAtPosition(sf::Vector2f position, const PreviewThumbnailGridState& grid)
    {
        if (grid.Items.empty()) { return -1; }
        const uint32_t pageStart = getPreviewThumbnailPageStart(grid);
        const uint32_t pageCapacity = getPreviewThumbnailPageCapacity(grid);
        const uint32_t pageEnd = std::min(static_cast<uint32_t>(grid.Items.size()), pageStart + pageCapacity);

        for (uint32_t index = pageStart; index < pageEnd; ++index)
        {
            if (getPreviewThumbnailCellBounds(grid, index).contains(position)) { return static_cast<int32_t>(index); }
        }

        return -1;
    }

    inline bool movePreviewThumbnailSelection(PreviewThumbnailGridState& grid, int32_t deltaX, int32_t deltaY)
    {
        if (grid.Items.empty()) { return false; }
        const uint32_t columns = std::max(1u, grid.Columns);
        uint32_t newIndex = std::min(grid.SelectedIndex, static_cast<uint32_t>(grid.Items.size() - 1u));

        if (deltaX < 0 && newIndex % columns > 0u) { --newIndex; }
        if (deltaX > 0 && newIndex % columns + 1u < columns && newIndex + 1u < grid.Items.size()) { ++newIndex; }
        if (deltaY < 0 && newIndex >= columns) { newIndex -= columns; }
        if (deltaY > 0 && newIndex + columns < grid.Items.size()) { newIndex += columns; }

        if (newIndex == grid.SelectedIndex) { return false; }
        grid.SelectedIndex = newIndex;
        grid.HoveredIndex = -1;
        return true;
    }
}
