#pragma once

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

#include "GeneratedShip.h"
#include "AttributeRerollStudio.h"
#include "GenerationCalibration.h"
#include "ShipGenerationDebugInfo.h"
#include "ShipIdleAnimation.h"

#include "PreviewCommandPanel.h"
#include "PreviewState.h"

namespace PixelShipGeneratorPreview
{
    struct PreviewRenderData
    {
        PreviewMode Mode = PreviewMode::STATIC;
        const sf::Sprite* PreviewSprite = nullptr;
        const sf::Texture* CurrentStaticTexture = nullptr;
        const sf::Texture* NativePreviewTexture = nullptr;
        const sf::Texture* PinnedTexture = nullptr;
        const GalleryState* Gallery = nullptr;
        const FavoritesState* Favorites = nullptr;
        const PreviewGenerationRecipe* Recipe = nullptr;
        const GenerationLocks* Locks = nullptr;
        const PreviewDiagnosticState* Diagnostics = nullptr;
        const PreviewComparisonState* Comparison = nullptr;
        const PixelShipGenerator::GeneratedShip* Ship = nullptr;
        const PixelShipGenerator::ShipGenerationDebugInfo* GenerationDebugInfo = nullptr;
        const PixelShipGenerator::ShipIdleAnimation* IdleAnimation = nullptr;
        const PixelShipGenerator::ShipIdleAnimationSettings* IdleAnimationSettings = nullptr;
        const PreviewCommandPanel* CommandPanel = nullptr;
        std::size_t HistoryIndex = 0u;
        std::size_t HistoryCount = 0u;
        uint32_t AnimationFrameIndex = 0u;
        bool CurrentIsFavorite = false;
        const std::string* StatusMessage = nullptr;
        const CalibrationCandidatePair* CalibrationPair = nullptr;
        const CalibrationObjectiveBatch* ObjectiveBatch = nullptr;
        const GenerationCalibrationSession* CalibrationSession = nullptr;
        PixelShipGenerator::GenerationWeightGroup CalibrationGroup = PixelShipGenerator::GenerationWeightGroup::ENGINE_LAYOUT;
        CalibrationContextFilter CalibrationFilter;
        const sf::Texture* CalibrationTextureA = nullptr;
        const sf::Texture* CalibrationTextureB = nullptr;
        bool CalibrationShowValues = true;
        const AttributeRerollStudioState* RerollStudio = nullptr;
        const sf::Texture* RerollStudioCandidateTexture = nullptr;
    };

    class PreviewRenderer
    {
    public:
        void render(sf::RenderWindow& window, const PreviewRenderData& data) const;

    private:
        void renderCommandPanel(sf::RenderWindow& window, const PreviewCommandPanel& commandPanel) const;
        void renderCalibration(sf::RenderWindow& window, const PreviewRenderData& data) const;
        void renderComparison(sf::RenderWindow& window, const PreviewRenderData& data) const;
        void renderRerollStudio(sf::RenderWindow& window, const PreviewRenderData& data) const;
        uint32_t renderSideBySideSprites(sf::RenderWindow& window, const PreviewGenerationRecipe& leftRecipe, const sf::Texture& leftTexture, const std::string& leftLabel, const sf::Color& leftColor, const PreviewGenerationRecipe& rightRecipe, const sf::Texture& rightTexture, const std::string& rightLabel, const sf::Color& rightColor, float spriteRegionTop, float spriteRegionHeight) const;
        void renderFavorites(sf::RenderWindow& window, const FavoritesState& favoritesState) const;
        void renderGallery(sf::RenderWindow& window, const GalleryState& galleryState) const;
        void renderThumbnailGrid(sf::RenderWindow& window, const PreviewThumbnailGridState& gridState) const;
        void renderSingle(sf::RenderWindow& window, const sf::Sprite& previewSprite) const;
        void renderNativePreview(sf::RenderWindow& window, const PreviewRenderData& data) const;
        void renderPersistentStatePanel(sf::RenderWindow& window, const PreviewRenderData& data) const;
        void renderHelpOverlay(sf::RenderWindow& window) const;
        void renderGenerationInspector(sf::RenderWindow& window, const PreviewRenderData& data) const;
        void renderPaletteInspector(sf::RenderWindow& window, const PreviewRenderData& data) const;
    };
}
