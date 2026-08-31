#pragma once

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include <SpectralShipGen/GeneratedShip.h>
#include <SpectralShipGen/ShipFactionType.h>
#include <SpectralShipGen/ShipGenerationDebugInfo.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipGenerator.h>

#include "AttributeRerollStudio.h"
#include "GenerationCalibration.h"
#include "GenerationCalibrationSerializer.h"
#include "PreviewCommand.h"
#include "PreviewCommandPanel.h"
#include "PreviewConfigurationEditor.h"
#include "FactionProfileSelection.h"
#include "PaletteProfileSelection.h"
#include "RuntimeCustomPresetWorkspace.h"
#include "StructuralProfileSelection.h"
#include "PreviewAnimationSession.h"
#include "PreviewCollectionSession.h"
#include "PreviewRenderer.h"
#include "PreviewState.h"
#include "PreviewWorkspace.h"
#include "PreviewWorkspaceNavigation.h"

namespace SpectralShipGenStudioPreview
{
    class ShipGeneratorPreviewApp
    {
    public:
        explicit ShipGeneratorPreviewApp(std::string startupRecipePath = {});

        int run();

    private:
        void addCurrentToFavorites();
        void addFavoriteThumbnail(const PreviewGenerationRecipe& recipe, const sf::Texture& texture);
        void addResolutionBookmark();
        void appendHistoryEntry(const PreviewGenerationRecipe& recipe);
        void changeConfigurationBundle(int32_t delta);
        void changeFaction(int32_t delta);
        void changeInspectionGroup(int32_t delta);
        void changeInspectionView(int32_t delta);
        void changePalette(int32_t delta);
        void changeProfilesItem(int32_t delta);
        void changeProfilesSection(int32_t delta);
        void changeResolution(int32_t delta);
        void changeStyle(int32_t delta);
        void clearPinnedShip();
        bool buildGallery(uint64_t batchSeed);
        PreviewCommandPanelState createCommandPanelState() const;
        SpectralShipGen::Image createDiagnosticImage() const;
        void cycleAnimationType();
        void cycleAnimationBaseState();
        void cycleAnimationPlaybackSpeed();
        void cycleDiagnosticView();
        void cycleMovementPhase();
        void cycleFiringTarget();
        void triggerAnimationFire();
        void setAnimationNormalizedTime(uint32_t value);
        void applySelectedAnimationState();
        void returnAnimationToIdle();
        bool beginComposedFiringEvent();
        void enterAnimationPlayback();
        void enterGenerateIdlePlayback();
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
        void enterConfigurationEditor();
        void enterConfigurationEditorDefault();
        void enterFactionConfigurationEditor();
        void enterFactionConfigurationEditorDefault();
        void enterPaletteConfigurationEditor();
        void enterPaletteConfigurationEditorDefault();
        void enterConfigurationBundleEditor();
        void enterConfigurationBundleEditorDefault();
        void handleConfigurationEditorEvent(const ConfigurationEditorEvent& event);
        void deleteConfigurationEditorPreset();
        void exportConfigurationEditorPreset();
        void importConfigurationEditorPreset();
        void enterGalleryMode();
        void enterGalleryModeFromKnownSeed();
        void enterFavoritesMode();
        void exportRecipe();
        void executeCommand(const PreviewCommand& command);
        void newProfilesItem();
        void editSelectedProfilesItem();
        void duplicateSelectedProfilesItem();
        void deleteSelectedProfilesItem();
        void importSelectedProfilesItem();
        void exportSelectedProfilesItem();
        void useSelectedProfilesItem();
        void exitGalleryMode();
        void exitFavoritesMode();
        void generateFromMasterSeed();
        void generateNew();
        bool generateShipFromRecipe(const PreviewGenerationRecipe& recipe, SpectralShipGen::GeneratedShip& outShip, SpectralShipGen::ShipGenerationDebugInfo* debugInfo = nullptr);
        std::string getAnimationEffectDisplay() const;
        const std::vector<SpectralShipGen::Image>& getActiveAnimationFrames() const;
        uint64_t getActiveAnimationSeed() const;
        std::optional<PreviewCommand> getKeyboardCommand(sf::Keyboard::Key key, bool shift, bool control) const;
        bool hasCurrentShip() const;
        bool hasKeyboardInputFocus() const;
        void handleBackOrCancel();
        void handleKeyPressed(const sf::Event::KeyEvent& event);
        void handleMouseMoved(const sf::Event::MouseMoveEvent& event);
        void handleMousePressed(const sf::Event::MouseButtonEvent& event);
        void handleMouseReleased(const sf::Event::MouseButtonEvent& event);
        void handleMouseWheelScrolled(const sf::Event::MouseWheelScrollEvent& event);
        void handleTextEntered(const sf::Event::TextEvent& event);
        bool isCommandActive(PreviewCommandType type) const;
        bool isCommandEnabled(const PreviewCommand& command) const;
        bool importRecipeFromPath(const std::filesystem::path& path);
        void importRecipe();
        bool isCurrentFavorite() const;
        bool isDiagnosticImageViewActive() const;
        void moveAnimationFrame(int32_t delta);
        void moveFavoritesSelection(int32_t deltaX, int32_t deltaY);
        void changeFavoritesPage(int32_t delta);
        void moveGallerySelection(int32_t deltaX, int32_t deltaY);
        void moveGenerationStage(int32_t delta);
        bool loadFavorite(uint32_t index);
        void openSelectedFavoriteInWorkspace(PreviewWorkspace workspace);
        void exportSelectedFavoriteImage();
        void loadFavoriteCollection();
        void loadPreviewAppPreferences();
        void loadUserPresetLibraryState();
        void next();
        void pinCurrentShip();
        void previous();
        void removeCurrentFromFavorites();
        void removeSelectedFavorite();
        void removeFavoriteThumbnail(std::size_t index);
        void removeResolutionBookmark();
        void printControls() const;
        void printCurrentSeeds() const;
        void processEvents();
        bool refreshDiagnosticTexture();
        void refreshDisplayedTexture();
        void refreshGalleryFavoriteMarkers();
        void rebuildFavoriteThumbnails();
        bool regenerate();
        bool regenerateAnimation();
        bool refreshAnimationTextures();
        void render();
        void reroll();
        void saveCurrent();
        void saveFavoriteCollection();
        void savePreviewAppPreferences();
        bool saveUserPresetLibraryState();
        void saveSpritesheet();
        void selectGalleryCandidate(uint32_t index);
        void selectResolutionBookmark(uint32_t index);
        void setDisplayedAnimationFrame(uint32_t frameIndex);
        void setDisplayedStaticFrame();
        void setFaction(SpectralShipGen::ShipFactionType faction);
        void selectRuntimeFactionPreset(RuntimeCustomPresetId id);
        void selectFactionProfileEntry(const FactionProfileSelectionEntry& entry);
        std::string getCurrentFactionProfileDisplayName() const;
        void selectRuntimePalettePreset(RuntimeCustomPresetId id);
        void selectPaletteProfileEntry(const PaletteProfileSelectionEntry& entry);
        std::string getCurrentPaletteDisplayName() const;
        std::string getCurrentConfigurationBundleDisplayName() const;
        std::string getProfilesItemDisplayName() const;
        void applyRuntimeConfigurationBundle(RuntimeCustomPresetId id);
        void setDimensions(const SpectralShipGen::ShipDimensions& dimensions);
        void setHeight(uint32_t height);
        void setResolution(uint32_t resolution);
        void setWidth(uint32_t width);
        void setStyle(SpectralShipGen::ShipStyle style);
        void selectRuntimeStructuralPreset(RuntimeCustomPresetId id);
        void selectStructuralProfileEntry(const StructuralProfileSelectionEntry& entry);
        std::string getCurrentStructuralProfileDisplayName() const;
        void toggleAspectRatioLock();
        void toggleAttachments();
        void toggleComparisonView();
        void toggleGenerationStageView();
        void toggleGenerationInspector();
        void toggleHelpOverlay();
        void toggleInspectionPresentation();
        void togglePaletteInspector();
        void toggleGalleryCandidateFavorite(uint32_t index);
        void update();
        void updateCommandPanelState();
        void updateWindowTitle();
        void setStatusMessage(const std::string& message);
        void switchWorkspace(PreviewWorkspace workspace);

        PreviewGenerationRecipe& getCurrentRecipe();
        const PreviewGenerationRecipe& getCurrentRecipe() const;

    private:
        sf::RenderWindow m_Window;
        SpectralShipGen::ShipGenerator m_Generator;
        std::mt19937_64 m_SeedGenerator;
        PreviewRenderer m_Renderer;
        PreviewCommandPanel m_CommandPanel;
        PreviewConfigurationEditor m_ConfigurationEditor;
        RuntimeCustomPresetWorkspace m_CustomPresetWorkspace;
        PreviewAnimationSession m_AnimationSession;
        PreviewCollectionSession m_Collections{ PreviewGenerationRecipe{} };
        PreviewWorkspaceSession m_WorkspaceSession;
        PreviewWorkspaceNavigation m_WorkspaceNavigation;

        PreviewMode m_PreviewMode = PreviewMode::STATIC;
        PreviewMode m_ConfigurationEditorReturnMode = PreviewMode::STATIC;
        std::optional<RuntimeCustomPresetId> m_SelectedStructuralPresetId;
        std::optional<RuntimeCustomPresetId> m_SelectedFactionPresetId;
        std::optional<SpectralShipGen::ShipFactionType> m_SelectedBuiltInPalettePreset;
        std::optional<RuntimeCustomPresetId> m_SelectedPalettePresetId;
        std::optional<RuntimeCustomPresetId> m_SelectedConfigurationBundleId;
        std::optional<RuntimeCustomPresetId> m_ProfilesSelectedBundleId;
        ProfilesSection m_ProfilesSection = ProfilesSection::STRUCTURAL;
        std::optional<RuntimeCustomPresetId> m_ConfigurationEditorTargetPresetId;
        std::optional<RuntimeCustomPresetId> m_ConfigurationEditorTargetFactionPresetId;
        std::optional<RuntimeCustomPresetId> m_ConfigurationEditorTargetPalettePresetId;
        std::optional<RuntimeCustomPresetId> m_ConfigurationEditorTargetBundleId;
        PreviewDiagnosticState m_Diagnostics;
        GalleryState m_GalleryState;
        FavoritesState m_FavoritesState;
        GenerationLocks m_Locks;
        PreviewComparisonState m_Comparison;
        AttributeRerollStudioState m_RerollStudio;
        PreviewMode m_RerollStudioReturnMode = PreviewMode::STATIC;
        SpectralShipGen::GeneratedShip m_RerollCandidateShip;
        SpectralShipGen::ShipGenerationDebugInfo m_RerollCandidateDebugInfo;
        sf::Texture m_RerollCandidateTexture;
        GenerationCalibrationSession m_CalibrationSession;
        CalibrationCandidatePair m_CalibrationPair;
        CalibrationObjectiveBatch m_CalibrationObjectiveBatch;
        SpectralShipGen::GenerationWeightGroup m_CalibrationGroup = SpectralShipGen::GenerationWeightGroup::ENGINE_LAYOUT;
        bool m_CalibrationShowValues = true;
        bool m_CalibrationContextFilterEnabled = false;
        bool m_AspectRatioLocked = true;

        SpectralShipGen::GeneratedShip m_GeneratedShip;
        SpectralShipGen::ShipGenerationDebugInfo m_GenerationDebugInfo;
        sf::Clock m_AnimationClock;

        sf::Image m_PreviewImage;
        sf::Texture m_PreviewTexture;
        sf::Texture m_DiagnosticTexture;
        sf::Texture m_PinnedTexture;
        sf::Texture m_CalibrationTextureA;
        sf::Texture m_CalibrationTextureB;
        std::vector<sf::Texture> m_AnimationTextures;
        std::vector<sf::Texture> m_GenerateIdleAnimationTextures;
        uint32_t m_GenerateIdleFrameIndex = 0u;
        double m_GenerateIdlePlaybackAccumulatorMicroseconds = 0.0;
        sf::Sprite m_PreviewSprite;
        std::string m_StartupRecipePath;
        std::string m_StatusMessage;
        std::optional<uint32_t> m_PendingFavoriteRemovalIndex;
    };
}
