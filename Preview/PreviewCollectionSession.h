#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "PreviewGenerationRecipe.h"
#include "PreviewResolution.h"

namespace SpectralShipGenStudioPreview
{
    struct PreviewGalleryRecipeEntry
    {
        PreviewGenerationRecipe Recipe;
        bool Valid = false;
        bool Favorite = false;
    };

    class PreviewCollectionSession
    {
    public:
        explicit PreviewCollectionSession(const PreviewGenerationRecipe& initialRecipe, std::size_t maximumHistorySize = 20u);

        PreviewGenerationRecipe& getCurrentRecipe();
        const PreviewGenerationRecipe& getCurrentRecipe() const;

        void appendHistoryEntry(const PreviewGenerationRecipe& recipe);
        bool moveHistoryPrevious();
        bool moveHistoryNext();
        std::size_t getHistoryIndex() const;
        std::size_t getHistoryCount() const;

        bool addFavorite(const PreviewGenerationRecipe& recipe);
        bool removeFavorite(const PreviewGenerationRecipe& recipe);
        bool isFavorite(const PreviewGenerationRecipe& recipe) const;
        std::optional<std::size_t> findFavoriteIndex(const PreviewGenerationRecipe& recipe) const;
        const PreviewGenerationRecipe* getFavorite(std::size_t index) const;
        const std::vector<PreviewGenerationRecipe>& getFavorites() const;
        void setFavorites(std::vector<PreviewGenerationRecipe> favorites);

        void beginGallery(uint64_t batchSeed, const PreviewGenerationRecipe& templateRecipe);
        void clearGallery();
        void addGalleryRecipe(const PreviewGenerationRecipe& recipe);
        void addInvalidGalleryRecipe();
        const PreviewGenerationRecipe* getGalleryRecipe(std::size_t index) const;
        bool isGalleryFavorite(std::size_t index) const;
        std::optional<bool> toggleGalleryFavorite(std::size_t index);
        const std::vector<PreviewGalleryRecipeEntry>& getGalleryRecipes() const;
        uint64_t getGalleryBatchSeed() const;
        const PreviewGenerationRecipe& getGalleryTemplateRecipe() const;

        bool addResolutionBookmark(const SpectralShipGen::ShipDimensions& dimensions);
        bool removeResolutionBookmark(const SpectralShipGen::ShipDimensions& dimensions);
        bool hasResolutionBookmark(const SpectralShipGen::ShipDimensions& dimensions) const;
        const SpectralShipGen::ShipDimensions* getResolutionBookmark(std::size_t index) const;
        const std::vector<SpectralShipGen::ShipDimensions>& getResolutionBookmarks() const;
        void setResolutionBookmarks(std::vector<SpectralShipGen::ShipDimensions> bookmarks);

    private:
        static bool dimensionsLess(const SpectralShipGen::ShipDimensions& first, const SpectralShipGen::ShipDimensions& second);

        std::size_t m_MaximumHistorySize = 20u;
        std::vector<PreviewGenerationRecipe> m_History;
        std::size_t m_HistoryIndex = 0u;
        std::vector<PreviewGenerationRecipe> m_Favorites;
        std::vector<PreviewGalleryRecipeEntry> m_GalleryRecipes;
        uint64_t m_GalleryBatchSeed = 0u;
        PreviewGenerationRecipe m_GalleryTemplateRecipe;
        std::vector<SpectralShipGen::ShipDimensions> m_ResolutionBookmarks;
    };
}
