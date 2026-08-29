#include "ShipGeneratorPreviewApp.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>

#include "SFMLImageAdapter.h"
#include "GenerationCalibration.h"
#include "GenerationCalibrationSerializer.h"
#include "PreviewCommand.h"
#include "PreviewPreferences.h"
#include "ShipGenerationRecipeSerializer.h"
#include "PreviewThumbnailGrid.h"
#include "ShipGenerationSeeds.h"
#include "ShipGenerationSettings.h"
#include "ShipSpritesheetUtils.h"

namespace
{
    constexpr std::size_t MaximumHistorySize = 20u;
    constexpr uint32_t GalleryCandidateMaximumAttempts = 16u;
    const std::filesystem::path PreviewPreferencesPath = "pixel_ship_generator_preview_preferences.json";
    const std::filesystem::path CalibrationSessionPath = "generation_calibration_session.json";
    const std::filesystem::path CalibrationReportPath = "generation_calibration_report.csv";
    const std::filesystem::path CalibrationTuningProfilePath = "generation_tuning_profile.json";


    template <typename T, std::size_t Size>
    std::size_t findOrderedValueIndex(const std::array<T, Size>& values, const T& value)
    {
        const auto iterator = std::find(values.begin(), values.end(), value);
        return iterator == values.end() ? 0u : static_cast<std::size_t>(std::distance(values.begin(), iterator));
    }

    std::size_t getWrappedIndex(std::size_t currentIndex, int32_t delta, std::size_t valueCount)
    {
        if (valueCount == 0u)
        {
            return 0u;
        }

        const int32_t count = static_cast<int32_t>(valueCount);
        int32_t index = static_cast<int32_t>(currentIndex) + delta;
        while (index < 0) { index += count; }
        while (index >= count) { index -= count; }
        return static_cast<std::size_t>(index);
    }

    uint32_t calculateDisplayScale(uint32_t imageWidth, uint32_t imageHeight, uint32_t availableWidth, uint32_t availableHeight)
    {
        if (imageWidth == 0u || imageHeight == 0u)
        {
            return 1u;
        }

        const uint32_t horizontalScale = availableWidth / imageWidth;
        const uint32_t verticalScale = availableHeight / imageHeight;

        return std::max(1u, std::min(horizontalScale, verticalScale));
    }

    PixelShipGeneratorPreview::PreviewGenerationRecipe createGalleryCandidateRecipe(const PixelShipGeneratorPreview::PreviewGenerationRecipe& templateRecipe, uint64_t masterSeed)
    {
        PixelShipGeneratorPreview::PreviewGenerationRecipe recipe = templateRecipe;
        recipe.Seeds = PixelShipGenerator::deriveShipGenerationSeeds(masterSeed);
        recipe.DomainSeedOverrides.clearAll();
        recipe.RandomStreamMode = PixelShipGenerator::GenerationRandomStreamMode::DOMAIN_SUBSTREAMS;
        return recipe;
    }

    PixelShipGeneratorPreview::PreviewGenerationRecipe createRecipeFromMasterSeed(uint64_t masterSeed, const PixelShipGeneratorPreview::PreviewGenerationRecipe& currentRecipe)
    {
        PixelShipGeneratorPreview::PreviewGenerationRecipe recipe = currentRecipe;
        recipe.Seeds = PixelShipGenerator::deriveShipGenerationSeeds(masterSeed);
        recipe.DomainSeedOverrides.clearAll();
        recipe.RandomStreamMode = PixelShipGenerator::GenerationRandomStreamMode::DOMAIN_SUBSTREAMS;
        return recipe;
    }

    std::mt19937_64 createSeedGenerator()
    {
        std::random_device randomDevice;
        std::seed_seq seedSequence{ randomDevice(), randomDevice(), randomDevice(), randomDevice() };
        return std::mt19937_64(seedSequence);
    }

    uint64_t deriveGalleryCandidateMasterSeed(uint64_t batchSeed, uint32_t candidateIndex, uint32_t attempt = 0u)
    {
        const uint64_t indexValue = static_cast<uint64_t>(candidateIndex) * 0x9E3779B97F4A7C15ull;
        const uint64_t attemptValue = static_cast<uint64_t>(attempt) * 0xBF58476D1CE4E5B9ull;
        return PixelShipGenerator::mixGenerationSeed64(batchSeed ^ 0xA24BAED4963EE407ull ^ indexValue ^ attemptValue);
    }


    std::string getFactionName(PixelShipGenerator::ShipFactionType faction)
    {
        switch (faction)
        {
        case PixelShipGenerator::ShipFactionType::FRONTIER: return "frontier";
        case PixelShipGenerator::ShipFactionType::MILITARY: return "military";
        case PixelShipGenerator::ShipFactionType::ASCENDANT: return "ascendant";
        case PixelShipGenerator::ShipFactionType::XENO: return "xeno";
        case PixelShipGenerator::ShipFactionType::CORPORATE: return "corporate";
        case PixelShipGenerator::ShipFactionType::RELIC: return "relic";
        default: return "unknown";
        }
    }

    std::string getFactionDisplayName(PixelShipGenerator::ShipFactionType faction)
    {
        switch (faction)
        {
        case PixelShipGenerator::ShipFactionType::FRONTIER: return "FRONTIER";
        case PixelShipGenerator::ShipFactionType::MILITARY: return "MILITARY";
        case PixelShipGenerator::ShipFactionType::ASCENDANT: return "ASCENDANT";
        case PixelShipGenerator::ShipFactionType::XENO: return "XENO";
        case PixelShipGenerator::ShipFactionType::CORPORATE: return "CORPORATE";
        case PixelShipGenerator::ShipFactionType::RELIC: return "RELIC";
        default: return "UNKNOWN";
        }
    }

    std::string getLockDisplay(bool locked)
    {
        return locked ? "LOCK" : "OPEN";
    }

    std::string getResolutionString(const PixelShipGeneratorPreview::PreviewGenerationRecipe& recipe)
    {
        if (recipe.Dimensions.Width == recipe.Dimensions.Height)
        {
            return std::to_string(recipe.Dimensions.Width);
        }

        return std::to_string(recipe.Dimensions.Width) + "x" + std::to_string(recipe.Dimensions.Height);
    }

    std::string getStyleName(PixelShipGenerator::ShipStyle style)
    {
        switch (style)
        {
        case PixelShipGenerator::ShipStyle::SLEEK: return "SLEEK";
        case PixelShipGenerator::ShipStyle::FIGHTER: return "FIGHTER";
        case PixelShipGenerator::ShipStyle::HEAVY: return "HEAVY";
        case PixelShipGenerator::ShipStyle::INDUSTRIAL: return "INDUSTRIAL";
        case PixelShipGenerator::ShipStyle::SPEARHEAD: return "SPEARHEAD";
        case PixelShipGenerator::ShipStyle::DELTA: return "DELTA";
        default: return "UNKNOWN";
        }
    }

    std::string getAnimationTypeDisplayName(PixelShipGenerator::ShipAnimationType type)
    {
        switch (type)
        {
        case PixelShipGenerator::ShipAnimationType::IDLE: return "IDLE";
        case PixelShipGenerator::ShipAnimationType::MOVE_LEFT: return "MOVE LEFT";
        case PixelShipGenerator::ShipAnimationType::MOVE_RIGHT: return "MOVE RIGHT";
        case PixelShipGenerator::ShipAnimationType::MOVE_UP: return "MOVE UP";
        case PixelShipGenerator::ShipAnimationType::MOVE_DOWN: return "MOVE DOWN";
        case PixelShipGenerator::ShipAnimationType::FIRE: return "FIRE";
        default: return "UNSUPPORTED";
        }
    }

    bool isMovementAnimationType(PixelShipGenerator::ShipAnimationType type)
    {
        return type == PixelShipGenerator::ShipAnimationType::MOVE_LEFT || type == PixelShipGenerator::ShipAnimationType::MOVE_RIGHT || type == PixelShipGenerator::ShipAnimationType::MOVE_UP || type == PixelShipGenerator::ShipAnimationType::MOVE_DOWN;
    }

    double wrapNormalizedAnimationTime(double normalizedTime)
    {
        double wrapped = normalizedTime - std::floor(normalizedTime);
        if (wrapped < 0.0) { wrapped += 1.0; }
        return wrapped;
    }

    std::string getAnimationTypeFileToken(PixelShipGenerator::ShipAnimationType type)
    {
        switch (type)
        {
        case PixelShipGenerator::ShipAnimationType::IDLE: return "idle";
        case PixelShipGenerator::ShipAnimationType::MOVE_LEFT: return "move_left";
        case PixelShipGenerator::ShipAnimationType::MOVE_RIGHT: return "move_right";
        case PixelShipGenerator::ShipAnimationType::MOVE_UP: return "move_up";
        case PixelShipGenerator::ShipAnimationType::MOVE_DOWN: return "move_down";
        case PixelShipGenerator::ShipAnimationType::FIRE: return "fire";
        default: return "animation";
        }
    }

    std::string getMovementPhaseDisplayName(PixelShipGenerator::ShipMovementAnimationPhase phase)
    {
        switch (phase)
        {
        case PixelShipGenerator::ShipMovementAnimationPhase::ENTER: return "ENTER";
        case PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN: return "SUSTAIN";
        case PixelShipGenerator::ShipMovementAnimationPhase::EXIT: return "EXIT";
        default: return "UNKNOWN";
        }
    }

    std::string getMovementPhaseFileToken(PixelShipGenerator::ShipMovementAnimationPhase phase)
    {
        switch (phase)
        {
        case PixelShipGenerator::ShipMovementAnimationPhase::ENTER: return "enter";
        case PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN: return "sustain";
        case PixelShipGenerator::ShipMovementAnimationPhase::EXIT: return "exit";
        default: return "phase";
        }
    }

    std::string getWeaponTypeDisplayName(PixelShipGenerator::ShipWeaponType type)
    {
        switch (type)
        {
        case PixelShipGenerator::ShipWeaponType::SINGLE_CANNON: return "SINGLE CANNON";
        case PixelShipGenerator::ShipWeaponType::TWIN_CANNON: return "TWIN CANNON";
        case PixelShipGenerator::ShipWeaponType::COMPACT_TURRET: return "COMPACT TURRET";
        case PixelShipGenerator::ShipWeaponType::RAIL_WEAPON: return "RAIL WEAPON";
        case PixelShipGenerator::ShipWeaponType::WEAPON_POD: return "WEAPON POD";
        default: return "WEAPON";
        }
    }

    std::string getSaveBaseName(const PixelShipGeneratorPreview::PreviewGenerationRecipe& recipe)
    {
        return "ship_" + getResolutionString(recipe) + "_" + getStyleName(recipe.Style) + "_" + getFactionName(recipe.Faction) + "_seed_" + std::to_string(recipe.Seeds.Master);
    }

    std::filesystem::path getAvailableOutputPath(const std::string& baseName, const std::string& extension)
    {
        std::filesystem::path path = baseName + extension;
        uint32_t suffix = 1u;
        while (std::filesystem::exists(path))
        {
            path = baseName + "_" + std::to_string(suffix) + extension;
            ++suffix;
        }
        return path;
    }

    std::filesystem::path getAvailableSavePath(const std::string& baseName)
    {
        return getAvailableOutputPath(baseName, ".png");
    }

    std::filesystem::path getAvailableRecipePath(const std::string& baseName)
    {
        return getAvailableOutputPath(baseName, ".shipgen.json");
    }

    std::string getFrameNumberString(uint32_t frameIndex)
    {
        return frameIndex + 1u < 10u ? "0" + std::to_string(frameIndex + 1u) : std::to_string(frameIndex + 1u);
    }

    bool saveCoreImage(const PixelShipGenerator::Image& image, const std::filesystem::path& path)
    {
        const sf::Image sfmlImage = PixelShipGenerator::SFMLImageAdapter::createSFMLImage(image);

        if (sfmlImage.saveToFile(path.string()))
        {
            std::cout << "Saved: " << path.string() << '\n';
            return true;
        }

        std::cerr << "Failed to save: " << path.string() << '\n';
        return false;
    }


    bool readPathFromConsole(std::filesystem::path& path)
    {
        std::cout << "Enter .shipgen.json path: ";
        std::string input;
        std::getline(std::cin >> std::ws, input);
        if (input.empty())
        {
            std::cerr << "Recipe path cannot be empty.\n";
            return false;
        }
        path = std::filesystem::path(input);
        return true;
    }

    bool isSupportedPreviewRecipeResolution(const PixelShipGeneratorPreview::PreviewGenerationRecipe& recipe)
    {
        return PixelShipGeneratorPreview::isSelectablePreviewDimensions(recipe.Dimensions);
    }

    bool dimensionsLess(const PixelShipGenerator::ShipDimensions& first, const PixelShipGenerator::ShipDimensions& second)
    {
        if (first.Width != second.Width) { return first.Width < second.Width; }
        return first.Height < second.Height;
    }
    bool readSeedFromConsole(uint64_t& seed)
    {
        std::cout << "Enter uint64_t seed: ";

        std::string input;
        std::getline(std::cin >> std::ws, input);

        if (input.empty() || input.front() == '-')
        {
            std::cerr << "Invalid seed.\n";
            return false;
        }

        try
        {
            std::size_t parsedCharacters = 0u;
            const unsigned long long parsedSeed = std::stoull(input, &parsedCharacters, 10);

            if (parsedCharacters != input.size())
            {
                std::cerr << "Invalid seed.\n";
                return false;
            }

            seed = static_cast<uint64_t>(parsedSeed);
            return true;
        }
        catch (const std::invalid_argument&)
        {
            std::cerr << "Invalid seed.\n";
        }
        catch (const std::out_of_range&)
        {
            std::cerr << "Seed is outside the uint64_t range.\n";
        }

        return false;
    }
}

namespace PixelShipGeneratorPreview
{
    ShipGeneratorPreviewApp::ShipGeneratorPreviewApp(std::string startupRecipePath)
        : m_Window(sf::VideoMode(PreviewWindowWidth, PreviewWindowHeight), "Pixel Ship Generator", sf::Style::Titlebar | sf::Style::Close), m_SeedGenerator(createSeedGenerator()), m_StartupRecipePath(std::move(startupRecipePath))
    {
        m_Window.setVerticalSyncEnabled(true);
        m_Window.setKeyRepeatEnabled(false);

        PreviewGenerationRecipe initialRecipe;
        initialRecipe.Seeds = PixelShipGenerator::deriveShipGenerationSeeds(m_SeedGenerator());
        m_History.push_back(initialRecipe);
        m_CalibrationSession = createGenerationCalibrationSession(m_SeedGenerator());
        loadPreviewAppPreferences();
    }

    int ShipGeneratorPreviewApp::run()
    {
        printControls();
        regenerate();
        if (!m_StartupRecipePath.empty()) { importRecipeFromPath(m_StartupRecipePath); }
        updateCommandPanelState();

        while (m_Window.isOpen())
        {
            processEvents();
            update();
            render();
        }

        return 0;
    }

    void ShipGeneratorPreviewApp::addCurrentToFavorites()
    {
        if (isCurrentFavorite()) { return; }

        PreviewThumbnailItem favorite;
        favorite.Recipe = getCurrentRecipe();
        const sf::Image favoriteImage = PixelShipGenerator::SFMLImageAdapter::createSFMLImage(m_GeneratedShip.FinalImage);

        if (!favorite.Texture.loadFromImage(favoriteImage))
        {
            std::cerr << "Failed to create Favorite thumbnail texture.\n";
            return;
        }

        favorite.Texture.setSmooth(false);
        favorite.Valid = true;
        m_FavoritesState.Grid.Items.push_back(std::move(favorite));
        m_FavoritesState.Grid.SelectedIndex = static_cast<uint32_t>(m_FavoritesState.Grid.Items.size() - 1u);
        m_FavoritesState.Grid.HoveredIndex = -1;
        updateWindowTitle();
        std::cout << "Added Favorite: seed " << getCurrentRecipe().Seeds.Master << '\n';
    }

    void ShipGeneratorPreviewApp::addResolutionBookmark()
    {
        const PixelShipGenerator::ShipDimensions dimensions = getCurrentRecipe().Dimensions;
        if (!isSelectablePreviewDimensions(dimensions) || m_ResolutionBookmarks.size() >= MaximumResolutionBookmarks) { return; }
        const auto iterator = std::lower_bound(m_ResolutionBookmarks.begin(), m_ResolutionBookmarks.end(), dimensions, dimensionsLess);
        if (iterator != m_ResolutionBookmarks.end() && *iterator == dimensions) { return; }
        m_ResolutionBookmarks.insert(iterator, dimensions);
        savePreviewAppPreferences();
        setStatusMessage("Resolution bookmark added: " + std::to_string(dimensions.Width) + "x" + std::to_string(dimensions.Height));
    }

    void ShipGeneratorPreviewApp::appendHistoryEntry(const PreviewGenerationRecipe& recipe)
    {
        if (m_HistoryIndex + 1u < m_History.size())
        {
            m_History.erase(m_History.begin() + static_cast<std::ptrdiff_t>(m_HistoryIndex + 1u), m_History.end());
        }

        m_History.push_back(recipe);

        if (m_History.size() > MaximumHistorySize)
        {
            m_History.erase(m_History.begin());
        }

        m_HistoryIndex = m_History.size() - 1u;
        regenerate();
    }

    void ShipGeneratorPreviewApp::changeFaction(int32_t delta)
    {
        const std::size_t currentIndex = findOrderedValueIndex(SupportedPreviewFactions, getCurrentRecipe().Faction);
        setFaction(SupportedPreviewFactions[getWrappedIndex(currentIndex, delta, SupportedPreviewFactions.size())]);
    }

    void ShipGeneratorPreviewApp::changeResolution(int32_t delta)
    {
        const uint32_t current = getCurrentRecipe().Dimensions.Width;
        uint32_t resolution = current;

        if (delta < 0)
        {
            const auto iterator = std::lower_bound(DirectPreviewResolutions.begin(), DirectPreviewResolutions.end(), current);
            resolution = iterator == DirectPreviewResolutions.begin() ? DirectPreviewResolutions.back() : *(iterator - 1);
        }
        else if (delta > 0)
        {
            const auto iterator = std::upper_bound(DirectPreviewResolutions.begin(), DirectPreviewResolutions.end(), current);
            resolution = iterator == DirectPreviewResolutions.end() ? DirectPreviewResolutions.front() : *iterator;
        }

        setResolution(resolution);
    }

    void ShipGeneratorPreviewApp::changeStyle(int32_t delta)
    {
        const std::size_t currentIndex = findOrderedValueIndex(SupportedPreviewStyles, getCurrentRecipe().Style);
        setStyle(SupportedPreviewStyles[getWrappedIndex(currentIndex, delta, SupportedPreviewStyles.size())]);
    }

    void ShipGeneratorPreviewApp::clearPinnedShip()
    {
        m_Comparison.Pinned.Valid = false;
        m_Comparison.ViewEnabled = false;
        m_Comparison.Pinned.Ship.clear();
        updateWindowTitle();
        std::cout << "Pinned comparison reference cleared.\n";
    }

    bool ShipGeneratorPreviewApp::buildGallery(uint64_t batchSeed)
    {
        m_GalleryState.BatchSeed = batchSeed;
        m_GalleryState.TemplateRecipe = getCurrentRecipe();
        m_GalleryState.Grid.SelectedIndex = 0u;
        m_GalleryState.Grid.HoveredIndex = -1;
        m_GalleryState.Grid.Items.clear();
        m_GalleryState.Grid.Items.reserve(m_GalleryState.CandidateCount);

        for (uint32_t index = 0u; index < m_GalleryState.CandidateCount; ++index)
        {
            PreviewThumbnailItem candidate;

            for (uint32_t attempt = 0u; attempt < GalleryCandidateMaximumAttempts; ++attempt)
            {
                const uint64_t masterSeed = deriveGalleryCandidateMasterSeed(batchSeed, index, attempt);
                const PreviewGenerationRecipe recipe = createGalleryCandidateRecipe(m_GalleryState.TemplateRecipe, masterSeed);
                PixelShipGenerator::GeneratedShip ship;

                if (!generateShipFromRecipe(recipe, ship)) { continue; }
                const sf::Image candidateImage = PixelShipGenerator::SFMLImageAdapter::createSFMLImage(ship.FinalImage);
                if (!candidate.Texture.loadFromImage(candidateImage)) { continue; }

                candidate.Texture.setSmooth(false);
                candidate.Recipe = recipe;
                candidate.Valid = true;
                break;
            }

            m_GalleryState.Grid.Items.push_back(std::move(candidate));
        }

        m_PreviewMode = PreviewMode::GALLERY;
        updateWindowTitle();
        std::cout << "Gallery BatchSeed: " << m_GalleryState.BatchSeed << '\n';
        std::cout << "Gallery candidates: " << m_GalleryState.CandidateCount << '\n';
        std::cout << '\n';
        return true;
    }


    PixelShipGenerator::Image ShipGeneratorPreviewApp::createDiagnosticImage() const
    {
        const uint32_t width = m_GeneratedShip.HullMask.getWidth();
        const uint32_t height = m_GeneratedShip.HullMask.getHeight();
        PixelShipGenerator::Image image;
        image.reset(width, height);
        image.clear(PixelShipGenerator::Color(0u, 0u, 0u, 0u));

        const PixelShipGenerator::Color& hullColor = PreviewDiagnosticColors::Hull;
        const PixelShipGenerator::Color& cockpitColor = PreviewDiagnosticColors::Cockpit;
        const PixelShipGenerator::Color& engineColor = PreviewDiagnosticColors::Engine;
        const PixelShipGenerator::Color& exhaustColor = PreviewDiagnosticColors::Exhaust;
        const PixelShipGenerator::Color& accentColor = PreviewDiagnosticColors::Accent;
        const PixelShipGenerator::Color& mechanicalColor = PreviewDiagnosticColors::Mechanical;
        const PixelShipGenerator::Color& lightColor = PreviewDiagnosticColors::Light;
        const PixelShipGenerator::Color& attachmentColor = PreviewDiagnosticColors::Attachment;
        const PixelShipGenerator::Color& overlapColor = PreviewDiagnosticColors::Overlap;

        if (m_Diagnostics.GenerationStageView && !m_GenerationDebugInfo.HullStages.empty())
        {
            const uint32_t stageIndex = std::min(m_Diagnostics.GenerationStageIndex, static_cast<uint32_t>(m_GenerationDebugInfo.HullStages.size() - 1u));
            const PixelShipGenerator::PixelMask& stageMask = m_GenerationDebugInfo.HullStages[stageIndex].HullMask;

            for (uint32_t y = 0u; y < height; ++y)
            {
                for (uint32_t x = 0u; x < width; ++x)
                {
                    if (stageMask.get(x, y)) { image.setPixel(x, y, hullColor); }
                }
            }

            return image;
        }

        if (m_Diagnostics.ViewMode == DiagnosticViewMode::FINAL)
        {
            return m_GeneratedShip.FinalImage;
        }

        if (m_Diagnostics.ViewMode == DiagnosticViewMode::HULL_LAYERS)
        {
            if (m_GenerationDebugInfo.HullLayerMask.getWidth() != width || m_GenerationDebugInfo.HullLayerMask.getHeight() != height)
            {
                return image;
            }

            for (uint32_t y = 0u; y < height; ++y)
            {
                for (uint32_t x = 0u; x < width; ++x)
                {
                    if (!m_GenerationDebugInfo.HullLayerMask.get(x, y)) { continue; }
                    image.setPixel(x, y, m_GenerationDebugInfo.HullLayerUpperMask.get(x, y) ? PreviewDiagnosticColors::HullLayerUpper : PreviewDiagnosticColors::HullLayerLower);
                }
            }
            return image;
        }

        if (m_Diagnostics.ViewMode == DiagnosticViewMode::CORE_TREATMENT)
        {
            if (m_GenerationDebugInfo.CoreRegionMask.getWidth() != width || m_GenerationDebugInfo.CoreRegionMask.getHeight() != height)
            {
                return image;
            }

            for (uint32_t y = 0u; y < height; ++y)
            {
                for (uint32_t x = 0u; x < width; ++x)
                {
                    if (m_GenerationDebugInfo.CoreRegionMask.get(x, y)) { image.setPixel(x, y, PreviewDiagnosticColors::CoreRegion); }
                    if (m_GenerationDebugInfo.CoreSecondaryMaterialMask.get(x, y)) { image.setPixel(x, y, PreviewDiagnosticColors::CoreSecondary); }
                    if (m_GenerationDebugInfo.CoreRecessedMask.get(x, y)) { image.setPixel(x, y, PreviewDiagnosticColors::CoreRecessed); }
                    if (m_GenerationDebugInfo.CoreRaisedMask.get(x, y)) { image.setPixel(x, y, PreviewDiagnosticColors::CoreRaised); }
                    if (m_GenerationDebugInfo.CoreLuminousMask.get(x, y)) { image.setPixel(x, y, PreviewDiagnosticColors::CoreLuminous); }
                }
            }
            return image;
        }

        if (m_Diagnostics.ViewMode == DiagnosticViewMode::SEMANTIC_LOAD)
        {
            if (m_GenerationDebugInfo.SpatialRegionMapWidth != width || m_GenerationDebugInfo.SpatialRegionMapHeight != height || m_GenerationDebugInfo.SpatialRegionMap.size() != static_cast<std::size_t>(width) * height)
            {
                return image;
            }

            for (uint32_t y = 0u; y < height; ++y)
            {
                for (uint32_t x = 0u; x < width; ++x)
                {
                    const uint8_t regionValue = m_GenerationDebugInfo.SpatialRegionMap[static_cast<std::size_t>(y) * width + x];
                    if (regionValue >= static_cast<uint8_t>(PixelShipGenerator::GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END)) { continue; }
                    const std::size_t regionIndex = static_cast<std::size_t>(regionValue);
                    const uint32_t capacity = m_GenerationDebugInfo.SpatialRegionCapacities[regionIndex];
                    const uint32_t utilization = capacity == 0u ? 0u : (m_GenerationDebugInfo.SpatialRegionLoads[regionIndex] * 100u) / capacity;
                    if (utilization < 40u) { image.setPixel(x, y, PreviewDiagnosticColors::SpatialLow); }
                    else if (utilization < 75u) { image.setPixel(x, y, PreviewDiagnosticColors::SpatialModerate); }
                    else if (utilization < 100u) { image.setPixel(x, y, PreviewDiagnosticColors::SpatialHigh); }
                    else { image.setPixel(x, y, PreviewDiagnosticColors::SpatialOverloaded); }
                }
            }
            return image;
        }

        if (m_Diagnostics.ViewMode == DiagnosticViewMode::MACRO_ASYMMETRY)
        {
            for (uint32_t y = 0u; y < height; ++y)
            {
                for (uint32_t x = 0u; x < width; ++x)
                {
                    if (m_GeneratedShip.HullMask.get(x, y)) { image.setPixel(x, y, PreviewDiagnosticColors::MacroAsymmetryBase); }
                    if (m_GenerationDebugInfo.MacroAsymmetryMask.getWidth() == width && m_GenerationDebugInfo.MacroAsymmetryMask.getHeight() == height && m_GenerationDebugInfo.MacroAsymmetryMask.get(x, y))
                    {
                        image.setPixel(x, y, PreviewDiagnosticColors::MacroAsymmetryFeature);
                    }
                }
            }
            return image;
        }

        for (uint32_t y = 0u; y < height; ++y)
        {
            for (uint32_t x = 0u; x < width; ++x)
            {
                const bool hull = m_GeneratedShip.HullMask.get(x, y);
                const bool cockpit = m_GeneratedShip.CockpitMask.get(x, y);
                const bool engine = m_GeneratedShip.EngineMask.get(x, y);
                const bool exhaust = m_GeneratedShip.EngineExhaustMask.get(x, y);
                const bool accent = m_GeneratedShip.AccentMask.get(x, y);
                const bool mechanical = m_GeneratedShip.MechanicalDetailMask.get(x, y);
                const bool light = m_GeneratedShip.LightMask.get(x, y);
                const bool attachment = m_GeneratedShip.AttachmentMask.get(x, y);
                const bool details = accent || mechanical || light;

                switch (m_Diagnostics.ViewMode)
                {
                case DiagnosticViewMode::HULL:
                    if (hull) { image.setPixel(x, y, hullColor); }
                    break;
                case DiagnosticViewMode::COCKPIT:
                    if (cockpit) { image.setPixel(x, y, cockpitColor); }
                    break;
                case DiagnosticViewMode::ENGINES:
                    if (exhaust) { image.setPixel(x, y, exhaustColor); }
                    else if (engine) { image.setPixel(x, y, engineColor); }
                    break;
                case DiagnosticViewMode::DETAILS:
                    if (light) { image.setPixel(x, y, lightColor); }
                    else if (mechanical) { image.setPixel(x, y, mechanicalColor); }
                    else if (accent) { image.setPixel(x, y, accentColor); }
                    break;
                case DiagnosticViewMode::ATTACHMENTS:
                    if (attachment) { image.setPixel(x, y, attachmentColor); }
                    break;
                case DiagnosticViewMode::HULL_LAYERS:
                    break;
                case DiagnosticViewMode::CORE_TREATMENT:
                    break;
                case DiagnosticViewMode::SEMANTIC_LOAD:
                    break;
                case DiagnosticViewMode::MACRO_ASYMMETRY:
                    break;
                case DiagnosticViewMode::COMBINED:
                {
                    const uint32_t nonHullCategoryCount = static_cast<uint32_t>(cockpit) + static_cast<uint32_t>(engine || exhaust) + static_cast<uint32_t>(details) + static_cast<uint32_t>(attachment);
                    if (nonHullCategoryCount > 1u) { image.setPixel(x, y, overlapColor); }
                    else if (light) { image.setPixel(x, y, lightColor); }
                    else if (mechanical) { image.setPixel(x, y, mechanicalColor); }
                    else if (accent) { image.setPixel(x, y, accentColor); }
                    else if (cockpit) { image.setPixel(x, y, cockpitColor); }
                    else if (exhaust) { image.setPixel(x, y, exhaustColor); }
                    else if (engine) { image.setPixel(x, y, engineColor); }
                    else if (attachment) { image.setPixel(x, y, attachmentColor); }
                    else if (hull) { image.setPixel(x, y, hullColor); }
                    break;
                }
                default: break;
                }
            }
        }

        return image;
    }

    void ShipGeneratorPreviewApp::cycleAnimationType()
    {
        m_TransientStatePreviewActive = false;
        m_StatePreviewFrames.clear();

        constexpr std::array<PixelShipGenerator::ShipAnimationType, 6u> Types =
        {
            PixelShipGenerator::ShipAnimationType::IDLE,
            PixelShipGenerator::ShipAnimationType::MOVE_LEFT,
            PixelShipGenerator::ShipAnimationType::MOVE_RIGHT,
            PixelShipGenerator::ShipAnimationType::MOVE_UP,
            PixelShipGenerator::ShipAnimationType::MOVE_DOWN,
            PixelShipGenerator::ShipAnimationType::FIRE
        };

        const auto iterator = std::find(Types.begin(), Types.end(), m_SelectedAnimationType);
        const std::size_t currentIndex = iterator == Types.end() ? 0u : static_cast<std::size_t>(std::distance(Types.begin(), iterator));
        m_SelectedAnimationType = Types[(currentIndex + 1u) % Types.size()];
        m_MovementAnimationPhase = PixelShipGenerator::ShipMovementAnimationPhase::ENTER;
        m_AnimationFrameIndex = 0u;
        if (!regenerateAnimation()) { return; }

        if (m_PreviewMode == PreviewMode::ANIMATION || m_PreviewMode == PreviewMode::FRAME_INSPECTION) { setDisplayedAnimationFrame(0u); }
        else { setDisplayedStaticFrame(); }
        setStatusMessage("Animation type: " + getAnimationTypeDisplayName(m_SelectedAnimationType));
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::cycleMovementPhase()
    {
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE || m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE) { return; }

        const uint32_t phaseCount = static_cast<uint32_t>(PixelShipGenerator::ShipMovementAnimationPhase::SHIP_MOVEMENT_ANIMATION_PHASE_END);
        const uint32_t nextPhase = (static_cast<uint32_t>(m_MovementAnimationPhase) + 1u) % phaseCount;
        m_MovementAnimationPhase = static_cast<PixelShipGenerator::ShipMovementAnimationPhase>(nextPhase);
        m_AnimationFrameIndex = 0u;
        m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
        m_AnimationClock.restart();
        if (!refreshAnimationTextures()) { return; }

        if (m_PreviewMode == PreviewMode::ANIMATION || m_PreviewMode == PreviewMode::FRAME_INSPECTION) { setDisplayedAnimationFrame(0u); }
        setStatusMessage("Movement phase: " + getMovementPhaseDisplayName(m_MovementAnimationPhase));
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::cycleFiringTarget()
    {
        if (m_SelectedAnimationType != PixelShipGenerator::ShipAnimationType::FIRE || m_FiringTargets.empty()) { return; }
        m_SelectedFiringTargetIndex = (m_SelectedFiringTargetIndex + 1u) % static_cast<uint32_t>(m_FiringTargets.size());
        m_AnimationFrameIndex = 0u;
        m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
        m_AnimationClock.restart();
        if (m_TransientStatePreviewActive)
        {
            if (!beginComposedFiringEvent()) { return; }
        }
        else
        {
            if (!regenerateAnimation()) { return; }
            if (m_PreviewMode == PreviewMode::ANIMATION || m_PreviewMode == PreviewMode::FRAME_INSPECTION) { setDisplayedAnimationFrame(0u); }
        }
        if (m_FiringAnimation.Diagnostics.ValidTarget && !m_FiringAnimation.Diagnostics.Weapons.empty())
        {
            const auto& weapon = m_FiringAnimation.Diagnostics.Weapons.front();
            setStatusMessage("Firing target: " + getWeaponTypeDisplayName(weapon.Type) + " component " + std::to_string(m_FiringAnimation.Target.WeaponComponentIndex));
        }
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::applySelectedAnimationState()
    {
        m_TransientStatePreviewActive = false;
        m_StatePreviewFrames.clear();

        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE)
        {
            beginComposedFiringEvent();
            return;
        }

        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE)
        {
            returnAnimationToIdle();
            return;
        }

        if (!isMovementAnimationType(m_SelectedAnimationType)) { return; }
        const PixelShipGenerator::ShipAnimationType target = m_SelectedAnimationType;
        if (m_RuntimeMovementType == target)
        {
            m_MovementTransitionPending = false;
            m_PendingMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
            m_MovementAnimationPhase = PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN;
            if (!regenerateAnimation()) { return; }
            const auto* clip = getActiveMovementClip();
            if (clip != nullptr && !clip->Frames.empty())
            {
                const uint32_t index = static_cast<uint32_t>(std::floor(wrapNormalizedAnimationTime(m_RuntimeMovementNormalizedTime) * static_cast<double>(clip->Frames.size()))) % static_cast<uint32_t>(clip->Frames.size());
                m_AnimationFrameIndex = index;
            }
            enterAnimationPlayback();
            setStatusMessage("State: " + getAnimationTypeDisplayName(target) + " SUSTAIN");
            return;
        }

        if (m_RuntimeMovementType == PixelShipGenerator::ShipAnimationType::IDLE)
        {
            m_RuntimeMovementType = target;
            m_MovementTransitionPending = false;
            m_PendingMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
            m_MovementAnimationPhase = PixelShipGenerator::ShipMovementAnimationPhase::ENTER;
            m_RuntimeMovementNormalizedTime = 0.0;
            if (!regenerateAnimation()) { return; }
            enterAnimationPlayback();
            setStatusMessage("State transition: IDLE -> " + getAnimationTypeDisplayName(target));
            return;
        }

        const PixelShipGenerator::ShipAnimationType current = m_RuntimeMovementType;
        const PixelShipGenerator::ShipMovementTransitionPlan plan = m_AnimationStateCoordinator.planMovementTransition(current, target, m_MovementAnimationSettings);
        m_PendingMovementType = target;
        m_MovementTransitionPending = plan.ExitCurrentMovement && plan.EnterTargetMovement;
        m_SelectedAnimationType = current;
        m_MovementAnimationPhase = PixelShipGenerator::ShipMovementAnimationPhase::EXIT;
        if (!regenerateAnimation()) { return; }
        enterAnimationPlayback();
        setStatusMessage("State transition: " + getAnimationTypeDisplayName(current) + " -> NEUTRAL -> " + getAnimationTypeDisplayName(target));
    }

    void ShipGeneratorPreviewApp::returnAnimationToIdle()
    {
        m_TransientStatePreviewActive = false;
        m_StatePreviewFrames.clear();
        if (m_RuntimeMovementType == PixelShipGenerator::ShipAnimationType::IDLE)
        {
            m_SelectedAnimationType = PixelShipGenerator::ShipAnimationType::IDLE;
            m_MovementTransitionPending = false;
            m_PendingMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
            if (!regenerateAnimation()) { return; }
            enterAnimationPlayback();
            setStatusMessage("State: IDLE");
            return;
        }

        const PixelShipGenerator::ShipAnimationType current = m_RuntimeMovementType;
        const PixelShipGenerator::ShipMovementTransitionPlan plan = m_AnimationStateCoordinator.planMovementTransition(current, PixelShipGenerator::ShipAnimationType::IDLE, m_MovementAnimationSettings);
        m_PendingMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
        m_MovementTransitionPending = plan.ExitCurrentMovement;
        m_SelectedAnimationType = current;
        m_MovementAnimationPhase = PixelShipGenerator::ShipMovementAnimationPhase::EXIT;
        if (!regenerateAnimation()) { return; }
        enterAnimationPlayback();
        setStatusMessage("State transition: " + getAnimationTypeDisplayName(current) + " -> IDLE");
    }

    bool ShipGeneratorPreviewApp::beginComposedFiringEvent()
    {
        m_FiringTargets = m_FiringAnimator.getAvailableTargets(m_GeneratedShip);
        if (m_FiringTargets.empty())
        {
            setStatusMessage("FIRE unavailable: generated ship has no movable weapon component.");
            return false;
        }

        m_SelectedFiringTargetIndex %= static_cast<uint32_t>(m_FiringTargets.size());
        const PixelShipGenerator::ShipFiringAnimationTarget target = m_FiringTargets[m_SelectedFiringTargetIndex];
        m_FiringAnimation = m_FiringAnimator.generate(m_GeneratedShip, target, m_FiringAnimationSettings);
        if (m_FiringAnimation.Frames.empty()) { return false; }

        PixelShipGenerator::ShipMovementAnimation movement;
        uint32_t sustainDurationMilliseconds = 0u;
        if (m_RuntimeMovementType != PixelShipGenerator::ShipAnimationType::IDLE)
        {
            if (m_RuntimeMovementType == PixelShipGenerator::ShipAnimationType::MOVE_LEFT || m_RuntimeMovementType == PixelShipGenerator::ShipAnimationType::MOVE_RIGHT)
            {
                movement = m_LateralMovementAnimator.generate(m_GeneratedShip, m_RuntimeMovementType, m_MovementAnimationSettings);
            }
            else
            {
                movement = m_LongitudinalMovementAnimator.generate(m_GeneratedShip, m_RuntimeMovementType, m_MovementAnimationSettings);
            }
            sustainDurationMilliseconds = movement.Sustain.DurationMilliseconds;
        }

        m_StatePreviewFrames.clear();
        m_StatePreviewFrames.reserve(m_FiringAnimation.NormalizedSampleTimes.size());
        for (double firingTime : m_FiringAnimation.NormalizedSampleTimes)
        {
            PixelShipGenerator::ShipAnimationStateRequest request;
            request.UnderlyingMovementType = m_RuntimeMovementType;
            request.MovementPhase = PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN;
            if (sustainDurationMilliseconds > 0u)
            {
                const double elapsedMilliseconds = firingTime * static_cast<double>(m_FiringAnimation.DurationMilliseconds);
                request.MovementNormalizedTime = wrapNormalizedAnimationTime(m_RuntimeMovementNormalizedTime + elapsedMilliseconds / static_cast<double>(sustainDurationMilliseconds));
            }
            request.FireActive = true;
            request.FiringTarget = target;
            request.FiringNormalizedTime = firingTime;
            m_StatePreviewFrames.push_back(m_AnimationStateCoordinator.evaluate(m_GeneratedShip, request, m_MovementAnimationSettings, m_FiringAnimationSettings).Pose.Frame);
        }

        if (sustainDurationMilliseconds > 0u)
        {
            m_ResumeMovementNormalizedTime = wrapNormalizedAnimationTime(m_RuntimeMovementNormalizedTime + static_cast<double>(m_FiringAnimation.DurationMilliseconds) / static_cast<double>(sustainDurationMilliseconds));
        }
        else
        {
            m_ResumeMovementNormalizedTime = 0.0;
        }

        m_StatePreviewFrameDurationMilliseconds = m_FiringAnimation.FrameDurationMilliseconds;
        m_TransientStatePreviewActive = true;
        m_AnimationFrameIndex = 0u;
        if (!refreshAnimationTextures()) { return false; }
        enterAnimationPlayback();
        setStatusMessage(std::string("Transient FIRE over ") + (m_RuntimeMovementType == PixelShipGenerator::ShipAnimationType::IDLE ? "NEUTRAL" : getAnimationTypeDisplayName(m_RuntimeMovementType)));
        return true;
    }

    void ShipGeneratorPreviewApp::cycleDiagnosticView()
    {
        m_Diagnostics.GenerationStageView = false;
        const uint32_t nextView = (static_cast<uint32_t>(m_Diagnostics.ViewMode) + 1u) % static_cast<uint32_t>(DiagnosticViewMode::DIAGNOSTIC_VIEW_MODE_END);
        m_Diagnostics.ViewMode = static_cast<DiagnosticViewMode>(nextView);
        refreshDiagnosticTexture();
        refreshDisplayedTexture();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::enterAnimationPlayback()
    {
        const std::vector<PixelShipGenerator::Image>& frames = getActiveAnimationFrames();
        if (frames.empty()) { return; }

        m_PreviewMode = PreviewMode::ANIMATION;
        setDisplayedAnimationFrame(m_AnimationFrameIndex % static_cast<uint32_t>(frames.size()));
        m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
        m_AnimationClock.restart();
        updateWindowTitle();
    }

    CalibrationContextFilter ShipGeneratorPreviewApp::getCalibrationContextFilter() const
    {
        CalibrationContextFilter filter;
        if (!m_CalibrationContextFilterEnabled) { return filter; }
        const PreviewGenerationRecipe& recipe = getCurrentRecipe();
        filter.Style = recipe.Style;
        filter.Faction = recipe.Faction;
        filter.DimensionBucket = getCalibrationDimensionBucket(recipe.Dimensions);
        return filter;
    }

    void ShipGeneratorPreviewApp::enterAttributeRerollStudio()
    {
        m_RerollStudioReturnMode = m_PreviewMode;
        beginAttributeRerollStudio(m_RerollStudio, getCurrentRecipe());
        m_PreviewMode = PreviewMode::REROLL_STUDIO;
        m_Diagnostics.HelpVisible = false;
        m_Diagnostics.GenerationInspectorVisible = false;
        m_Diagnostics.PaletteInspectorVisible = false;
        setStatusMessage("Reroll Studio opened. Select attributes, then generate a non-destructive candidate.");
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::cancelAttributeRerollStudio()
    {
        if (!m_RerollStudio.Active) { return; }
        resetAttributeRerollStudio(m_RerollStudio);
        m_PreviewMode = m_RerollStudioReturnMode;
        refreshDisplayedTexture();
        setStatusMessage("Reroll candidate discarded. Original ship retained.");
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::acceptAttributeRerollStudioCandidate()
    {
        if (!m_RerollStudio.Active || !m_RerollStudio.CandidateValid) { return; }
        const PreviewGenerationRecipe acceptedRecipe = m_RerollStudio.CandidateRecipe;
        resetAttributeRerollStudio(m_RerollStudio);
        m_PreviewMode = PreviewMode::STATIC;
        appendHistoryEntry(acceptedRecipe);
        setStatusMessage("Reroll candidate accepted and added to History.");
    }

    bool ShipGeneratorPreviewApp::generateAttributeRerollStudioCandidate()
    {
        if (!m_RerollStudio.Active || !hasSelectedAttributeRerollDomains(m_RerollStudio)) { return false; }

        const AttributeRerollStudioState previousState = m_RerollStudio;
        const PreviewGenerationRecipe candidateRecipe = PixelShipGeneratorPreview::generateAttributeRerollCandidate(m_RerollStudio, m_SeedGenerator());
        PixelShipGenerator::GeneratedShip candidateShip;
        PixelShipGenerator::ShipGenerationDebugInfo candidateDebugInfo;
        if (!generateShipFromRecipe(candidateRecipe, candidateShip, &candidateDebugInfo))
        {
            m_RerollStudio = previousState;
            setStatusMessage("Reroll candidate generation failed; BaseRecipe remains unchanged.");
            return false;
        }

        const sf::Image image = PixelShipGenerator::SFMLImageAdapter::createSFMLImage(candidateShip.FinalImage);
        sf::Texture candidateTexture;
        if (!candidateTexture.loadFromImage(image))
        {
            m_RerollStudio = previousState;
            setStatusMessage("Failed to create Reroll Studio candidate texture.");
            return false;
        }
        candidateTexture.setSmooth(false);

        m_RerollCandidateShip = std::move(candidateShip);
        m_RerollCandidateDebugInfo = std::move(candidateDebugInfo);
        m_RerollCandidateTexture = std::move(candidateTexture);
        setStatusMessage("Candidate " + std::to_string(m_RerollStudio.CandidateSequence) + " generated from unchanged BaseRecipe.");
        updateWindowTitle();
        return true;
    }

    void ShipGeneratorPreviewApp::enterCalibrationLab()
    {
        m_PreviewMode = PreviewMode::CALIBRATION;
        m_Diagnostics.HelpVisible = false;
        m_Diagnostics.GenerationInspectorVisible = false;
        m_Diagnostics.PaletteInspectorVisible = false;
        generateCalibrationPair();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::exitCalibrationLab()
    {
        m_PreviewMode = PreviewMode::STATIC;
        m_CalibrationPair = {};
        setDisplayedStaticFrame();
        setStatusMessage("Calibration Lab closed. Production defaults were not modified.");
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::changeCalibrationGroup(int32_t delta)
    {
        const uint32_t count = static_cast<uint32_t>(PixelShipGenerator::GenerationWeightGroup::GENERATION_WEIGHT_GROUP_END);
        if (count == 0u) { return; }
        int32_t value = static_cast<int32_t>(m_CalibrationGroup) + delta;
        while (value < 0) { value += static_cast<int32_t>(count); }
        while (value >= static_cast<int32_t>(count)) { value -= static_cast<int32_t>(count); }
        m_CalibrationGroup = static_cast<PixelShipGenerator::GenerationWeightGroup>(value);
        generateCalibrationPair();
    }

    bool ShipGeneratorPreviewApp::generateCalibrationPair()
    {
        m_CalibrationPair = generateNextCalibrationPair(m_Generator, m_CalibrationSession, getCurrentRecipe(), m_CalibrationGroup);
        if (!m_CalibrationPair.Valid)
        {
            setStatusMessage("Unable to construct a controlled A/B pair for this context.");
            return false;
        }

        const sf::Image imageA = PixelShipGenerator::SFMLImageAdapter::createSFMLImage(m_CalibrationPair.ShipA.FinalImage);
        const sf::Image imageB = PixelShipGenerator::SFMLImageAdapter::createSFMLImage(m_CalibrationPair.ShipB.FinalImage);
        if (!m_CalibrationTextureA.loadFromImage(imageA) || !m_CalibrationTextureB.loadFromImage(imageB))
        {
            m_CalibrationPair.Valid = false;
            setStatusMessage("Failed to create Calibration Lab textures.");
            return false;
        }
        m_CalibrationTextureA.setSmooth(false);
        m_CalibrationTextureB.setSmooth(false);
        setStatusMessage("Controlled pair ready: " + std::string(getCalibrationGroupName(m_CalibrationGroup)));
        updateWindowTitle();
        return true;
    }

    void ShipGeneratorPreviewApp::recordCalibrationDisplayPreference(bool preferLeft)
    {
        if (!m_CalibrationPair.Valid) { return; }
        const bool preferA = preferLeft == m_CalibrationPair.DisplayAOnLeft;
        recordCalibrationPreferenceResult(preferA ? CalibrationPreferenceResult::PREFER_A : CalibrationPreferenceResult::PREFER_B);
    }

    void ShipGeneratorPreviewApp::recordCalibrationPreferenceResult(CalibrationPreferenceResult result)
    {
        if (!m_CalibrationPair.Valid) { return; }
        recordCalibrationPreference(m_CalibrationSession, m_CalibrationPair, result);
        std::string error;
        if (!saveGenerationCalibrationSession(m_CalibrationSession, CalibrationSessionPath, error) && !error.empty())
        {
            std::cerr << "Calibration autosave failed: " << error << '\n';
        }
        generateCalibrationPair();
    }

    void ShipGeneratorPreviewApp::setCalibrationWeight(uint32_t encodedValue)
    {
        const uint32_t optionIndex = encodedValue >> 16u;
        const uint32_t value = encodedValue & 0xFFFFu;
        if (optionIndex >= PixelShipGenerator::getGenerationWeightOptionCount(m_CalibrationGroup)) { return; }
        PixelShipGenerator::setGenerationTuningWeight(m_CalibrationSession.TunedProfile, getCurrentRecipe().Style, m_CalibrationGroup, optionIndex, value);
        setStatusMessage("Temporary tuning updated. Generate a new pair to evaluate it.");
    }

    void ShipGeneratorPreviewApp::saveCalibrationSession()
    {
        std::string error;
        if (saveGenerationCalibrationSession(m_CalibrationSession, CalibrationSessionPath, error)) { setStatusMessage("Calibration session saved: " + CalibrationSessionPath.string()); }
        else { setStatusMessage("Calibration save failed: " + error); }
    }

    void ShipGeneratorPreviewApp::loadCalibrationSession()
    {
        const GenerationCalibrationSessionLoadResult result = loadGenerationCalibrationSession(CalibrationSessionPath);
        if (!result.Success) { setStatusMessage("Calibration load failed: " + result.Error); return; }
        m_CalibrationSession = result.Session;
        setStatusMessage("Calibration session loaded.");
        generateCalibrationPair();
    }

    void ShipGeneratorPreviewApp::exportCalibrationReport()
    {
        std::string error;
        if (exportGenerationCalibrationCsv(m_CalibrationSession, CalibrationReportPath, error)) { setStatusMessage("Calibration CSV exported: " + CalibrationReportPath.string()); }
        else { setStatusMessage("Calibration CSV export failed: " + error); }
    }

    void ShipGeneratorPreviewApp::runCalibrationObjectiveBatch()
    {
        setStatusMessage("Running 12-sample production vs tuned objective batch...");
        m_CalibrationObjectiveBatch = collectCalibrationObjectiveBatch(m_Generator, m_CalibrationSession, getCurrentRecipe(), 12u);
        if (m_CalibrationObjectiveBatch.Valid) { setStatusMessage("Objective batch complete. Preference scoring remains separate."); }
        else { setStatusMessage("Objective batch could not collect sufficient successful generations."); }
    }

    void ShipGeneratorPreviewApp::exportCalibrationTuningProfile()
    {
        std::string error;
        if (PixelShipGeneratorPreview::exportGenerationTuningProfile(m_CalibrationSession.TunedProfile, CalibrationTuningProfilePath, error)) { setStatusMessage("Tuning profile exported: " + CalibrationTuningProfilePath.string()); }
        else { setStatusMessage("Tuning export failed: " + error); }
    }

    void ShipGeneratorPreviewApp::enterFrameInspection()
    {
        const std::vector<PixelShipGenerator::Image>& frames = getActiveAnimationFrames();
        if (frames.empty()) { return; }

        m_PreviewMode = PreviewMode::FRAME_INSPECTION;
        setDisplayedAnimationFrame(m_AnimationFrameIndex % static_cast<uint32_t>(frames.size()));
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::enterGalleryMode()
    {
        buildGallery(m_SeedGenerator());
    }

    void ShipGeneratorPreviewApp::enterGalleryModeFromKnownSeed()
    {
        uint64_t batchSeed = 0u;

        if (!readSeedFromConsole(batchSeed))
        {
            return;
        }

        buildGallery(batchSeed);
    }

    void ShipGeneratorPreviewApp::enterFavoritesMode()
    {
        if (m_FavoritesState.Grid.Items.empty()) { return; }
        m_FavoritesState.Grid.SelectedIndex = std::min(m_FavoritesState.Grid.SelectedIndex, static_cast<uint32_t>(m_FavoritesState.Grid.Items.size() - 1u));
        m_FavoritesState.Grid.HoveredIndex = -1;
        m_PreviewMode = PreviewMode::FAVORITES;
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::exitGalleryMode()
    {
        m_PreviewMode = PreviewMode::STATIC;
        m_GalleryState.Grid.Items.clear();
        m_GalleryState.Grid.HoveredIndex = -1;
        setDisplayedStaticFrame();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::exitFavoritesMode()
    {
        m_PreviewMode = PreviewMode::STATIC;
        m_FavoritesState.Grid.HoveredIndex = -1;
        setDisplayedStaticFrame();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::exportRecipe()
    {
        ShipGenerationRecipeDocument document;
        std::string baseName;

        if (m_PreviewMode == PreviewMode::CALIBRATION)
        {
            const CalibrationGroupStatistics statistics = calculateCalibrationGroupStatistics(m_CalibrationSession, m_CalibrationGroup, getCalibrationContextFilter());
            m_Window.setTitle("Pixel Ship Generator | Calibration Lab | " + std::string(getCalibrationGroupName(m_CalibrationGroup)) + " | " + getCalibrationEvidenceName(statistics.Evidence) + " | " + std::to_string(statistics.UsefulComparisonCount) + " comparisons");
            return;
        }

        if (m_PreviewMode == PreviewMode::FAVORITES)
        {
            if (m_FavoritesState.Grid.Items.empty() || m_FavoritesState.Grid.SelectedIndex >= m_FavoritesState.Grid.Items.size()) { return; }
            const PreviewThumbnailItem& favorite = m_FavoritesState.Grid.Items[m_FavoritesState.Grid.SelectedIndex];
            if (!favorite.Valid) { return; }
            document.Recipe = favorite.Recipe;
            baseName = getSaveBaseName(favorite.Recipe);
        }
        else
        {
            document.Recipe = getCurrentRecipe();
            document.AnimationSettings = m_IdleAnimationSettings;
            baseName = getSaveBaseName(getCurrentRecipe());
        }

        const std::filesystem::path path = getAvailableRecipePath(baseName);
        std::string error;
        if (!saveShipGenerationRecipe(document, path, error))
        {
            setStatusMessage(error);
            std::cerr << error << '\n';
            return;
        }

        const std::string message = "Recipe exported: " + path.string();
        setStatusMessage(message);
        std::cout << message << '\n';
    }

    void ShipGeneratorPreviewApp::importRecipe()
    {
        std::filesystem::path path;
        if (!readPathFromConsole(path))
        {
            setStatusMessage("Recipe import cancelled: invalid path.");
            return;
        }
        importRecipeFromPath(path);
    }

    bool ShipGeneratorPreviewApp::importRecipeFromPath(const std::filesystem::path& path)
    {
        const ShipGenerationRecipeLoadResult loadResult = loadShipGenerationRecipe(path);
        if (!loadResult.Success)
        {
            setStatusMessage(loadResult.Error);
            std::cerr << loadResult.Error << '\n';
            return false;
        }

        const ShipGenerationRecipeDocument& document = loadResult.Document;
        if (!isSupportedPreviewRecipeResolution(document.Recipe))
        {
            const std::string error = "Unsupported PreviewApp resolution: " + std::to_string(document.Recipe.Dimensions.Width) + "x" + std::to_string(document.Recipe.Dimensions.Height) + ".";
            setStatusMessage(error);
            std::cerr << error << '\n';
            return false;
        }

        PixelShipGenerator::GeneratedShip candidateShip;
        if (!generateShipFromRecipe(document.Recipe, candidateShip, nullptr))
        {
            setStatusMessage("Recipe parsed correctly, but ship generation failed.");
            return false;
        }

        if (document.AnimationSettings.has_value())
        {
            try
            {
                const PixelShipGenerator::ShipIdleAnimation candidateAnimation = m_IdleAnimator.generate(candidateShip, *document.AnimationSettings);
                if (candidateAnimation.Frames.empty())
                {
                    setStatusMessage("Recipe animation settings produced no frames.");
                    return false;
                }
            }
            catch (const std::exception& exception)
            {
                const std::string error = std::string("Recipe animation validation failed: ") + exception.what();
                setStatusMessage(error);
                std::cerr << error << '\n';
                return false;
            }
        }

        if (document.AnimationSettings.has_value()) { m_IdleAnimationSettings = *document.AnimationSettings; }
        m_PreviewMode = PreviewMode::STATIC;
        m_Diagnostics.HelpVisible = false;
        m_Diagnostics.GenerationInspectorVisible = false;
        m_Diagnostics.PaletteInspectorVisible = false;
        m_GalleryState.Grid.Items.clear();
        m_FavoritesState.Grid.HoveredIndex = -1;
        if (document.Recipe == getCurrentRecipe()) { regenerate(); }
        else { appendHistoryEntry(document.Recipe); }

        const std::string message = "Recipe loaded: " + path.string();
        setStatusMessage(message);
        std::cout << message << '\n';
        return true;
    }

    void ShipGeneratorPreviewApp::generateFromMasterSeed()
    {
        uint64_t masterSeed = 0u;

        if (!readSeedFromConsole(masterSeed))
        {
            return;
        }

        const PreviewGenerationRecipe currentRecipe = getCurrentRecipe();
        const PreviewGenerationRecipe recipe = createRecipeFromMasterSeed(masterSeed, currentRecipe);
        appendHistoryEntry(recipe);
    }

    void ShipGeneratorPreviewApp::generateNew()
    {
        const PreviewGenerationRecipe currentRecipe = getCurrentRecipe();
        const PreviewGenerationRecipe newRecipe = createRecipeFromMasterSeed(m_SeedGenerator(), currentRecipe);
        appendHistoryEntry(newRecipe);
    }

    bool ShipGeneratorPreviewApp::generateShipFromRecipe(const PreviewGenerationRecipe& recipe, PixelShipGenerator::GeneratedShip& outShip, PixelShipGenerator::ShipGenerationDebugInfo* debugInfo)
    {
        PixelShipGenerator::ShipGenerationSettings settings;
        settings.Seed = recipe.Seeds.Master;
        settings.Dimensions.Width = recipe.Dimensions.Width;
        settings.Dimensions.Height = recipe.Dimensions.Height;
        settings.Style = recipe.Style;
        settings.Faction = recipe.Faction;
        settings.DetailDensity = recipe.DetailDensity;
        settings.AsymmetricDetailChance = recipe.AsymmetricDetailChance;
        settings.AttachmentsEnabled = recipe.AttachmentsEnabled;
        settings.SeedOverrides.Structure = recipe.Seeds.Structure;
        settings.SeedOverrides.Palette = recipe.Seeds.Palette;
        settings.SeedOverrides.Details = recipe.Seeds.Details;
        settings.SeedOverrides.Attachments = recipe.Seeds.Attachments;
        settings.DomainSeedOverrides = recipe.DomainSeedOverrides;
        settings.RandomStreamMode = recipe.RandomStreamMode;

        try
        {
            outShip = debugInfo != nullptr ? m_Generator.generate(settings, debugInfo) : m_Generator.generate(settings);
            return true;
        }
        catch (const std::exception& exception)
        {
            std::cerr << "Generation failed: " << exception.what() << '\n';
            return false;
        }
    }

    PreviewCommandPanelState ShipGeneratorPreviewApp::createCommandPanelState() const
    {
        PreviewCommandPanelState state;

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PreviewCommandType::PREVIEW_COMMAND_TYPE_END); ++index)
        {
            const PreviewCommand command{ static_cast<PreviewCommandType>(index), 0u };
            state.Enabled[index] = isCommandEnabled(command);
            state.Active[index] = isCommandActive(command.Type);
        }

        state.Mode = m_PreviewMode == PreviewMode::CALIBRATION ? PreviewCommandPanelMode::CALIBRATION : m_PreviewMode == PreviewMode::REROLL_STUDIO ? PreviewCommandPanelMode::REROLL_STUDIO : PreviewCommandPanelMode::NORMAL;
        if (m_PreviewMode == PreviewMode::REROLL_STUDIO)
        {
            state.RerollStudioSelectedDomains = m_RerollStudio.SelectedDomains;
        }

        state.StyleValue = getStyleName(getCurrentRecipe().Style);
        state.FactionValue = getFactionDisplayName(getCurrentRecipe().Faction);
        state.CurrentDimensions = getCurrentRecipe().Dimensions;
        state.AspectRatioLocked = m_AspectRatioLocked;
        state.ResolutionBookmarkCount = static_cast<uint32_t>(std::min<std::size_t>(m_ResolutionBookmarks.size(), state.ResolutionBookmarks.size()));
        for (uint32_t index = 0u; index < state.ResolutionBookmarkCount; ++index) { state.ResolutionBookmarks[index] = m_ResolutionBookmarks[index]; }

        if (m_PreviewMode == PreviewMode::CALIBRATION)
        {
            state.CalibrationGroupValue = getCalibrationGroupName(m_CalibrationGroup);
            const CalibrationContextFilter filter = getCalibrationContextFilter();
            const CalibrationGroupStatistics statistics = calculateCalibrationGroupStatistics(m_CalibrationSession, m_CalibrationGroup, filter);
            state.CalibrationEvidenceValue = std::string(getCalibrationEvidenceName(statistics.Evidence)) + " / " + std::to_string(statistics.UsefulComparisonCount);
            const std::vector<uint32_t> suggested = calculateSuggestedGroupWeights(m_CalibrationSession, getCurrentRecipe().Style, m_CalibrationGroup, filter);
            const uint32_t optionCount = PixelShipGenerator::getGenerationWeightOptionCount(m_CalibrationGroup);
            const bool binary = PixelShipGenerator::getGenerationWeightGroupKind(m_CalibrationGroup) == PixelShipGenerator::GenerationWeightGroupKind::BINARY_PROBABILITY;
            for (uint32_t index = 0u; index < state.CalibrationWeightRows.size(); ++index)
            {
                PreviewCalibrationWeightRowState& row = state.CalibrationWeightRows[index];
                row.Valid = index < optionCount;
                if (!row.Valid) { continue; }
                row.Label = getCalibrationOptionName(m_CalibrationGroup, index);
                row.CurrentWeight = PixelShipGenerator::getGenerationTuningWeight(m_CalibrationSession.TunedProfile, getCurrentRecipe().Style, m_CalibrationGroup, index);
                row.DefaultWeight = PixelShipGenerator::getGenerationTuningWeight(m_CalibrationSession.DefaultProfile, getCurrentRecipe().Style, m_CalibrationGroup, index);
                row.SuggestedWeight = index < suggested.size() ? suggested[index] : row.CurrentWeight;
                row.ProbabilityPercent = static_cast<uint32_t>(std::lround(PixelShipGenerator::getGenerationTuningNormalizedProbability(m_CalibrationSession.TunedProfile, getCurrentRecipe().Style, m_CalibrationGroup, index) * 100.0));
                row.Maximum = binary ? 100u : std::max<uint32_t>(300u, std::max(row.CurrentWeight, std::max(row.DefaultWeight, row.SuggestedWeight)));
            }
        }
        return state;
    }

    void ShipGeneratorPreviewApp::executeCommand(const PreviewCommand& command)
    {
        if (!isCommandEnabled(command))
        {
            return;
        }

        switch (command.Type)
        {
        case PreviewCommandType::GENERATE_NEW:
            if (m_PreviewMode == PreviewMode::GALLERY) { buildGallery(m_SeedGenerator()); }
            else { generateNew(); }
            break;
        case PreviewCommandType::REROLL: reroll(); break;
        case PreviewCommandType::GENERATE_FROM_MASTER_SEED: generateFromMasterSeed(); break;
        case PreviewCommandType::PREVIOUS_HISTORY: previous(); break;
        case PreviewCommandType::NEXT_HISTORY: next(); break;
        case PreviewCommandType::OPEN_GALLERY: enterGalleryMode(); break;
        case PreviewCommandType::OPEN_GALLERY_FROM_SEED: enterGalleryModeFromKnownSeed(); break;
        case PreviewCommandType::GALLERY_LEFT: moveGallerySelection(-1, 0); break;
        case PreviewCommandType::GALLERY_RIGHT: moveGallerySelection(1, 0); break;
        case PreviewCommandType::GALLERY_UP: moveGallerySelection(0, -1); break;
        case PreviewCommandType::GALLERY_DOWN: moveGallerySelection(0, 1); break;
        case PreviewCommandType::SELECT_GALLERY_CANDIDATE: selectGalleryCandidate(m_GalleryState.Grid.SelectedIndex); break;
        case PreviewCommandType::ADD_CURRENT_TO_FAVORITES: addCurrentToFavorites(); break;
        case PreviewCommandType::REMOVE_CURRENT_FROM_FAVORITES: removeCurrentFromFavorites(); break;
        case PreviewCommandType::OPEN_FAVORITES: enterFavoritesMode(); break;
        case PreviewCommandType::CLOSE_FAVORITES: exitFavoritesMode(); break;
        case PreviewCommandType::FAVORITES_LEFT: moveFavoritesSelection(-1, 0); break;
        case PreviewCommandType::FAVORITES_RIGHT: moveFavoritesSelection(1, 0); break;
        case PreviewCommandType::FAVORITES_UP: moveFavoritesSelection(0, -1); break;
        case PreviewCommandType::FAVORITES_DOWN: moveFavoritesSelection(0, 1); break;
        case PreviewCommandType::SELECT_FAVORITE: loadFavorite(m_FavoritesState.Grid.SelectedIndex); break;
        case PreviewCommandType::SELECT_STYLE:
            if (command.Value < SupportedPreviewStyles.size()) { setStyle(SupportedPreviewStyles[command.Value]); }
            break;
        case PreviewCommandType::PREVIOUS_STYLE: changeStyle(-1); break;
        case PreviewCommandType::NEXT_STYLE: changeStyle(1); break;
        case PreviewCommandType::SELECT_FACTION:
            if (command.Value < SupportedPreviewFactions.size()) { setFaction(SupportedPreviewFactions[command.Value]); }
            break;
        case PreviewCommandType::PREVIOUS_FACTION: changeFaction(-1); break;
        case PreviewCommandType::NEXT_FACTION: changeFaction(1); break;
        case PreviewCommandType::SELECT_RESOLUTION:
            if (command.Value < DirectPreviewResolutions.size()) { setResolution(DirectPreviewResolutions[command.Value]); }
            break;
        case PreviewCommandType::PREVIOUS_RESOLUTION: changeResolution(-1); break;
        case PreviewCommandType::NEXT_RESOLUTION: changeResolution(1); break;
        case PreviewCommandType::SET_WIDTH:
            setWidth(command.Value);
            break;
        case PreviewCommandType::SET_HEIGHT:
            setHeight(command.Value);
            break;
        case PreviewCommandType::TOGGLE_ASPECT_RATIO_LOCK:
            toggleAspectRatioLock();
            break;
        case PreviewCommandType::ADD_RESOLUTION_BOOKMARK: addResolutionBookmark(); break;
        case PreviewCommandType::REMOVE_RESOLUTION_BOOKMARK: removeResolutionBookmark(); break;
        case PreviewCommandType::SELECT_RESOLUTION_BOOKMARK: selectResolutionBookmark(command.Value); break;
        case PreviewCommandType::TOGGLE_ATTACHMENTS_ENABLED: toggleAttachments(); break;
        case PreviewCommandType::TOGGLE_STRUCTURE_LOCK: m_Locks.Structure = !m_Locks.Structure; updateWindowTitle(); break;
        case PreviewCommandType::TOGGLE_PALETTE_LOCK: m_Locks.Palette = !m_Locks.Palette; updateWindowTitle(); break;
        case PreviewCommandType::TOGGLE_DETAILS_LOCK: m_Locks.Details = !m_Locks.Details; updateWindowTitle(); break;
        case PreviewCommandType::TOGGLE_ATTACHMENTS_LOCK: m_Locks.Attachments = !m_Locks.Attachments; updateWindowTitle(); break;
        case PreviewCommandType::TOGGLE_HELP: toggleHelpOverlay(); break;
        case PreviewCommandType::TOGGLE_GENERATION_INSPECTOR: toggleGenerationInspector(); break;
        case PreviewCommandType::TOGGLE_PALETTE_INSPECTOR: togglePaletteInspector(); break;
        case PreviewCommandType::CYCLE_DIAGNOSTIC_VIEW: cycleDiagnosticView(); break;
        case PreviewCommandType::TOGGLE_GENERATION_STAGE_VIEW: toggleGenerationStageView(); break;
        case PreviewCommandType::PREVIOUS_GENERATION_STAGE: moveGenerationStage(-1); break;
        case PreviewCommandType::NEXT_GENERATION_STAGE: moveGenerationStage(1); break;
        case PreviewCommandType::CYCLE_ANIMATION_TYPE: cycleAnimationType(); break;
        case PreviewCommandType::CYCLE_MOVEMENT_PHASE: cycleMovementPhase(); break;
        case PreviewCommandType::CYCLE_FIRING_TARGET: cycleFiringTarget(); break;
        case PreviewCommandType::APPLY_ANIMATION_STATE: applySelectedAnimationState(); break;
        case PreviewCommandType::RETURN_ANIMATION_TO_IDLE: returnAnimationToIdle(); break;
        case PreviewCommandType::TOGGLE_ANIMATION:
            if (m_PreviewMode == PreviewMode::ANIMATION) { m_PreviewMode = PreviewMode::STATIC; setDisplayedStaticFrame(); updateWindowTitle(); }
            else { enterAnimationPlayback(); }
            break;
        case PreviewCommandType::TOGGLE_FRAME_INSPECTION:
            if (m_PreviewMode == PreviewMode::FRAME_INSPECTION) { m_PreviewMode = PreviewMode::STATIC; setDisplayedStaticFrame(); updateWindowTitle(); }
            else { enterFrameInspection(); }
            break;
        case PreviewCommandType::PREVIOUS_FRAME: moveAnimationFrame(-1); break;
        case PreviewCommandType::NEXT_FRAME: moveAnimationFrame(1); break;
        case PreviewCommandType::PIN_CURRENT: pinCurrentShip(); break;
        case PreviewCommandType::CLEAR_PIN: clearPinnedShip(); break;
        case PreviewCommandType::TOGGLE_COMPARISON: toggleComparisonView(); break;
        case PreviewCommandType::SAVE_CURRENT: saveCurrent(); break;
        case PreviewCommandType::EXPORT_RECIPE: exportRecipe(); break;
        case PreviewCommandType::IMPORT_RECIPE: importRecipe(); break;
        case PreviewCommandType::SAVE_SPRITESHEET: saveSpritesheet(); break;
        case PreviewCommandType::OPEN_REROLL_STUDIO: enterAttributeRerollStudio(); break;
        case PreviewCommandType::REROLL_STUDIO_TOGGLE_DOMAIN:
            if (command.Value < PixelShipGenerator::GenerationDomainCount)
            {
                toggleAttributeRerollDomain(m_RerollStudio, static_cast<PixelShipGenerator::GenerationDomain>(command.Value));
                m_RerollStudio.CandidateValid = false;
                m_RerollStudio.CandidateRecipe = m_RerollStudio.BaseRecipe;
            }
            break;
        case PreviewCommandType::REROLL_STUDIO_SELECT_ALL:
            selectAllAttributeRerollDomains(m_RerollStudio);
            m_RerollStudio.CandidateValid = false;
            m_RerollStudio.CandidateRecipe = m_RerollStudio.BaseRecipe;
            break;
        case PreviewCommandType::REROLL_STUDIO_CLEAR:
            clearAttributeRerollDomains(m_RerollStudio);
            m_RerollStudio.CandidateValid = false;
            m_RerollStudio.CandidateRecipe = m_RerollStudio.BaseRecipe;
            break;
        case PreviewCommandType::REROLL_STUDIO_SELECT_STRUCTURE:
            selectAttributeRerollParentChannel(m_RerollStudio, PixelShipGenerator::GenerationSeedChannel::STRUCTURE);
            m_RerollStudio.CandidateValid = false;
            m_RerollStudio.CandidateRecipe = m_RerollStudio.BaseRecipe;
            break;
        case PreviewCommandType::REROLL_STUDIO_SELECT_APPEARANCE:
            selectAttributeRerollAppearanceDomains(m_RerollStudio);
            m_RerollStudio.CandidateValid = false;
            m_RerollStudio.CandidateRecipe = m_RerollStudio.BaseRecipe;
            break;
        case PreviewCommandType::REROLL_STUDIO_GENERATE_CANDIDATE: generateAttributeRerollStudioCandidate(); break;
        case PreviewCommandType::REROLL_STUDIO_ACCEPT: acceptAttributeRerollStudioCandidate(); break;
        case PreviewCommandType::REROLL_STUDIO_CANCEL: cancelAttributeRerollStudio(); break;
        case PreviewCommandType::OPEN_CALIBRATION_LAB: enterCalibrationLab(); break;
        case PreviewCommandType::CALIBRATION_PREVIOUS_GROUP: changeCalibrationGroup(-1); break;
        case PreviewCommandType::CALIBRATION_NEXT_GROUP: changeCalibrationGroup(1); break;
        case PreviewCommandType::CALIBRATION_SET_WEIGHT: setCalibrationWeight(command.Value); break;
        case PreviewCommandType::CALIBRATION_GENERATE_PAIR: generateCalibrationPair(); break;
        case PreviewCommandType::CALIBRATION_PREFER_LEFT: recordCalibrationDisplayPreference(true); break;
        case PreviewCommandType::CALIBRATION_NO_PREFERENCE: recordCalibrationPreferenceResult(CalibrationPreferenceResult::NO_PREFERENCE); break;
        case PreviewCommandType::CALIBRATION_PREFER_RIGHT: recordCalibrationDisplayPreference(false); break;
        case PreviewCommandType::CALIBRATION_SKIP: recordCalibrationPreferenceResult(CalibrationPreferenceResult::SKIP); break;
        case PreviewCommandType::CALIBRATION_RESET_GROUP:
            resetCalibrationGroup(m_CalibrationSession, getCurrentRecipe().Style, m_CalibrationGroup);
            setStatusMessage("Selected tuning group reset to production defaults.");
            break;
        case PreviewCommandType::CALIBRATION_RESET_ALL:
            resetAllCalibrationTuning(m_CalibrationSession);
            setStatusMessage("All temporary tuning reset to production defaults.");
            break;
        case PreviewCommandType::CALIBRATION_APPLY_SUGGESTED:
            applySuggestedGroupWeights(m_CalibrationSession, getCurrentRecipe().Style, m_CalibrationGroup, getCalibrationContextFilter());
            setStatusMessage("Suggested weights applied to temporary tuning only.");
            break;
        case PreviewCommandType::CALIBRATION_TOGGLE_SHOW_VALUES: m_CalibrationShowValues = !m_CalibrationShowValues; break;
        case PreviewCommandType::CALIBRATION_TOGGLE_CONTEXT_FILTER: m_CalibrationContextFilterEnabled = !m_CalibrationContextFilterEnabled; break;
        case PreviewCommandType::CALIBRATION_RUN_OBJECTIVE_BATCH: runCalibrationObjectiveBatch(); break;
        case PreviewCommandType::CALIBRATION_SAVE_SESSION: saveCalibrationSession(); break;
        case PreviewCommandType::CALIBRATION_LOAD_SESSION: loadCalibrationSession(); break;
        case PreviewCommandType::CALIBRATION_EXPORT_REPORT: exportCalibrationReport(); break;
        case PreviewCommandType::CALIBRATION_EXPORT_TUNING_PROFILE: exportCalibrationTuningProfile(); break;
        case PreviewCommandType::CALIBRATION_EXIT: exitCalibrationLab(); break;
        case PreviewCommandType::BACK_OR_EXIT:
            if (m_Diagnostics.HelpVisible || m_Diagnostics.GenerationInspectorVisible || m_Diagnostics.PaletteInspectorVisible)
            {
                m_Diagnostics.HelpVisible = false;
                m_Diagnostics.GenerationInspectorVisible = false;
                m_Diagnostics.PaletteInspectorVisible = false;
            }
            else if (m_PreviewMode == PreviewMode::REROLL_STUDIO)
            {
                cancelAttributeRerollStudio();
            }
            else if (m_PreviewMode == PreviewMode::CALIBRATION)
            {
                exitCalibrationLab();
            }
            else if (m_PreviewMode == PreviewMode::GALLERY)
            {
                exitGalleryMode();
            }
            else if (m_PreviewMode == PreviewMode::FAVORITES)
            {
                exitFavoritesMode();
            }
            else if (m_PreviewMode == PreviewMode::FRAME_INSPECTION)
            {
                m_PreviewMode = PreviewMode::STATIC;
                setDisplayedStaticFrame();
                updateWindowTitle();
            }
            else
            {
                m_Window.close();
            }
            break;
        default: break;
        }

        updateCommandPanelState();
    }

    std::optional<PreviewCommand> ShipGeneratorPreviewApp::getKeyboardCommand(sf::Keyboard::Key key, bool shift) const
    {
        if (m_PreviewMode == PreviewMode::REROLL_STUDIO)
        {
            switch (key)
            {
            case sf::Keyboard::R:
            case sf::Keyboard::Space: return PreviewCommand{ PreviewCommandType::REROLL_STUDIO_GENERATE_CANDIDATE, 0u };
            case sf::Keyboard::Enter: return PreviewCommand{ PreviewCommandType::REROLL_STUDIO_ACCEPT, 0u };
            case sf::Keyboard::Escape: return PreviewCommand{ PreviewCommandType::REROLL_STUDIO_CANCEL, 0u };
            default: return std::nullopt;
            }
        }

        if (m_PreviewMode == PreviewMode::CALIBRATION)
        {
            switch (key)
            {
            case sf::Keyboard::A:
            case sf::Keyboard::Left: return PreviewCommand{ PreviewCommandType::CALIBRATION_PREFER_LEFT, 0u };
            case sf::Keyboard::D:
            case sf::Keyboard::Right: return PreviewCommand{ PreviewCommandType::CALIBRATION_PREFER_RIGHT, 0u };
            case sf::Keyboard::S:
            case sf::Keyboard::Down: return PreviewCommand{ PreviewCommandType::CALIBRATION_NO_PREFERENCE, 0u };
            case sf::Keyboard::Space: return PreviewCommand{ PreviewCommandType::CALIBRATION_SKIP, 0u };
            case sf::Keyboard::N: return PreviewCommand{ PreviewCommandType::CALIBRATION_GENERATE_PAIR, 0u };
            case sf::Keyboard::Escape: return PreviewCommand{ PreviewCommandType::CALIBRATION_EXIT, 0u };
            default: return std::nullopt;
            }
        }

        if (key == sf::Keyboard::F5) { return PreviewCommand{ PreviewCommandType::TOGGLE_HELP, 0u }; }
        if (key == sf::Keyboard::F6) { return PreviewCommand{ PreviewCommandType::TOGGLE_GENERATION_INSPECTOR, 0u }; }
        if (key == sf::Keyboard::F7) { return PreviewCommand{ PreviewCommandType::TOGGLE_PALETTE_INSPECTOR, 0u }; }

        const bool overlayVisible = m_Diagnostics.HelpVisible || m_Diagnostics.GenerationInspectorVisible || m_Diagnostics.PaletteInspectorVisible;

        if (overlayVisible)
        {
            return key == sf::Keyboard::Escape ? std::optional<PreviewCommand>(PreviewCommand{ PreviewCommandType::BACK_OR_EXIT, 0u }) : std::nullopt;
        }

        if (key == sf::Keyboard::F10) { return PreviewCommand{ PreviewCommandType::CLEAR_PIN, 0u }; }

        if (m_PreviewMode != PreviewMode::GALLERY && m_PreviewMode != PreviewMode::FAVORITES)
        {
            if (key == sf::Keyboard::F9) { return PreviewCommand{ PreviewCommandType::PIN_CURRENT, 0u }; }
            if (key == sf::Keyboard::F11) { return PreviewCommand{ PreviewCommandType::TOGGLE_COMPARISON, 0u }; }
            if (key == sf::Keyboard::M) { return PreviewCommand{ PreviewCommandType::CYCLE_DIAGNOSTIC_VIEW, 0u }; }
            if (key == sf::Keyboard::F8) { return PreviewCommand{ PreviewCommandType::TOGGLE_GENERATION_STAGE_VIEW, 0u }; }
            if (key == sf::Keyboard::LBracket) { return PreviewCommand{ PreviewCommandType::PREVIOUS_GENERATION_STAGE, 0u }; }
            if (key == sf::Keyboard::RBracket) { return PreviewCommand{ PreviewCommandType::NEXT_GENERATION_STAGE, 0u }; }
        }

        if (m_PreviewMode == PreviewMode::FAVORITES)
        {
            switch (key)
            {
            case sf::Keyboard::Left: return PreviewCommand{ PreviewCommandType::FAVORITES_LEFT, 0u };
            case sf::Keyboard::Right: return PreviewCommand{ PreviewCommandType::FAVORITES_RIGHT, 0u };
            case sf::Keyboard::Up: return PreviewCommand{ PreviewCommandType::FAVORITES_UP, 0u };
            case sf::Keyboard::Down: return PreviewCommand{ PreviewCommandType::FAVORITES_DOWN, 0u };
            case sf::Keyboard::Enter: return PreviewCommand{ PreviewCommandType::SELECT_FAVORITE, 0u };
            case sf::Keyboard::Escape: return PreviewCommand{ PreviewCommandType::BACK_OR_EXIT, 0u };
            default: return std::nullopt;
            }
        }

        if (m_PreviewMode == PreviewMode::GALLERY)
        {
            switch (key)
            {
            case sf::Keyboard::Left: return PreviewCommand{ PreviewCommandType::GALLERY_LEFT, 0u };
            case sf::Keyboard::Right: return PreviewCommand{ PreviewCommandType::GALLERY_RIGHT, 0u };
            case sf::Keyboard::Up: return PreviewCommand{ PreviewCommandType::GALLERY_UP, 0u };
            case sf::Keyboard::Down: return PreviewCommand{ PreviewCommandType::GALLERY_DOWN, 0u };
            case sf::Keyboard::Enter: return PreviewCommand{ PreviewCommandType::SELECT_GALLERY_CANDIDATE, 0u };
            case sf::Keyboard::N:
            case sf::Keyboard::Space: return PreviewCommand{ PreviewCommandType::GENERATE_NEW, 0u };
            case sf::Keyboard::H: return PreviewCommand{ PreviewCommandType::OPEN_GALLERY_FROM_SEED, 0u };
            case sf::Keyboard::Escape: return PreviewCommand{ PreviewCommandType::BACK_OR_EXIT, 0u };
            default: return std::nullopt;
            }
        }

        if (m_PreviewMode == PreviewMode::FRAME_INSPECTION)
        {
            if (key == sf::Keyboard::J) { return PreviewCommand{ shift ? PreviewCommandType::RETURN_ANIMATION_TO_IDLE : PreviewCommandType::APPLY_ANIMATION_STATE, 0u }; }
            switch (key)
            {
            case sf::Keyboard::Left:
            case sf::Keyboard::P:
            case sf::Keyboard::BackSpace: return PreviewCommand{ PreviewCommandType::PREVIOUS_FRAME, 0u };
            case sf::Keyboard::Right: return PreviewCommand{ PreviewCommandType::NEXT_FRAME, 0u };
            case sf::Keyboard::I: return PreviewCommand{ PreviewCommandType::TOGGLE_ANIMATION, 0u };
            case sf::Keyboard::K: return PreviewCommand{ PreviewCommandType::TOGGLE_FRAME_INSPECTION, 0u };
            case sf::Keyboard::Escape: return PreviewCommand{ PreviewCommandType::BACK_OR_EXIT, 0u };
            case sf::Keyboard::S: return PreviewCommand{ PreviewCommandType::SAVE_CURRENT, 0u };
            case sf::Keyboard::Y: return PreviewCommand{ PreviewCommandType::SAVE_SPRITESHEET, 0u };
            case sf::Keyboard::O: return PreviewCommand{ PreviewCommandType::CYCLE_ANIMATION_TYPE, 0u };
            case sf::Keyboard::L: return PreviewCommand{ PreviewCommandType::CYCLE_MOVEMENT_PHASE, 0u };
            case sf::Keyboard::U: return PreviewCommand{ PreviewCommandType::CYCLE_FIRING_TARGET, 0u };
            default: return std::nullopt;
            }
        }

        if (shift)
        {
            switch (key)
            {
            case sf::Keyboard::Num1: return PreviewCommand{ PreviewCommandType::SELECT_RESOLUTION_BOOKMARK, 0u };
            case sf::Keyboard::Num2: return PreviewCommand{ PreviewCommandType::SELECT_RESOLUTION_BOOKMARK, 1u };
            case sf::Keyboard::Num3: return PreviewCommand{ PreviewCommandType::SELECT_RESOLUTION_BOOKMARK, 2u };
            case sf::Keyboard::Num4: return PreviewCommand{ PreviewCommandType::SELECT_RESOLUTION_BOOKMARK, 3u };
            case sf::Keyboard::Num5: return PreviewCommand{ PreviewCommandType::SELECT_RESOLUTION_BOOKMARK, 4u };
            case sf::Keyboard::Num6: return PreviewCommand{ PreviewCommandType::SELECT_RESOLUTION_BOOKMARK, 5u };
            default: break;
            }
        }

        switch (key)
        {
        case sf::Keyboard::J: return PreviewCommand{ shift ? PreviewCommandType::RETURN_ANIMATION_TO_IDLE : PreviewCommandType::APPLY_ANIMATION_STATE, 0u };
        case sf::Keyboard::Escape: return PreviewCommand{ PreviewCommandType::BACK_OR_EXIT, 0u };
        case sf::Keyboard::N:
        case sf::Keyboard::Space: return PreviewCommand{ PreviewCommandType::GENERATE_NEW, 0u };
        case sf::Keyboard::Left:
        case sf::Keyboard::P:
        case sf::Keyboard::BackSpace: return PreviewCommand{ PreviewCommandType::PREVIOUS_HISTORY, 0u };
        case sf::Keyboard::Right: return PreviewCommand{ PreviewCommandType::NEXT_HISTORY, 0u };
        case sf::Keyboard::S: return PreviewCommand{ PreviewCommandType::SAVE_CURRENT, 0u };
        case sf::Keyboard::Y: return PreviewCommand{ PreviewCommandType::SAVE_SPRITESHEET, 0u };
        case sf::Keyboard::O: return PreviewCommand{ PreviewCommandType::CYCLE_ANIMATION_TYPE, 0u };
        case sf::Keyboard::L: return PreviewCommand{ PreviewCommandType::CYCLE_MOVEMENT_PHASE, 0u };
        case sf::Keyboard::U: return PreviewCommand{ PreviewCommandType::CYCLE_FIRING_TARGET, 0u };
        case sf::Keyboard::I: return PreviewCommand{ PreviewCommandType::TOGGLE_ANIMATION, 0u };
        case sf::Keyboard::K: return PreviewCommand{ PreviewCommandType::TOGGLE_FRAME_INSPECTION, 0u };
        case sf::Keyboard::G: return PreviewCommand{ PreviewCommandType::GENERATE_FROM_MASTER_SEED, 0u };
        case sf::Keyboard::Num1: return PreviewCommand{ PreviewCommandType::SELECT_RESOLUTION, 0u };
        case sf::Keyboard::Num2: return PreviewCommand{ PreviewCommandType::SELECT_RESOLUTION, 1u };
        case sf::Keyboard::Num3: return PreviewCommand{ PreviewCommandType::SELECT_RESOLUTION, 2u };
        case sf::Keyboard::Num4: return PreviewCommand{ PreviewCommandType::SELECT_RESOLUTION, 3u };
        case sf::Keyboard::Num5: return PreviewCommand{ PreviewCommandType::SELECT_RESOLUTION, 4u };
        case sf::Keyboard::Num6: return PreviewCommand{ PreviewCommandType::SELECT_RESOLUTION, 5u };
        case sf::Keyboard::Num7: return PreviewCommand{ PreviewCommandType::SELECT_RESOLUTION, 6u };
        case sf::Keyboard::F1: return PreviewCommand{ PreviewCommandType::SELECT_STYLE, 0u };
        case sf::Keyboard::F2: return PreviewCommand{ PreviewCommandType::SELECT_STYLE, 1u };
        case sf::Keyboard::F3: return PreviewCommand{ PreviewCommandType::SELECT_STYLE, 2u };
        case sf::Keyboard::F4: return PreviewCommand{ PreviewCommandType::SELECT_STYLE, 3u };
        case sf::Keyboard::Z: return PreviewCommand{ PreviewCommandType::SELECT_FACTION, 0u };
        case sf::Keyboard::X: return PreviewCommand{ PreviewCommandType::SELECT_FACTION, 1u };
        case sf::Keyboard::C: return PreviewCommand{ PreviewCommandType::SELECT_FACTION, 2u };
        case sf::Keyboard::V: return PreviewCommand{ PreviewCommandType::SELECT_FACTION, 3u };
        case sf::Keyboard::A: return PreviewCommand{ PreviewCommandType::TOGGLE_ATTACHMENTS_ENABLED, 0u };
        case sf::Keyboard::Q: return PreviewCommand{ PreviewCommandType::TOGGLE_STRUCTURE_LOCK, 0u };
        case sf::Keyboard::W: return PreviewCommand{ PreviewCommandType::TOGGLE_PALETTE_LOCK, 0u };
        case sf::Keyboard::E: return PreviewCommand{ PreviewCommandType::TOGGLE_DETAILS_LOCK, 0u };
        case sf::Keyboard::T: return PreviewCommand{ PreviewCommandType::TOGGLE_ATTACHMENTS_LOCK, 0u };
        case sf::Keyboard::R: return PreviewCommand{ PreviewCommandType::REROLL, 0u };
        case sf::Keyboard::B: return PreviewCommand{ PreviewCommandType::OPEN_GALLERY, 0u };
        case sf::Keyboard::H: return PreviewCommand{ PreviewCommandType::OPEN_GALLERY_FROM_SEED, 0u };
        default: return std::nullopt;
        }
    }

    void ShipGeneratorPreviewApp::handleKeyPressed(const sf::Event::KeyEvent& event)
    {
        const std::optional<PreviewCommand> command = getKeyboardCommand(event.code, event.shift);
        if (command.has_value()) { executeCommand(*command); }
    }

    void ShipGeneratorPreviewApp::handleMouseMoved(const sf::Event::MouseMoveEvent& event)
    {
        const sf::Vector2f position = m_Window.mapPixelToCoords(sf::Vector2i(event.x, event.y));
        m_CommandPanel.onMouseMove(position);

        if (m_Diagnostics.HelpVisible || m_Diagnostics.GenerationInspectorVisible || m_Diagnostics.PaletteInspectorVisible) { return; }
        if (m_PreviewMode == PreviewMode::GALLERY) { m_GalleryState.Grid.HoveredIndex = findPreviewThumbnailItemAtPosition(position, m_GalleryState.Grid); }
        if (m_PreviewMode == PreviewMode::FAVORITES) { m_FavoritesState.Grid.HoveredIndex = findPreviewThumbnailItemAtPosition(position, m_FavoritesState.Grid); }
    }

    void ShipGeneratorPreviewApp::handleMousePressed(const sf::Event::MouseButtonEvent& event)
    {
        if (event.button != sf::Mouse::Left)
        {
            return;
        }

        const sf::Vector2f position = m_Window.mapPixelToCoords(sf::Vector2i(event.x, event.y));
        m_CommandPanel.onMousePress(position);

        if (m_CommandPanel.getPressedButtonIndex() >= 0 || m_CommandPanel.isDimensionSliderDragging())
        {
            return;
        }

        if (m_Diagnostics.HelpVisible || m_Diagnostics.GenerationInspectorVisible || m_Diagnostics.PaletteInspectorVisible)
        {
            return;
        }

        if (m_PreviewMode == PreviewMode::GALLERY)
        {
            const int32_t candidateIndex = findPreviewThumbnailItemAtPosition(position, m_GalleryState.Grid);
            if (candidateIndex < 0) { return; }
            m_GalleryState.Grid.SelectedIndex = static_cast<uint32_t>(candidateIndex);
            executeCommand({ PreviewCommandType::SELECT_GALLERY_CANDIDATE, static_cast<uint32_t>(candidateIndex) });
            return;
        }

        if (m_PreviewMode == PreviewMode::FAVORITES)
        {
            const int32_t favoriteIndex = findPreviewThumbnailItemAtPosition(position, m_FavoritesState.Grid);
            if (favoriteIndex < 0) { return; }
            m_FavoritesState.Grid.SelectedIndex = static_cast<uint32_t>(favoriteIndex);
            executeCommand({ PreviewCommandType::SELECT_FAVORITE, static_cast<uint32_t>(favoriteIndex) });
        }
    }

    void ShipGeneratorPreviewApp::handleMouseReleased(const sf::Event::MouseButtonEvent& event)
    {
        if (event.button != sf::Mouse::Left)
        {
            return;
        }

        const sf::Vector2f position = m_Window.mapPixelToCoords(sf::Vector2i(event.x, event.y));
        const std::optional<PreviewCommand> command = m_CommandPanel.onMouseRelease(position);
        if (command.has_value()) { executeCommand(*command); }
    }

    bool ShipGeneratorPreviewApp::isCommandActive(PreviewCommandType type) const
    {
        switch (type)
        {
        case PreviewCommandType::TOGGLE_ATTACHMENTS_ENABLED: return getCurrentRecipe().AttachmentsEnabled;
        case PreviewCommandType::TOGGLE_STRUCTURE_LOCK: return m_Locks.Structure;
        case PreviewCommandType::TOGGLE_PALETTE_LOCK: return m_Locks.Palette;
        case PreviewCommandType::TOGGLE_DETAILS_LOCK: return m_Locks.Details;
        case PreviewCommandType::TOGGLE_ATTACHMENTS_LOCK: return m_Locks.Attachments;
        case PreviewCommandType::TOGGLE_HELP: return m_Diagnostics.HelpVisible;
        case PreviewCommandType::TOGGLE_GENERATION_INSPECTOR: return m_Diagnostics.GenerationInspectorVisible;
        case PreviewCommandType::TOGGLE_PALETTE_INSPECTOR: return m_Diagnostics.PaletteInspectorVisible;
        case PreviewCommandType::TOGGLE_GENERATION_STAGE_VIEW: return m_Diagnostics.GenerationStageView;
        case PreviewCommandType::TOGGLE_ANIMATION: return m_PreviewMode == PreviewMode::ANIMATION;
        case PreviewCommandType::TOGGLE_FRAME_INSPECTION: return m_PreviewMode == PreviewMode::FRAME_INSPECTION;
        case PreviewCommandType::OPEN_FAVORITES: return m_PreviewMode == PreviewMode::FAVORITES;
        case PreviewCommandType::TOGGLE_COMPARISON: return m_Comparison.ViewEnabled && m_Comparison.Pinned.Valid;
        case PreviewCommandType::TOGGLE_ASPECT_RATIO_LOCK: return m_AspectRatioLocked;
        case PreviewCommandType::CALIBRATION_TOGGLE_SHOW_VALUES: return m_CalibrationShowValues;
        case PreviewCommandType::CALIBRATION_TOGGLE_CONTEXT_FILTER: return m_CalibrationContextFilterEnabled;
        default: return false;
        }
    }

    bool ShipGeneratorPreviewApp::isCommandEnabled(const PreviewCommand& command) const
    {
        const PreviewCommandType type = command.Type;
        const bool overlayVisible = m_Diagnostics.HelpVisible || m_Diagnostics.GenerationInspectorVisible || m_Diagnostics.PaletteInspectorVisible;
        const bool browserMode = m_PreviewMode == PreviewMode::GALLERY || m_PreviewMode == PreviewMode::FAVORITES;

        if (type == PreviewCommandType::TOGGLE_HELP) { return true; }
        if (type == PreviewCommandType::TOGGLE_GENERATION_INSPECTOR || type == PreviewCommandType::TOGGLE_PALETTE_INSPECTOR) { return !browserMode; }
        if (type == PreviewCommandType::BACK_OR_EXIT) { return true; }
        if (overlayVisible) { return false; }

        if (m_PreviewMode == PreviewMode::REROLL_STUDIO)
        {
            switch (type)
            {
            case PreviewCommandType::REROLL_STUDIO_TOGGLE_DOMAIN:
                return command.Value < PixelShipGenerator::GenerationDomainCount;
            case PreviewCommandType::REROLL_STUDIO_SELECT_ALL:
            case PreviewCommandType::REROLL_STUDIO_CLEAR:
            case PreviewCommandType::REROLL_STUDIO_SELECT_STRUCTURE:
            case PreviewCommandType::REROLL_STUDIO_SELECT_APPEARANCE:
            case PreviewCommandType::REROLL_STUDIO_CANCEL:
                return true;
            case PreviewCommandType::REROLL_STUDIO_GENERATE_CANDIDATE:
                return hasSelectedAttributeRerollDomains(m_RerollStudio);
            case PreviewCommandType::REROLL_STUDIO_ACCEPT:
                return m_RerollStudio.CandidateValid;
            default:
                return false;
            }
        }

        if (m_PreviewMode == PreviewMode::CALIBRATION)
        {
            switch (type)
            {
            case PreviewCommandType::CALIBRATION_PREVIOUS_GROUP:
            case PreviewCommandType::CALIBRATION_NEXT_GROUP:
            case PreviewCommandType::CALIBRATION_SET_WEIGHT:
            case PreviewCommandType::CALIBRATION_GENERATE_PAIR:
            case PreviewCommandType::CALIBRATION_RESET_GROUP:
            case PreviewCommandType::CALIBRATION_RESET_ALL:
            case PreviewCommandType::CALIBRATION_APPLY_SUGGESTED:
            case PreviewCommandType::CALIBRATION_TOGGLE_SHOW_VALUES:
            case PreviewCommandType::CALIBRATION_TOGGLE_CONTEXT_FILTER:
            case PreviewCommandType::CALIBRATION_RUN_OBJECTIVE_BATCH:
            case PreviewCommandType::CALIBRATION_SAVE_SESSION:
            case PreviewCommandType::CALIBRATION_LOAD_SESSION:
            case PreviewCommandType::CALIBRATION_EXPORT_REPORT:
            case PreviewCommandType::CALIBRATION_EXPORT_TUNING_PROFILE:
            case PreviewCommandType::CALIBRATION_EXIT:
                return true;
            case PreviewCommandType::CALIBRATION_PREFER_LEFT:
            case PreviewCommandType::CALIBRATION_NO_PREFERENCE:
            case PreviewCommandType::CALIBRATION_PREFER_RIGHT:
            case PreviewCommandType::CALIBRATION_SKIP:
                return m_CalibrationPair.Valid;
            default:
                return false;
            }
        }

        if (m_PreviewMode == PreviewMode::GALLERY)
        {
            if (type == PreviewCommandType::GENERATE_NEW || type == PreviewCommandType::OPEN_GALLERY_FROM_SEED) { return true; }
            if (type == PreviewCommandType::CLEAR_PIN) { return m_Comparison.Pinned.Valid; }
            if (type == PreviewCommandType::SELECT_GALLERY_CANDIDATE) { return !m_GalleryState.Grid.Items.empty() && m_GalleryState.Grid.SelectedIndex < m_GalleryState.Grid.Items.size() && m_GalleryState.Grid.Items[m_GalleryState.Grid.SelectedIndex].Valid; }
            if (m_GalleryState.Grid.Items.empty()) { return false; }
            const uint32_t selectedIndex = std::min(m_GalleryState.Grid.SelectedIndex, static_cast<uint32_t>(m_GalleryState.Grid.Items.size() - 1u));
            const uint32_t columns = std::max(1u, m_GalleryState.Grid.Columns);
            if (type == PreviewCommandType::GALLERY_LEFT) { return selectedIndex % columns > 0u; }
            if (type == PreviewCommandType::GALLERY_RIGHT) { return selectedIndex % columns + 1u < columns && selectedIndex + 1u < m_GalleryState.Grid.Items.size(); }
            if (type == PreviewCommandType::GALLERY_UP) { return selectedIndex >= columns; }
            if (type == PreviewCommandType::GALLERY_DOWN) { return selectedIndex + columns < m_GalleryState.Grid.Items.size(); }
            return false;
        }

        if (m_PreviewMode == PreviewMode::FAVORITES)
        {
            if (type == PreviewCommandType::CLOSE_FAVORITES) { return true; }
            if (type == PreviewCommandType::CLEAR_PIN) { return m_Comparison.Pinned.Valid; }
            if (type == PreviewCommandType::EXPORT_RECIPE) { return !m_FavoritesState.Grid.Items.empty() && m_FavoritesState.Grid.SelectedIndex < m_FavoritesState.Grid.Items.size() && m_FavoritesState.Grid.Items[m_FavoritesState.Grid.SelectedIndex].Valid; }
            if (type == PreviewCommandType::SELECT_FAVORITE) { return !m_FavoritesState.Grid.Items.empty() && m_FavoritesState.Grid.SelectedIndex < m_FavoritesState.Grid.Items.size() && m_FavoritesState.Grid.Items[m_FavoritesState.Grid.SelectedIndex].Valid; }
            if (m_FavoritesState.Grid.Items.empty()) { return false; }
            const uint32_t selectedIndex = std::min(m_FavoritesState.Grid.SelectedIndex, static_cast<uint32_t>(m_FavoritesState.Grid.Items.size() - 1u));
            const uint32_t columns = std::max(1u, m_FavoritesState.Grid.Columns);
            if (type == PreviewCommandType::FAVORITES_LEFT) { return selectedIndex % columns > 0u; }
            if (type == PreviewCommandType::FAVORITES_RIGHT) { return selectedIndex % columns + 1u < columns && selectedIndex + 1u < m_FavoritesState.Grid.Items.size(); }
            if (type == PreviewCommandType::FAVORITES_UP) { return selectedIndex >= columns; }
            if (type == PreviewCommandType::FAVORITES_DOWN) { return selectedIndex + columns < m_FavoritesState.Grid.Items.size(); }
            return false;
        }

        if (type == PreviewCommandType::ADD_CURRENT_TO_FAVORITES) { return !isCurrentFavorite(); }
        if (type == PreviewCommandType::REMOVE_CURRENT_FROM_FAVORITES) { return isCurrentFavorite(); }
        if (type == PreviewCommandType::ADD_RESOLUTION_BOOKMARK)
        {
            const PixelShipGenerator::ShipDimensions dimensions = getCurrentRecipe().Dimensions;
            const auto iterator = std::lower_bound(m_ResolutionBookmarks.begin(), m_ResolutionBookmarks.end(), dimensions, dimensionsLess);
            const bool alreadyBookmarked = iterator != m_ResolutionBookmarks.end() && *iterator == dimensions;
            return m_ResolutionBookmarks.size() < MaximumResolutionBookmarks && !alreadyBookmarked;
        }
        if (type == PreviewCommandType::REMOVE_RESOLUTION_BOOKMARK)
        {
            const PixelShipGenerator::ShipDimensions dimensions = getCurrentRecipe().Dimensions;
            const auto iterator = std::lower_bound(m_ResolutionBookmarks.begin(), m_ResolutionBookmarks.end(), dimensions, dimensionsLess);
            return iterator != m_ResolutionBookmarks.end() && *iterator == dimensions;
        }
        if (type == PreviewCommandType::SELECT_RESOLUTION_BOOKMARK) { return command.Value < m_ResolutionBookmarks.size(); }
        if (type == PreviewCommandType::OPEN_FAVORITES) { return !m_FavoritesState.Grid.Items.empty(); }
        if (type == PreviewCommandType::CLOSE_FAVORITES || type == PreviewCommandType::FAVORITES_LEFT || type == PreviewCommandType::FAVORITES_RIGHT || type == PreviewCommandType::FAVORITES_UP || type == PreviewCommandType::FAVORITES_DOWN || type == PreviewCommandType::SELECT_FAVORITE) { return false; }
        if (type == PreviewCommandType::PIN_CURRENT) { return true; }
        if (type == PreviewCommandType::CLEAR_PIN) { return m_Comparison.Pinned.Valid; }
        if (type == PreviewCommandType::TOGGLE_COMPARISON) { return m_Comparison.Pinned.Valid; }
        if (type == PreviewCommandType::CYCLE_DIAGNOSTIC_VIEW) { return true; }
        if (type == PreviewCommandType::TOGGLE_GENERATION_STAGE_VIEW) { return !m_GenerationDebugInfo.HullStages.empty(); }
        if (type == PreviewCommandType::PREVIOUS_GENERATION_STAGE || type == PreviewCommandType::NEXT_GENERATION_STAGE) { return m_Diagnostics.GenerationStageView && !m_GenerationDebugInfo.HullStages.empty(); }
        if (type == PreviewCommandType::SAVE_CURRENT) { return true; }
        if (type == PreviewCommandType::EXPORT_RECIPE || type == PreviewCommandType::IMPORT_RECIPE) { return true; }
        if (type == PreviewCommandType::SAVE_SPRITESHEET) { return !getActiveAnimationFrames().empty(); }
        if (type == PreviewCommandType::TOGGLE_ANIMATION || type == PreviewCommandType::TOGGLE_FRAME_INSPECTION) { return !getActiveAnimationFrames().empty(); }
        if (type == PreviewCommandType::CYCLE_ANIMATION_TYPE) { return true; }
        if (type == PreviewCommandType::CYCLE_MOVEMENT_PHASE) { return m_SelectedAnimationType != PixelShipGenerator::ShipAnimationType::IDLE && m_SelectedAnimationType != PixelShipGenerator::ShipAnimationType::FIRE; }
        if (type == PreviewCommandType::CYCLE_FIRING_TARGET) { return m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE && m_FiringTargets.size() > 1u; }
        if (type == PreviewCommandType::APPLY_ANIMATION_STATE) { return true; }
        if (type == PreviewCommandType::RETURN_ANIMATION_TO_IDLE) { return m_RuntimeMovementType != PixelShipGenerator::ShipAnimationType::IDLE || m_TransientStatePreviewActive; }

        if (m_PreviewMode == PreviewMode::FRAME_INSPECTION)
        {
            return type == PreviewCommandType::PREVIOUS_FRAME || type == PreviewCommandType::NEXT_FRAME || type == PreviewCommandType::CYCLE_ANIMATION_TYPE || type == PreviewCommandType::CYCLE_MOVEMENT_PHASE || type == PreviewCommandType::CYCLE_FIRING_TARGET || type == PreviewCommandType::APPLY_ANIMATION_STATE || type == PreviewCommandType::RETURN_ANIMATION_TO_IDLE;
        }
        if (type == PreviewCommandType::PREVIOUS_FRAME || type == PreviewCommandType::NEXT_FRAME || type == PreviewCommandType::GALLERY_LEFT || type == PreviewCommandType::GALLERY_RIGHT || type == PreviewCommandType::GALLERY_UP || type == PreviewCommandType::GALLERY_DOWN || type == PreviewCommandType::SELECT_GALLERY_CANDIDATE) { return false; }
        if (type == PreviewCommandType::PREVIOUS_HISTORY) { return m_HistoryIndex > 0u; }
        if (type == PreviewCommandType::NEXT_HISTORY) { return m_HistoryIndex + 1u < m_History.size(); }
        if (type == PreviewCommandType::REROLL) { return !(m_Locks.Structure && m_Locks.Palette && m_Locks.Details && m_Locks.Attachments); }

        switch (type)
        {
        case PreviewCommandType::GENERATE_NEW:
        case PreviewCommandType::GENERATE_FROM_MASTER_SEED:
        case PreviewCommandType::OPEN_REROLL_STUDIO:
        case PreviewCommandType::OPEN_CALIBRATION_LAB:
        case PreviewCommandType::OPEN_GALLERY:
        case PreviewCommandType::OPEN_GALLERY_FROM_SEED:
        case PreviewCommandType::SELECT_STYLE:
        case PreviewCommandType::PREVIOUS_STYLE:
        case PreviewCommandType::NEXT_STYLE:
        case PreviewCommandType::SELECT_FACTION:
        case PreviewCommandType::PREVIOUS_FACTION:
        case PreviewCommandType::NEXT_FACTION:
        case PreviewCommandType::SELECT_RESOLUTION:
        case PreviewCommandType::PREVIOUS_RESOLUTION:
        case PreviewCommandType::NEXT_RESOLUTION:
        case PreviewCommandType::SET_WIDTH:
        case PreviewCommandType::SET_HEIGHT:
        case PreviewCommandType::TOGGLE_ASPECT_RATIO_LOCK:
        case PreviewCommandType::TOGGLE_ATTACHMENTS_ENABLED:
        case PreviewCommandType::TOGGLE_STRUCTURE_LOCK:
        case PreviewCommandType::TOGGLE_PALETTE_LOCK:
        case PreviewCommandType::TOGGLE_DETAILS_LOCK:
        case PreviewCommandType::TOGGLE_ATTACHMENTS_LOCK:
            return true;
        default:
            return false;
        }
    }

    void ShipGeneratorPreviewApp::moveAnimationFrame(int32_t delta)
    {
        const std::vector<PixelShipGenerator::Image>& frames = getActiveAnimationFrames();
        if (frames.empty())
        {
            return;
        }

        const int32_t frameCount = static_cast<int32_t>(frames.size());
        int32_t frameIndex = static_cast<int32_t>(m_AnimationFrameIndex) + delta;
        if (frameIndex < 0) { frameIndex = frameCount - 1; }
        if (frameIndex >= frameCount) { frameIndex = 0; }
        setDisplayedAnimationFrame(static_cast<uint32_t>(frameIndex));
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::moveFavoritesSelection(int32_t deltaX, int32_t deltaY)
    {
        if (movePreviewThumbnailSelection(m_FavoritesState.Grid, deltaX, deltaY)) { updateWindowTitle(); }
    }

    void ShipGeneratorPreviewApp::moveGallerySelection(int32_t deltaX, int32_t deltaY)
    {
        if (movePreviewThumbnailSelection(m_GalleryState.Grid, deltaX, deltaY)) { updateWindowTitle(); }
    }

    void ShipGeneratorPreviewApp::loadFavorite(uint32_t index)
    {
        if (index >= m_FavoritesState.Grid.Items.size()) { return; }
        const PreviewThumbnailItem& favorite = m_FavoritesState.Grid.Items[index];
        if (!favorite.Valid) { return; }

        const PreviewGenerationRecipe recipe = favorite.Recipe;
        m_PreviewMode = PreviewMode::STATIC;
        m_FavoritesState.Grid.HoveredIndex = -1;

        if (recipe == getCurrentRecipe())
        {
            setDisplayedStaticFrame();
            updateWindowTitle();
            return;
        }

        appendHistoryEntry(recipe);
    }

    void ShipGeneratorPreviewApp::loadPreviewAppPreferences()
    {
        const PreviewPreferencesLoadResult result = loadPreviewPreferences(PreviewPreferencesPath);
        if (!result.Success)
        {
            std::cerr << result.Error << '\n';
            return;
        }

        m_ResolutionBookmarks = result.Preferences.ResolutionBookmarks;
    }

    void ShipGeneratorPreviewApp::next()
    {
        if (m_HistoryIndex + 1u >= m_History.size())
        {
            return;
        }

        ++m_HistoryIndex;
        regenerate();
    }

    void ShipGeneratorPreviewApp::pinCurrentShip()
    {
        const sf::Image pinnedImage = PixelShipGenerator::SFMLImageAdapter::createSFMLImage(m_GeneratedShip.FinalImage);

        if (!m_PinnedTexture.loadFromImage(pinnedImage))
        {
            std::cerr << "Failed to create pinned comparison texture.\n";
            return;
        }

        m_PinnedTexture.setSmooth(false);
        m_Comparison.Pinned.Recipe = getCurrentRecipe();
        m_Comparison.Pinned.Ship = m_GeneratedShip;
        m_Comparison.Pinned.Valid = true;
        m_Comparison.ViewEnabled = true;

        if (m_PreviewMode == PreviewMode::ANIMATION || m_PreviewMode == PreviewMode::FRAME_INSPECTION)
        {
            m_PreviewMode = PreviewMode::STATIC;
            setDisplayedStaticFrame();
        }

        updateWindowTitle();
        std::cout << "Pinned ship seed: " << m_Comparison.Pinned.Recipe.Seeds.Master << '\n';
    }

    void ShipGeneratorPreviewApp::previous()
    {
        if (m_HistoryIndex == 0u)
        {
            return;
        }

        --m_HistoryIndex;
        regenerate();
    }

    void ShipGeneratorPreviewApp::removeCurrentFromFavorites()
    {
        const std::optional<std::size_t> favoriteIndex = findFavoriteIndex(getCurrentRecipe());
        if (!favoriteIndex.has_value()) { return; }

        m_FavoritesState.Grid.Items.erase(m_FavoritesState.Grid.Items.begin() + static_cast<std::ptrdiff_t>(*favoriteIndex));
        if (m_FavoritesState.Grid.Items.empty())
        {
            m_FavoritesState.Grid.SelectedIndex = 0u;
            m_FavoritesState.Grid.HoveredIndex = -1;
        }
        else
        {
            m_FavoritesState.Grid.SelectedIndex = std::min(m_FavoritesState.Grid.SelectedIndex, static_cast<uint32_t>(m_FavoritesState.Grid.Items.size() - 1u));
            if (m_FavoritesState.Grid.HoveredIndex >= static_cast<int32_t>(m_FavoritesState.Grid.Items.size())) { m_FavoritesState.Grid.HoveredIndex = -1; }
        }

        updateWindowTitle();
        std::cout << "Removed Favorite. Remaining: " << m_FavoritesState.Grid.Items.size() << '\n';
    }

    void ShipGeneratorPreviewApp::removeResolutionBookmark()
    {
        const PixelShipGenerator::ShipDimensions dimensions = getCurrentRecipe().Dimensions;
        const auto iterator = std::lower_bound(m_ResolutionBookmarks.begin(), m_ResolutionBookmarks.end(), dimensions, dimensionsLess);
        if (iterator == m_ResolutionBookmarks.end() || *iterator != dimensions) { return; }
        m_ResolutionBookmarks.erase(iterator);
        savePreviewAppPreferences();
        setStatusMessage("Resolution bookmark removed: " + std::to_string(dimensions.Width) + "x" + std::to_string(dimensions.Height));
    }

    void ShipGeneratorPreviewApp::printControls() const
    {
        std::cout << "Controls:\n";

        for (const PreviewCommandData& commandData : getPreviewCommandDataTable())
        {
            if (commandData.Shortcut[0] == '\0') { continue; }
            std::cout << "  " << commandData.Shortcut << "    " << commandData.Description << '\n';
        }

        std::cout << "\n\n";
    }

    void ShipGeneratorPreviewApp::printCurrentSeeds() const
    {
        const PreviewGenerationRecipe& recipe = getCurrentRecipe();
        std::cout << "\tMaster:      " << recipe.Seeds.Master << '\n';
        std::cout << "\tStructure:   " << recipe.Seeds.Structure << '\n';
        std::cout << "\tPalette:     " << recipe.Seeds.Palette << '\n';
        std::cout << "\tDetails:     " << recipe.Seeds.Details << '\n';
        std::cout << "\tAttachments: " << recipe.Seeds.Attachments << '\n';
        if (!getActiveAnimationFrames().empty()) { std::cout << "\tAnimation:   " << getActiveAnimationSeed() << '\n'; }
        std::cout << '\n';
    }

    void ShipGeneratorPreviewApp::processEvents()
    {
        sf::Event event;

        while (m_Window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                m_Window.close();
            }

            if (event.type == sf::Event::MouseMoved)
            {
                handleMouseMoved(event.mouseMove);
            }

            if (event.type == sf::Event::MouseButtonPressed)
            {
                handleMousePressed(event.mouseButton);
            }

            if (event.type == sf::Event::MouseButtonReleased)
            {
                handleMouseReleased(event.mouseButton);
            }

            if (event.type == sf::Event::KeyPressed)
            {
                handleKeyPressed(event.key);
            }
        }
    }

    bool ShipGeneratorPreviewApp::regenerate()
    {
        const PreviewGenerationRecipe& recipe = getCurrentRecipe();

        if (!recipe.Dimensions.isSquare())
        {
            m_AspectRatioLocked = false;
        }

        if (!generateShipFromRecipe(recipe, m_GeneratedShip, &m_GenerationDebugInfo))
        {
            return false;
        }

        m_PreviewImage = PixelShipGenerator::SFMLImageAdapter::createSFMLImage(m_GeneratedShip.FinalImage);

        if (!m_PreviewTexture.loadFromImage(m_PreviewImage))
        {
            std::cerr << "Failed to create preview texture.\n";
            return false;
        }

        m_PreviewTexture.setSmooth(false);

        const uint32_t scale = calculateDisplayScale(recipe.Dimensions.Width, recipe.Dimensions.Height, PreviewAreaWidth, PreviewAreaHeight);
        const uint32_t renderedWidth = recipe.Dimensions.Width * scale;
        const uint32_t renderedHeight = recipe.Dimensions.Height * scale;
        const uint32_t positionX = (PreviewEnlargedContentWidth - renderedWidth) / 2u;
        const uint32_t positionY = (PreviewWindowHeight - renderedHeight) / 2u;

        m_PreviewSprite.setScale(static_cast<float>(scale), static_cast<float>(scale));
        m_PreviewSprite.setPosition(static_cast<float>(positionX), static_cast<float>(positionY));

        m_RuntimeMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
        m_PendingMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
        m_MovementTransitionPending = false;
        m_TransientStatePreviewActive = false;
        m_RuntimeMovementNormalizedTime = 0.0;
        m_ResumeMovementNormalizedTime = 0.0;
        m_StatePreviewFrames.clear();

        if (!regenerateAnimation())
        {
            return false;
        }

        m_AnimationFrameIndex = 0u;

        if (m_Diagnostics.GenerationStageIndex >= m_GenerationDebugInfo.HullStages.size())
        {
            m_Diagnostics.GenerationStageIndex = 0u;
        }

        if (!refreshDiagnosticTexture())
        {
            return false;
        }

        refreshDisplayedTexture();
        updateWindowTitle();
        printCurrentSeeds();

        return true;
    }

    bool ShipGeneratorPreviewApp::regenerateAnimation()
    {
        m_IdleAnimation = m_IdleAnimator.generate(m_GeneratedShip, m_IdleAnimationSettings);

        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::MOVE_LEFT || m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::MOVE_RIGHT)
        {
            m_MovementAnimation = m_LateralMovementAnimator.generate(m_GeneratedShip, m_SelectedAnimationType, m_MovementAnimationSettings);
        }
        else if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::MOVE_UP || m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::MOVE_DOWN)
        {
            m_MovementAnimation = m_LongitudinalMovementAnimator.generate(m_GeneratedShip, m_SelectedAnimationType, m_MovementAnimationSettings);
        }
        else if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE)
        {
            m_FiringTargets = m_FiringAnimator.getAvailableTargets(m_GeneratedShip);
            if (m_FiringTargets.empty())
            {
                m_FiringAnimation = {};
                m_AnimationTextures.clear();
                m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
                m_AnimationClock.restart();
                setStatusMessage("FIRE unavailable: generated ship has no movable weapon component.");
                return true;
            }
            m_SelectedFiringTargetIndex %= static_cast<uint32_t>(m_FiringTargets.size());
            m_FiringAnimation = m_FiringAnimator.generate(m_GeneratedShip, m_FiringTargets[m_SelectedFiringTargetIndex], m_FiringAnimationSettings);
        }

        m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
        m_AnimationClock.restart();
        return refreshAnimationTextures();
    }

    bool ShipGeneratorPreviewApp::refreshAnimationTextures()
    {
        const std::vector<PixelShipGenerator::Image>& frames = getActiveAnimationFrames();
        m_AnimationTextures.clear();
        m_AnimationTextures.resize(frames.size());

        for (std::size_t index = 0u; index < frames.size(); ++index)
        {
            const sf::Image image = PixelShipGenerator::SFMLImageAdapter::createSFMLImage(frames[index]);

            if (!m_AnimationTextures[index].loadFromImage(image))
            {
                std::cerr << "Failed to create animation texture.\n";
                m_AnimationTextures.clear();
                return false;
            }

            m_AnimationTextures[index].setSmooth(false);
        }

        if (m_AnimationFrameIndex >= m_AnimationTextures.size())
        {
            m_AnimationFrameIndex = 0u;
        }

        return !m_AnimationTextures.empty();
    }

    void ShipGeneratorPreviewApp::render()
    {
        updateCommandPanelState();
        PreviewRenderData data;
        data.Mode = m_PreviewMode;
        data.PreviewSprite = &m_PreviewSprite;
        data.CurrentStaticTexture = &m_PreviewTexture;
        data.NativePreviewTexture = &m_PreviewTexture;
        if ((m_PreviewMode == PreviewMode::ANIMATION || m_PreviewMode == PreviewMode::FRAME_INSPECTION) && m_AnimationFrameIndex < m_AnimationTextures.size())
        {
            data.NativePreviewTexture = &m_AnimationTextures[m_AnimationFrameIndex];
        }
        data.PinnedTexture = m_Comparison.Pinned.Valid ? &m_PinnedTexture : nullptr;
        data.Gallery = &m_GalleryState;
        data.Favorites = &m_FavoritesState;
        data.Recipe = &getCurrentRecipe();
        data.Locks = &m_Locks;
        data.Diagnostics = &m_Diagnostics;
        data.Comparison = &m_Comparison;
        data.Ship = &m_GeneratedShip;
        data.GenerationDebugInfo = &m_GenerationDebugInfo;
        data.SelectedAnimationType = m_SelectedAnimationType;
        data.IdleAnimation = &m_IdleAnimation;
        data.IdleAnimationSettings = &m_IdleAnimationSettings;
        data.MovementAnimation = &m_MovementAnimation;
        data.MovementAnimationSettings = &m_MovementAnimationSettings;
        data.MovementPhase = m_MovementAnimationPhase;
        data.RuntimeMovementType = m_RuntimeMovementType;
        data.PendingMovementType = m_PendingMovementType;
        data.MovementTransitionPending = m_MovementTransitionPending;
        data.TransientStatePreviewActive = m_TransientStatePreviewActive;
        data.FiringAnimation = &m_FiringAnimation;
        data.FiringAnimationSettings = &m_FiringAnimationSettings;
        data.HistoryIndex = m_HistoryIndex;
        data.HistoryCount = m_History.size();
        data.AnimationFrameIndex = m_AnimationFrameIndex;
        data.CommandPanel = &m_CommandPanel;
        data.CurrentIsFavorite = isCurrentFavorite();
        data.StatusMessage = &m_StatusMessage;
        data.CalibrationPair = m_PreviewMode == PreviewMode::CALIBRATION ? &m_CalibrationPair : nullptr;
        data.ObjectiveBatch = m_PreviewMode == PreviewMode::CALIBRATION && m_CalibrationObjectiveBatch.Valid ? &m_CalibrationObjectiveBatch : nullptr;
        data.CalibrationSession = m_PreviewMode == PreviewMode::CALIBRATION ? &m_CalibrationSession : nullptr;
        data.CalibrationGroup = m_CalibrationGroup;
        data.CalibrationShowValues = m_CalibrationShowValues;
        data.CalibrationFilter = getCalibrationContextFilter();
        data.CalibrationTextureA = m_CalibrationPair.Valid ? &m_CalibrationTextureA : nullptr;
        data.CalibrationTextureB = m_CalibrationPair.Valid ? &m_CalibrationTextureB : nullptr;
        data.RerollStudio = m_PreviewMode == PreviewMode::REROLL_STUDIO ? &m_RerollStudio : nullptr;
        data.RerollStudioCandidateTexture = m_RerollStudio.CandidateValid ? &m_RerollCandidateTexture : nullptr;
        m_Renderer.render(m_Window, data);
    }

    void ShipGeneratorPreviewApp::reroll()
    {
        if (m_Locks.Structure && m_Locks.Palette && m_Locks.Details && m_Locks.Attachments)
        {
            std::cout << "Reroll ignored: all generation channels are locked.\n";
            return;
        }

        PreviewGenerationRecipe recipe = getCurrentRecipe();

        if (!m_Locks.Structure)
        {
            recipe.Seeds.Structure = m_SeedGenerator();
            PixelShipGenerator::clearGenerationDomainOverridesForChannel(recipe.DomainSeedOverrides, PixelShipGenerator::GenerationSeedChannel::STRUCTURE);
        }

        if (!m_Locks.Palette)
        {
            recipe.Seeds.Palette = m_SeedGenerator();
            PixelShipGenerator::clearGenerationDomainOverridesForChannel(recipe.DomainSeedOverrides, PixelShipGenerator::GenerationSeedChannel::PALETTE);
        }

        if (!m_Locks.Details)
        {
            recipe.Seeds.Details = m_SeedGenerator();
            PixelShipGenerator::clearGenerationDomainOverridesForChannel(recipe.DomainSeedOverrides, PixelShipGenerator::GenerationSeedChannel::DETAILS);
        }

        if (!m_Locks.Attachments)
        {
            recipe.Seeds.Attachments = m_SeedGenerator();
            PixelShipGenerator::clearGenerationDomainOverridesForChannel(recipe.DomainSeedOverrides, PixelShipGenerator::GenerationSeedChannel::ATTACHMENTS);
        }

        appendHistoryEntry(recipe);
    }

    void ShipGeneratorPreviewApp::saveCurrent()
    {
        const PreviewGenerationRecipe& recipe = getCurrentRecipe();
        const std::string baseName = getSaveBaseName(recipe);

        if (m_PreviewMode == PreviewMode::ANIMATION || m_PreviewMode == PreviewMode::FRAME_INSPECTION)
        {
            const std::vector<PixelShipGenerator::Image>& frames = getActiveAnimationFrames();
            if (m_AnimationFrameIndex >= frames.size())
            {
                return;
            }

            std::string animationName = baseName + "_" + getAnimationTypeFileToken(m_SelectedAnimationType);
            if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE)
            {
                animationName += "_weapon" + std::to_string(m_FiringAnimation.Target.WeaponComponentIndex);
            }
            else if (m_SelectedAnimationType != PixelShipGenerator::ShipAnimationType::IDLE)
            {
                animationName += "_" + getMovementPhaseFileToken(m_MovementAnimationPhase);
            }
            const std::filesystem::path savePath = getAvailableSavePath(animationName + "_frame_" + getFrameNumberString(m_AnimationFrameIndex));
            saveCoreImage(frames[m_AnimationFrameIndex], savePath);
            return;
        }

        saveCoreImage(m_GeneratedShip.FinalImage, getAvailableSavePath(baseName));
    }

    void ShipGeneratorPreviewApp::savePreviewAppPreferences()
    {
        PreviewPreferences preferences;
        preferences.ResolutionBookmarks = m_ResolutionBookmarks;
        std::string error;
        if (!savePreviewPreferences(preferences, PreviewPreferencesPath, error))
        {
            setStatusMessage(error);
            std::cerr << error << '\n';
        }
    }

    void ShipGeneratorPreviewApp::saveSpritesheet()
    {
        if (m_TransientStatePreviewActive)
        {
            setStatusMessage("Composed transition/event previews are runtime-only. Export the base movement/FIRE assets separately.");
            return;
        }

        const PreviewGenerationRecipe& recipe = getCurrentRecipe();
        const std::string baseName = getSaveBaseName(recipe);

        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE)
        {
            if (m_IdleAnimation.Frames.empty())
            {
                return;
            }

            const PixelShipGenerator::Image spritesheet = PixelShipGenerator::createHorizontalSpritesheet(m_IdleAnimation);
            saveCoreImage(spritesheet, getAvailableSavePath(baseName + "_idle_" + std::to_string(m_IdleAnimation.Frames.size()) + "frames"));
            return;
        }

        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE)
        {
            if (m_FiringAnimation.Frames.empty()) { return; }
            const PixelShipGenerator::Image spritesheet = PixelShipGenerator::createHorizontalSpritesheet(m_FiringAnimation);
            const std::string fileName = baseName + "_fire_weapon" + std::to_string(m_FiringAnimation.Target.WeaponComponentIndex) + "_" + std::to_string(m_FiringAnimation.DurationMilliseconds) + "ms_" + std::to_string(m_FiringAnimation.Frames.size()) + "frames";
            saveCoreImage(spritesheet, getAvailableSavePath(fileName));
            return;
        }

        if (m_MovementAnimation.Type != m_SelectedAnimationType)
        {
            return;
        }

        const std::array<const PixelShipGenerator::ShipMovementAnimationClip*, 3u> clips =
        {
            &m_MovementAnimation.Enter,
            &m_MovementAnimation.Sustain,
            &m_MovementAnimation.Exit
        };

        for (const PixelShipGenerator::ShipMovementAnimationClip* clip : clips)
        {
            if (clip == nullptr || clip->Frames.empty())
            {
                continue;
            }

            const PixelShipGenerator::Image spritesheet = PixelShipGenerator::createHorizontalSpritesheet(*clip);
            const std::string fileName = baseName + "_" + getAnimationTypeFileToken(m_SelectedAnimationType) + "_" + getMovementPhaseFileToken(clip->Phase) + "_" + std::to_string(clip->Frames.size()) + "frames";
            saveCoreImage(spritesheet, getAvailableSavePath(fileName));
        }
    }

    void ShipGeneratorPreviewApp::selectGalleryCandidate(uint32_t index)
    {
        if (index >= m_GalleryState.Grid.Items.size())
        {
            return;
        }

        const PreviewThumbnailItem& candidate = m_GalleryState.Grid.Items[index];

        if (!candidate.Valid)
        {
            return;
        }

        m_PreviewMode = PreviewMode::STATIC;
        appendHistoryEntry(candidate.Recipe);
        m_GalleryState.Grid.Items.clear();
    }

    void ShipGeneratorPreviewApp::selectResolutionBookmark(uint32_t index)
    {
        if (index >= m_ResolutionBookmarks.size()) { return; }
        setDimensions(m_ResolutionBookmarks[index]);
    }

    std::optional<std::size_t> ShipGeneratorPreviewApp::findFavoriteIndex(const PreviewGenerationRecipe& recipe) const
    {
        const auto iterator = std::find_if(m_FavoritesState.Grid.Items.begin(), m_FavoritesState.Grid.Items.end(), [&](const PreviewThumbnailItem& favorite) { return favorite.Valid && favorite.Recipe == recipe; });
        if (iterator == m_FavoritesState.Grid.Items.end()) { return std::nullopt; }
        return static_cast<std::size_t>(std::distance(m_FavoritesState.Grid.Items.begin(), iterator));
    }

    const PixelShipGenerator::ShipMovementAnimationClip* ShipGeneratorPreviewApp::getActiveMovementClip() const
    {
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE || m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE || m_MovementAnimation.Type != m_SelectedAnimationType)
        {
            return nullptr;
        }
        return &PixelShipGenerator::getMovementAnimationClip(m_MovementAnimation, m_MovementAnimationPhase);
    }

    const std::vector<PixelShipGenerator::Image>& ShipGeneratorPreviewApp::getActiveAnimationFrames() const
    {
        if (m_TransientStatePreviewActive) { return m_StatePreviewFrames; }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE)
        {
            return m_IdleAnimation.Frames;
        }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE)
        {
            return m_FiringAnimation.Frames;
        }

        const PixelShipGenerator::ShipMovementAnimationClip* clip = getActiveMovementClip();
        if (clip != nullptr)
        {
            return clip->Frames;
        }

        static const std::vector<PixelShipGenerator::Image> EmptyFrames;
        return EmptyFrames;
    }

    double ShipGeneratorPreviewApp::getActiveAnimationFrameDurationMilliseconds() const
    {
        if (m_TransientStatePreviewActive) { return m_StatePreviewFrameDurationMilliseconds; }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE)
        {
            return m_IdleAnimation.FrameDurationMilliseconds;
        }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE)
        {
            return m_FiringAnimation.FrameDurationMilliseconds;
        }
        const PixelShipGenerator::ShipMovementAnimationClip* clip = getActiveMovementClip();
        return clip != nullptr ? clip->FrameDurationMilliseconds : 0.0;
    }

    uint64_t ShipGeneratorPreviewApp::getActiveAnimationSeed() const
    {
        if (m_TransientStatePreviewActive) { return m_FiringAnimation.Seed; }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE) { return m_IdleAnimation.Seed; }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE) { return m_FiringAnimation.Seed; }
        return m_MovementAnimation.Seed;
    }

    const PixelShipGenerator::AnimationSamplingPlan& ShipGeneratorPreviewApp::getActiveAnimationSampling() const
    {
        if (m_TransientStatePreviewActive) { return m_FiringAnimation.Sampling; }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE)
        {
            return m_IdleAnimation.Sampling;
        }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE)
        {
            return m_FiringAnimation.Sampling;
        }
        const PixelShipGenerator::ShipMovementAnimationClip* clip = getActiveMovementClip();
        return clip != nullptr ? clip->Sampling : m_IdleAnimation.Sampling;
    }

    uint32_t ShipGeneratorPreviewApp::getActiveAnimationDurationMilliseconds() const
    {
        if (m_TransientStatePreviewActive) { return m_FiringAnimation.DurationMilliseconds; }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE)
        {
            return m_IdleAnimation.DurationMilliseconds;
        }
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE)
        {
            return m_FiringAnimation.DurationMilliseconds;
        }
        const PixelShipGenerator::ShipMovementAnimationClip* clip = getActiveMovementClip();
        return clip != nullptr ? clip->DurationMilliseconds : 0u;
    }

    std::string ShipGeneratorPreviewApp::getAnimationEffectDisplay() const
    {
        std::string effects;
        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE)
        {
            if (m_IdleAnimationSettings.EngineFlicker) { effects += "E"; }
            if (m_IdleAnimationSettings.LightBlinking) { effects += "L"; }
            if (m_IdleAnimationSettings.MechanicalMicroMovement) { effects += "M"; }
            if (m_IdleAnimationSettings.HoverOffset) { effects += "H"; }
            if (m_IdleAnimationSettings.SmallDetailVariation) { effects += "D"; }
        }
        else if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE)
        {
            effects += "W";
            if (m_FiringAnimation.Diagnostics.PreFireMotion) { effects += "P"; }
        }
        else
        {
            if (m_MovementAnimationSettings.EngineVectoring) { effects += "E"; }
            if (m_MovementAnimationSettings.WeaponStabilization) { effects += "W"; }
            if (m_MovementAnimationSettings.AttachmentArticulation) { effects += "A"; }
        }
        return effects.empty() ? "NONE" : effects;
    }

    void ShipGeneratorPreviewApp::setDisplayedAnimationFrame(uint32_t frameIndex)
    {
        if (frameIndex >= m_AnimationTextures.size())
        {
            return;
        }

        m_AnimationFrameIndex = frameIndex;
        refreshDisplayedTexture();
    }

    void ShipGeneratorPreviewApp::setDisplayedStaticFrame()
    {
        refreshDisplayedTexture();
    }

    void ShipGeneratorPreviewApp::setFaction(PixelShipGenerator::ShipFactionType faction)
    {
        PreviewGenerationRecipe& recipe = getCurrentRecipe();

        if (recipe.Faction == faction)
        {
            return;
        }

        recipe.Faction = faction;
        regenerate();
    }

    void ShipGeneratorPreviewApp::setDimensions(const PixelShipGenerator::ShipDimensions& dimensions)
    {
        if (!isSelectablePreviewDimensions(dimensions)) { return; }
        PreviewGenerationRecipe& recipe = getCurrentRecipe();
        if (recipe.Dimensions == dimensions) { return; }
        recipe.Dimensions = dimensions;
        if (!dimensions.isSquare()) { m_AspectRatioLocked = false; }
        regenerate();
    }

    void ShipGeneratorPreviewApp::setHeight(uint32_t height)
    {
        height = clampPreviewDimensionValue(height);
        if (m_AspectRatioLocked) { setResolution(height); return; }
        const PixelShipGenerator::ShipDimensions current = getCurrentRecipe().Dimensions;
        height = std::clamp(height, getMinimumPreviewHeightForWidth(current.Width), getMaximumPreviewHeightForWidth(current.Width));
        setDimensions({ current.Width, height });
    }

    void ShipGeneratorPreviewApp::setResolution(uint32_t resolution)
    {
        if (!isSelectablePreviewResolution(resolution)) { return; }
        setDimensions({ resolution, resolution });
    }

    void ShipGeneratorPreviewApp::setWidth(uint32_t width)
    {
        width = clampPreviewDimensionValue(width);
        if (m_AspectRatioLocked) { setResolution(width); return; }
        const PixelShipGenerator::ShipDimensions current = getCurrentRecipe().Dimensions;
        width = std::clamp(width, getMinimumPreviewWidthForHeight(current.Height), getMaximumPreviewWidthForHeight(current.Height));
        setDimensions({ width, current.Height });
    }

    void ShipGeneratorPreviewApp::setStyle(PixelShipGenerator::ShipStyle style)
    {
        PreviewGenerationRecipe& recipe = getCurrentRecipe();

        if (recipe.Style == style)
        {
            return;
        }

        recipe.Style = style;
        regenerate();
    }

    void ShipGeneratorPreviewApp::toggleAspectRatioLock()
    {
        if (m_AspectRatioLocked)
        {
            m_AspectRatioLocked = false;
            updateWindowTitle();
            return;
        }

        m_AspectRatioLocked = true;
        const uint32_t squareValue = getCurrentRecipe().Dimensions.Width;
        setResolution(squareValue);
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::toggleAttachments()
    {
        PreviewGenerationRecipe& recipe = getCurrentRecipe();
        recipe.AttachmentsEnabled = !recipe.AttachmentsEnabled;
        regenerate();
    }


    bool ShipGeneratorPreviewApp::isCurrentFavorite() const
    {
        return findFavoriteIndex(getCurrentRecipe()).has_value();
    }

    bool ShipGeneratorPreviewApp::isDiagnosticImageViewActive() const
    {
        return m_Diagnostics.GenerationStageView || m_Diagnostics.ViewMode != DiagnosticViewMode::FINAL;
    }

    void ShipGeneratorPreviewApp::moveGenerationStage(int32_t delta)
    {
        if (!m_Diagnostics.GenerationStageView || m_GenerationDebugInfo.HullStages.empty())
        {
            return;
        }

        const int32_t stageCount = static_cast<int32_t>(m_GenerationDebugInfo.HullStages.size());
        int32_t stageIndex = static_cast<int32_t>(m_Diagnostics.GenerationStageIndex) + delta;
        if (stageIndex < 0) { stageIndex = stageCount - 1; }
        if (stageIndex >= stageCount) { stageIndex = 0; }
        m_Diagnostics.GenerationStageIndex = static_cast<uint32_t>(stageIndex);
        refreshDiagnosticTexture();
        refreshDisplayedTexture();
        updateWindowTitle();
    }

    bool ShipGeneratorPreviewApp::refreshDiagnosticTexture()
    {
        if (!isDiagnosticImageViewActive())
        {
            return true;
        }

        const PixelShipGenerator::Image diagnosticImage = createDiagnosticImage();
        const sf::Image image = PixelShipGenerator::SFMLImageAdapter::createSFMLImage(diagnosticImage);

        if (!m_DiagnosticTexture.loadFromImage(image))
        {
            std::cerr << "Failed to create diagnostic texture.\n";
            return false;
        }

        m_DiagnosticTexture.setSmooth(false);
        return true;
    }

    void ShipGeneratorPreviewApp::refreshDisplayedTexture()
    {
        if (isDiagnosticImageViewActive())
        {
            m_PreviewSprite.setTexture(m_DiagnosticTexture, true);
            return;
        }

        if ((m_PreviewMode == PreviewMode::ANIMATION || m_PreviewMode == PreviewMode::FRAME_INSPECTION) && m_AnimationFrameIndex < m_AnimationTextures.size())
        {
            m_PreviewSprite.setTexture(m_AnimationTextures[m_AnimationFrameIndex], true);
            return;
        }

        m_PreviewSprite.setTexture(m_PreviewTexture, true);
    }

    void ShipGeneratorPreviewApp::toggleComparisonView()
    {
        if (!m_Comparison.Pinned.Valid)
        {
            m_Comparison.ViewEnabled = false;
            std::cout << "Comparison view ignored: no ship is pinned.\n";
            updateWindowTitle();
            return;
        }

        m_Comparison.ViewEnabled = !m_Comparison.ViewEnabled;

        if (m_Comparison.ViewEnabled && (m_PreviewMode == PreviewMode::ANIMATION || m_PreviewMode == PreviewMode::FRAME_INSPECTION))
        {
            m_PreviewMode = PreviewMode::STATIC;
            setDisplayedStaticFrame();
        }

        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::toggleGenerationStageView()
    {
        if (m_GenerationDebugInfo.HullStages.empty())
        {
            return;
        }

        m_Diagnostics.GenerationStageView = !m_Diagnostics.GenerationStageView;
        m_Diagnostics.GenerationStageIndex = std::min(m_Diagnostics.GenerationStageIndex, static_cast<uint32_t>(m_GenerationDebugInfo.HullStages.size() - 1u));
        refreshDiagnosticTexture();
        refreshDisplayedTexture();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::toggleGenerationInspector()
    {
        if (m_PreviewMode == PreviewMode::GALLERY || m_PreviewMode == PreviewMode::FAVORITES) { return; }

        const bool newValue = !m_Diagnostics.GenerationInspectorVisible;
        m_Diagnostics.HelpVisible = false;
        m_Diagnostics.PaletteInspectorVisible = false;
        m_Diagnostics.GenerationInspectorVisible = newValue;
    }

    void ShipGeneratorPreviewApp::toggleHelpOverlay()
    {
        const bool newValue = !m_Diagnostics.HelpVisible;
        m_Diagnostics.GenerationInspectorVisible = false;
        m_Diagnostics.PaletteInspectorVisible = false;
        m_Diagnostics.HelpVisible = newValue;
    }

    void ShipGeneratorPreviewApp::togglePaletteInspector()
    {
        if (m_PreviewMode == PreviewMode::GALLERY || m_PreviewMode == PreviewMode::FAVORITES) { return; }

        const bool newValue = !m_Diagnostics.PaletteInspectorVisible;
        m_Diagnostics.HelpVisible = false;
        m_Diagnostics.GenerationInspectorVisible = false;
        m_Diagnostics.PaletteInspectorVisible = newValue;
    }

    void ShipGeneratorPreviewApp::update()
    {
        const std::vector<PixelShipGenerator::Image>& frames = getActiveAnimationFrames();
        if (m_PreviewMode != PreviewMode::ANIMATION || frames.empty())
        {
            return;
        }

        const double frameDurationMicroseconds = std::max(1.0, getActiveAnimationFrameDurationMilliseconds() * 1000.0);
        const double elapsedMicroseconds = std::max(0.0, static_cast<double>(m_AnimationClock.restart().asMicroseconds()));
        m_AnimationPlaybackAccumulatorMicroseconds += elapsedMicroseconds;
        if (m_AnimationPlaybackAccumulatorMicroseconds < frameDurationMicroseconds)
        {
            return;
        }

        const uint32_t elapsedFrames = std::max(1u, static_cast<uint32_t>(m_AnimationPlaybackAccumulatorMicroseconds / frameDurationMicroseconds));
        m_AnimationPlaybackAccumulatorMicroseconds -= static_cast<double>(elapsedFrames) * frameDurationMicroseconds;
        const uint32_t frameCount = static_cast<uint32_t>(frames.size());

        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE)
        {
            const uint64_t targetFrame = static_cast<uint64_t>(m_AnimationFrameIndex) + static_cast<uint64_t>(elapsedFrames);
            if (targetFrame < frameCount)
            {
                setDisplayedAnimationFrame(static_cast<uint32_t>(targetFrame));
            }
            else if (m_TransientStatePreviewActive)
            {
                m_TransientStatePreviewActive = false;
                m_StatePreviewFrames.clear();
                m_AnimationFrameIndex = 0u;
                m_AnimationPlaybackAccumulatorMicroseconds = 0.0;

                if (m_RuntimeMovementType != PixelShipGenerator::ShipAnimationType::IDLE)
                {
                    m_SelectedAnimationType = m_RuntimeMovementType;
                    m_MovementAnimationPhase = PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN;
                    m_RuntimeMovementNormalizedTime = m_ResumeMovementNormalizedTime;
                    if (regenerateAnimation())
                    {
                        const PixelShipGenerator::ShipMovementAnimationClip* sustain = getActiveMovementClip();
                        if (sustain != nullptr && !sustain->Frames.empty())
                        {
                            const uint32_t index = static_cast<uint32_t>(std::floor(m_RuntimeMovementNormalizedTime * static_cast<double>(sustain->Frames.size()))) % static_cast<uint32_t>(sustain->Frames.size());
                            m_PreviewMode = PreviewMode::ANIMATION;
                            setDisplayedAnimationFrame(index);
                        }
                    }
                }
                else
                {
                    m_SelectedAnimationType = PixelShipGenerator::ShipAnimationType::IDLE;
                    if (regenerateAnimation())
                    {
                        m_PreviewMode = PreviewMode::ANIMATION;
                        setDisplayedAnimationFrame(0u);
                    }
                }
            }
            else
            {
                m_PreviewMode = PreviewMode::STATIC;
                m_AnimationFrameIndex = 0u;
                m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
                setDisplayedStaticFrame();
            }
            updateWindowTitle();
            return;
        }

        if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE || m_MovementAnimationPhase == PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN)
        {
            const uint32_t nextFrame = (m_AnimationFrameIndex + elapsedFrames) % frameCount;
            setDisplayedAnimationFrame(nextFrame);
            if (m_SelectedAnimationType == m_RuntimeMovementType && isMovementAnimationType(m_SelectedAnimationType))
            {
                const PixelShipGenerator::ShipMovementAnimationClip* sustain = getActiveMovementClip();
                if (sustain != nullptr && nextFrame < sustain->NormalizedSampleTimes.size()) { m_RuntimeMovementNormalizedTime = sustain->NormalizedSampleTimes[nextFrame]; }
            }
            updateWindowTitle();
            return;
        }

        const uint64_t targetFrame = static_cast<uint64_t>(m_AnimationFrameIndex) + static_cast<uint64_t>(elapsedFrames);
        if (m_MovementAnimationPhase == PixelShipGenerator::ShipMovementAnimationPhase::ENTER)
        {
            if (targetFrame < frameCount)
            {
                setDisplayedAnimationFrame(static_cast<uint32_t>(targetFrame));
            }
            else
            {
                const uint64_t leftoverFrames = targetFrame - frameCount;
                m_MovementAnimationPhase = PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN;
                refreshAnimationTextures();
                const PixelShipGenerator::ShipMovementAnimationClip* sustain = getActiveMovementClip();
                const uint32_t sustainCount = sustain != nullptr ? static_cast<uint32_t>(sustain->Frames.size()) : 0u;
                if (sustainCount > 0u)
                {
                    const uint32_t nextFrame = static_cast<uint32_t>(leftoverFrames % sustainCount);
                    setDisplayedAnimationFrame(nextFrame);
                    if (sustain != nullptr && nextFrame < sustain->NormalizedSampleTimes.size()) { m_RuntimeMovementNormalizedTime = sustain->NormalizedSampleTimes[nextFrame]; }
                }
            }
            updateWindowTitle();
            return;
        }

        if (targetFrame < frameCount)
        {
            setDisplayedAnimationFrame(static_cast<uint32_t>(targetFrame));
        }
        else if (m_MovementTransitionPending)
        {
            const PixelShipGenerator::ShipAnimationType target = m_PendingMovementType;
            m_MovementTransitionPending = false;
            m_PendingMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
            m_AnimationFrameIndex = 0u;
            m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
            m_RuntimeMovementNormalizedTime = 0.0;

            if (target == PixelShipGenerator::ShipAnimationType::IDLE)
            {
                m_RuntimeMovementType = PixelShipGenerator::ShipAnimationType::IDLE;
                m_SelectedAnimationType = PixelShipGenerator::ShipAnimationType::IDLE;
                if (regenerateAnimation())
                {
                    m_PreviewMode = PreviewMode::ANIMATION;
                    setDisplayedAnimationFrame(0u);
                }
            }
            else
            {
                m_RuntimeMovementType = target;
                m_SelectedAnimationType = target;
                m_MovementAnimationPhase = PixelShipGenerator::ShipMovementAnimationPhase::ENTER;
                if (regenerateAnimation())
                {
                    m_PreviewMode = PreviewMode::ANIMATION;
                    setDisplayedAnimationFrame(0u);
                }
            }
        }
        else
        {
            m_PreviewMode = PreviewMode::STATIC;
            m_AnimationFrameIndex = 0u;
            m_AnimationPlaybackAccumulatorMicroseconds = 0.0;
            setDisplayedStaticFrame();
        }
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::updateCommandPanelState()
    {
        m_CommandPanel.updateState(createCommandPanelState());
    }

    void ShipGeneratorPreviewApp::setStatusMessage(const std::string& message)
    {
        m_StatusMessage = message;
    }

    void ShipGeneratorPreviewApp::updateWindowTitle()
    {
        if (m_PreviewMode == PreviewMode::GALLERY)
        {
            const std::string title = "Pixel Ship Generator | Gallery | BatchSeed: " + std::to_string(m_GalleryState.BatchSeed) + " | " + std::to_string(m_GalleryState.CandidateCount) + " candidates | Style: " + getStyleName(m_GalleryState.TemplateRecipe.Style) + " | Faction: " + getFactionName(m_GalleryState.TemplateRecipe.Faction) + " | " + std::to_string(m_GalleryState.TemplateRecipe.Dimensions.Width) + "x" + std::to_string(m_GalleryState.TemplateRecipe.Dimensions.Height) + " | Selected " + std::to_string(m_GalleryState.Grid.SelectedIndex + 1u) + "/" + std::to_string(m_GalleryState.Grid.Items.size());
            m_Window.setTitle(title);
            return;
        }

        if (m_PreviewMode == PreviewMode::FAVORITES)
        {
            const std::string selected = m_FavoritesState.Grid.Items.empty() ? "0/0" : std::to_string(m_FavoritesState.Grid.SelectedIndex + 1u) + "/" + std::to_string(m_FavoritesState.Grid.Items.size());
            m_Window.setTitle("Pixel Ship Generator | Favorites | Selected " + selected + " | Current seed: " + std::to_string(getCurrentRecipe().Seeds.Master));
            return;
        }

        if (m_PreviewMode == PreviewMode::REROLL_STUDIO)
        {
            const std::size_t selectedCount = static_cast<std::size_t>(std::count(m_RerollStudio.SelectedDomains.begin(), m_RerollStudio.SelectedDomains.end(), true));
            const std::string candidate = m_RerollStudio.CandidateValid ? ("Candidate " + std::to_string(m_RerollStudio.CandidateSequence)) : "No Candidate";
            m_Window.setTitle("Pixel Ship Generator | Attribute Reroll Studio | Selected " + std::to_string(selectedCount) + " domains | " + candidate + " | Base seed: " + std::to_string(m_RerollStudio.BaseRecipe.Seeds.Master));
            return;
        }

        const PreviewGenerationRecipe& recipe = getCurrentRecipe();
        const std::string attachmentState = recipe.AttachmentsEnabled ? "ON" : "OFF";
        std::string title = "Pixel Ship Generator | Seed: " + std::to_string(recipe.Seeds.Master) + " | " + std::to_string(recipe.Dimensions.Width) + "x" + std::to_string(recipe.Dimensions.Height) + " | Style: " + getStyleName(recipe.Style) + " | Faction: " + getFactionName(recipe.Faction) + " | Attachments: " + attachmentState + " | Favorite: " + (isCurrentFavorite() ? "YES" : "NO") + " | Favorites: " + std::to_string(m_FavoritesState.Grid.Items.size()) + " | S:" + getLockDisplay(m_Locks.Structure) + " P:" + getLockDisplay(m_Locks.Palette) + " D:" + getLockDisplay(m_Locks.Details) + " A:" + getLockDisplay(m_Locks.Attachments) + " | History " + std::to_string(m_HistoryIndex + 1u) + "/" + std::to_string(m_History.size());

        if (m_Comparison.ViewEnabled && m_Comparison.Pinned.Valid)
        {
            title += " | Compare: PIN " + std::to_string(m_Comparison.Pinned.Recipe.Seeds.Master);
        }

        const std::vector<PixelShipGenerator::Image>& animationFrames = getActiveAnimationFrames();
        if ((m_PreviewMode == PreviewMode::ANIMATION || m_PreviewMode == PreviewMode::FRAME_INSPECTION) && !animationFrames.empty())
        {
            title += m_PreviewMode == PreviewMode::ANIMATION ? " | Anim: PLAY " : " | Anim: FRAME ";
            title += getAnimationTypeDisplayName(m_SelectedAnimationType);
            if (m_SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE)
            {
                title += " | Weapon: " + std::to_string(m_FiringAnimation.Target.WeaponComponentIndex);
                if (m_TransientStatePreviewActive)
                {
                    title += " | Underlying: " + getAnimationTypeDisplayName(m_RuntimeMovementType);
                }
            }
            else if (m_SelectedAnimationType != PixelShipGenerator::ShipAnimationType::IDLE)
            {
                title += " | Phase: " + getMovementPhaseDisplayName(m_MovementAnimationPhase);
                if (m_MovementTransitionPending)
                {
                    title += " | Next: " + getAnimationTypeDisplayName(m_PendingMovementType);
                }
            }
            title += " | Frame " + std::to_string(m_AnimationFrameIndex + 1u) + "/" + std::to_string(animationFrames.size());
            title += " | AnimationSeed: " + std::to_string(getActiveAnimationSeed());
            title += " | FX: " + getAnimationEffectDisplay();
        }

        m_Window.setTitle(title);
    }

    PreviewGenerationRecipe& ShipGeneratorPreviewApp::getCurrentRecipe()
    {
        return m_History[m_HistoryIndex];
    }

    const PreviewGenerationRecipe& ShipGeneratorPreviewApp::getCurrentRecipe() const
    {
        return m_History[m_HistoryIndex];
    }
}
