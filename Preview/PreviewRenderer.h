#pragma once

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

#include "GeneratedShip.h"
#include "AttributeRerollStudio.h"
#include "GenerationCalibration.h"
#include "ShipGenerationDebugInfo.h"
#include "ShipFiringAnimation.h"
#include "ShipIdleAnimation.h"
#include "ShipMovementAnimation.h"

#include "PreviewCommandPanel.h"
#include "PreviewConfigurationEditor.h"
#include "PreviewState.h"
#include "PreviewWorkspaceNavigation.h"

namespace PixelShipGeneratorPreview
{
    struct PreviewRenderData
    {
        PreviewMode Mode = PreviewMode::STATIC;
        PreviewWorkspace Workspace = PreviewWorkspace::GENERATE;
        const PreviewWorkspaceNavigation* WorkspaceNavigation = nullptr;
        const sf::Sprite* PreviewSprite = nullptr;
        const sf::Texture* CurrentStaticTexture = nullptr;
        const sf::Texture* NativePreviewTexture = nullptr;
        const sf::Texture* PinnedTexture = nullptr;
        const GalleryState* Gallery = nullptr;
        const FavoritesState* Favorites = nullptr;
        const PreviewGenerationRecipe* Recipe = nullptr;
        std::string StructuralDisplayName;
        std::string FactionDisplayName;
        std::string PaletteDisplayName;
        std::string ConfigurationBundleDisplayName;
        const GenerationLocks* Locks = nullptr;
        const PreviewDiagnosticState* Diagnostics = nullptr;
        const PreviewComparisonState* Comparison = nullptr;
        const PixelShipGenerator::GeneratedShip* Ship = nullptr;
        const PixelShipGenerator::ShipGenerationDebugInfo* GenerationDebugInfo = nullptr;
        PixelShipGenerator::ShipAnimationType SelectedAnimationType = PixelShipGenerator::ShipAnimationType::IDLE;
        const PixelShipGenerator::ShipIdleAnimation* IdleAnimation = nullptr;
        const PixelShipGenerator::ShipIdleAnimationSettings* IdleAnimationSettings = nullptr;
        const PixelShipGenerator::ShipMovementAnimation* MovementAnimation = nullptr;
        const PixelShipGenerator::ShipMovementAnimationSettings* MovementAnimationSettings = nullptr;
        PixelShipGenerator::ShipMovementAnimationPhase MovementPhase = PixelShipGenerator::ShipMovementAnimationPhase::ENTER;
        PixelShipGenerator::ShipAnimationType RuntimeMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
        PixelShipGenerator::ShipAnimationType PendingMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
        bool MovementTransitionPending = false;
        bool TransientStatePreviewActive = false;
        const PixelShipGenerator::ShipFiringAnimation* FiringAnimation = nullptr;
        const PixelShipGenerator::ShipFiringAnimationSettings* FiringAnimationSettings = nullptr;
        const PreviewCommandPanel* CommandPanel = nullptr;
        const PreviewConfigurationEditor* ConfigurationEditor = nullptr;
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
        void renderConfigurationEditor(sf::RenderWindow& window, const PreviewConfigurationEditor& editor) const;
        void renderCalibration(sf::RenderWindow& window, const PreviewRenderData& data) const;
        void renderComparison(sf::RenderWindow& window, const PreviewRenderData& data) const;
        void renderRerollStudio(sf::RenderWindow& window, const PreviewRenderData& data) const;
        uint32_t renderSideBySideSprites(sf::RenderWindow& window, const PreviewGenerationRecipe& leftRecipe, const sf::Texture& leftTexture, const std::string& leftLabel, const sf::Color& leftColor, const PreviewGenerationRecipe& rightRecipe, const sf::Texture& rightTexture, const std::string& rightLabel, const sf::Color& rightColor, float spriteRegionTop, float spriteRegionHeight) const;
        void renderFavorites(sf::RenderWindow& window, const FavoritesState& favoritesState) const;
        void renderGallery(sf::RenderWindow& window, const GalleryState& galleryState) const;
        void renderThumbnailGrid(sf::RenderWindow& window, const PreviewThumbnailGridState& gridState, bool showFavoriteMarkers) const;
        void renderSingle(sf::RenderWindow& window, const sf::Sprite& previewSprite) const;
        void renderNativePreview(sf::RenderWindow& window, const PreviewRenderData& data) const;
        void renderPersistentStatePanel(sf::RenderWindow& window, const PreviewRenderData& data) const;
        void renderInspectionStatePanel(sf::RenderWindow& window, const PreviewRenderData& data, float x, float& y) const;
        void renderInspectionEmptyState(sf::RenderWindow& window) const;
        void renderWorkspaceNavigation(sf::RenderWindow& window, const PreviewWorkspaceNavigation& navigation) const;
        void renderHelpOverlay(sf::RenderWindow& window, PreviewWorkspace workspace) const;
        void renderGenerationInspector(sf::RenderWindow& window, const PreviewRenderData& data) const;
        void renderPaletteInspector(sf::RenderWindow& window, const PreviewRenderData& data) const;
    };
}
