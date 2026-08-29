#include "PreviewCollectionSession.h"

#include <algorithm>
#include <iterator>
#include <utility>

namespace PixelShipGeneratorPreview
{
    PreviewCollectionSession::PreviewCollectionSession(const PreviewGenerationRecipe& initialRecipe, std::size_t maximumHistorySize)
        : m_MaximumHistorySize(std::max<std::size_t>(1u, maximumHistorySize))
    {
        m_History.push_back(initialRecipe);
    }

    PreviewGenerationRecipe& PreviewCollectionSession::getCurrentRecipe()
    {
        return m_History[m_HistoryIndex];
    }

    const PreviewGenerationRecipe& PreviewCollectionSession::getCurrentRecipe() const
    {
        return m_History[m_HistoryIndex];
    }

    void PreviewCollectionSession::appendHistoryEntry(const PreviewGenerationRecipe& recipe)
    {
        if (m_HistoryIndex + 1u < m_History.size())
        {
            m_History.erase(m_History.begin() + static_cast<std::ptrdiff_t>(m_HistoryIndex + 1u), m_History.end());
        }

        m_History.push_back(recipe);
        if (m_History.size() > m_MaximumHistorySize)
        {
            m_History.erase(m_History.begin());
        }
        m_HistoryIndex = m_History.size() - 1u;
    }

    bool PreviewCollectionSession::moveHistoryPrevious()
    {
        if (m_HistoryIndex == 0u) { return false; }
        --m_HistoryIndex;
        return true;
    }

    bool PreviewCollectionSession::moveHistoryNext()
    {
        if (m_HistoryIndex + 1u >= m_History.size()) { return false; }
        ++m_HistoryIndex;
        return true;
    }

    std::size_t PreviewCollectionSession::getHistoryIndex() const { return m_HistoryIndex; }
    std::size_t PreviewCollectionSession::getHistoryCount() const { return m_History.size(); }

    bool PreviewCollectionSession::addFavorite(const PreviewGenerationRecipe& recipe)
    {
        if (isFavorite(recipe)) { return false; }
        m_Favorites.push_back(recipe);
        return true;
    }

    bool PreviewCollectionSession::removeFavorite(const PreviewGenerationRecipe& recipe)
    {
        const std::optional<std::size_t> index = findFavoriteIndex(recipe);
        if (!index.has_value()) { return false; }
        m_Favorites.erase(m_Favorites.begin() + static_cast<std::ptrdiff_t>(*index));
        return true;
    }

    bool PreviewCollectionSession::isFavorite(const PreviewGenerationRecipe& recipe) const
    {
        return findFavoriteIndex(recipe).has_value();
    }

    std::optional<std::size_t> PreviewCollectionSession::findFavoriteIndex(const PreviewGenerationRecipe& recipe) const
    {
        const auto iterator = std::find(m_Favorites.begin(), m_Favorites.end(), recipe);
        if (iterator == m_Favorites.end()) { return std::nullopt; }
        return static_cast<std::size_t>(std::distance(m_Favorites.begin(), iterator));
    }

    const PreviewGenerationRecipe* PreviewCollectionSession::getFavorite(std::size_t index) const
    {
        return index < m_Favorites.size() ? &m_Favorites[index] : nullptr;
    }

    const std::vector<PreviewGenerationRecipe>& PreviewCollectionSession::getFavorites() const { return m_Favorites; }

    void PreviewCollectionSession::beginGallery(uint64_t batchSeed, const PreviewGenerationRecipe& templateRecipe)
    {
        m_GalleryBatchSeed = batchSeed;
        m_GalleryTemplateRecipe = templateRecipe;
        m_GalleryRecipes.clear();
    }

    void PreviewCollectionSession::clearGallery()
    {
        m_GalleryRecipes.clear();
    }

    void PreviewCollectionSession::addGalleryRecipe(const PreviewGenerationRecipe& recipe)
    {
        m_GalleryRecipes.push_back({ recipe, true });
    }

    void PreviewCollectionSession::addInvalidGalleryRecipe()
    {
        m_GalleryRecipes.push_back({});
    }

    const PreviewGenerationRecipe* PreviewCollectionSession::getGalleryRecipe(std::size_t index) const
    {
        return index < m_GalleryRecipes.size() && m_GalleryRecipes[index].Valid ? &m_GalleryRecipes[index].Recipe : nullptr;
    }

    const std::vector<PreviewGalleryRecipeEntry>& PreviewCollectionSession::getGalleryRecipes() const { return m_GalleryRecipes; }
    uint64_t PreviewCollectionSession::getGalleryBatchSeed() const { return m_GalleryBatchSeed; }
    const PreviewGenerationRecipe& PreviewCollectionSession::getGalleryTemplateRecipe() const { return m_GalleryTemplateRecipe; }

    bool PreviewCollectionSession::addResolutionBookmark(const PixelShipGenerator::ShipDimensions& dimensions)
    {
        if (!isSelectablePreviewDimensions(dimensions) || m_ResolutionBookmarks.size() >= MaximumResolutionBookmarks) { return false; }
        const auto iterator = std::lower_bound(m_ResolutionBookmarks.begin(), m_ResolutionBookmarks.end(), dimensions, dimensionsLess);
        if (iterator != m_ResolutionBookmarks.end() && *iterator == dimensions) { return false; }
        m_ResolutionBookmarks.insert(iterator, dimensions);
        return true;
    }

    bool PreviewCollectionSession::removeResolutionBookmark(const PixelShipGenerator::ShipDimensions& dimensions)
    {
        const auto iterator = std::lower_bound(m_ResolutionBookmarks.begin(), m_ResolutionBookmarks.end(), dimensions, dimensionsLess);
        if (iterator == m_ResolutionBookmarks.end() || *iterator != dimensions) { return false; }
        m_ResolutionBookmarks.erase(iterator);
        return true;
    }

    bool PreviewCollectionSession::hasResolutionBookmark(const PixelShipGenerator::ShipDimensions& dimensions) const
    {
        const auto iterator = std::lower_bound(m_ResolutionBookmarks.begin(), m_ResolutionBookmarks.end(), dimensions, dimensionsLess);
        return iterator != m_ResolutionBookmarks.end() && *iterator == dimensions;
    }

    const PixelShipGenerator::ShipDimensions* PreviewCollectionSession::getResolutionBookmark(std::size_t index) const
    {
        return index < m_ResolutionBookmarks.size() ? &m_ResolutionBookmarks[index] : nullptr;
    }

    const std::vector<PixelShipGenerator::ShipDimensions>& PreviewCollectionSession::getResolutionBookmarks() const { return m_ResolutionBookmarks; }

    void PreviewCollectionSession::setResolutionBookmarks(std::vector<PixelShipGenerator::ShipDimensions> bookmarks)
    {
        bookmarks.erase(std::remove_if(bookmarks.begin(), bookmarks.end(), [](const PixelShipGenerator::ShipDimensions& dimensions) { return !isSelectablePreviewDimensions(dimensions); }), bookmarks.end());
        std::sort(bookmarks.begin(), bookmarks.end(), dimensionsLess);
        bookmarks.erase(std::unique(bookmarks.begin(), bookmarks.end()), bookmarks.end());
        if (bookmarks.size() > MaximumResolutionBookmarks) { bookmarks.resize(MaximumResolutionBookmarks); }
        m_ResolutionBookmarks = std::move(bookmarks);
    }

    bool PreviewCollectionSession::dimensionsLess(const PixelShipGenerator::ShipDimensions& first, const PixelShipGenerator::ShipDimensions& second)
    {
        if (first.Width != second.Width) { return first.Width < second.Width; }
        return first.Height < second.Height;
    }
}
