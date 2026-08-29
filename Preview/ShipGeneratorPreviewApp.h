#pragma once

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "GeneratedShip.h"
#include "ShipFactionType.h"
#include "ShipGenerationDebugInfo.h"
#include "ShipGenerationProfile.h"
#include "ShipGenerator.h"
#include "ShipIdleAnimation.h"
#include "ShipIdleAnimator.h"

#include "AttributeRerollStudio.h"
#include "GenerationCalibration.h"
#include "GenerationCalibrationSerializer.h"
#include "PreviewCommand.h"
#include "PreviewCommandPanel.h"
#include "PreviewRenderer.h"
#include "PreviewState.h"

namespace PixelShipGeneratorPreview
{
    class ShipGeneratorPreviewApp
    {
    public:
        explicit ShipGeneratorPreviewApp(std::string startupRecipePath = {});

        int run();

    private:
        void addCurrentToFavorites();
        void addResolutionBookmark();
        void appendHistoryEntry(const PreviewGenerationRecipe& recipe);
        void changeFaction(int32_t delta);
        void changeResolution(int32_t delta);
        void changeStyle(int32_t delta);
        void clearPinnedShip();
        bool buildGallery(uint64_t batchSeed);
        PreviewCommandPanelState createCommandPanelState() const;
        PixelShipGenerator::Image createDiagnosticImage() const;
        void cycleDiagnosticView();
        void enterAnimationPlayback();
        void enterAttributeRerollStudio();
        void cancelAttributeRerollStudio();
        void acceptAttributeRerollStudioCandidate();
        bool generateAttributeRerollStudioCandidate();
        void enterCalibrationLab();
        void exitCalibrationLab();
        void changeCalibrationGroup(int32_t delta);
        bool generateCalibrationPair();
        void recordCalibrationDisplayPreference(bool preferLeft);
        void recordCalibrationPreferenceResult(CalibrationPreferenceResult result);
        void setCalibrationWeight(uint32_t encodedValue);
        void saveCalibrationSession();
        void loadCalibrationSession();
        void exportCalibrationReport();
        void exportCalibrationTuningProfile();
        void runCalibrationObjectiveBatch();
        CalibrationContextFilter getCalibrationContextFilter() const;
        void enterFrameInspection();
        void enterGalleryMode();
        void enterGalleryModeFromKnownSeed();
        void enterFavoritesMode();
        void exportRecipe();
        void executeCommand(const PreviewCommand& command);
        void exitGalleryMode();
        void exitFavoritesMode();
        void generateFromMasterSeed();
        void generateNew();
        bool generateShipFromRecipe(const PreviewGenerationRecipe& recipe, PixelShipGenerator::GeneratedShip& outShip, PixelShipGenerator::ShipGenerationDebugInfo* debugInfo = nullptr);
        std::optional<std::size_t> findFavoriteIndex(const PreviewGenerationRecipe& recipe) const;
        std::string getAnimationEffectDisplay() const;
        std::optional<PreviewCommand> getKeyboardCommand(sf::Keyboard::Key key, bool shift) const;
        void handleKeyPressed(const sf::Event::KeyEvent& event);
        void handleMouseMoved(const sf::Event::MouseMoveEvent& event);
        void handleMousePressed(const sf::Event::MouseButtonEvent& event);
        void handleMouseReleased(const sf::Event::MouseButtonEvent& event);
        bool isCommandActive(PreviewCommandType type) const;
        bool isCommandEnabled(const PreviewCommand& command) const;
        bool importRecipeFromPath(const std::filesystem::path& path);
        void importRecipe();
        bool isCurrentFavorite() const;
        bool isDiagnosticImageViewActive() const;
        void moveAnimationFrame(int32_t delta);
        void moveFavoritesSelection(int32_t deltaX, int32_t deltaY);
        void moveGallerySelection(int32_t deltaX, int32_t deltaY);
        void moveGenerationStage(int32_t delta);
        void loadFavorite(uint32_t index);
        void loadPreviewAppPreferences();
        void next();
        void pinCurrentShip();
        void previous();
        void removeCurrentFromFavorites();
        void removeResolutionBookmark();
        void printControls() const;
        void printCurrentSeeds() const;
        void processEvents();
        bool refreshDiagnosticTexture();
        void refreshDisplayedTexture();
        bool regenerate();
        bool regenerateAnimation();
        void render();
        void reroll();
        void saveCurrent();
        void savePreviewAppPreferences();
        void saveSpritesheet();
        void selectGalleryCandidate(uint32_t index);
        void selectResolutionBookmark(uint32_t index);
        void setDisplayedAnimationFrame(uint32_t frameIndex);
        void setDisplayedStaticFrame();
        void setFaction(PixelShipGenerator::ShipFactionType faction);
        void setDimensions(const PixelShipGenerator::ShipDimensions& dimensions);
        void setHeight(uint32_t height);
        void setResolution(uint32_t resolution);
        void setWidth(uint32_t width);
        void setStyle(PixelShipGenerator::ShipStyle style);
        void toggleAspectRatioLock();
        void toggleAttachments();
        void toggleComparisonView();
        void toggleGenerationStageView();
        void toggleGenerationInspector();
        void toggleHelpOverlay();
        void togglePaletteInspector();
        void update();
        void updateCommandPanelState();
        void updateWindowTitle();
        void setStatusMessage(const std::string& message);

        PreviewGenerationRecipe& getCurrentRecipe();
        const PreviewGenerationRecipe& getCurrentRecipe() const;

    private:
        sf::RenderWindow m_Window;
        PixelShipGenerator::ShipGenerator m_Generator;
        PixelShipGenerator::ShipIdleAnimator m_IdleAnimator;
        std::mt19937_64 m_SeedGenerator;
        PreviewRenderer m_Renderer;
        PreviewCommandPanel m_CommandPanel;

        PreviewMode m_PreviewMode = PreviewMode::STATIC;
        PreviewDiagnosticState m_Diagnostics;
        GalleryState m_GalleryState;
        FavoritesState m_FavoritesState;
        GenerationLocks m_Locks;
        PreviewComparisonState m_Comparison;
        AttributeRerollStudioState m_RerollStudio;
        PreviewMode m_RerollStudioReturnMode = PreviewMode::STATIC;
        PixelShipGenerator::GeneratedShip m_RerollCandidateShip;
        PixelShipGenerator::ShipGenerationDebugInfo m_RerollCandidateDebugInfo;
        sf::Texture m_RerollCandidateTexture;
        GenerationCalibrationSession m_CalibrationSession;
        CalibrationCandidatePair m_CalibrationPair;
        CalibrationObjectiveBatch m_CalibrationObjectiveBatch;
        PixelShipGenerator::GenerationWeightGroup m_CalibrationGroup = PixelShipGenerator::GenerationWeightGroup::ENGINE_LAYOUT;
        bool m_CalibrationShowValues = true;
        bool m_CalibrationContextFilterEnabled = false;
        std::vector<PreviewGenerationRecipe> m_History;
        std::vector<PixelShipGenerator::ShipDimensions> m_ResolutionBookmarks;
        std::size_t m_HistoryIndex = 0u;
        bool m_AspectRatioLocked = true;

        PixelShipGenerator::GeneratedShip m_GeneratedShip;
        PixelShipGenerator::ShipGenerationDebugInfo m_GenerationDebugInfo;
        PixelShipGenerator::ShipIdleAnimationSettings m_IdleAnimationSettings;
        PixelShipGenerator::ShipIdleAnimation m_IdleAnimation;
        uint32_t m_AnimationFrameIndex = 0u;
        sf::Clock m_AnimationClock;
        double m_AnimationPlaybackAccumulatorMicroseconds = 0.0;

        sf::Image m_PreviewImage;
        sf::Texture m_PreviewTexture;
        sf::Texture m_DiagnosticTexture;
        sf::Texture m_PinnedTexture;
        sf::Texture m_CalibrationTextureA;
        sf::Texture m_CalibrationTextureB;
        std::vector<sf::Texture> m_AnimationTextures;
        sf::Sprite m_PreviewSprite;
        std::string m_StartupRecipePath;
        std::string m_StatusMessage;
    };
}
