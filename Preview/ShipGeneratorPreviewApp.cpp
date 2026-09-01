#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

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

#if defined(_WIN32)
#include <Windows.h>
#ifdef DUPLICATE
#undef DUPLICATE
#endif
#endif

#include "SFMLImageAdapter.h"
#include <SpectralShipGen/BuiltInPresetCatalog.h>
#include "GenerationCalibration.h"
#include "GenerationCalibrationSerializer.h"
#include "PreviewCommand.h"
#include "PreviewFavoritesPersistence.h"
#include "PreviewPreferences.h"
#include "UserPresetPersistence.h"
#include "ShipGenerationRecipeSerializer.h"
#include "PreviewThumbnailGrid.h"
#include <SpectralShipGen/ShipGenerationSeeds.h>
#include <SpectralShipGen/ShipGenerationSettings.h>
#include <SpectralShipGen/ShipSpritesheetUtils.h>

namespace
{
    using namespace SpectralShipGenStudioPreview;

    constexpr uint32_t GalleryCandidateMaximumAttempts = 16u;
    const std::filesystem::path PreviewPreferencesPath = "spectral_ship_gen_preview_preferences.json";
    const std::filesystem::path PreviewFavoritesPath = "spectral_ship_gen_preview_favorites.json";
    const std::filesystem::path UserPresetLibraryPath = "spectral_ship_gen_preview_user_presets.json";
    const std::filesystem::path CalibrationSessionPath = "generation_calibration_session.json";
    const std::filesystem::path CalibrationReportPath = "generation_calibration_report.csv";
    const std::filesystem::path CalibrationTuningProfilePath = "generation_tuning_profile.json";

    constexpr uint32_t PreviewFallbackDesktopFrameReserveWidth = 64u;
    constexpr uint32_t PreviewFallbackDesktopFrameReserveHeight = 64u;

    PreviewPhysicalSize getPreviewAvailableClientArea()
    {
#if defined(_WIN32)
        RECT workArea{};
        if (SystemParametersInfoW(SPI_GETWORKAREA, 0u, &workArea, 0u) != FALSE)
        {
            RECT requestedClient{ 0, 0, static_cast<LONG>(PreviewWindowWidth), static_cast<LONG>(PreviewWindowHeight) };
            if (AdjustWindowRect(&requestedClient, WS_OVERLAPPEDWINDOW, FALSE) != FALSE)
            {
                const uint32_t workAreaWidth = static_cast<uint32_t>(std::max<LONG>(1, workArea.right - workArea.left));
                const uint32_t workAreaHeight = static_cast<uint32_t>(std::max<LONG>(1, workArea.bottom - workArea.top));
                const uint32_t outerWidth = static_cast<uint32_t>(requestedClient.right - requestedClient.left);
                const uint32_t outerHeight = static_cast<uint32_t>(requestedClient.bottom - requestedClient.top);
                const uint32_t frameWidth = outerWidth > PreviewWindowWidth ? outerWidth - PreviewWindowWidth : 0u;
                const uint32_t frameHeight = outerHeight > PreviewWindowHeight ? outerHeight - PreviewWindowHeight : 0u;
                return {
                    workAreaWidth > frameWidth ? workAreaWidth - frameWidth : 1u,
                    workAreaHeight > frameHeight ? workAreaHeight - frameHeight : 1u
                };
            }
        }
#endif

        const sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
        return {
            desktopMode.width > PreviewFallbackDesktopFrameReserveWidth ? desktopMode.width - PreviewFallbackDesktopFrameReserveWidth : desktopMode.width,
            desktopMode.height > PreviewFallbackDesktopFrameReserveHeight ? desktopMode.height - PreviewFallbackDesktopFrameReserveHeight : desktopMode.height
        };
    }

    sf::VideoMode getInitialPreviewVideoMode()
    {
        const PreviewPhysicalSize availableClientArea = getPreviewAvailableClientArea();
        const PreviewPhysicalSize initialSize = fitPreviewWindowToAvailableClientArea(
            availableClientArea.Width,
            availableClientArea.Height,
            PreviewWindowWidth,
            PreviewWindowHeight);
        return sf::VideoMode(initialSize.Width, initialSize.Height);
    }


    template <typename T, std::size_t Size>
    std::size_t findOrderedValueIndex(const std::array<T, Size>& values, const T& value)
    {
        const auto iterator = std::find(values.begin(), values.end(), value);
        return iterator == values.end() ? 0u : static_cast<std::size_t>(std::distance(values.begin(), iterator));
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

    SpectralShipGenStudioPreview::PreviewGenerationRecipe createGalleryCandidateRecipe(const SpectralShipGenStudioPreview::PreviewGenerationRecipe& templateRecipe, uint64_t masterSeed)
    {
        SpectralShipGenStudioPreview::PreviewGenerationRecipe recipe = templateRecipe;
        recipe.Seeds = SpectralShipGen::deriveShipGenerationSeeds(masterSeed);
        recipe.DomainSeedOverrides.clearAll();
        return recipe;
    }

    SpectralShipGenStudioPreview::PreviewGenerationRecipe createRecipeFromMasterSeed(uint64_t masterSeed, const SpectralShipGenStudioPreview::PreviewGenerationRecipe& currentRecipe)
    {
        SpectralShipGenStudioPreview::PreviewGenerationRecipe recipe = currentRecipe;
        recipe.Seeds = SpectralShipGen::deriveShipGenerationSeeds(masterSeed);
        recipe.DomainSeedOverrides.clearAll();
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
        return SpectralShipGen::mixGenerationSeed64(batchSeed ^ 0xA24BAED4963EE407ull ^ indexValue ^ attemptValue);
    }


    std::string getFactionName(SpectralShipGen::ShipFactionType faction)
    {
        switch (faction)
        {
        case SpectralShipGen::ShipFactionType::FRONTIER: return "frontier";
        case SpectralShipGen::ShipFactionType::MILITARY: return "military";
        case SpectralShipGen::ShipFactionType::ASCENDANT: return "ascendant";
        case SpectralShipGen::ShipFactionType::XENO: return "xeno";
        case SpectralShipGen::ShipFactionType::CORPORATE: return "corporate";
        case SpectralShipGen::ShipFactionType::RELIC: return "relic";
        default: return "unknown";
        }
    }

    std::string getFactionDisplayName(SpectralShipGen::ShipFactionType faction)
    {
        switch (faction)
        {
        case SpectralShipGen::ShipFactionType::FRONTIER: return "FRONTIER";
        case SpectralShipGen::ShipFactionType::MILITARY: return "MILITARY";
        case SpectralShipGen::ShipFactionType::ASCENDANT: return "ASCENDANT";
        case SpectralShipGen::ShipFactionType::XENO: return "XENO";
        case SpectralShipGen::ShipFactionType::CORPORATE: return "CORPORATE";
        case SpectralShipGen::ShipFactionType::RELIC: return "RELIC";
        default: return "UNKNOWN";
        }
    }

    std::string getLockDisplay(bool locked)
    {
        return locked ? "LOCK" : "OPEN";
    }

    std::string getResolutionString(const SpectralShipGenStudioPreview::PreviewGenerationRecipe& recipe)
    {
        if (recipe.Dimensions.Width == recipe.Dimensions.Height)
        {
            return std::to_string(recipe.Dimensions.Width);
        }

        return std::to_string(recipe.Dimensions.Width) + "x" + std::to_string(recipe.Dimensions.Height);
    }

    std::string getStyleName(SpectralShipGen::ShipStyle style)
    {
        switch (style)
        {
        case SpectralShipGen::ShipStyle::SLEEK: return "SLEEK";
        case SpectralShipGen::ShipStyle::FIGHTER: return "FIGHTER";
        case SpectralShipGen::ShipStyle::HEAVY: return "HEAVY";
        case SpectralShipGen::ShipStyle::INDUSTRIAL: return "INDUSTRIAL";
        case SpectralShipGen::ShipStyle::SPEARHEAD: return "SPEARHEAD";
        case SpectralShipGen::ShipStyle::DELTA: return "DELTA";
        default: return "UNKNOWN";
        }
    }

    std::string getRecipeStructuralDisplayName(const SpectralShipGenStudioPreview::PreviewGenerationRecipe& recipe)
    {
        return recipe.StructuralPreset.has_value() ? getStyleName(*recipe.StructuralPreset) : "CUSTOM";
    }

    std::string getRecipeFactionDisplayName(const SpectralShipGenStudioPreview::PreviewGenerationRecipe& recipe)
    {
        return recipe.FactionPreset.has_value() ? getFactionDisplayName(*recipe.FactionPreset) : "CUSTOM";
    }

    std::string getRecipeFactionFileToken(const SpectralShipGenStudioPreview::PreviewGenerationRecipe& recipe)
    {
        return recipe.FactionPreset.has_value() ? getFactionName(*recipe.FactionPreset) : "custom";
    }

    SpectralShipGen::ShipPaletteGenerationProfile getRecipeFactionPaletteGenerationProfile(const SpectralShipGenStudioPreview::PreviewGenerationRecipe& recipe)
    {
        return recipe.FactionPreset.has_value()
            ? SpectralShipGen::getShipPaletteGenerationProfile(*recipe.FactionPreset)
            : SpectralShipGen::getShipPaletteGenerationProfile(recipe.FactionProfile);
    }

    std::string getAnimationTypeDisplayName(SpectralShipGen::ShipAnimationType type)
    {
        switch (type)
        {
        case SpectralShipGen::ShipAnimationType::IDLE: return "IDLE";
        case SpectralShipGen::ShipAnimationType::MOVE_LEFT: return "MOVE LEFT";
        case SpectralShipGen::ShipAnimationType::MOVE_RIGHT: return "MOVE RIGHT";
        case SpectralShipGen::ShipAnimationType::MOVE_UP: return "MOVE UP";
        case SpectralShipGen::ShipAnimationType::MOVE_DOWN: return "MOVE DOWN";
        case SpectralShipGen::ShipAnimationType::FIRE: return "FIRE";
        default: return "UNSUPPORTED";
        }
    }

    std::string getPlaybackSpeedDisplay(double speed)
    {
        if (speed <= 0.25) { return "0.25x"; }
        if (speed <= 0.5) { return "0.5x"; }
        if (speed <= 1.0) { return "1x"; }
        if (speed <= 2.0) { return "2x"; }
        return "4x";
    }

    std::string getAnimationTypeFileToken(SpectralShipGen::ShipAnimationType type)
    {
        switch (type)
        {
        case SpectralShipGen::ShipAnimationType::IDLE: return "idle";
        case SpectralShipGen::ShipAnimationType::MOVE_LEFT: return "move_left";
        case SpectralShipGen::ShipAnimationType::MOVE_RIGHT: return "move_right";
        case SpectralShipGen::ShipAnimationType::MOVE_UP: return "move_up";
        case SpectralShipGen::ShipAnimationType::MOVE_DOWN: return "move_down";
        case SpectralShipGen::ShipAnimationType::FIRE: return "fire";
        default: return "animation";
        }
    }

    std::string getMovementPhaseDisplayName(SpectralShipGen::ShipMovementAnimationPhase phase)
    {
        switch (phase)
        {
        case SpectralShipGen::ShipMovementAnimationPhase::ENTER: return "ENTER";
        case SpectralShipGen::ShipMovementAnimationPhase::SUSTAIN: return "SUSTAIN";
        case SpectralShipGen::ShipMovementAnimationPhase::EXIT: return "EXIT";
        default: return "UNKNOWN";
        }
    }

    std::string getMovementPhaseFileToken(SpectralShipGen::ShipMovementAnimationPhase phase)
    {
        switch (phase)
        {
        case SpectralShipGen::ShipMovementAnimationPhase::ENTER: return "enter";
        case SpectralShipGen::ShipMovementAnimationPhase::SUSTAIN: return "sustain";
        case SpectralShipGen::ShipMovementAnimationPhase::EXIT: return "exit";
        default: return "phase";
        }
    }

    std::string getWeaponTypeDisplayName(SpectralShipGen::ShipWeaponType type)
    {
        switch (type)
        {
        case SpectralShipGen::ShipWeaponType::SINGLE_CANNON: return "SINGLE CANNON";
        case SpectralShipGen::ShipWeaponType::TWIN_CANNON: return "TWIN CANNON";
        case SpectralShipGen::ShipWeaponType::COMPACT_TURRET: return "COMPACT TURRET";
        case SpectralShipGen::ShipWeaponType::RAIL_WEAPON: return "RAIL WEAPON";
        case SpectralShipGen::ShipWeaponType::WEAPON_POD: return "WEAPON POD";
        default: return "WEAPON";
        }
    }

    std::string getSaveBaseName(const SpectralShipGenStudioPreview::PreviewGenerationRecipe& recipe)
    {
        return "ship_" + getResolutionString(recipe) + "_" + getRecipeStructuralDisplayName(recipe) + "_" + getRecipeFactionFileToken(recipe) + "_seed_" + std::to_string(recipe.Seeds.Master);
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

    std::string getFileSafePresetName(const std::string& displayName)
    {
        std::string name = displayName;
        for (char& character : name)
        {
            const unsigned char value = static_cast<unsigned char>(character);
            if (value < 32u || character == '<' || character == '>' || character == ':' || character == '"' ||
                character == '/' || character == '\\' || character == '|' || character == '?' || character == '*')
            {
                character = '_';
            }
        }
        while (!name.empty() && (name.back() == ' ' || name.back() == '.')) { name.pop_back(); }
        return name.empty() ? "user_preset" : name;
    }

    std::filesystem::path getAvailableUserPresetPath(const std::string& displayName)
    {
        return getAvailableOutputPath("preset_" + getFileSafePresetName(displayName), ".shipgenpreset.json");
    }

    std::filesystem::path getAvailableConfigurationBundlePath(const std::string& displayName)
    {
        return getAvailableOutputPath("configuration_" + getFileSafePresetName(displayName), ".shipgenbundle.json");
    }

    std::string getFrameNumberString(uint32_t frameIndex)
    {
        return frameIndex + 1u < 10u ? "0" + std::to_string(frameIndex + 1u) : std::to_string(frameIndex + 1u);
    }

    bool saveCoreImage(const SpectralShipGen::Image& image, const std::filesystem::path& path)
    {
        const sf::Image sfmlImage = SpectralShipGen::SFMLImageAdapter::createSFMLImage(image);

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

    bool readUserPresetPathFromConsole(std::filesystem::path& path)
    {
        std::cout << "Enter .shipgenpreset.json path: ";
        std::string input;
        std::getline(std::cin >> std::ws, input);
        if (input.empty())
        {
            std::cerr << "User preset path cannot be empty.\n";
            return false;
        }
        path = std::filesystem::path(input);
        return true;
    }

    UserPresetCategory userPresetCategory(ConfigurationEditorProfileKind kind)
    {
        switch (kind)
        {
        case ConfigurationEditorProfileKind::STRUCTURAL: return UserPresetCategory::STRUCTURAL;
        case ConfigurationEditorProfileKind::FACTION: return UserPresetCategory::FACTION;
        case ConfigurationEditorProfileKind::PALETTE: return UserPresetCategory::PALETTE;
        case ConfigurationEditorProfileKind::FULL_CONFIGURATION: return UserPresetCategory::FULL_CONFIGURATION;
        default: return UserPresetCategory::USER_PRESET_CATEGORY_END;
        }
    }

    bool isSupportedPreviewRecipeResolution(const SpectralShipGenStudioPreview::PreviewGenerationRecipe& recipe)
    {
        return SpectralShipGenStudioPreview::isSelectablePreviewDimensions(recipe.Dimensions);
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

namespace SpectralShipGenStudioPreview
{
    ShipGeneratorPreviewApp::ShipGeneratorPreviewApp(std::string startupRecipePath)
        : m_Window(getInitialPreviewVideoMode(), "SpectralShipGen Studio", sf::Style::Titlebar | sf::Style::Resize | sf::Style::Close), m_SeedGenerator(createSeedGenerator()), m_StartupRecipePath(std::move(startupRecipePath))
    {
        m_Window.setVerticalSyncEnabled(true);
        m_Window.setKeyRepeatEnabled(false);
        m_LogicalView.reset(sf::FloatRect(0.0f, 0.0f, static_cast<float>(PreviewWindowWidth), static_cast<float>(PreviewWindowHeight)));
        updateLogicalWindowView(m_Window.getSize().x, m_Window.getSize().y);

        PreviewGenerationRecipe initialRecipe;
        initialRecipe.Seeds = SpectralShipGen::deriveShipGenerationSeeds(m_SeedGenerator());
        m_Collections = PreviewCollectionSession(initialRecipe);
        m_CalibrationSession = createGenerationCalibrationSession(m_SeedGenerator());
        loadPreviewAppPreferences();
        loadUserPresetLibraryState();
        loadFavoriteCollection();
    }

    int ShipGeneratorPreviewApp::run()
    {
        printControls();
        regenerate();
        rebuildFavoriteThumbnails();
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
        const PreviewGenerationRecipe recipe = getCurrentRecipe();
        if (!m_Collections.addFavorite(recipe)) { return; }
        addFavoriteThumbnail(recipe, m_PreviewTexture);
        refreshGalleryFavoriteMarkers();
        saveFavoriteCollection();
        updateWindowTitle();
        setStatusMessage("Favorite added.");
        std::cout << "Added Favorite: seed " << recipe.Seeds.Master << '\n';
    }

    void ShipGeneratorPreviewApp::addFavoriteThumbnail(const PreviewGenerationRecipe& recipe, const sf::Texture& texture)
    {
        PreviewThumbnailItem favorite;
        favorite.Recipe = recipe;
        favorite.Texture = texture;
        favorite.Texture.setSmooth(false);
        favorite.Valid = true;
        favorite.Favorite = true;
        m_FavoritesState.Grid.Items.push_back(std::move(favorite));
        m_FavoritesState.Grid.SelectedIndex = static_cast<uint32_t>(m_FavoritesState.Grid.Items.size() - 1u);
        m_FavoritesState.Grid.HoveredIndex = -1;
        updateCommandPanelState();
    }

    void ShipGeneratorPreviewApp::addResolutionBookmark()
    {
        const SpectralShipGen::ShipDimensions dimensions = getCurrentRecipe().Dimensions;
        if (!m_Collections.addResolutionBookmark(dimensions)) { return; }
        savePreviewAppPreferences();
        setStatusMessage("Resolution bookmark added: " + std::to_string(dimensions.Width) + "x" + std::to_string(dimensions.Height));
    }

    void ShipGeneratorPreviewApp::appendHistoryEntry(const PreviewGenerationRecipe& recipe)
    {
        m_Collections.appendHistoryEntry(recipe);
        regenerate();
    }

    void ShipGeneratorPreviewApp::changeFaction(int32_t delta)
    {
        const std::vector<FactionProfileSelectionEntry> entries = buildFactionProfileSelection(m_CustomPresetWorkspace);
        const std::size_t selectableCount = entries.empty() ? 0u : entries.size() - 1u;
        if (selectableCount == 0u) { return; }
        const std::size_t currentIndex = findFactionProfileSelectionIndex(entries, getCurrentRecipe(), m_SelectedFactionPresetId);
        selectFactionProfileEntry(entries[getWrappedPreviewSelectorIndex(currentIndex, delta, selectableCount)]);
    }

    void ShipGeneratorPreviewApp::changePalette(int32_t delta)
    {
        const std::vector<PaletteProfileSelectionEntry> entries = buildPaletteProfileSelection(m_CustomPresetWorkspace);
        const std::size_t selectableCount = entries.empty() ? 0u : entries.size() - 1u;
        if (selectableCount == 0u) { return; }
        const std::size_t currentIndex = findPaletteProfileSelectionIndex(entries, getCurrentRecipe(), m_SelectedBuiltInPalettePreset, m_SelectedPalettePresetId);
        selectPaletteProfileEntry(entries[getWrappedPreviewSelectorIndex(currentIndex, delta, selectableCount)]);
    }

    void ShipGeneratorPreviewApp::changeConfigurationBundle(int32_t delta)
    {
        const auto& bundles = m_CustomPresetWorkspace.getConfigurationBundles();
        if (bundles.empty()) { m_SelectedConfigurationBundleId.reset(); return; }

        const std::size_t entryCount = bundles.size() + 1u; // Individual Components + saved bundles.
        std::size_t currentIndex = 0u;
        if (m_SelectedConfigurationBundleId.has_value())
        {
            const auto iterator = std::find_if(bundles.begin(), bundles.end(), [&](const RuntimeConfigurationBundle& entry) { return entry.Id == *m_SelectedConfigurationBundleId; });
            if (iterator != bundles.end()) { currentIndex = static_cast<std::size_t>(std::distance(bundles.begin(), iterator)) + 1u; }
        }

        const std::size_t nextIndex = getWrappedPreviewSelectorIndex(currentIndex, delta, entryCount);
        if (nextIndex == 0u)
        {
            m_SelectedConfigurationBundleId.reset();
            setStatusMessage("Using individual configuration components.");
            updateWindowTitle();
            return;
        }
        applyRuntimeConfigurationBundle(bundles[nextIndex - 1u].Id);
    }

    void ShipGeneratorPreviewApp::changeProfilesSection(int32_t delta)
    {
        m_ProfilesSection = getWrappedProfilesSection(m_ProfilesSection, delta);
        if (m_ProfilesSection == ProfilesSection::FULL_CONFIGURATION)
        {
            const auto& bundles = m_CustomPresetWorkspace.getConfigurationBundles();
            if (bundles.empty()) { m_ProfilesSelectedBundleId.reset(); }
            else if (!m_ProfilesSelectedBundleId.has_value() || m_CustomPresetWorkspace.findConfigurationBundle(*m_ProfilesSelectedBundleId) == nullptr)
            {
                m_ProfilesSelectedBundleId = bundles.front().Id;
            }
        }
        setStatusMessage("Profiles section: " + std::string(getProfilesSectionName(m_ProfilesSection)));
    }

    void ShipGeneratorPreviewApp::changeProfilesItem(int32_t delta)
    {
        switch (m_ProfilesSection)
        {
        case ProfilesSection::STRUCTURAL: changeStyle(delta); break;
        case ProfilesSection::FACTION: changeFaction(delta); break;
        case ProfilesSection::PALETTE: changePalette(delta); break;
        case ProfilesSection::FULL_CONFIGURATION:
        {
            const auto& bundles = m_CustomPresetWorkspace.getConfigurationBundles();
            if (bundles.empty()) { m_ProfilesSelectedBundleId.reset(); return; }
            std::size_t currentIndex = 0u;
            if (m_ProfilesSelectedBundleId.has_value())
            {
                const auto iterator = std::find_if(bundles.begin(), bundles.end(), [&](const RuntimeConfigurationBundle& entry) { return entry.Id == *m_ProfilesSelectedBundleId; });
                if (iterator != bundles.end()) { currentIndex = static_cast<std::size_t>(std::distance(bundles.begin(), iterator)); }
            }
            m_ProfilesSelectedBundleId = bundles[getWrappedPreviewSelectorIndex(currentIndex, delta, bundles.size())].Id;
            break;
        }
        default: break;
        }
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
        const std::vector<StructuralProfileSelectionEntry> entries = buildStructuralProfileSelection(m_CustomPresetWorkspace);
        const std::size_t selectableCount = entries.empty() ? 0u : entries.size() - 1u;
        if (selectableCount == 0u) { return; }
        const std::size_t currentIndex = findStructuralProfileSelectionIndex(entries, getCurrentRecipe(), m_SelectedStructuralPresetId);
        selectStructuralProfileEntry(entries[getWrappedPreviewSelectorIndex(currentIndex, delta, selectableCount)]);
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
        m_Collections.beginGallery(batchSeed, m_GalleryState.TemplateRecipe);
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
                SpectralShipGen::GeneratedShip ship;

                if (!generateShipFromRecipe(recipe, ship)) { continue; }
                const sf::Image candidateImage = SpectralShipGen::SFMLImageAdapter::createSFMLImage(ship.FinalImage);
                if (!candidate.Texture.loadFromImage(candidateImage)) { continue; }

                candidate.Texture.setSmooth(false);
                candidate.Recipe = recipe;
                candidate.Valid = true;
                break;
            }

            if (candidate.Valid) { m_Collections.addGalleryRecipe(candidate.Recipe); }
            else { m_Collections.addInvalidGalleryRecipe(); }
            m_GalleryState.Grid.Items.push_back(std::move(candidate));
        }
        refreshGalleryFavoriteMarkers();

        m_PreviewMode = PreviewMode::GALLERY;
        updateWindowTitle();
        std::cout << "Gallery BatchSeed: " << m_GalleryState.BatchSeed << '\n';
        std::cout << "Gallery candidates: " << m_GalleryState.CandidateCount << '\n';
        std::cout << '\n';
        return true;
    }


    SpectralShipGen::Image ShipGeneratorPreviewApp::createDiagnosticImage() const
    {
        return createPreviewInspectionImage(
            m_GeneratedShip,
            m_GenerationDebugInfo,
            m_Diagnostics.ViewMode,
            m_Diagnostics.InspectionPresentation,
            m_Diagnostics.GenerationStageView,
            m_Diagnostics.GenerationStageIndex);
    }

    void ShipGeneratorPreviewApp::cycleAnimationType()
    {
        const PreviewAnimationActionResult result = m_AnimationSession.cycleAnimationType(m_GeneratedShip);
        if (!result.Success || !refreshAnimationTextures()) { return; }
        m_AnimationClock.restart();
        if (m_PreviewMode == PreviewMode::ANIMATION || m_PreviewMode == PreviewMode::FRAME_INSPECTION) { setDisplayedAnimationFrame(0u); }
        else { setDisplayedStaticFrame(); }
        if (!result.StatusMessage.empty()) { setStatusMessage(result.StatusMessage); }
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::cycleAnimationBaseState()
    {
        const PreviewAnimationActionResult result = m_AnimationSession.cycleBaseMovementState(m_GeneratedShip);
        if (!result.Success)
        {
            if (!result.StatusMessage.empty()) { setStatusMessage(result.StatusMessage); }
            return;
        }
        if (result.ActiveFramesChanged && !refreshAnimationTextures()) { return; }
        if (result.StartPlayback) { enterAnimationPlayback(); }
        if (!result.StatusMessage.empty()) { setStatusMessage(result.StatusMessage); }
        updateCommandPanelState();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::cycleAnimationPlaybackSpeed()
    {
        const PreviewAnimationActionResult result = m_AnimationSession.cyclePlaybackSpeed();
        if (!result.StatusMessage.empty()) { setStatusMessage("Playback speed: " + getPlaybackSpeedDisplay(m_AnimationSession.getPlaybackSpeed())); }
        m_AnimationClock.restart();
        updateCommandPanelState();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::cycleMovementPhase()
    {
        const PreviewAnimationActionResult result = m_AnimationSession.cycleMovementPhase();
        if (!result.ActiveFramesChanged) { return; }
        m_AnimationClock.restart();
        if (!refreshAnimationTextures()) { return; }
        if (m_PreviewMode == PreviewMode::ANIMATION || m_PreviewMode == PreviewMode::FRAME_INSPECTION) { setDisplayedAnimationFrame(0u); }
        if (!result.StatusMessage.empty()) { setStatusMessage(result.StatusMessage); }
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::cycleFiringTarget()
    {
        const PreviewAnimationActionResult result = m_AnimationSession.cycleFiringTarget(m_GeneratedShip);
        if (!result.Success)
        {
            if (!result.StatusMessage.empty()) { setStatusMessage(result.StatusMessage); }
            return;
        }
        m_AnimationClock.restart();
        if (result.ActiveFramesChanged && !refreshAnimationTextures()) { return; }
        if (m_PreviewMode == PreviewMode::ANIMATION || m_PreviewMode == PreviewMode::FRAME_INSPECTION) { setDisplayedAnimationFrame(0u); }
        if (!result.StatusMessage.empty()) { setStatusMessage(result.StatusMessage); }
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::triggerAnimationFire()
    {
        const PreviewAnimationActionResult result = m_AnimationSession.triggerFiringEvent(m_GeneratedShip);
        if (!result.Success)
        {
            if (!result.StatusMessage.empty()) { setStatusMessage(result.StatusMessage); }
            updateCommandPanelState();
            updateWindowTitle();
            return;
        }
        if (result.ActiveFramesChanged && !refreshAnimationTextures()) { return; }
        if (result.StartPlayback) { enterAnimationPlayback(); }
        if (!result.StatusMessage.empty()) { setStatusMessage(result.StatusMessage); }
        updateCommandPanelState();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::setAnimationNormalizedTime(uint32_t value)
    {
        if (!m_AnimationSession.setNormalizedTime(static_cast<double>(std::min(value, 1000u)) / 1000.0)) { return; }
        m_PreviewMode = PreviewMode::FRAME_INSPECTION;
        setDisplayedAnimationFrame(m_AnimationSession.getFrameIndex());
        m_AnimationSession.resetPlaybackAccumulator();
        m_AnimationClock.restart();
        updateCommandPanelState();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::applySelectedAnimationState()
    {
        const PreviewAnimationActionResult result = m_AnimationSession.applySelectedState(m_GeneratedShip);
        if (!result.Success)
        {
            if (!result.StatusMessage.empty()) { setStatusMessage(result.StatusMessage); }
            return;
        }
        if (result.ActiveFramesChanged && !refreshAnimationTextures()) { return; }
        if (result.StartPlayback) { enterAnimationPlayback(); }
        if (!result.StatusMessage.empty()) { setStatusMessage(result.StatusMessage); }
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::returnAnimationToIdle()
    {
        const PreviewAnimationActionResult result = m_AnimationSession.returnToIdle(m_GeneratedShip);
        if (!result.Success)
        {
            if (!result.StatusMessage.empty()) { setStatusMessage(result.StatusMessage); }
            return;
        }
        if (result.ActiveFramesChanged && !refreshAnimationTextures()) { return; }
        if (result.StartPlayback) { enterAnimationPlayback(); }
        if (!result.StatusMessage.empty()) { setStatusMessage(result.StatusMessage); }
        updateWindowTitle();
    }

    bool ShipGeneratorPreviewApp::beginComposedFiringEvent()
    {
        const PreviewAnimationActionResult result = m_AnimationSession.beginComposedFiringEvent(m_GeneratedShip);
        if (!result.Success)
        {
            if (!result.StatusMessage.empty()) { setStatusMessage(result.StatusMessage); }
            return false;
        }
        if (result.ActiveFramesChanged && !refreshAnimationTextures()) { return false; }
        if (result.StartPlayback) { enterAnimationPlayback(); }
        if (!result.StatusMessage.empty()) { setStatusMessage(result.StatusMessage); }
        updateWindowTitle();
        return true;
    }

    void ShipGeneratorPreviewApp::changeInspectionGroup(int32_t delta)
    {
        m_Diagnostics.GenerationStageView = false;
        m_Diagnostics.InspectionGroup = getWrappedPreviewInspectionGroup(m_Diagnostics.InspectionGroup, delta);
        m_Diagnostics.ViewMode = getDefaultDiagnosticViewForGroup(m_Diagnostics.InspectionGroup);
        refreshDiagnosticTexture();
        refreshDisplayedTexture();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::changeInspectionView(int32_t delta)
    {
        m_Diagnostics.GenerationStageView = false;
        m_Diagnostics.ViewMode = getWrappedDiagnosticView(m_Diagnostics.InspectionGroup, m_Diagnostics.ViewMode, delta);
        refreshDiagnosticTexture();
        refreshDisplayedTexture();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::cycleDiagnosticView()
    {
        changeInspectionView(1);
    }

    void ShipGeneratorPreviewApp::enterAnimationPlayback()
    {
        const std::vector<SpectralShipGen::Image>& frames = getActiveAnimationFrames();
        if (frames.empty()) { return; }

        m_PreviewMode = PreviewMode::ANIMATION;
        setDisplayedAnimationFrame(m_AnimationSession.getFrameIndex() % static_cast<uint32_t>(frames.size()));
        m_AnimationSession.resetPlaybackAccumulator();
        m_AnimationClock.restart();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::enterGenerateIdlePlayback()
    {
        if (m_GenerateIdleAnimationTextures.empty()) { return; }

        m_PreviewMode = PreviewMode::ANIMATION;
        m_GenerateIdleFrameIndex %= static_cast<uint32_t>(m_GenerateIdleAnimationTextures.size());
        m_GenerateIdlePlaybackAccumulatorMicroseconds = 0.0;
        m_AnimationClock.restart();
        refreshDisplayedTexture();
        updateWindowTitle();
    }

    CalibrationContextFilter ShipGeneratorPreviewApp::getCalibrationContextFilter() const
    {
        CalibrationContextFilter filter;
        if (!m_CalibrationContextFilterEnabled) { return filter; }
        const PreviewGenerationRecipe& recipe = getCurrentRecipe();
        filter.Style = recipe.StructuralPreset;
        filter.Faction = recipe.FactionPreset;
        filter.DimensionBucket = getCalibrationDimensionBucket(recipe.Dimensions);
        return filter;
    }

    void ShipGeneratorPreviewApp::enterAttributeRerollStudio()
    {
        beginAttributeRerollStudio(m_RerollStudio, getCurrentRecipe());
        m_PreviewMode = PreviewMode::REROLL_STUDIO;
        m_Diagnostics.HelpVisible = false;
        m_Diagnostics.GenerationInspectorVisible = false;
        m_Diagnostics.PaletteInspectorVisible = false;
        setStatusMessage("Reroll workspace ready. Select attributes, then generate a non-destructive candidate.");
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::cancelAttributeRerollStudio()
    {
        if (!m_RerollStudio.Active) { return; }
        m_RerollStudio.CandidateValid = false;
        m_RerollStudio.CandidateRecipe = m_RerollStudio.BaseRecipe;
        refreshDisplayedTexture();
        setStatusMessage("Reroll candidate discarded. Base ship retained.");
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::acceptAttributeRerollStudioCandidate()
    {
        if (!m_RerollStudio.Active || !m_RerollStudio.CandidateValid) { return; }
        const PreviewGenerationRecipe acceptedRecipe = m_RerollStudio.CandidateRecipe;
        appendHistoryEntry(acceptedRecipe);
        beginAttributeRerollStudio(m_RerollStudio, getCurrentRecipe());
        m_PreviewMode = PreviewMode::REROLL_STUDIO;
        setStatusMessage("Reroll candidate accepted and added to History.");
        updateWindowTitle();
    }

    bool ShipGeneratorPreviewApp::generateAttributeRerollStudioCandidate()
    {
        if (!m_RerollStudio.Active || !hasSelectedAttributeRerollDomains(m_RerollStudio)) { return false; }

        const AttributeRerollStudioState previousState = m_RerollStudio;
        const PreviewGenerationRecipe candidateRecipe = SpectralShipGenStudioPreview::generateAttributeRerollCandidate(m_RerollStudio, m_SeedGenerator());
        SpectralShipGen::GeneratedShip candidateShip;
        SpectralShipGen::ShipGenerationDebugInfo candidateDebugInfo;
        if (!generateShipFromRecipe(candidateRecipe, candidateShip, &candidateDebugInfo))
        {
            m_RerollStudio = previousState;
            setStatusMessage("Reroll candidate generation failed; BaseRecipe remains unchanged.");
            return false;
        }

        const sf::Image image = SpectralShipGen::SFMLImageAdapter::createSFMLImage(candidateShip.FinalImage);
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
        if (!getCurrentRecipe().StructuralPreset.has_value())
        {
            setStatusMessage("Calibration Lab requires a built-in Structural preset.");
            return;
        }
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
        const uint32_t count = static_cast<uint32_t>(SpectralShipGen::GenerationWeightGroup::GENERATION_WEIGHT_GROUP_END);
        if (count == 0u) { return; }
        int32_t value = static_cast<int32_t>(m_CalibrationGroup) + delta;
        while (value < 0) { value += static_cast<int32_t>(count); }
        while (value >= static_cast<int32_t>(count)) { value -= static_cast<int32_t>(count); }
        m_CalibrationGroup = static_cast<SpectralShipGen::GenerationWeightGroup>(value);
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

        const sf::Image imageA = SpectralShipGen::SFMLImageAdapter::createSFMLImage(m_CalibrationPair.ShipA.FinalImage);
        const sf::Image imageB = SpectralShipGen::SFMLImageAdapter::createSFMLImage(m_CalibrationPair.ShipB.FinalImage);
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
        if (optionIndex >= SpectralShipGen::getGenerationWeightOptionCount(m_CalibrationGroup)) { return; }
        const std::optional<SpectralShipGen::ShipStyle>& structuralPreset = getCurrentRecipe().StructuralPreset;
        if (!structuralPreset.has_value()) { return; }
        SpectralShipGen::setGenerationTuningWeight(m_CalibrationSession.TunedProfile, *structuralPreset, m_CalibrationGroup, optionIndex, value);
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
        if (SpectralShipGenStudioPreview::exportGenerationTuningProfile(m_CalibrationSession.TunedProfile, CalibrationTuningProfilePath, error)) { setStatusMessage("Tuning profile exported: " + CalibrationTuningProfilePath.string()); }
        else { setStatusMessage("Tuning export failed: " + error); }
    }

    void ShipGeneratorPreviewApp::enterFrameInspection()
    {
        const std::vector<SpectralShipGen::Image>& frames = getActiveAnimationFrames();
        if (frames.empty()) { return; }

        m_PreviewMode = PreviewMode::FRAME_INSPECTION;
        setDisplayedAnimationFrame(m_AnimationSession.getFrameIndex() % static_cast<uint32_t>(frames.size()));
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::enterConfigurationEditor()
    {
        if (m_PreviewMode == PreviewMode::GALLERY || m_PreviewMode == PreviewMode::FAVORITES || m_PreviewMode == PreviewMode::REROLL_STUDIO || m_PreviewMode == PreviewMode::CALIBRATION || m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR) { return; }

        const PreviewGenerationRecipe& recipe = getCurrentRecipe();
        SpectralShipGen::ShipGenerationProfile profile;
        std::string name;
        m_ConfigurationEditorTargetPresetId.reset();
        if (!recipe.StructuralPreset.has_value())
        {
            if (m_SelectedStructuralPresetId.has_value())
            {
                const RuntimeStructuralPreset* preset = m_CustomPresetWorkspace.findStructural(*m_SelectedStructuralPresetId);
                if (preset != nullptr)
                {
                    profile = preset->Profile;
                    name = preset->Name;
                    m_ConfigurationEditorTargetPresetId = preset->Id;
                }
            }
            if (name.empty())
            {
                profile = recipe.StructuralProfile;
                name = "Current Custom";
            }
        }
        else
        {
            profile = SpectralShipGen::getShipGenerationProfile(*recipe.StructuralPreset);
            name = getStyleName(*recipe.StructuralPreset) + " Copy";
        }

        m_ConfigurationEditorReturnMode = m_PreviewMode;
        m_ConfigurationEditor.setPanelBounds({ static_cast<float>(PreviewContentWidth), static_cast<float>(PreviewWorkspaceNavigationHeight), static_cast<float>(PreviewWindowWidth - PreviewContentWidth), static_cast<float>(PreviewWindowHeight - PreviewWorkspaceNavigationHeight) });
        m_ConfigurationEditor.openStructuralProfile(std::move(name), profile);
        m_ConfigurationEditor.setExistingCustomPreset(m_ConfigurationEditorTargetPresetId.has_value());
        m_PreviewMode = PreviewMode::CONFIGURATION_EDITOR;
        setDisplayedStaticFrame();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::enterConfigurationEditorDefault()
    {
        if (m_PreviewMode == PreviewMode::GALLERY || m_PreviewMode == PreviewMode::FAVORITES || m_PreviewMode == PreviewMode::REROLL_STUDIO || m_PreviewMode == PreviewMode::CALIBRATION || m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR) { return; }
        m_ConfigurationEditorReturnMode = m_PreviewMode;
        m_ConfigurationEditorTargetPresetId.reset();
        m_ConfigurationEditor.setPanelBounds({ static_cast<float>(PreviewContentWidth), static_cast<float>(PreviewWorkspaceNavigationHeight), static_cast<float>(PreviewWindowWidth - PreviewContentWidth), static_cast<float>(PreviewWindowHeight - PreviewWorkspaceNavigationHeight) });
        m_ConfigurationEditor.openStructuralProfile("Custom Profile", SpectralShipGen::ShipGenerationProfile{});
        m_PreviewMode = PreviewMode::CONFIGURATION_EDITOR;
        setDisplayedStaticFrame();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::enterFactionConfigurationEditor()
    {
        if (m_PreviewMode == PreviewMode::GALLERY || m_PreviewMode == PreviewMode::FAVORITES || m_PreviewMode == PreviewMode::REROLL_STUDIO || m_PreviewMode == PreviewMode::CALIBRATION || m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR) { return; }

        const PreviewGenerationRecipe& recipe = getCurrentRecipe();
        SpectralShipGen::ShipFactionProfile profile;
        std::string name;
        m_ConfigurationEditorTargetFactionPresetId.reset();
        if (!recipe.FactionPreset.has_value())
        {
            if (m_SelectedFactionPresetId.has_value())
            {
                const RuntimeFactionPreset* preset = m_CustomPresetWorkspace.findFaction(*m_SelectedFactionPresetId);
                if (preset != nullptr)
                {
                    profile = preset->Profile;
                    name = preset->Name;
                    m_ConfigurationEditorTargetFactionPresetId = preset->Id;
                }
            }
            if (name.empty())
            {
                profile = recipe.FactionProfile;
                name = "Current Custom Faction";
            }
        }
        else
        {
            profile = SpectralShipGen::getShipFactionProfile(*recipe.FactionPreset);
            name = getFactionDisplayName(*recipe.FactionPreset) + " Copy";
        }

        m_ConfigurationEditorReturnMode = m_PreviewMode;
        m_ConfigurationEditor.setPanelBounds({ static_cast<float>(PreviewContentWidth), static_cast<float>(PreviewWorkspaceNavigationHeight), static_cast<float>(PreviewWindowWidth - PreviewContentWidth), static_cast<float>(PreviewWindowHeight - PreviewWorkspaceNavigationHeight) });
        m_ConfigurationEditor.openFactionProfile(std::move(name), profile);
        m_ConfigurationEditor.setExistingCustomPreset(m_ConfigurationEditorTargetFactionPresetId.has_value());
        m_PreviewMode = PreviewMode::CONFIGURATION_EDITOR;
        setDisplayedStaticFrame();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::enterFactionConfigurationEditorDefault()
    {
        if (m_PreviewMode == PreviewMode::GALLERY || m_PreviewMode == PreviewMode::FAVORITES || m_PreviewMode == PreviewMode::REROLL_STUDIO || m_PreviewMode == PreviewMode::CALIBRATION || m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR) { return; }
        m_ConfigurationEditorReturnMode = m_PreviewMode;
        m_ConfigurationEditorTargetFactionPresetId.reset();
        m_ConfigurationEditor.setPanelBounds({ static_cast<float>(PreviewContentWidth), static_cast<float>(PreviewWorkspaceNavigationHeight), static_cast<float>(PreviewWindowWidth - PreviewContentWidth), static_cast<float>(PreviewWindowHeight - PreviewWorkspaceNavigationHeight) });
        m_ConfigurationEditor.openFactionProfile("Custom Faction", SpectralShipGen::ShipFactionProfile{});
        m_PreviewMode = PreviewMode::CONFIGURATION_EDITOR;
        setDisplayedStaticFrame();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::enterPaletteConfigurationEditor()
    {
        if (m_PreviewMode == PreviewMode::GALLERY || m_PreviewMode == PreviewMode::FAVORITES || m_PreviewMode == PreviewMode::REROLL_STUDIO || m_PreviewMode == PreviewMode::CALIBRATION || m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR) { return; }

        const PreviewGenerationRecipe& recipe = getCurrentRecipe();
        SpectralShipGen::ShipPaletteConfiguration configuration = recipe.PaletteConfiguration;
        std::string name = getCurrentPaletteDisplayName();
        m_ConfigurationEditorTargetPalettePresetId.reset();

        if (m_SelectedPalettePresetId.has_value())
        {
            const RuntimePalettePreset* preset = m_CustomPresetWorkspace.findPalette(*m_SelectedPalettePresetId);
            if (preset != nullptr)
            {
                configuration = preset->Configuration;
                name = preset->Name;
                m_ConfigurationEditorTargetPalettePresetId = preset->Id;
            }
        }
        else if (m_SelectedBuiltInPalettePreset.has_value())
        {
            configuration.Mode = SpectralShipGen::ShipPaletteSourceMode::EXPLICIT_GENERATED;
            configuration.Generated = SpectralShipGen::getBuiltInPalettePresetProfile(*m_SelectedBuiltInPalettePreset);
            name = std::string(SpectralShipGen::getBuiltInPalettePresetId(*m_SelectedBuiltInPalettePreset)) + " Palette Copy";
        }
        else if (configuration.Mode == SpectralShipGen::ShipPaletteSourceMode::FACTION_PROFILE_GENERATED)
        {
            configuration.Mode = SpectralShipGen::ShipPaletteSourceMode::EXPLICIT_GENERATED;
            configuration.Generated = getRecipeFactionPaletteGenerationProfile(recipe);
            name = "Faction Palette Copy";
        }

        if (configuration.Mode != SpectralShipGen::ShipPaletteSourceMode::FIXED) { configuration.Fixed = m_GeneratedShip.Palette; }
        if (configuration.Mode == SpectralShipGen::ShipPaletteSourceMode::FIXED)
        {
            configuration.Generated = getRecipeFactionPaletteGenerationProfile(recipe);
        }

        m_ConfigurationEditorReturnMode = m_PreviewMode;
        m_ConfigurationEditor.setPanelBounds({ static_cast<float>(PreviewContentWidth), static_cast<float>(PreviewWorkspaceNavigationHeight), static_cast<float>(PreviewWindowWidth - PreviewContentWidth), static_cast<float>(PreviewWindowHeight - PreviewWorkspaceNavigationHeight) });
        m_ConfigurationEditor.openPaletteConfiguration(std::move(name), configuration);
        m_ConfigurationEditor.setExistingCustomPreset(m_ConfigurationEditorTargetPalettePresetId.has_value());
        m_PreviewMode = PreviewMode::CONFIGURATION_EDITOR;
        setDisplayedStaticFrame();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::enterPaletteConfigurationEditorDefault()
    {
        if (m_PreviewMode == PreviewMode::GALLERY || m_PreviewMode == PreviewMode::FAVORITES || m_PreviewMode == PreviewMode::REROLL_STUDIO || m_PreviewMode == PreviewMode::CALIBRATION || m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR) { return; }
        const PreviewGenerationRecipe& recipe = getCurrentRecipe();
        SpectralShipGen::ShipPaletteConfiguration configuration;
        configuration.Mode = SpectralShipGen::ShipPaletteSourceMode::EXPLICIT_GENERATED;
        configuration.Generated = getRecipeFactionPaletteGenerationProfile(recipe);
        configuration.Fixed = m_GeneratedShip.Palette;
        m_ConfigurationEditorReturnMode = m_PreviewMode;
        m_ConfigurationEditorTargetPalettePresetId.reset();
        m_ConfigurationEditor.setPanelBounds({ static_cast<float>(PreviewContentWidth), static_cast<float>(PreviewWorkspaceNavigationHeight), static_cast<float>(PreviewWindowWidth - PreviewContentWidth), static_cast<float>(PreviewWindowHeight - PreviewWorkspaceNavigationHeight) });
        m_ConfigurationEditor.openPaletteConfiguration("Custom Palette", configuration);
        m_PreviewMode = PreviewMode::CONFIGURATION_EDITOR;
        setDisplayedStaticFrame();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::enterConfigurationBundleEditor()
    {
        if (m_PreviewMode == PreviewMode::GALLERY || m_PreviewMode == PreviewMode::FAVORITES || m_PreviewMode == PreviewMode::REROLL_STUDIO || m_PreviewMode == PreviewMode::CALIBRATION || m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR) { return; }
        if (!m_ProfilesSelectedBundleId.has_value()) { enterConfigurationBundleEditorDefault(); return; }
        const RuntimeConfigurationBundle* preset = m_CustomPresetWorkspace.findConfigurationBundle(*m_ProfilesSelectedBundleId);
        if (preset == nullptr) { m_ProfilesSelectedBundleId.reset(); enterConfigurationBundleEditorDefault(); return; }

        m_ConfigurationEditorReturnMode = m_PreviewMode;
        m_ConfigurationEditorTargetBundleId = preset->Id;
        m_ConfigurationEditor.setPanelBounds({ static_cast<float>(PreviewContentWidth), static_cast<float>(PreviewWorkspaceNavigationHeight), static_cast<float>(PreviewWindowWidth - PreviewContentWidth), static_cast<float>(PreviewWindowHeight - PreviewWorkspaceNavigationHeight) });
        m_ConfigurationEditor.openConfigurationBundle(preset->Name, preset->Bundle);
        m_ConfigurationEditor.setExistingCustomPreset(true);
        m_PreviewMode = PreviewMode::CONFIGURATION_EDITOR;
        setDisplayedStaticFrame();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::enterConfigurationBundleEditorDefault()
    {
        if (m_PreviewMode == PreviewMode::GALLERY || m_PreviewMode == PreviewMode::FAVORITES || m_PreviewMode == PreviewMode::REROLL_STUDIO || m_PreviewMode == PreviewMode::CALIBRATION || m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR) { return; }
        m_ConfigurationEditorReturnMode = m_PreviewMode;
        m_ConfigurationEditorTargetBundleId.reset();
        m_ConfigurationEditor.setPanelBounds({ static_cast<float>(PreviewContentWidth), static_cast<float>(PreviewWorkspaceNavigationHeight), static_cast<float>(PreviewWindowWidth - PreviewContentWidth), static_cast<float>(PreviewWindowHeight - PreviewWorkspaceNavigationHeight) });
        m_ConfigurationEditor.openConfigurationBundle("Full Configuration", makeConfigurationBundle(getCurrentRecipe(), getCurrentStructuralProfileDisplayName(), getCurrentFactionProfileDisplayName(), getCurrentPaletteDisplayName()));
        m_ConfigurationEditor.setExistingCustomPreset(false);
        m_PreviewMode = PreviewMode::CONFIGURATION_EDITOR;
        setDisplayedStaticFrame();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::handleConfigurationEditorEvent(const ConfigurationEditorEvent& event)
    {
        const ConfigurationEditorProfileKind kind = m_ConfigurationEditor.getProfileKind();
        switch (event.Action)
        {
        case ConfigurationEditorAction::APPLY:
        {
            if (kind == ConfigurationEditorProfileKind::FULL_CONFIGURATION)
            {
                RuntimeCustomPresetId id = 0u;
                const std::optional<RuntimeCustomPresetId> editedId = m_ConfigurationEditorTargetBundleId;
                if (editedId.has_value() && m_CustomPresetWorkspace.updateConfigurationBundle(*editedId, m_ConfigurationEditor.getName(), m_ConfigurationEditor.getDraftConfigurationBundle())) { id = *editedId; }
                else { id = m_CustomPresetWorkspace.addConfigurationBundle(m_ConfigurationEditor.getName(), m_ConfigurationEditor.getDraftConfigurationBundle()); }
                const RuntimeConfigurationBundle* preset = m_CustomPresetWorkspace.findConfigurationBundle(id);
                const bool persisted = saveUserPresetLibraryState();
                if (m_SelectedConfigurationBundleId == editedId) { m_SelectedConfigurationBundleId.reset(); }
                m_ProfilesSelectedBundleId = id;
                m_ConfigurationEditorTargetBundleId.reset();
                m_ConfigurationEditor.close();
                m_PreviewMode = PreviewMode::STATIC;
                setDisplayedStaticFrame();
                setStatusMessage("Saved configuration bundle: " + (preset != nullptr ? preset->Name : m_ConfigurationEditor.getName()) + (persisted ? "" : " (persistence failed)"));
                updateWindowTitle();
                break;
            }

            PreviewGenerationRecipe recipe = getCurrentRecipe();
            m_SelectedConfigurationBundleId.reset();
            if (kind == ConfigurationEditorProfileKind::FACTION)
            {
                RuntimeCustomPresetId id = 0u;
                if (m_ConfigurationEditorTargetFactionPresetId.has_value() && m_CustomPresetWorkspace.updateFaction(*m_ConfigurationEditorTargetFactionPresetId, m_ConfigurationEditor.getName(), m_ConfigurationEditor.getDraftFactionProfile())) { id = *m_ConfigurationEditorTargetFactionPresetId; }
                else { id = m_CustomPresetWorkspace.addFaction(m_ConfigurationEditor.getName(), m_ConfigurationEditor.getDraftFactionProfile()); }
                const RuntimeFactionPreset* preset = m_CustomPresetWorkspace.findFaction(id);
                const bool persisted = saveUserPresetLibraryState();
                recipe.FactionPreset.reset();
                recipe.FactionProfile = m_ConfigurationEditor.getDraftFactionProfile();
                m_SelectedFactionPresetId = id;
                m_ConfigurationEditorTargetFactionPresetId.reset();
                m_ConfigurationEditor.close();
                m_PreviewMode = PreviewMode::STATIC;
                appendHistoryEntry(recipe);
                setStatusMessage("Applied user faction profile: " + (preset != nullptr ? preset->Name : m_ConfigurationEditor.getName()) + (persisted ? "" : " (persistence failed)"));
            }
            else if (kind == ConfigurationEditorProfileKind::PALETTE)
            {
                RuntimeCustomPresetId id = 0u;
                if (m_ConfigurationEditorTargetPalettePresetId.has_value() && m_CustomPresetWorkspace.updatePalette(*m_ConfigurationEditorTargetPalettePresetId, m_ConfigurationEditor.getName(), m_ConfigurationEditor.getDraftPaletteConfiguration())) { id = *m_ConfigurationEditorTargetPalettePresetId; }
                else { id = m_CustomPresetWorkspace.addPalette(m_ConfigurationEditor.getName(), m_ConfigurationEditor.getDraftPaletteConfiguration()); }
                const RuntimePalettePreset* preset = m_CustomPresetWorkspace.findPalette(id);
                const bool persisted = saveUserPresetLibraryState();
                recipe.PaletteConfiguration = m_ConfigurationEditor.getDraftPaletteConfiguration();
                m_SelectedBuiltInPalettePreset.reset();
                m_SelectedPalettePresetId = id;
                m_ConfigurationEditorTargetPalettePresetId.reset();
                m_ConfigurationEditor.close();
                m_PreviewMode = PreviewMode::STATIC;
                appendHistoryEntry(recipe);
                setStatusMessage("Applied user palette: " + (preset != nullptr ? preset->Name : m_ConfigurationEditor.getName()) + (persisted ? "" : " (persistence failed)"));
            }
            else
            {
                RuntimeCustomPresetId id = 0u;
                if (m_ConfigurationEditorTargetPresetId.has_value() && m_CustomPresetWorkspace.updateStructural(*m_ConfigurationEditorTargetPresetId, m_ConfigurationEditor.getName(), m_ConfigurationEditor.getDraftProfile())) { id = *m_ConfigurationEditorTargetPresetId; }
                else { id = m_CustomPresetWorkspace.addStructural(m_ConfigurationEditor.getName(), m_ConfigurationEditor.getDraftProfile()); }
                const RuntimeStructuralPreset* preset = m_CustomPresetWorkspace.findStructural(id);
                const bool persisted = saveUserPresetLibraryState();
                recipe.StructuralPreset.reset();
                recipe.StructuralProfile = m_ConfigurationEditor.getDraftProfile();
                m_SelectedStructuralPresetId = id;
                m_ConfigurationEditorTargetPresetId.reset();
                m_ConfigurationEditor.close();
                m_PreviewMode = PreviewMode::STATIC;
                appendHistoryEntry(recipe);
                setStatusMessage("Applied user structural profile: " + (preset != nullptr ? preset->Name : m_ConfigurationEditor.getName()) + (persisted ? "" : " (persistence failed)"));
            }
            break;
        }
        case ConfigurationEditorAction::DUPLICATE:
        {
            if (kind == ConfigurationEditorProfileKind::FULL_CONFIGURATION)
            {
                const RuntimeCustomPresetId id = m_CustomPresetWorkspace.addConfigurationBundle(m_ConfigurationEditor.getName(), m_ConfigurationEditor.getDraftConfigurationBundle());
                const RuntimeConfigurationBundle* preset = m_CustomPresetWorkspace.findConfigurationBundle(id);
                if (preset != nullptr)
                {
                    m_ProfilesSelectedBundleId = id;
                    m_ConfigurationEditorTargetBundleId = id;
                    m_ConfigurationEditor.openConfigurationBundle(preset->Name, preset->Bundle);
                    m_ConfigurationEditor.setExistingCustomPreset(true);
                    const bool persisted = saveUserPresetLibraryState();
                    setStatusMessage("Editing duplicated configuration bundle: " + preset->Name + (persisted ? "" : " (persistence failed)"));
                }
            }
            else if (kind == ConfigurationEditorProfileKind::FACTION)
            {
                const RuntimeCustomPresetId id = m_CustomPresetWorkspace.addFaction(m_ConfigurationEditor.getName(), m_ConfigurationEditor.getDraftFactionProfile());
                const RuntimeFactionPreset* preset = m_CustomPresetWorkspace.findFaction(id);
                if (preset != nullptr) { m_ConfigurationEditorTargetFactionPresetId = id; m_ConfigurationEditor.openFactionProfile(preset->Name, preset->Profile); m_ConfigurationEditor.setExistingCustomPreset(true); const bool persisted = saveUserPresetLibraryState(); setStatusMessage("Editing duplicated user faction: " + preset->Name + (persisted ? "" : " (persistence failed)")); }
            }
            else if (kind == ConfigurationEditorProfileKind::PALETTE)
            {
                const RuntimeCustomPresetId id = m_CustomPresetWorkspace.addPalette(m_ConfigurationEditor.getName(), m_ConfigurationEditor.getDraftPaletteConfiguration());
                const RuntimePalettePreset* preset = m_CustomPresetWorkspace.findPalette(id);
                if (preset != nullptr) { m_ConfigurationEditorTargetPalettePresetId = id; m_ConfigurationEditor.openPaletteConfiguration(preset->Name, preset->Configuration); m_ConfigurationEditor.setExistingCustomPreset(true); const bool persisted = saveUserPresetLibraryState(); setStatusMessage("Editing duplicated user palette: " + preset->Name + (persisted ? "" : " (persistence failed)")); }
            }
            else
            {
                const RuntimeCustomPresetId id = m_CustomPresetWorkspace.addStructural(m_ConfigurationEditor.getName(), m_ConfigurationEditor.getDraftProfile());
                const RuntimeStructuralPreset* preset = m_CustomPresetWorkspace.findStructural(id);
                if (preset != nullptr) { m_ConfigurationEditorTargetPresetId = id; m_ConfigurationEditor.openStructuralProfile(preset->Name, preset->Profile); m_ConfigurationEditor.setExistingCustomPreset(true); const bool persisted = saveUserPresetLibraryState(); setStatusMessage("Editing duplicated user profile: " + preset->Name + (persisted ? "" : " (persistence failed)")); }
            }
            break;
        }
        case ConfigurationEditorAction::DELETE_PRESET:
            deleteConfigurationEditorPreset();
            break;
        case ConfigurationEditorAction::EXPORT_PRESET:
            exportConfigurationEditorPreset();
            break;
        case ConfigurationEditorAction::IMPORT_PRESET:
            importConfigurationEditorPreset();
            break;
        case ConfigurationEditorAction::REPLACE_BUNDLE_STRUCTURAL:
        case ConfigurationEditorAction::REPLACE_BUNDLE_FACTION:
        case ConfigurationEditorAction::REPLACE_BUNDLE_PALETTE:
        {
            const ConfigurationBundle current = makeConfigurationBundle(getCurrentRecipe(), getCurrentStructuralProfileDisplayName(), getCurrentFactionProfileDisplayName(), getCurrentPaletteDisplayName());
            if (event.Action == ConfigurationEditorAction::REPLACE_BUNDLE_STRUCTURAL) { m_ConfigurationEditor.replaceBundleStructural(current.StructuralDisplayName, current.StructuralProfile); }
            else if (event.Action == ConfigurationEditorAction::REPLACE_BUNDLE_FACTION) { m_ConfigurationEditor.replaceBundleFaction(current.FactionDisplayName, current.FactionProfile); }
            else { m_ConfigurationEditor.replaceBundlePalette(current.PaletteDisplayName, current.PaletteConfiguration); }
            break;
        }
        case ConfigurationEditorAction::CANCEL:
            m_ConfigurationEditorTargetPresetId.reset();
            m_ConfigurationEditorTargetFactionPresetId.reset();
            m_ConfigurationEditorTargetPalettePresetId.reset();
            m_ConfigurationEditorTargetBundleId.reset();
            m_ConfigurationEditor.close();
            m_PreviewMode = m_ConfigurationEditorReturnMode;
            if (m_PreviewMode == PreviewMode::ANIMATION) { m_AnimationClock.restart(); }
            if (m_PreviewMode == PreviewMode::STATIC) { setDisplayedStaticFrame(); }
            setStatusMessage("Configuration editor changes discarded.");
            updateWindowTitle();
            break;
        case ConfigurationEditorAction::RESET:
        case ConfigurationEditorAction::CONFIGURATION_EDITOR_ACTION_END:
        default:
            break;
        }
    }

    void ShipGeneratorPreviewApp::deleteConfigurationEditorPreset()
    {
        const ConfigurationEditorProfileKind kind = m_ConfigurationEditor.getProfileKind();
        bool removed = false;
        std::string name;

        if (kind == ConfigurationEditorProfileKind::FULL_CONFIGURATION && m_ConfigurationEditorTargetBundleId.has_value())
        {
            const RuntimeCustomPresetId id = *m_ConfigurationEditorTargetBundleId;
            if (const RuntimeConfigurationBundle* preset = m_CustomPresetWorkspace.findConfigurationBundle(id); preset != nullptr) { name = preset->Name; }
            removed = m_CustomPresetWorkspace.removeConfigurationBundle(id);
            if (m_SelectedConfigurationBundleId == id) { m_SelectedConfigurationBundleId.reset(); }
            if (m_ProfilesSelectedBundleId == id)
            {
                m_ProfilesSelectedBundleId.reset();
                const auto& bundles = m_CustomPresetWorkspace.getConfigurationBundles();
                if (!bundles.empty()) { m_ProfilesSelectedBundleId = bundles.front().Id; }
            }
            m_ConfigurationEditorTargetBundleId.reset();
        }
        else if (kind == ConfigurationEditorProfileKind::FACTION && m_ConfigurationEditorTargetFactionPresetId.has_value())
        {
            const RuntimeCustomPresetId id = *m_ConfigurationEditorTargetFactionPresetId;
            if (const RuntimeFactionPreset* preset = m_CustomPresetWorkspace.findFaction(id); preset != nullptr) { name = preset->Name; }
            removed = m_CustomPresetWorkspace.removeFaction(id);
            if (m_SelectedFactionPresetId == id) { m_SelectedFactionPresetId.reset(); }
            m_ConfigurationEditorTargetFactionPresetId.reset();
        }
        else if (kind == ConfigurationEditorProfileKind::PALETTE && m_ConfigurationEditorTargetPalettePresetId.has_value())
        {
            const RuntimeCustomPresetId id = *m_ConfigurationEditorTargetPalettePresetId;
            if (const RuntimePalettePreset* preset = m_CustomPresetWorkspace.findPalette(id); preset != nullptr) { name = preset->Name; }
            removed = m_CustomPresetWorkspace.removePalette(id);
            if (m_SelectedPalettePresetId == id) { m_SelectedPalettePresetId.reset(); }
            m_ConfigurationEditorTargetPalettePresetId.reset();
        }
        else if (kind == ConfigurationEditorProfileKind::STRUCTURAL && m_ConfigurationEditorTargetPresetId.has_value())
        {
            const RuntimeCustomPresetId id = *m_ConfigurationEditorTargetPresetId;
            if (const RuntimeStructuralPreset* preset = m_CustomPresetWorkspace.findStructural(id); preset != nullptr) { name = preset->Name; }
            removed = m_CustomPresetWorkspace.removeStructural(id);
            if (m_SelectedStructuralPresetId == id) { m_SelectedStructuralPresetId.reset(); }
            m_ConfigurationEditorTargetPresetId.reset();
        }

        if (!removed)
        {
            setStatusMessage("Delete ignored: built-in and unsaved presets are immutable/non-persistent.");
            return;
        }

        const bool persisted = saveUserPresetLibraryState();
        m_ConfigurationEditor.close();
        m_PreviewMode = m_ConfigurationEditorReturnMode;
        if (m_PreviewMode == PreviewMode::ANIMATION) { m_AnimationClock.restart(); }
        if (m_PreviewMode == PreviewMode::STATIC) { setDisplayedStaticFrame(); }
        setStatusMessage("Deleted user preset: " + name + (persisted ? "" : " (persistence failed)"));
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::exportConfigurationEditorPreset()
    {
        const ConfigurationEditorProfileKind kind = m_ConfigurationEditor.getProfileKind();
        const UserPresetCategory category = userPresetCategory(kind);
        std::optional<RuntimeCustomPresetId> id;
        std::string name;
        if (kind == ConfigurationEditorProfileKind::FULL_CONFIGURATION) { id = m_ConfigurationEditorTargetBundleId; if (id.has_value()) { if (const RuntimeConfigurationBundle* preset = m_CustomPresetWorkspace.findConfigurationBundle(*id); preset != nullptr) { name = preset->Name; } } }
        else if (kind == ConfigurationEditorProfileKind::FACTION) { id = m_ConfigurationEditorTargetFactionPresetId; if (id.has_value()) { if (const RuntimeFactionPreset* preset = m_CustomPresetWorkspace.findFaction(*id); preset != nullptr) { name = preset->Name; } } }
        else if (kind == ConfigurationEditorProfileKind::PALETTE) { id = m_ConfigurationEditorTargetPalettePresetId; if (id.has_value()) { if (const RuntimePalettePreset* preset = m_CustomPresetWorkspace.findPalette(*id); preset != nullptr) { name = preset->Name; } } }
        else { id = m_ConfigurationEditorTargetPresetId; if (id.has_value()) { if (const RuntimeStructuralPreset* preset = m_CustomPresetWorkspace.findStructural(*id); preset != nullptr) { name = preset->Name; } } }

        if (!id.has_value() || name.empty())
        {
            setStatusMessage("Export requires a saved user preset. Duplicate or Apply first.");
            return;
        }

        const std::filesystem::path path = kind == ConfigurationEditorProfileKind::FULL_CONFIGURATION ? getAvailableConfigurationBundlePath(name) : getAvailableUserPresetPath(name);
        std::string error;
        if (!exportUserPreset(m_CustomPresetWorkspace, category, *id, path, error))
        {
            std::cerr << error << '\n';
            setStatusMessage("User preset export failed: " + error);
            return;
        }
        setStatusMessage("Exported user preset: " + path.string());
        std::cout << "Exported user preset: " << path.string() << '\n';
    }

    void ShipGeneratorPreviewApp::importConfigurationEditorPreset()
    {
        std::filesystem::path path;
        if (!readUserPresetPathFromConsole(path)) { return; }

        const ConfigurationEditorProfileKind kind = m_ConfigurationEditor.getProfileKind();
        const UserPresetImportResult imported = SpectralShipGenStudioPreview::importUserPreset(m_CustomPresetWorkspace, userPresetCategory(kind), path);
        if (!imported.Success)
        {
            std::cerr << imported.Error << '\n';
            setStatusMessage("User preset import failed: " + imported.Error);
            return;
        }

        const bool persisted = saveUserPresetLibraryState();
        if (kind == ConfigurationEditorProfileKind::FULL_CONFIGURATION)
        {
            m_ConfigurationEditorTargetBundleId = imported.ImportedId;
            m_ProfilesSelectedBundleId = imported.ImportedId;
            const RuntimeConfigurationBundle* preset = m_CustomPresetWorkspace.findConfigurationBundle(imported.ImportedId);
            if (preset != nullptr) { m_ConfigurationEditor.openConfigurationBundle(preset->Name, preset->Bundle); }
        }
        else if (kind == ConfigurationEditorProfileKind::FACTION)
        {
            m_ConfigurationEditorTargetFactionPresetId = imported.ImportedId;
            const RuntimeFactionPreset* preset = m_CustomPresetWorkspace.findFaction(imported.ImportedId);
            if (preset != nullptr) { m_ConfigurationEditor.openFactionProfile(preset->Name, preset->Profile); }
        }
        else if (kind == ConfigurationEditorProfileKind::PALETTE)
        {
            m_ConfigurationEditorTargetPalettePresetId = imported.ImportedId;
            const RuntimePalettePreset* preset = m_CustomPresetWorkspace.findPalette(imported.ImportedId);
            if (preset != nullptr) { m_ConfigurationEditor.openPaletteConfiguration(preset->Name, preset->Configuration); }
        }
        else
        {
            m_ConfigurationEditorTargetPresetId = imported.ImportedId;
            const RuntimeStructuralPreset* preset = m_CustomPresetWorkspace.findStructural(imported.ImportedId);
            if (preset != nullptr) { m_ConfigurationEditor.openStructuralProfile(preset->Name, preset->Profile); }
        }
        m_ConfigurationEditor.setExistingCustomPreset(true);
        setStatusMessage("Imported user preset: " + imported.DisplayName + (imported.DisplayNameDisambiguated ? " (name disambiguated)" : "") + (persisted ? "" : " (persistence failed)"));
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::newProfilesItem()
    {
        switch (m_ProfilesSection)
        {
        case ProfilesSection::STRUCTURAL: enterConfigurationEditorDefault(); break;
        case ProfilesSection::FACTION: enterFactionConfigurationEditorDefault(); break;
        case ProfilesSection::PALETTE: enterPaletteConfigurationEditorDefault(); break;
        case ProfilesSection::FULL_CONFIGURATION: enterConfigurationBundleEditorDefault(); break;
        default: break;
        }
    }

    void ShipGeneratorPreviewApp::editSelectedProfilesItem()
    {
        switch (m_ProfilesSection)
        {
        case ProfilesSection::STRUCTURAL: enterConfigurationEditor(); break;
        case ProfilesSection::FACTION: enterFactionConfigurationEditor(); break;
        case ProfilesSection::PALETTE: enterPaletteConfigurationEditor(); break;
        case ProfilesSection::FULL_CONFIGURATION: enterConfigurationBundleEditor(); break;
        default: break;
        }
    }

    void ShipGeneratorPreviewApp::duplicateSelectedProfilesItem()
    {
        editSelectedProfilesItem();
        if (m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR) { handleConfigurationEditorEvent({ ConfigurationEditorAction::DUPLICATE }); }
    }

    void ShipGeneratorPreviewApp::deleteSelectedProfilesItem()
    {
        editSelectedProfilesItem();
        if (m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR) { handleConfigurationEditorEvent({ ConfigurationEditorAction::DELETE_PRESET }); }
    }

    void ShipGeneratorPreviewApp::importSelectedProfilesItem()
    {
        newProfilesItem();
        if (m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR) { handleConfigurationEditorEvent({ ConfigurationEditorAction::IMPORT_PRESET }); }
    }

    void ShipGeneratorPreviewApp::exportSelectedProfilesItem()
    {
        editSelectedProfilesItem();
        if (m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR) { handleConfigurationEditorEvent({ ConfigurationEditorAction::EXPORT_PRESET }); }
    }

    void ShipGeneratorPreviewApp::useSelectedProfilesItem()
    {
        if (m_ProfilesSection != ProfilesSection::FULL_CONFIGURATION || !m_ProfilesSelectedBundleId.has_value()) { return; }
        applyRuntimeConfigurationBundle(*m_ProfilesSelectedBundleId);
        setStatusMessage("Full Configuration applied. Switch to Generate to use the updated components.");
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
        if (!m_FavoritesState.Grid.Items.empty())
        {
            m_FavoritesState.Grid.SelectedIndex = std::min(m_FavoritesState.Grid.SelectedIndex, static_cast<uint32_t>(m_FavoritesState.Grid.Items.size() - 1u));
        }
        m_FavoritesState.Grid.HoveredIndex = -1;
        m_PendingFavoriteRemovalIndex.reset();
        m_PreviewMode = PreviewMode::FAVORITES;
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::exitGalleryMode()
    {
        m_PreviewMode = PreviewMode::STATIC;
        m_GalleryState.Grid.Items.clear();
        m_Collections.clearGallery();
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
            m_Window.setTitle("SpectralShipGen Studio | Calibration Lab | " + std::string(getCalibrationGroupName(m_CalibrationGroup)) + " | " + getCalibrationEvidenceName(statistics.Evidence) + " | " + std::to_string(statistics.UsefulComparisonCount) + " comparisons");
            return;
        }

        if (m_PreviewMode == PreviewMode::FAVORITES)
        {
            if (m_FavoritesState.Grid.Items.empty() || m_FavoritesState.Grid.SelectedIndex >= m_FavoritesState.Grid.Items.size()) { return; }
            const PreviewGenerationRecipe* favorite = m_Collections.getFavorite(m_FavoritesState.Grid.SelectedIndex);
            if (favorite == nullptr || !m_FavoritesState.Grid.Items[m_FavoritesState.Grid.SelectedIndex].Valid) { return; }
            document.Recipe = *favorite;
            baseName = getSaveBaseName(*favorite);
        }
        else
        {
            document.Recipe = getCurrentRecipe();
            document.AnimationSettings = m_AnimationSession.getIdleSettings();
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

        SpectralShipGen::GeneratedShip candidateShip;
        if (!generateShipFromRecipe(document.Recipe, candidateShip, nullptr))
        {
            setStatusMessage("Recipe parsed correctly, but ship generation failed.");
            return false;
        }

        if (document.AnimationSettings.has_value())
        {
            try
            {
                const SpectralShipGen::ShipIdleAnimation candidateAnimation = SpectralShipGen::ShipIdleAnimator{}.generate(candidateShip, *document.AnimationSettings);
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

        if (document.AnimationSettings.has_value()) { m_AnimationSession.getIdleSettings() = *document.AnimationSettings; }
        m_PreviewMode = PreviewMode::STATIC;
        m_Diagnostics.HelpVisible = false;
        m_Diagnostics.GenerationInspectorVisible = false;
        m_Diagnostics.PaletteInspectorVisible = false;
        m_GalleryState.Grid.Items.clear();
        m_Collections.clearGallery();
        m_FavoritesState.Grid.HoveredIndex = -1;
        m_SelectedConfigurationBundleId.reset();
        if (document.Recipe == getCurrentRecipe()) { regenerate(); }
        else { m_SelectedStructuralPresetId.reset(); m_SelectedFactionPresetId.reset(); m_SelectedBuiltInPalettePreset.reset(); m_SelectedPalettePresetId.reset(); appendHistoryEntry(document.Recipe); }

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

    bool ShipGeneratorPreviewApp::generateShipFromRecipe(const PreviewGenerationRecipe& recipe, SpectralShipGen::GeneratedShip& outShip, SpectralShipGen::ShipGenerationDebugInfo* debugInfo)
    {
        try
        {
            outShip = m_Generator.generate(recipe, debugInfo);
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

        if (m_PreviewMode == PreviewMode::CALIBRATION) { state.Mode = PreviewCommandPanelMode::CALIBRATION; }
        else if (m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::REROLL) { state.Mode = PreviewCommandPanelMode::REROLL_STUDIO; }
        else
        {
            switch (m_WorkspaceSession.getActiveWorkspace())
            {
            case PreviewWorkspace::PROFILES: state.Mode = PreviewCommandPanelMode::PROFILES; break;
            case PreviewWorkspace::INSPECT: state.Mode = PreviewCommandPanelMode::INSPECT; break;
            case PreviewWorkspace::FAVORITES: state.Mode = PreviewCommandPanelMode::FAVORITES; break;
            case PreviewWorkspace::ANIMATION: state.Mode = PreviewCommandPanelMode::ANIMATION; break;
            case PreviewWorkspace::GENERATE:
            default: state.Mode = PreviewCommandPanelMode::GENERATE; break;
            }
        }
        if (m_PreviewMode == PreviewMode::REROLL_STUDIO)
        {
            state.RerollStudioSelectedDomains = m_RerollStudio.SelectedDomains;
        }

        state.StyleValue = getCurrentStructuralProfileDisplayName();
        state.FactionValue = getCurrentFactionProfileDisplayName();
        state.PaletteValue = getCurrentPaletteDisplayName();
        state.ConfigurationBundleValue = getCurrentConfigurationBundleDisplayName();
        state.ProfilesSectionValue = getProfilesSectionName(m_ProfilesSection);
        state.ProfilesItemValue = getProfilesItemDisplayName();
        state.InspectionGroupValue = getPreviewInspectionGroupName(m_Diagnostics.InspectionGroup);
        state.InspectionViewValue = getDiagnosticViewModeName(m_Diagnostics.ViewMode);
        state.InspectionPresentationValue = getPreviewInspectionPresentationName(m_Diagnostics.InspectionPresentation);
        const uint32_t favoritePageCount = getPreviewThumbnailPageCount(m_FavoritesState.Grid);
        state.FavoritesPageValue = favoritePageCount == 0u ? "0 / 0" : std::to_string(getPreviewThumbnailCurrentPage(m_FavoritesState.Grid) + 1u) + " / " + std::to_string(favoritePageCount);
        state.AnimationTypeValue = getAnimationTypeDisplayName(m_AnimationSession.getSelectedAnimationType());
        state.AnimationBaseStateValue = m_AnimationSession.getRuntimeMovementType() == SpectralShipGen::ShipAnimationType::IDLE ? "NEUTRAL" : getAnimationTypeDisplayName(m_AnimationSession.getRuntimeMovementType());
        state.AnimationPhaseValue = m_AnimationSession.getSemanticPhaseDisplay();
        state.AnimationPlaybackSpeedValue = getPlaybackSpeedDisplay(m_AnimationSession.getPlaybackSpeed());
        state.AnimationTimelineValue = static_cast<uint32_t>(std::lround(std::clamp(m_AnimationSession.getActiveNormalizedTime(), 0.0, 1.0) * 1000.0));
        state.AnimationTimelineDetail = std::to_string(m_AnimationSession.getFrameIndex() + 1u) + "/" + std::to_string(m_AnimationSession.getActiveFrames().size());
        state.CurrentDimensions = getCurrentRecipe().Dimensions;
        state.AspectRatioLocked = m_AspectRatioLocked;
        state.ResolutionBookmarkCount = static_cast<uint32_t>(std::min<std::size_t>(m_Collections.getResolutionBookmarks().size(), state.ResolutionBookmarks.size()));
        for (uint32_t index = 0u; index < state.ResolutionBookmarkCount; ++index) { state.ResolutionBookmarks[index] = m_Collections.getResolutionBookmarks()[index]; }

        if (m_PreviewMode == PreviewMode::CALIBRATION)
        {
            state.CalibrationGroupValue = getCalibrationGroupName(m_CalibrationGroup);
            const CalibrationContextFilter filter = getCalibrationContextFilter();
            const CalibrationGroupStatistics statistics = calculateCalibrationGroupStatistics(m_CalibrationSession, m_CalibrationGroup, filter);
            state.CalibrationEvidenceValue = std::string(getCalibrationEvidenceName(statistics.Evidence)) + " / " + std::to_string(statistics.UsefulComparisonCount);
            const std::optional<SpectralShipGen::ShipStyle>& structuralPreset = getCurrentRecipe().StructuralPreset;
            if (structuralPreset.has_value())
            {
                const std::vector<uint32_t> suggested = calculateSuggestedGroupWeights(m_CalibrationSession, *structuralPreset, m_CalibrationGroup, filter);
                const uint32_t optionCount = SpectralShipGen::getGenerationWeightOptionCount(m_CalibrationGroup);
                const bool binary = SpectralShipGen::getGenerationWeightGroupKind(m_CalibrationGroup) == SpectralShipGen::GenerationWeightGroupKind::BINARY_PROBABILITY;
                for (uint32_t index = 0u; index < state.CalibrationWeightRows.size(); ++index)
                {
                    PreviewCalibrationWeightRowState& row = state.CalibrationWeightRows[index];
                    row.Valid = index < optionCount;
                    if (!row.Valid) { continue; }
                    row.Label = getCalibrationOptionName(m_CalibrationGroup, index);
                    row.CurrentWeight = SpectralShipGen::getGenerationTuningWeight(m_CalibrationSession.TunedProfile, *structuralPreset, m_CalibrationGroup, index);
                    row.DefaultWeight = SpectralShipGen::getGenerationTuningWeight(m_CalibrationSession.DefaultProfile, *structuralPreset, m_CalibrationGroup, index);
                    row.SuggestedWeight = index < suggested.size() ? suggested[index] : row.CurrentWeight;
                    row.ProbabilityPercent = static_cast<uint32_t>(std::lround(SpectralShipGen::getGenerationTuningNormalizedProbability(m_CalibrationSession.TunedProfile, *structuralPreset, m_CalibrationGroup, index) * 100.0));
                    row.Maximum = binary ? 100u : std::max<uint32_t>(300u, std::max(row.CurrentWeight, std::max(row.DefaultWeight, row.SuggestedWeight)));
                }
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
        case PreviewCommandType::OPEN_FAVORITES: switchWorkspace(PreviewWorkspace::FAVORITES); break;
        case PreviewCommandType::CLOSE_FAVORITES: exitFavoritesMode(); break;
        case PreviewCommandType::FAVORITES_LEFT: moveFavoritesSelection(-1, 0); break;
        case PreviewCommandType::FAVORITES_RIGHT: moveFavoritesSelection(1, 0); break;
        case PreviewCommandType::FAVORITES_UP: moveFavoritesSelection(0, -1); break;
        case PreviewCommandType::FAVORITES_DOWN: moveFavoritesSelection(0, 1); break;
        case PreviewCommandType::FAVORITES_PREVIOUS_PAGE: changeFavoritesPage(-1); break;
        case PreviewCommandType::FAVORITES_NEXT_PAGE: changeFavoritesPage(1); break;
        case PreviewCommandType::SELECT_FAVORITE: openSelectedFavoriteInWorkspace(PreviewWorkspace::GENERATE); break;
        case PreviewCommandType::OPEN_FAVORITE_INSPECT: openSelectedFavoriteInWorkspace(PreviewWorkspace::INSPECT); break;
        case PreviewCommandType::OPEN_FAVORITE_ANIMATION: openSelectedFavoriteInWorkspace(PreviewWorkspace::ANIMATION); break;
        case PreviewCommandType::OPEN_FAVORITE_REROLL: openSelectedFavoriteInWorkspace(PreviewWorkspace::REROLL); break;
        case PreviewCommandType::REMOVE_SELECTED_FAVORITE: removeSelectedFavorite(); break;
        case PreviewCommandType::EXPORT_FAVORITE_IMAGE: exportSelectedFavoriteImage(); break;
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
        case PreviewCommandType::PREVIOUS_PALETTE: changePalette(-1); break;
        case PreviewCommandType::NEXT_PALETTE: changePalette(1); break;
        case PreviewCommandType::PREVIOUS_CONFIGURATION_BUNDLE: changeConfigurationBundle(-1); break;
        case PreviewCommandType::NEXT_CONFIGURATION_BUNDLE: changeConfigurationBundle(1); break;
        case PreviewCommandType::PROFILES_PREVIOUS_SECTION: changeProfilesSection(-1); break;
        case PreviewCommandType::PROFILES_NEXT_SECTION: changeProfilesSection(1); break;
        case PreviewCommandType::PROFILES_PREVIOUS_ITEM: changeProfilesItem(-1); break;
        case PreviewCommandType::PROFILES_NEXT_ITEM: changeProfilesItem(1); break;
        case PreviewCommandType::PROFILES_NEW_DEFAULT: newProfilesItem(); break;
        case PreviewCommandType::PROFILES_EDIT_SELECTED: editSelectedProfilesItem(); break;
        case PreviewCommandType::PROFILES_DUPLICATE_SELECTED: duplicateSelectedProfilesItem(); break;
        case PreviewCommandType::PROFILES_DELETE_SELECTED: deleteSelectedProfilesItem(); break;
        case PreviewCommandType::PROFILES_IMPORT_SELECTED: importSelectedProfilesItem(); break;
        case PreviewCommandType::PROFILES_EXPORT_SELECTED: exportSelectedProfilesItem(); break;
        case PreviewCommandType::PROFILES_USE_SELECTED: useSelectedProfilesItem(); break;
        case PreviewCommandType::OPEN_STRUCTURAL_EDITOR: enterConfigurationEditor(); break;
        case PreviewCommandType::OPEN_FACTION_EDITOR: enterFactionConfigurationEditor(); break;
        case PreviewCommandType::OPEN_PALETTE_EDITOR: enterPaletteConfigurationEditor(); break;
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
        case PreviewCommandType::INSPECTION_PREVIOUS_GROUP: changeInspectionGroup(-1); break;
        case PreviewCommandType::INSPECTION_NEXT_GROUP: changeInspectionGroup(1); break;
        case PreviewCommandType::INSPECTION_PREVIOUS_VIEW: changeInspectionView(-1); break;
        case PreviewCommandType::INSPECTION_NEXT_VIEW: changeInspectionView(1); break;
        case PreviewCommandType::TOGGLE_INSPECTION_PRESENTATION: toggleInspectionPresentation(); break;
        case PreviewCommandType::OPEN_GENERATE_WORKSPACE: switchWorkspace(PreviewWorkspace::GENERATE); break;
        case PreviewCommandType::OPEN_ANIMATION_WORKSPACE: switchWorkspace(PreviewWorkspace::ANIMATION); break;
        case PreviewCommandType::TOGGLE_GENERATION_INSPECTOR: toggleGenerationInspector(); break;
        case PreviewCommandType::TOGGLE_PALETTE_INSPECTOR: togglePaletteInspector(); break;
        case PreviewCommandType::CYCLE_DIAGNOSTIC_VIEW: cycleDiagnosticView(); break;
        case PreviewCommandType::TOGGLE_GENERATION_STAGE_VIEW: toggleGenerationStageView(); break;
        case PreviewCommandType::PREVIOUS_GENERATION_STAGE: moveGenerationStage(-1); break;
        case PreviewCommandType::NEXT_GENERATION_STAGE: moveGenerationStage(1); break;
        case PreviewCommandType::CYCLE_ANIMATION_TYPE: cycleAnimationType(); break;
        case PreviewCommandType::CYCLE_ANIMATION_BASE_STATE: cycleAnimationBaseState(); break;
        case PreviewCommandType::CYCLE_MOVEMENT_PHASE: cycleMovementPhase(); break;
        case PreviewCommandType::CYCLE_FIRING_TARGET: cycleFiringTarget(); break;
        case PreviewCommandType::TRIGGER_ANIMATION_FIRE: triggerAnimationFire(); break;
        case PreviewCommandType::CYCLE_ANIMATION_PLAYBACK_SPEED: cycleAnimationPlaybackSpeed(); break;
        case PreviewCommandType::SET_ANIMATION_NORMALIZED_TIME: setAnimationNormalizedTime(command.Value); break;
        case PreviewCommandType::APPLY_ANIMATION_STATE: applySelectedAnimationState(); break;
        case PreviewCommandType::RETURN_ANIMATION_TO_IDLE: returnAnimationToIdle(); break;
        case PreviewCommandType::TOGGLE_ANIMATION:
            if (m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::GENERATE)
            {
                if (m_PreviewMode == PreviewMode::ANIMATION)
                {
                    m_PreviewMode = PreviewMode::STATIC;
                    setDisplayedStaticFrame();
                    updateWindowTitle();
                }
                else { enterGenerateIdlePlayback(); }
            }
            else if (m_PreviewMode == PreviewMode::ANIMATION) { enterFrameInspection(); }
            else { enterAnimationPlayback(); }
            break;
        case PreviewCommandType::TOGGLE_FRAME_INSPECTION:
            enterFrameInspection();
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
        case PreviewCommandType::OPEN_REROLL_STUDIO: switchWorkspace(PreviewWorkspace::REROLL); break;
        case PreviewCommandType::REROLL_STUDIO_TOGGLE_DOMAIN:
            if (command.Value < SpectralShipGen::GenerationDomainCount)
            {
                toggleAttributeRerollDomain(m_RerollStudio, static_cast<SpectralShipGen::GenerationDomain>(command.Value));
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
            selectAttributeRerollParentChannel(m_RerollStudio, SpectralShipGen::GenerationSeedChannel::STRUCTURE);
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
            if (getCurrentRecipe().StructuralPreset.has_value()) { resetCalibrationGroup(m_CalibrationSession, *getCurrentRecipe().StructuralPreset, m_CalibrationGroup); }
            setStatusMessage("Selected tuning group reset to production defaults.");
            break;
        case PreviewCommandType::CALIBRATION_RESET_ALL:
            resetAllCalibrationTuning(m_CalibrationSession);
            setStatusMessage("All temporary tuning reset to production defaults.");
            break;
        case PreviewCommandType::CALIBRATION_APPLY_SUGGESTED:
            if (getCurrentRecipe().StructuralPreset.has_value()) { applySuggestedGroupWeights(m_CalibrationSession, *getCurrentRecipe().StructuralPreset, m_CalibrationGroup, getCalibrationContextFilter()); }
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
        case PreviewCommandType::BACK_OR_EXIT: handleBackOrCancel(); break;
        default: break;
        }

        updateCommandPanelState();
    }

    std::optional<PreviewCommand> ShipGeneratorPreviewApp::getKeyboardCommand(sf::Keyboard::Key key, bool shift, bool control) const
    {
        const PreviewWorkspace workspace = m_WorkspaceSession.getActiveWorkspace();

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
            default: return std::nullopt;
            }
        }

        if (workspace == PreviewWorkspace::GENERATE)
        {
            if (m_PreviewMode == PreviewMode::GALLERY)
            {
                switch (key)
                {
                case sf::Keyboard::Left: return PreviewCommand{ PreviewCommandType::GALLERY_LEFT, 0u };
                case sf::Keyboard::Right: return PreviewCommand{ PreviewCommandType::GALLERY_RIGHT, 0u };
                case sf::Keyboard::Up: return PreviewCommand{ PreviewCommandType::GALLERY_UP, 0u };
                case sf::Keyboard::Down: return PreviewCommand{ PreviewCommandType::GALLERY_DOWN, 0u };
                case sf::Keyboard::Enter: return PreviewCommand{ PreviewCommandType::SELECT_GALLERY_CANDIDATE, 0u };
                case sf::Keyboard::Space: return PreviewCommand{ PreviewCommandType::GENERATE_NEW, 0u };
                default: return std::nullopt;
                }
            }

            if (control && key == sf::Keyboard::O) { return PreviewCommand{ PreviewCommandType::IMPORT_RECIPE, 0u }; }
            if (control && key == sf::Keyboard::E) { return PreviewCommand{ PreviewCommandType::EXPORT_RECIPE, 0u }; }
            if (key == sf::Keyboard::Space) { return PreviewCommand{ shift ? PreviewCommandType::OPEN_GALLERY : PreviewCommandType::GENERATE_NEW, 0u }; }
            if (key == sf::Keyboard::Left) { return PreviewCommand{ PreviewCommandType::PREVIOUS_HISTORY, 0u }; }
            if (key == sf::Keyboard::Right) { return PreviewCommand{ PreviewCommandType::NEXT_HISTORY, 0u }; }
            return std::nullopt;
        }

        if (workspace == PreviewWorkspace::PROFILES)
        {
            if (control && key == sf::Keyboard::D) { return PreviewCommand{ PreviewCommandType::PROFILES_DUPLICATE_SELECTED, 0u }; }
            if (control && key == sf::Keyboard::O) { return PreviewCommand{ PreviewCommandType::PROFILES_IMPORT_SELECTED, 0u }; }
            if (control && key == sf::Keyboard::E) { return PreviewCommand{ PreviewCommandType::PROFILES_EXPORT_SELECTED, 0u }; }
            return std::nullopt;
        }

        if (workspace == PreviewWorkspace::REROLL)
        {
            if (key == sf::Keyboard::Space) { return PreviewCommand{ PreviewCommandType::REROLL_STUDIO_GENERATE_CANDIDATE, 0u }; }
            if (key == sf::Keyboard::Enter) { return PreviewCommand{ PreviewCommandType::REROLL_STUDIO_ACCEPT, 0u }; }
            return std::nullopt;
        }

        if (workspace == PreviewWorkspace::INSPECT)
        {
            return std::nullopt;
        }

        if (workspace == PreviewWorkspace::FAVORITES)
        {
            if (control && key == sf::Keyboard::E) { return PreviewCommand{ PreviewCommandType::EXPORT_RECIPE, 0u }; }
            switch (key)
            {
            case sf::Keyboard::Left: return PreviewCommand{ PreviewCommandType::FAVORITES_LEFT, 0u };
            case sf::Keyboard::Right: return PreviewCommand{ PreviewCommandType::FAVORITES_RIGHT, 0u };
            case sf::Keyboard::Up: return PreviewCommand{ PreviewCommandType::FAVORITES_UP, 0u };
            case sf::Keyboard::Down: return PreviewCommand{ PreviewCommandType::FAVORITES_DOWN, 0u };
            case sf::Keyboard::Enter: return PreviewCommand{ PreviewCommandType::SELECT_FAVORITE, 0u };
            case sf::Keyboard::Delete: return PreviewCommand{ PreviewCommandType::REMOVE_SELECTED_FAVORITE, 0u };
            default: return std::nullopt;
            }
        }

        if (workspace == PreviewWorkspace::ANIMATION)
        {
            if (key == sf::Keyboard::Space) { return PreviewCommand{ PreviewCommandType::TOGGLE_ANIMATION, 0u }; }
            if (m_PreviewMode == PreviewMode::FRAME_INSPECTION)
            {
                if (key == sf::Keyboard::Left) { return PreviewCommand{ PreviewCommandType::PREVIOUS_FRAME, 0u }; }
                if (key == sf::Keyboard::Right) { return PreviewCommand{ PreviewCommandType::NEXT_FRAME, 0u }; }
            }
        }

        return std::nullopt;
    }

    bool ShipGeneratorPreviewApp::hasCurrentShip() const
    {
        return hasPreviewInspectionShip(m_GeneratedShip);
    }

    bool ShipGeneratorPreviewApp::hasKeyboardInputFocus() const
    {
        return m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR && m_ConfigurationEditor.hasKeyboardFocus();
    }

    void ShipGeneratorPreviewApp::handleBackOrCancel()
    {
        PreviewBackContext context;
        context.Workspace = m_WorkspaceSession.getActiveWorkspace();
        context.Mode = m_PreviewMode;
        context.KeyboardInputFocused = hasKeyboardInputFocus();
        context.HelpVisible = m_Diagnostics.HelpVisible;
        context.GenerationInspectorVisible = m_Diagnostics.GenerationInspectorVisible;
        context.PaletteInspectorVisible = m_Diagnostics.PaletteInspectorVisible;
        context.RerollCandidateValid = m_RerollStudio.CandidateValid;

        switch (resolvePreviewBackAction(context))
        {
        case PreviewBackAction::RELEASE_KEYBOARD_FOCUS:
            m_ConfigurationEditor.releaseKeyboardFocus();
            break;
        case PreviewBackAction::CLOSE_HELP:
            m_Diagnostics.HelpVisible = false;
            break;
        case PreviewBackAction::CLOSE_CONTEXT_OVERLAY:
            m_Diagnostics.GenerationInspectorVisible = false;
            m_Diagnostics.PaletteInspectorVisible = false;
            break;
        case PreviewBackAction::CANCEL_CONFIGURATION_EDITOR:
            handleConfigurationEditorEvent(m_ConfigurationEditor.createCancelEvent());
            break;
        case PreviewBackAction::CLOSE_GALLERY:
            exitGalleryMode();
            break;
        case PreviewBackAction::EXIT_CALIBRATION:
            exitCalibrationLab();
            break;
        case PreviewBackAction::DISCARD_REROLL_CANDIDATE:
            m_RerollStudio.CandidateValid = false;
            m_RerollStudio.CandidateRecipe = m_RerollStudio.BaseRecipe;
            setStatusMessage("Reroll candidate discarded. Base ship retained.");
            updateWindowTitle();
            break;
        case PreviewBackAction::NONE:
        default:
            break;
        }
    }

    void ShipGeneratorPreviewApp::handleKeyPressed(const sf::Event::KeyEvent& event)
    {
        if (event.code == sf::Keyboard::Escape)
        {
            executeCommand({ PreviewCommandType::BACK_OR_EXIT, 0u });
            return;
        }

        if (hasKeyboardInputFocus()) { return; }

        if (event.code == sf::Keyboard::F1)
        {
            executeCommand({ PreviewCommandType::TOGGLE_HELP, 0u });
            return;
        }

        if (m_Diagnostics.HelpVisible || m_Diagnostics.GenerationInspectorVisible || m_Diagnostics.PaletteInspectorVisible) { return; }

        uint32_t workspaceShortcut = 0u;
        switch (event.code)
        {
        case sf::Keyboard::Num1:
        case sf::Keyboard::Numpad1: workspaceShortcut = 1u; break;
        case sf::Keyboard::Num2:
        case sf::Keyboard::Numpad2: workspaceShortcut = 2u; break;
        case sf::Keyboard::Num3:
        case sf::Keyboard::Numpad3: workspaceShortcut = 3u; break;
        case sf::Keyboard::Num4:
        case sf::Keyboard::Numpad4: workspaceShortcut = 4u; break;
        case sf::Keyboard::Num5:
        case sf::Keyboard::Numpad5: workspaceShortcut = 5u; break;
        case sf::Keyboard::Num6:
        case sf::Keyboard::Numpad6: workspaceShortcut = 6u; break;
        default: break;
        }
        if (workspaceShortcut != 0u)
        {
            const std::optional<PreviewWorkspace> workspace = getPreviewWorkspaceForShortcut(workspaceShortcut, false);
            if (workspace.has_value()) { switchWorkspace(*workspace); }
            return;
        }

        if (event.code == sf::Keyboard::B)
        {
            executeCommand({ PreviewCommandType::ADD_CURRENT_TO_FAVORITES, 0u });
            return;
        }

        if (m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR)
        {
            if (event.control && event.code == sf::Keyboard::D) { handleConfigurationEditorEvent({ ConfigurationEditorAction::DUPLICATE }); }
            else if (event.control && event.code == sf::Keyboard::O) { handleConfigurationEditorEvent({ ConfigurationEditorAction::IMPORT_PRESET }); }
            else if (event.control && event.code == sf::Keyboard::E) { handleConfigurationEditorEvent({ ConfigurationEditorAction::EXPORT_PRESET }); }
            return;
        }

        const std::optional<PreviewCommand> command = getKeyboardCommand(event.code, event.shift, event.control);
        if (command.has_value()) { executeCommand(*command); }
    }

    void ShipGeneratorPreviewApp::handleMouseMoved(const sf::Event::MouseMoveEvent& event)
    {
        const sf::Vector2f position = mapWindowPixelToLogical(event.x, event.y);
        if (m_Diagnostics.HelpVisible || m_Diagnostics.GenerationInspectorVisible || m_Diagnostics.PaletteInspectorVisible) { return; }

        m_WorkspaceNavigation.onMouseMove(position);
        if (m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR)
        {
            m_ConfigurationEditor.onMouseMove(position.x, position.y);
            return;
        }
        m_CommandPanel.onMouseMove(position);
        if (m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::ANIMATION && m_CommandPanel.getAnimationTimelineSlider().Dragging)
        {
            setAnimationNormalizedTime(m_CommandPanel.getAnimationTimelineSlider().Value);
        }

        if (m_PreviewMode == PreviewMode::GALLERY) { m_GalleryState.Grid.HoveredIndex = findPreviewThumbnailItemAtPosition(position, m_GalleryState.Grid); }
        if (m_PreviewMode == PreviewMode::FAVORITES) { m_FavoritesState.Grid.HoveredIndex = findPreviewThumbnailItemAtPosition(position, m_FavoritesState.Grid); }
    }

    void ShipGeneratorPreviewApp::handleMousePressed(const sf::Event::MouseButtonEvent& event)
    {
        if (event.button != sf::Mouse::Left && event.button != sf::Mouse::Right) { return; }
        if (!isWindowPixelInsideLogicalViewport(event.x, event.y)) { return; }

        const sf::Vector2f position = mapWindowPixelToLogical(event.x, event.y);
        if (m_Diagnostics.HelpVisible || m_Diagnostics.GenerationInspectorVisible || m_Diagnostics.PaletteInspectorVisible) { return; }

        if (event.button == sf::Mouse::Right)
        {
            if (m_PreviewMode != PreviewMode::GALLERY) { return; }
            const int32_t candidateIndex = findPreviewThumbnailItemAtPosition(position, m_GalleryState.Grid);
            if (candidateIndex >= 0) { toggleGalleryCandidateFavorite(static_cast<uint32_t>(candidateIndex)); }
            return;
        }

        if (m_WorkspaceNavigation.onMousePress(position)) { return; }

        if (m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR)
        {
            m_ConfigurationEditor.onMousePress(position.x, position.y);
            return;
        }
        m_CommandPanel.onMousePress(position);

        if (m_CommandPanel.getPressedButtonIndex() >= 0 || m_CommandPanel.isDimensionSliderDragging()) { return; }

        if (m_PreviewMode == PreviewMode::GALLERY)
        {
            const int32_t candidateIndex = findPreviewThumbnailItemAtPosition(position, m_GalleryState.Grid);
            if (candidateIndex < 0) { return; }
            m_GalleryState.Grid.SelectedIndex = static_cast<uint32_t>(candidateIndex);
            updateWindowTitle();
            return;
        }

        if (m_PreviewMode == PreviewMode::FAVORITES)
        {
            const int32_t favoriteIndex = findPreviewThumbnailItemAtPosition(position, m_FavoritesState.Grid);
            if (favoriteIndex < 0) { return; }
            m_FavoritesState.Grid.SelectedIndex = static_cast<uint32_t>(favoriteIndex);
            m_PendingFavoriteRemovalIndex.reset();
            updateWindowTitle();
        }
    }

    void ShipGeneratorPreviewApp::handleMouseReleased(const sf::Event::MouseButtonEvent& event)
    {
        if (event.button != sf::Mouse::Left) { return; }

        const sf::Vector2f position = mapWindowPixelToLogical(event.x, event.y);
        if (m_Diagnostics.HelpVisible || m_Diagnostics.GenerationInspectorVisible || m_Diagnostics.PaletteInspectorVisible)
        {
            m_WorkspaceNavigation.cancelPress();
            return;
        }

        const bool navigationPressActive = m_WorkspaceNavigation.getPressedButtonIndex() >= 0;
        const std::optional<PreviewWorkspace> workspace = m_WorkspaceNavigation.onMouseRelease(position);
        if (workspace.has_value())
        {
            switchWorkspace(*workspace);
            return;
        }
        if (navigationPressActive) { return; }

        if (m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR)
        {
            const std::optional<ConfigurationEditorEvent> editorEvent = m_ConfigurationEditor.onMouseRelease(position.x, position.y);
            if (editorEvent.has_value()) { handleConfigurationEditorEvent(*editorEvent); }
            return;
        }
        const std::optional<PreviewCommand> command = m_CommandPanel.onMouseRelease(position);
        if (command.has_value()) { executeCommand(*command); }
    }

    void ShipGeneratorPreviewApp::handleMouseWheelScrolled(const sf::Event::MouseWheelScrollEvent& event)
    {
        if (!isWindowPixelInsideLogicalViewport(event.x, event.y)) { return; }
        if (m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR) { m_ConfigurationEditor.onMouseWheelScrolled(event.delta); }
    }

    void ShipGeneratorPreviewApp::handleTextEntered(const sf::Event::TextEvent& event)
    {
        if (m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR) { m_ConfigurationEditor.onTextEntered(event.unicode); }
    }

    void ShipGeneratorPreviewApp::handleWindowResized(const sf::Event::SizeEvent& event)
    {
        updateLogicalWindowView(event.width, event.height);
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
        case PreviewCommandType::TOGGLE_INSPECTION_PRESENTATION: return m_Diagnostics.InspectionPresentation == PreviewInspectionPresentation::OVERLAY;
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
        if (type == PreviewCommandType::OPEN_CALIBRATION_LAB) { return getCurrentRecipe().StructuralPreset.has_value(); }
        const bool inspectWorkspace = m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::INSPECT;
        if ((inspectWorkspace || m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::ANIMATION) && type == PreviewCommandType::OPEN_GENERATE_WORKSPACE) { return true; }
        if (inspectWorkspace && type == PreviewCommandType::OPEN_REROLL_STUDIO) { return hasCurrentShip(); }
        if (inspectWorkspace && type == PreviewCommandType::ADD_CURRENT_TO_FAVORITES) { return hasCurrentShip() && !isCurrentFavorite(); }
        if (inspectWorkspace && type == PreviewCommandType::PIN_CURRENT) { return hasCurrentShip(); }
        if (type == PreviewCommandType::INSPECTION_PREVIOUS_GROUP || type == PreviewCommandType::INSPECTION_NEXT_GROUP || type == PreviewCommandType::INSPECTION_PREVIOUS_VIEW || type == PreviewCommandType::INSPECTION_NEXT_VIEW || type == PreviewCommandType::TOGGLE_INSPECTION_PRESENTATION) { return inspectWorkspace && hasCurrentShip() && !browserMode; }
        if (type == PreviewCommandType::OPEN_ANIMATION_WORKSPACE) { return inspectWorkspace && hasCurrentShip(); }
        if (type == PreviewCommandType::TOGGLE_GENERATION_INSPECTOR || type == PreviewCommandType::TOGGLE_PALETTE_INSPECTOR) { return inspectWorkspace && hasCurrentShip() && !browserMode; }
        if (type == PreviewCommandType::BACK_OR_EXIT) { return true; }
        if (overlayVisible) { return false; }

        if (m_PreviewMode == PreviewMode::REROLL_STUDIO)
        {
            switch (type)
            {
            case PreviewCommandType::REROLL_STUDIO_TOGGLE_DOMAIN:
                return command.Value < SpectralShipGen::GenerationDomainCount;
            case PreviewCommandType::REROLL_STUDIO_SELECT_ALL:
            case PreviewCommandType::REROLL_STUDIO_CLEAR:
            case PreviewCommandType::REROLL_STUDIO_SELECT_STRUCTURE:
            case PreviewCommandType::REROLL_STUDIO_SELECT_APPEARANCE:
            case PreviewCommandType::REROLL_STUDIO_CANCEL:
            case PreviewCommandType::TOGGLE_STRUCTURE_LOCK:
            case PreviewCommandType::TOGGLE_PALETTE_LOCK:
            case PreviewCommandType::TOGGLE_DETAILS_LOCK:
            case PreviewCommandType::TOGGLE_ATTACHMENTS_LOCK:
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
            if (isGalleryGenerationConfigurationCommand(type))
            {
                if (type == PreviewCommandType::PREVIOUS_CONFIGURATION_BUNDLE || type == PreviewCommandType::NEXT_CONFIGURATION_BUNDLE) { return !m_CustomPresetWorkspace.getConfigurationBundles().empty(); }
                return true;
            }
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

        if (type == PreviewCommandType::OPEN_STRUCTURAL_EDITOR || type == PreviewCommandType::OPEN_FACTION_EDITOR || type == PreviewCommandType::OPEN_PALETTE_EDITOR)
        {
            return m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::PROFILES && m_PreviewMode == PreviewMode::STATIC;
        }

        if (type == PreviewCommandType::PREVIOUS_CONFIGURATION_BUNDLE || type == PreviewCommandType::NEXT_CONFIGURATION_BUNDLE)
        {
            return m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::GENERATE && m_PreviewMode != PreviewMode::CONFIGURATION_EDITOR && !m_CustomPresetWorkspace.getConfigurationBundles().empty();
        }

        if (m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::PROFILES && m_PreviewMode == PreviewMode::STATIC)
        {
            const bool fullConfiguration = m_ProfilesSection == ProfilesSection::FULL_CONFIGURATION;
            const bool hasSelectedBundle = m_ProfilesSelectedBundleId.has_value() && m_CustomPresetWorkspace.findConfigurationBundle(*m_ProfilesSelectedBundleId) != nullptr;
            const bool selectedCustom = m_ProfilesSection == ProfilesSection::STRUCTURAL ? (m_SelectedStructuralPresetId.has_value() && m_CustomPresetWorkspace.findStructural(*m_SelectedStructuralPresetId) != nullptr)
                : m_ProfilesSection == ProfilesSection::FACTION ? (m_SelectedFactionPresetId.has_value() && m_CustomPresetWorkspace.findFaction(*m_SelectedFactionPresetId) != nullptr)
                : m_ProfilesSection == ProfilesSection::PALETTE ? (m_SelectedPalettePresetId.has_value() && m_CustomPresetWorkspace.findPalette(*m_SelectedPalettePresetId) != nullptr)
                : hasSelectedBundle;
            switch (type)
            {
            case PreviewCommandType::PROFILES_PREVIOUS_SECTION:
            case PreviewCommandType::PROFILES_NEXT_SECTION:
            case PreviewCommandType::PROFILES_NEW_DEFAULT:
            case PreviewCommandType::PROFILES_IMPORT_SELECTED:
                return true;
            case PreviewCommandType::PROFILES_PREVIOUS_ITEM:
            case PreviewCommandType::PROFILES_NEXT_ITEM:
                return !fullConfiguration || !m_CustomPresetWorkspace.getConfigurationBundles().empty();
            case PreviewCommandType::PROFILES_EDIT_SELECTED:
            case PreviewCommandType::PROFILES_DUPLICATE_SELECTED:
                return !fullConfiguration || hasSelectedBundle;
            case PreviewCommandType::PROFILES_DELETE_SELECTED:
            case PreviewCommandType::PROFILES_EXPORT_SELECTED:
                return selectedCustom;
            case PreviewCommandType::PROFILES_USE_SELECTED:
                return fullConfiguration && hasSelectedBundle;
            default:
                break;
            }
        }

        if (m_PreviewMode == PreviewMode::FAVORITES)
        {
            if (type == PreviewCommandType::CLOSE_FAVORITES) { return true; }
            if (type == PreviewCommandType::ADD_CURRENT_TO_FAVORITES) { return !isCurrentFavorite(); }
            if (type == PreviewCommandType::REMOVE_CURRENT_FROM_FAVORITES) { return isCurrentFavorite(); }
            if (type == PreviewCommandType::CLEAR_PIN) { return m_Comparison.Pinned.Valid; }

            const bool hasSelection = !m_FavoritesState.Grid.Items.empty() && m_FavoritesState.Grid.SelectedIndex < m_FavoritesState.Grid.Items.size();
            const bool selectedValid = hasSelection && m_FavoritesState.Grid.Items[m_FavoritesState.Grid.SelectedIndex].Valid;
            if (type == PreviewCommandType::EXPORT_RECIPE || type == PreviewCommandType::EXPORT_FAVORITE_IMAGE ||
                type == PreviewCommandType::SELECT_FAVORITE || type == PreviewCommandType::OPEN_FAVORITE_INSPECT ||
                type == PreviewCommandType::OPEN_FAVORITE_ANIMATION || type == PreviewCommandType::OPEN_FAVORITE_REROLL)
            {
                return selectedValid;
            }
            if (type == PreviewCommandType::REMOVE_SELECTED_FAVORITE) { return hasSelection; }
            const uint32_t favoritePageCount = getPreviewThumbnailPageCount(m_FavoritesState.Grid);
            const uint32_t favoritePage = getPreviewThumbnailCurrentPage(m_FavoritesState.Grid);
            if (type == PreviewCommandType::FAVORITES_PREVIOUS_PAGE) { return favoritePageCount > 0u && favoritePage > 0u; }
            if (type == PreviewCommandType::FAVORITES_NEXT_PAGE) { return favoritePage + 1u < favoritePageCount; }
            if (!hasSelection) { return false; }
            const uint32_t selectedIndex = m_FavoritesState.Grid.SelectedIndex;
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
            const SpectralShipGen::ShipDimensions dimensions = getCurrentRecipe().Dimensions;
            return m_Collections.getResolutionBookmarks().size() < MaximumResolutionBookmarks && !m_Collections.hasResolutionBookmark(dimensions);
        }
        if (type == PreviewCommandType::REMOVE_RESOLUTION_BOOKMARK)
        {
            const SpectralShipGen::ShipDimensions dimensions = getCurrentRecipe().Dimensions;
            return m_Collections.hasResolutionBookmark(dimensions);
        }
        if (type == PreviewCommandType::SELECT_RESOLUTION_BOOKMARK) { return command.Value < m_Collections.getResolutionBookmarks().size(); }
        if (type == PreviewCommandType::OPEN_FAVORITES) { return !m_FavoritesState.Grid.Items.empty(); }
        if (type == PreviewCommandType::CLOSE_FAVORITES || type == PreviewCommandType::FAVORITES_LEFT || type == PreviewCommandType::FAVORITES_RIGHT || type == PreviewCommandType::FAVORITES_UP || type == PreviewCommandType::FAVORITES_DOWN || type == PreviewCommandType::FAVORITES_PREVIOUS_PAGE || type == PreviewCommandType::FAVORITES_NEXT_PAGE ||
            type == PreviewCommandType::SELECT_FAVORITE || type == PreviewCommandType::OPEN_FAVORITE_INSPECT || type == PreviewCommandType::OPEN_FAVORITE_ANIMATION ||
            type == PreviewCommandType::OPEN_FAVORITE_REROLL || type == PreviewCommandType::REMOVE_SELECTED_FAVORITE || type == PreviewCommandType::EXPORT_FAVORITE_IMAGE)
        {
            return false;
        }
        if (type == PreviewCommandType::PIN_CURRENT) { return true; }
        if (type == PreviewCommandType::CLEAR_PIN) { return m_Comparison.Pinned.Valid; }
        if (type == PreviewCommandType::TOGGLE_COMPARISON) { return m_Comparison.Pinned.Valid; }
        if (type == PreviewCommandType::CYCLE_DIAGNOSTIC_VIEW) { return inspectWorkspace && hasCurrentShip(); }
        if (type == PreviewCommandType::TOGGLE_GENERATION_STAGE_VIEW) { return inspectWorkspace && hasCurrentShip() && !m_GenerationDebugInfo.HullStages.empty(); }
        if (type == PreviewCommandType::PREVIOUS_GENERATION_STAGE || type == PreviewCommandType::NEXT_GENERATION_STAGE) { return m_Diagnostics.GenerationStageView && !m_GenerationDebugInfo.HullStages.empty(); }
        if (type == PreviewCommandType::SAVE_CURRENT) { return hasCurrentShip(); }
        if (type == PreviewCommandType::EXPORT_RECIPE || type == PreviewCommandType::IMPORT_RECIPE) { return true; }
        if (type == PreviewCommandType::SAVE_SPRITESHEET)
        {
            return m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::GENERATE ? !m_AnimationSession.getIdleAnimation().Frames.empty() : !getActiveAnimationFrames().empty();
        }
        if (type == PreviewCommandType::TOGGLE_ANIMATION)
        {
            return m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::GENERATE ? !m_AnimationSession.getIdleAnimation().Frames.empty() : !getActiveAnimationFrames().empty();
        }
        if (type == PreviewCommandType::TOGGLE_FRAME_INSPECTION) { return !getActiveAnimationFrames().empty(); }
        if (type == PreviewCommandType::CYCLE_ANIMATION_TYPE) { return hasCurrentShip(); }
        if (type == PreviewCommandType::CYCLE_ANIMATION_BASE_STATE) { return hasCurrentShip(); }
        if (type == PreviewCommandType::TRIGGER_ANIMATION_FIRE) { return hasCurrentShip(); }
        if (type == PreviewCommandType::CYCLE_ANIMATION_PLAYBACK_SPEED) { return hasCurrentShip() && !getActiveAnimationFrames().empty(); }
        if (type == PreviewCommandType::SET_ANIMATION_NORMALIZED_TIME) { return hasCurrentShip() && !getActiveAnimationFrames().empty(); }
        if (type == PreviewCommandType::CYCLE_MOVEMENT_PHASE) { return m_AnimationSession.getSelectedAnimationType() != SpectralShipGen::ShipAnimationType::IDLE && m_AnimationSession.getSelectedAnimationType() != SpectralShipGen::ShipAnimationType::FIRE; }
        if (type == PreviewCommandType::CYCLE_FIRING_TARGET) { return m_AnimationSession.getSelectedAnimationType() == SpectralShipGen::ShipAnimationType::FIRE && m_AnimationSession.getFiringTargets().size() > 1u; }
        if (type == PreviewCommandType::APPLY_ANIMATION_STATE) { return true; }
        if (type == PreviewCommandType::RETURN_ANIMATION_TO_IDLE) { return m_AnimationSession.getRuntimeMovementType() != SpectralShipGen::ShipAnimationType::IDLE || m_AnimationSession.isTransientStatePreviewActive(); }

        if (m_PreviewMode == PreviewMode::FRAME_INSPECTION)
        {
            return type == PreviewCommandType::PREVIOUS_FRAME || type == PreviewCommandType::NEXT_FRAME || type == PreviewCommandType::CYCLE_ANIMATION_TYPE || type == PreviewCommandType::CYCLE_ANIMATION_BASE_STATE || type == PreviewCommandType::CYCLE_MOVEMENT_PHASE || type == PreviewCommandType::CYCLE_FIRING_TARGET || type == PreviewCommandType::TRIGGER_ANIMATION_FIRE || type == PreviewCommandType::CYCLE_ANIMATION_PLAYBACK_SPEED || type == PreviewCommandType::SET_ANIMATION_NORMALIZED_TIME || type == PreviewCommandType::APPLY_ANIMATION_STATE || type == PreviewCommandType::RETURN_ANIMATION_TO_IDLE;
        }
        if (type == PreviewCommandType::PREVIOUS_FRAME || type == PreviewCommandType::NEXT_FRAME || type == PreviewCommandType::GALLERY_LEFT || type == PreviewCommandType::GALLERY_RIGHT || type == PreviewCommandType::GALLERY_UP || type == PreviewCommandType::GALLERY_DOWN || type == PreviewCommandType::SELECT_GALLERY_CANDIDATE) { return false; }
        if (type == PreviewCommandType::PREVIOUS_HISTORY) { return m_Collections.getHistoryIndex() > 0u; }
        if (type == PreviewCommandType::NEXT_HISTORY) { return m_Collections.getHistoryIndex() + 1u < m_Collections.getHistoryCount(); }
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
        case PreviewCommandType::PREVIOUS_PALETTE:
        case PreviewCommandType::NEXT_PALETTE:
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

    sf::Vector2f ShipGeneratorPreviewApp::mapWindowPixelToLogical(int32_t x, int32_t y) const
    {
        return m_Window.mapPixelToCoords(sf::Vector2i(x, y), m_LogicalView);
    }

    void ShipGeneratorPreviewApp::moveAnimationFrame(int32_t delta)
    {
        if (!m_AnimationSession.moveFrame(delta)) { return; }
        setDisplayedAnimationFrame(m_AnimationSession.getFrameIndex());
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::moveFavoritesSelection(int32_t deltaX, int32_t deltaY)
    {
        if (!movePreviewThumbnailSelection(m_FavoritesState.Grid, deltaX, deltaY)) { return; }
        m_PendingFavoriteRemovalIndex.reset();
        updateCommandPanelState();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::changeFavoritesPage(int32_t delta)
    {
        if (m_FavoritesState.Grid.Items.empty() || delta == 0) { return; }
        const uint32_t pageCount = getPreviewThumbnailPageCount(m_FavoritesState.Grid);
        const uint32_t currentPage = getPreviewThumbnailCurrentPage(m_FavoritesState.Grid);
        const int64_t requestedPage = static_cast<int64_t>(currentPage) + static_cast<int64_t>(delta);
        const uint32_t targetPage = static_cast<uint32_t>(std::clamp<int64_t>(requestedPage, 0, static_cast<int64_t>(pageCount - 1u)));
        if (targetPage == currentPage) { return; }
        m_FavoritesState.Grid.SelectedIndex = targetPage * getPreviewThumbnailPageCapacity(m_FavoritesState.Grid);
        m_FavoritesState.Grid.HoveredIndex = -1;
        m_PendingFavoriteRemovalIndex.reset();
        updateCommandPanelState();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::moveGallerySelection(int32_t deltaX, int32_t deltaY)
    {
        if (movePreviewThumbnailSelection(m_GalleryState.Grid, deltaX, deltaY)) { updateWindowTitle(); }
    }

    bool ShipGeneratorPreviewApp::isWindowPixelInsideLogicalViewport(int32_t x, int32_t y) const
    {
        const sf::Vector2u size = m_Window.getSize();
        return isPreviewPixelInsideLogicalViewport(x, y, size.x, size.y, m_LogicalViewport);
    }

    bool ShipGeneratorPreviewApp::loadFavorite(uint32_t index)
    {
        const PreviewGenerationRecipe* favorite = m_Collections.getFavorite(index);
        if (favorite == nullptr || index >= m_FavoritesState.Grid.Items.size() || !m_FavoritesState.Grid.Items[index].Valid) { return false; }

        const PreviewGenerationRecipe recipe = *favorite;
        m_FavoritesState.Grid.HoveredIndex = -1;
        m_PendingFavoriteRemovalIndex.reset();

        if (recipe == getCurrentRecipe())
        {
            refreshDisplayedTexture();
            updateWindowTitle();
            return true;
        }

        m_SelectedStructuralPresetId.reset();
        m_SelectedFactionPresetId.reset();
        m_SelectedBuiltInPalettePreset.reset();
        m_SelectedPalettePresetId.reset();
        m_SelectedConfigurationBundleId.reset();
        appendHistoryEntry(recipe);
        return true;
    }

    void ShipGeneratorPreviewApp::openSelectedFavoriteInWorkspace(PreviewWorkspace workspace)
    {
        if (!loadFavorite(m_FavoritesState.Grid.SelectedIndex)) { return; }
        switchWorkspace(workspace);
        setStatusMessage("Favorite opened in " + std::string(getPreviewWorkspaceName(workspace)) + ".");
    }

    void ShipGeneratorPreviewApp::exportSelectedFavoriteImage()
    {
        const uint32_t index = m_FavoritesState.Grid.SelectedIndex;
        const PreviewGenerationRecipe* favorite = m_Collections.getFavorite(index);
        if (favorite == nullptr || index >= m_FavoritesState.Grid.Items.size() || !m_FavoritesState.Grid.Items[index].Valid) { return; }

        SpectralShipGen::GeneratedShip ship;
        if (!generateShipFromRecipe(*favorite, ship))
        {
            setStatusMessage("Favorite image export failed: recipe could not be regenerated.");
            return;
        }

        const std::filesystem::path path = getAvailableSavePath(getSaveBaseName(*favorite));
        if (saveCoreImage(ship.FinalImage, path)) { setStatusMessage("Favorite image exported: " + path.string()); }
    }

    void ShipGeneratorPreviewApp::loadUserPresetLibraryState()
    {
        UserPresetLibraryLoadResult result = loadUserPresetLibrary(UserPresetLibraryPath);
        if (!result.Success)
        {
            std::cerr << "User preset library load failed: " << result.Error << '\n';
            return;
        }
        m_CustomPresetWorkspace = std::move(result.Workspace);
        if (result.SkippedEntryCount > 0u)
        {
            std::cerr << "User preset library skipped " << result.SkippedEntryCount << " invalid entr" << (result.SkippedEntryCount == 1u ? "y" : "ies") << ".\n";
        }
    }

    bool ShipGeneratorPreviewApp::saveUserPresetLibraryState()
    {
        std::string error;
        if (saveUserPresetLibrary(m_CustomPresetWorkspace, UserPresetLibraryPath, error)) { return true; }
        std::cerr << "User preset library save failed: " << error << '\n';
        return false;
    }

    void ShipGeneratorPreviewApp::loadFavoriteCollection()
    {
        const PreviewFavoritesLoadResult result = loadPreviewFavorites(PreviewFavoritesPath);
        if (!result.Success)
        {
            std::cerr << result.Error << '\n';
            setStatusMessage(result.Error);
            return;
        }

        m_Collections.setFavorites(result.Favorites);
        if (result.SkippedEntryCount > 0u || result.DuplicateEntryCount > 0u)
        {
            std::cerr << "Favorites loaded with " << result.SkippedEntryCount << " invalid entries skipped and " << result.DuplicateEntryCount << " duplicates ignored.\n";
        }
    }

    void ShipGeneratorPreviewApp::loadPreviewAppPreferences()
    {
        const PreviewPreferencesLoadResult result = loadPreviewPreferences(PreviewPreferencesPath);
        if (!result.Success)
        {
            std::cerr << result.Error << '\n';
            return;
        }

        m_Collections.setResolutionBookmarks(result.Preferences.ResolutionBookmarks);
    }

    void ShipGeneratorPreviewApp::next()
    {
        if (m_Collections.moveHistoryNext()) { m_SelectedStructuralPresetId.reset(); m_SelectedFactionPresetId.reset(); m_SelectedBuiltInPalettePreset.reset(); m_SelectedPalettePresetId.reset(); m_SelectedConfigurationBundleId.reset(); regenerate(); }
    }

    void ShipGeneratorPreviewApp::pinCurrentShip()
    {
        const sf::Image pinnedImage = SpectralShipGen::SFMLImageAdapter::createSFMLImage(m_GeneratedShip.FinalImage);

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
        if (m_Collections.moveHistoryPrevious()) { m_SelectedStructuralPresetId.reset(); m_SelectedFactionPresetId.reset(); m_SelectedBuiltInPalettePreset.reset(); m_SelectedPalettePresetId.reset(); m_SelectedConfigurationBundleId.reset(); regenerate(); }
    }

    void ShipGeneratorPreviewApp::removeCurrentFromFavorites()
    {
        const PreviewGenerationRecipe recipe = getCurrentRecipe();
        const std::optional<std::size_t> favoriteIndex = m_Collections.findFavoriteIndex(recipe);
        if (!favoriteIndex.has_value() || !m_Collections.removeFavorite(recipe)) { return; }
        removeFavoriteThumbnail(*favoriteIndex);
        refreshGalleryFavoriteMarkers();
        saveFavoriteCollection();
        updateWindowTitle();
        setStatusMessage("Favorite removed.");
        std::cout << "Removed Favorite. Remaining: " << m_Collections.getFavorites().size() << '\n';
    }

    void ShipGeneratorPreviewApp::removeSelectedFavorite()
    {
        if (m_FavoritesState.Grid.Items.empty()) { return; }
        const uint32_t index = std::min(m_FavoritesState.Grid.SelectedIndex, static_cast<uint32_t>(m_FavoritesState.Grid.Items.size() - 1u));
        const PreviewGenerationRecipe* favorite = m_Collections.getFavorite(index);
        if (favorite == nullptr) { return; }

        if (!m_PendingFavoriteRemovalIndex.has_value() || *m_PendingFavoriteRemovalIndex != index)
        {
            m_PendingFavoriteRemovalIndex = index;
            setStatusMessage("Remove Favorite confirmation: press Delete or Remove again.");
            return;
        }

        const PreviewGenerationRecipe recipe = *favorite;
        m_PendingFavoriteRemovalIndex.reset();
        if (!m_Collections.removeFavorite(recipe)) { return; }
        removeFavoriteThumbnail(index);
        refreshGalleryFavoriteMarkers();
        saveFavoriteCollection();
        updateWindowTitle();
        setStatusMessage("Selected Favorite removed. Current ship and History were not changed.");
        std::cout << "Removed selected Favorite. Remaining: " << m_Collections.getFavorites().size() << '\n';
    }

    void ShipGeneratorPreviewApp::removeFavoriteThumbnail(std::size_t index)
    {
        m_PendingFavoriteRemovalIndex.reset();
        if (index >= m_FavoritesState.Grid.Items.size()) { return; }
        m_FavoritesState.Grid.Items.erase(m_FavoritesState.Grid.Items.begin() + static_cast<std::ptrdiff_t>(index));
        if (m_FavoritesState.Grid.Items.empty())
        {
            m_FavoritesState.Grid.SelectedIndex = 0u;
            m_FavoritesState.Grid.HoveredIndex = -1;
            updateCommandPanelState();
            return;
        }
        m_FavoritesState.Grid.SelectedIndex = std::min(m_FavoritesState.Grid.SelectedIndex, static_cast<uint32_t>(m_FavoritesState.Grid.Items.size() - 1u));
        if (m_FavoritesState.Grid.HoveredIndex >= static_cast<int32_t>(m_FavoritesState.Grid.Items.size())) { m_FavoritesState.Grid.HoveredIndex = -1; }
        updateCommandPanelState();
    }

    void ShipGeneratorPreviewApp::removeResolutionBookmark()
    {
        const SpectralShipGen::ShipDimensions dimensions = getCurrentRecipe().Dimensions;
        if (!m_Collections.removeResolutionBookmark(dimensions)) { return; }
        savePreviewAppPreferences();
        setStatusMessage("Resolution bookmark removed: " + std::to_string(dimensions.Width) + "x" + std::to_string(dimensions.Height));
    }

    void ShipGeneratorPreviewApp::printControls() const
    {
        const auto printSection = [](const char* name, const PreviewHelpSection& section)
        {
            std::cout << name << ":\n";
            for (std::size_t index = 0u; index < section.Count; ++index)
            {
                std::cout << "  " << section.Entries[index].Shortcut << "    " << section.Entries[index].Description << '\n';
            }
        };

        printSection("Global controls", getPreviewGlobalHelpSection());
        std::cout << '\n';
        printSection((std::string(getPreviewWorkspaceName(m_WorkspaceSession.getActiveWorkspace())) + " workspace").c_str(), getPreviewWorkspaceHelpSection(m_WorkspaceSession.getActiveWorkspace()));
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

            if (event.type == sf::Event::Resized)
            {
                handleWindowResized(event.size);
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

            if (event.type == sf::Event::MouseWheelScrolled)
            {
                handleMouseWheelScrolled(event.mouseWheelScroll);
            }

            if (event.type == sf::Event::TextEntered)
            {
                handleTextEntered(event.text);
            }

            if (event.type == sf::Event::KeyPressed)
            {
                handleKeyPressed(event.key);
            }
        }
    }

    void ShipGeneratorPreviewApp::refreshGalleryFavoriteMarkers()
    {
        const std::vector<PreviewGalleryRecipeEntry>& galleryRecipes = m_Collections.getGalleryRecipes();
        const std::size_t count = std::min(m_GalleryState.Grid.Items.size(), galleryRecipes.size());
        for (std::size_t index = 0u; index < count; ++index)
        {
            m_GalleryState.Grid.Items[index].Favorite = galleryRecipes[index].Valid && galleryRecipes[index].Favorite;
        }
    }

    void ShipGeneratorPreviewApp::rebuildFavoriteThumbnails()
    {
        m_PendingFavoriteRemovalIndex.reset();
        m_FavoritesState.Grid.Items.clear();
        m_FavoritesState.Grid.Items.reserve(m_Collections.getFavorites().size());

        for (const PreviewGenerationRecipe& recipe : m_Collections.getFavorites())
        {
            PreviewThumbnailItem favorite;
            favorite.Recipe = recipe;

            SpectralShipGen::GeneratedShip favoriteShip;
            if (generateShipFromRecipe(recipe, favoriteShip))
            {
                const sf::Image favoriteImage = SpectralShipGen::SFMLImageAdapter::createSFMLImage(favoriteShip.FinalImage);
                if (favorite.Texture.loadFromImage(favoriteImage))
                {
                    favorite.Texture.setSmooth(false);
                    favorite.Valid = true;
                    favorite.Favorite = true;
                }
                else
                {
                    std::cerr << "Failed to rebuild Favorite thumbnail texture for seed " << recipe.Seeds.Master << ".\n";
                }
            }

            m_FavoritesState.Grid.Items.push_back(std::move(favorite));
        }

        m_FavoritesState.Grid.SelectedIndex = 0u;
        m_FavoritesState.Grid.HoveredIndex = -1;
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

        m_PreviewImage = SpectralShipGen::SFMLImageAdapter::createSFMLImage(m_GeneratedShip.FinalImage);

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

        const PreviewAnimationActionResult animationResult = m_AnimationSession.resetForGeneratedShip(m_GeneratedShip);
        if (!animationResult.Success)
        {
            if (!animationResult.StatusMessage.empty()) { setStatusMessage(animationResult.StatusMessage); }
            return false;
        }
        if (!animationResult.StatusMessage.empty()) { setStatusMessage(animationResult.StatusMessage); }
        m_GenerateIdleFrameIndex = 0u;
        m_GenerateIdlePlaybackAccumulatorMicroseconds = 0.0;
        m_AnimationClock.restart();
        if (!refreshAnimationTextures()) { return false; }

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
        const PreviewAnimationActionResult result = m_AnimationSession.regenerateSelectedAnimation(m_GeneratedShip);
        if (!result.Success)
        {
            if (!result.StatusMessage.empty()) { setStatusMessage(result.StatusMessage); }
            return false;
        }
        if (!result.StatusMessage.empty()) { setStatusMessage(result.StatusMessage); }
        m_AnimationClock.restart();
        return refreshAnimationTextures();
    }

    bool ShipGeneratorPreviewApp::refreshAnimationTextures()
    {
        const auto loadTextures = [](const std::vector<SpectralShipGen::Image>& frames, std::vector<sf::Texture>& textures)
        {
            textures.clear();
            textures.resize(frames.size());
            for (std::size_t index = 0u; index < frames.size(); ++index)
            {
                const sf::Image image = SpectralShipGen::SFMLImageAdapter::createSFMLImage(frames[index]);
                if (!textures[index].loadFromImage(image))
                {
                    textures.clear();
                    return false;
                }
                textures[index].setSmooth(false);
            }
            return true;
        };

        if (!loadTextures(m_AnimationSession.getIdleAnimation().Frames, m_GenerateIdleAnimationTextures))
        {
            std::cerr << "Failed to create IDLE animation texture.\n";
            return false;
        }
        if (m_GenerateIdleFrameIndex >= m_GenerateIdleAnimationTextures.size()) { m_GenerateIdleFrameIndex = 0u; }

        const std::vector<SpectralShipGen::Image>& frames = getActiveAnimationFrames();
        if (!loadTextures(frames, m_AnimationTextures))
        {
            std::cerr << "Failed to create animation texture.\n";
            return false;
        }
        if (frames.empty())
        {
            m_AnimationSession.setFrameIndex(0u);
            return true;
        }
        if (m_AnimationSession.getFrameIndex() >= m_AnimationTextures.size()) { m_AnimationSession.setFrameIndex(0u); }
        return true;
    }

    void ShipGeneratorPreviewApp::render()
    {
        m_Window.setView(m_LogicalView);
        updateCommandPanelState();
        PreviewRenderData data;
        data.Mode = m_PreviewMode;
        data.Workspace = m_WorkspaceSession.getActiveWorkspace();
        data.WorkspaceNavigation = &m_WorkspaceNavigation;
        data.PreviewSprite = &m_PreviewSprite;
        data.CurrentStaticTexture = &m_PreviewTexture;
        data.NativePreviewTexture = &m_PreviewTexture;
        const bool generateIdlePlayback = m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::GENERATE && m_PreviewMode == PreviewMode::ANIMATION;
        if (generateIdlePlayback && m_GenerateIdleFrameIndex < m_GenerateIdleAnimationTextures.size())
        {
            data.NativePreviewTexture = &m_GenerateIdleAnimationTextures[m_GenerateIdleFrameIndex];
        }
        else if ((m_PreviewMode == PreviewMode::ANIMATION || m_PreviewMode == PreviewMode::FRAME_INSPECTION) && m_AnimationSession.getFrameIndex() < m_AnimationTextures.size())
        {
            data.NativePreviewTexture = &m_AnimationTextures[m_AnimationSession.getFrameIndex()];
        }
        data.PinnedTexture = m_Comparison.Pinned.Valid ? &m_PinnedTexture : nullptr;
        data.Gallery = &m_GalleryState;
        data.Favorites = &m_FavoritesState;
        data.Recipe = &getCurrentRecipe();
        data.StructuralDisplayName = getCurrentStructuralProfileDisplayName();
        data.FactionDisplayName = getCurrentFactionProfileDisplayName();
        data.PaletteDisplayName = getCurrentPaletteDisplayName();
        data.ConfigurationBundleDisplayName = getCurrentConfigurationBundleDisplayName();
        data.Locks = &m_Locks;
        data.Diagnostics = &m_Diagnostics;
        data.Comparison = &m_Comparison;
        data.Ship = hasCurrentShip() ? &m_GeneratedShip : nullptr;
        data.GenerationDebugInfo = hasCurrentShip() ? &m_GenerationDebugInfo : nullptr;
        data.SelectedAnimationType = m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::GENERATE ? SpectralShipGen::ShipAnimationType::IDLE : m_AnimationSession.getSelectedAnimationType();
        data.IdleAnimation = &m_AnimationSession.getIdleAnimation();
        data.IdleAnimationSettings = &m_AnimationSession.getIdleSettings();
        data.MovementAnimation = &m_AnimationSession.getMovementAnimation();
        data.MovementAnimationSettings = &m_AnimationSession.getMovementSettings();
        data.MovementPhase = m_AnimationSession.getMovementPhase();
        data.RuntimeMovementType = m_AnimationSession.getRuntimeMovementType();
        data.PendingMovementType = m_AnimationSession.getPendingMovementType();
        data.MovementTransitionPending = m_AnimationSession.isMovementTransitionPending();
        data.TransientStatePreviewActive = m_AnimationSession.isTransientStatePreviewActive();
        data.FiringAnimation = &m_AnimationSession.getFiringAnimation();
        data.FiringAnimationSettings = &m_AnimationSession.getFiringSettings();
        data.HistoryIndex = m_Collections.getHistoryIndex();
        data.HistoryCount = m_Collections.getHistoryCount();
        data.AnimationFrameIndex = m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::GENERATE ? m_GenerateIdleFrameIndex : m_AnimationSession.getFrameIndex();
        data.AnimationNormalizedTime = m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::ANIMATION ? m_AnimationSession.getActiveNormalizedTime() : 0.0;
        data.AnimationPlaybackSpeed = m_AnimationSession.getPlaybackSpeed();
        data.AnimationLooping = m_AnimationSession.isActiveLooping();
        data.AnimationAnimatedComponentCount = m_AnimationSession.getActiveAnimatedComponentCount();
        data.AnimationSemanticPhase = m_AnimationSession.getSemanticPhaseDisplay();
        data.CommandPanel = m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR ? nullptr : &m_CommandPanel;
        data.ConfigurationEditor = m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR ? &m_ConfigurationEditor : nullptr;
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
            SpectralShipGen::clearGenerationDomainOverridesForChannel(recipe.DomainSeedOverrides, SpectralShipGen::GenerationSeedChannel::STRUCTURE);
        }

        if (!m_Locks.Palette)
        {
            recipe.Seeds.Palette = m_SeedGenerator();
            SpectralShipGen::clearGenerationDomainOverridesForChannel(recipe.DomainSeedOverrides, SpectralShipGen::GenerationSeedChannel::PALETTE);
        }

        if (!m_Locks.Details)
        {
            recipe.Seeds.Details = m_SeedGenerator();
            SpectralShipGen::clearGenerationDomainOverridesForChannel(recipe.DomainSeedOverrides, SpectralShipGen::GenerationSeedChannel::DETAILS);
        }

        if (!m_Locks.Attachments)
        {
            recipe.Seeds.Attachments = m_SeedGenerator();
            SpectralShipGen::clearGenerationDomainOverridesForChannel(recipe.DomainSeedOverrides, SpectralShipGen::GenerationSeedChannel::ATTACHMENTS);
        }

        appendHistoryEntry(recipe);
    }

    void ShipGeneratorPreviewApp::saveFavoriteCollection()
    {
        std::string error;
        if (!savePreviewFavorites(m_Collections.getFavorites(), PreviewFavoritesPath, error))
        {
            setStatusMessage(error);
            std::cerr << error << '\n';
        }
    }

    void ShipGeneratorPreviewApp::saveCurrent()
    {
        const PreviewGenerationRecipe& recipe = getCurrentRecipe();
        const std::string baseName = getSaveBaseName(recipe);

        if (m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::GENERATE && m_PreviewMode == PreviewMode::ANIMATION)
        {
            const std::vector<SpectralShipGen::Image>& frames = m_AnimationSession.getIdleAnimation().Frames;
            if (m_GenerateIdleFrameIndex >= frames.size()) { return; }
            const std::filesystem::path savePath = getAvailableSavePath(baseName + "_idle_frame_" + getFrameNumberString(m_GenerateIdleFrameIndex));
            saveCoreImage(frames[m_GenerateIdleFrameIndex], savePath);
            return;
        }

        if (m_PreviewMode == PreviewMode::ANIMATION || m_PreviewMode == PreviewMode::FRAME_INSPECTION)
        {
            const std::vector<SpectralShipGen::Image>& frames = getActiveAnimationFrames();
            if (m_AnimationSession.getFrameIndex() >= frames.size())
            {
                return;
            }

            std::string animationName = baseName + "_" + getAnimationTypeFileToken(m_AnimationSession.getSelectedAnimationType());
            if (m_AnimationSession.getSelectedAnimationType() == SpectralShipGen::ShipAnimationType::FIRE)
            {
                animationName += "_weapon" + std::to_string(m_AnimationSession.getFiringAnimation().Target.WeaponComponentIndex);
            }
            else if (m_AnimationSession.getSelectedAnimationType() != SpectralShipGen::ShipAnimationType::IDLE)
            {
                animationName += "_" + getMovementPhaseFileToken(m_AnimationSession.getMovementPhase());
            }
            const std::filesystem::path savePath = getAvailableSavePath(animationName + "_frame_" + getFrameNumberString(m_AnimationSession.getFrameIndex()));
            saveCoreImage(frames[m_AnimationSession.getFrameIndex()], savePath);
            return;
        }

        saveCoreImage(m_GeneratedShip.FinalImage, getAvailableSavePath(baseName));
    }

    void ShipGeneratorPreviewApp::savePreviewAppPreferences()
    {
        PreviewPreferences preferences;
        preferences.ResolutionBookmarks = m_Collections.getResolutionBookmarks();
        std::string error;
        if (!savePreviewPreferences(preferences, PreviewPreferencesPath, error))
        {
            setStatusMessage(error);
            std::cerr << error << '\n';
        }
    }

    void ShipGeneratorPreviewApp::saveSpritesheet()
    {
        const PreviewGenerationRecipe& recipe = getCurrentRecipe();
        const std::string baseName = getSaveBaseName(recipe);

        if (m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::GENERATE)
        {
            if (m_AnimationSession.getIdleAnimation().Frames.empty()) { return; }
            const SpectralShipGen::Image spritesheet = SpectralShipGen::createHorizontalSpritesheet(m_AnimationSession.getIdleAnimation());
            saveCoreImage(spritesheet, getAvailableSavePath(baseName + "_idle_" + std::to_string(m_AnimationSession.getIdleAnimation().Frames.size()) + "frames"));
            return;
        }

        if (m_AnimationSession.isTransientStatePreviewActive())
        {
            setStatusMessage("Composed transition/event previews are runtime-only. Export the base movement/FIRE assets separately.");
            return;
        }

        if (m_AnimationSession.getSelectedAnimationType() == SpectralShipGen::ShipAnimationType::IDLE)
        {
            if (m_AnimationSession.getIdleAnimation().Frames.empty())
            {
                return;
            }

            const SpectralShipGen::Image spritesheet = SpectralShipGen::createHorizontalSpritesheet(m_AnimationSession.getIdleAnimation());
            saveCoreImage(spritesheet, getAvailableSavePath(baseName + "_idle_" + std::to_string(m_AnimationSession.getIdleAnimation().Frames.size()) + "frames"));
            return;
        }

        if (m_AnimationSession.getSelectedAnimationType() == SpectralShipGen::ShipAnimationType::FIRE)
        {
            if (m_AnimationSession.getFiringAnimation().Frames.empty()) { return; }
            const SpectralShipGen::Image spritesheet = SpectralShipGen::createHorizontalSpritesheet(m_AnimationSession.getFiringAnimation());
            const std::string fileName = baseName + "_fire_weapon" + std::to_string(m_AnimationSession.getFiringAnimation().Target.WeaponComponentIndex) + "_" + std::to_string(m_AnimationSession.getFiringAnimation().DurationMilliseconds) + "ms_" + std::to_string(m_AnimationSession.getFiringAnimation().Frames.size()) + "frames";
            saveCoreImage(spritesheet, getAvailableSavePath(fileName));
            return;
        }

        if (m_AnimationSession.getMovementAnimation().Type != m_AnimationSession.getSelectedAnimationType())
        {
            return;
        }

        const std::array<const SpectralShipGen::ShipMovementAnimationClip*, 3u> clips =
        {
            &m_AnimationSession.getMovementAnimation().Enter,
            &m_AnimationSession.getMovementAnimation().Sustain,
            &m_AnimationSession.getMovementAnimation().Exit
        };

        for (const SpectralShipGen::ShipMovementAnimationClip* clip : clips)
        {
            if (clip == nullptr || clip->Frames.empty())
            {
                continue;
            }

            const SpectralShipGen::Image spritesheet = SpectralShipGen::createHorizontalSpritesheet(*clip);
            const std::string fileName = baseName + "_" + getAnimationTypeFileToken(m_AnimationSession.getSelectedAnimationType()) + "_" + getMovementPhaseFileToken(clip->Phase) + "_" + std::to_string(clip->Frames.size()) + "frames";
            saveCoreImage(spritesheet, getAvailableSavePath(fileName));
        }
    }

    void ShipGeneratorPreviewApp::toggleGalleryCandidateFavorite(uint32_t index)
    {
        const PreviewGenerationRecipe* candidate = m_Collections.getGalleryRecipe(index);
        if (candidate == nullptr || index >= m_GalleryState.Grid.Items.size() || !m_GalleryState.Grid.Items[index].Valid) { return; }
        const PreviewGenerationRecipe recipe = *candidate;
        const std::optional<std::size_t> previousFavoriteIndex = m_Collections.findFavoriteIndex(recipe);
        const std::optional<bool> favorite = m_Collections.toggleGalleryFavorite(index);
        if (!favorite.has_value()) { return; }

        if (*favorite)
        {
            addFavoriteThumbnail(recipe, m_GalleryState.Grid.Items[index].Texture);
            setStatusMessage("Gallery candidate bookmarked.");
        }
        else
        {
            if (previousFavoriteIndex.has_value()) { removeFavoriteThumbnail(*previousFavoriteIndex); }
            setStatusMessage("Gallery candidate removed from Favorites.");
        }

        refreshGalleryFavoriteMarkers();
        saveFavoriteCollection();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::selectGalleryCandidate(uint32_t index)
    {
        const PreviewGenerationRecipe* candidate = m_Collections.getGalleryRecipe(index);
        if (candidate == nullptr || index >= m_GalleryState.Grid.Items.size() || !m_GalleryState.Grid.Items[index].Valid) { return; }

        m_PreviewMode = PreviewMode::STATIC;
        appendHistoryEntry(*candidate);
        m_GalleryState.Grid.Items.clear();
        m_Collections.clearGallery();
    }

    void ShipGeneratorPreviewApp::selectResolutionBookmark(uint32_t index)
    {
        if (index >= m_Collections.getResolutionBookmarks().size()) { return; }
        setDimensions(m_Collections.getResolutionBookmarks()[index]);
    }

    const std::vector<SpectralShipGen::Image>& ShipGeneratorPreviewApp::getActiveAnimationFrames() const
    {
        return m_AnimationSession.getActiveFrames();
    }

    uint64_t ShipGeneratorPreviewApp::getActiveAnimationSeed() const
    {
        return m_AnimationSession.getActiveSeed();
    }

    std::string ShipGeneratorPreviewApp::getAnimationEffectDisplay() const
    {
        return m_AnimationSession.getEffectDisplay();
    }

    void ShipGeneratorPreviewApp::setDisplayedAnimationFrame(uint32_t frameIndex)
    {
        if (frameIndex >= m_AnimationTextures.size()) { return; }
        m_AnimationSession.setFrameIndex(frameIndex);
        refreshDisplayedTexture();
    }

    void ShipGeneratorPreviewApp::setDisplayedStaticFrame()
    {
        refreshDisplayedTexture();
    }

    void ShipGeneratorPreviewApp::setFaction(SpectralShipGen::ShipFactionType faction)
    {
        PreviewGenerationRecipe& recipe = getCurrentRecipe();

        if (recipe.FactionPreset == faction)
        {
            return;
        }

        m_SelectedFactionPresetId.reset();
        m_SelectedConfigurationBundleId.reset();
        recipe.FactionPreset = faction;
        regenerate();
    }

    void ShipGeneratorPreviewApp::setDimensions(const SpectralShipGen::ShipDimensions& dimensions)
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
        const SpectralShipGen::ShipDimensions current = getCurrentRecipe().Dimensions;
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
        const SpectralShipGen::ShipDimensions current = getCurrentRecipe().Dimensions;
        width = std::clamp(width, getMinimumPreviewWidthForHeight(current.Height), getMaximumPreviewWidthForHeight(current.Height));
        setDimensions({ width, current.Height });
    }

    void ShipGeneratorPreviewApp::setStyle(SpectralShipGen::ShipStyle style)
    {
        PreviewGenerationRecipe& recipe = getCurrentRecipe();

        if (recipe.StructuralPreset == style)
        {
            return;
        }

        m_SelectedStructuralPresetId.reset();
        m_SelectedConfigurationBundleId.reset();
        recipe.StructuralPreset = style;
        regenerate();
    }

    void ShipGeneratorPreviewApp::selectRuntimeStructuralPreset(RuntimeCustomPresetId id)
    {
        const RuntimeStructuralPreset* preset = m_CustomPresetWorkspace.findStructural(id);
        if (preset == nullptr) { return; }
        PreviewGenerationRecipe& recipe = getCurrentRecipe();
        recipe.StructuralPreset.reset();
        recipe.StructuralProfile = preset->Profile;
        m_SelectedStructuralPresetId = id;
        m_SelectedConfigurationBundleId.reset();
        regenerate();
        setStatusMessage("Selected runtime structural profile: " + preset->Name);
    }

    void ShipGeneratorPreviewApp::selectStructuralProfileEntry(const StructuralProfileSelectionEntry& entry)
    {
        switch (entry.Kind)
        {
        case StructuralProfileSelectionKind::BUILT_IN: if (entry.Style.has_value()) { setStyle(*entry.Style); } break;
        case StructuralProfileSelectionKind::RUNTIME_CUSTOM: selectRuntimeStructuralPreset(entry.CustomPresetId); break;
        case StructuralProfileSelectionKind::ADD_PROFILE: enterConfigurationEditorDefault(); break;
        default: break;
        }
    }

    std::string ShipGeneratorPreviewApp::getCurrentStructuralProfileDisplayName() const
    {
        if (m_SelectedConfigurationBundleId.has_value())
        {
            const RuntimeConfigurationBundle* bundle = m_CustomPresetWorkspace.findConfigurationBundle(*m_SelectedConfigurationBundleId);
            if (bundle != nullptr && !bundle->Bundle.StructuralDisplayName.empty()) { return bundle->Bundle.StructuralDisplayName; }
        }
        const PreviewGenerationRecipe& recipe = getCurrentRecipe();
        if (recipe.StructuralPreset.has_value()) { return getStyleName(*recipe.StructuralPreset); }
        if (m_SelectedStructuralPresetId.has_value())
        {
            const RuntimeStructuralPreset* preset = m_CustomPresetWorkspace.findStructural(*m_SelectedStructuralPresetId);
            if (preset != nullptr) { return preset->Name; }
        }
        return "CUSTOM";
    }

    void ShipGeneratorPreviewApp::selectRuntimeFactionPreset(RuntimeCustomPresetId id)
    {
        const RuntimeFactionPreset* preset = m_CustomPresetWorkspace.findFaction(id);
        if (preset == nullptr) { return; }
        PreviewGenerationRecipe& recipe = getCurrentRecipe();
        recipe.FactionPreset.reset();
        recipe.FactionProfile = preset->Profile;
        m_SelectedFactionPresetId = id;
        m_SelectedConfigurationBundleId.reset();
        regenerate();
        setStatusMessage("Selected runtime faction profile: " + preset->Name);
    }

    void ShipGeneratorPreviewApp::selectFactionProfileEntry(const FactionProfileSelectionEntry& entry)
    {
        switch (entry.Kind)
        {
        case FactionProfileSelectionKind::BUILT_IN: if (entry.Faction.has_value()) { setFaction(*entry.Faction); } break;
        case FactionProfileSelectionKind::RUNTIME_CUSTOM: selectRuntimeFactionPreset(entry.CustomPresetId); break;
        case FactionProfileSelectionKind::ADD_FACTION: enterFactionConfigurationEditorDefault(); break;
        default: break;
        }
    }

    std::string ShipGeneratorPreviewApp::getCurrentFactionProfileDisplayName() const
    {
        if (m_SelectedConfigurationBundleId.has_value())
        {
            const RuntimeConfigurationBundle* bundle = m_CustomPresetWorkspace.findConfigurationBundle(*m_SelectedConfigurationBundleId);
            if (bundle != nullptr && !bundle->Bundle.FactionDisplayName.empty()) { return bundle->Bundle.FactionDisplayName; }
        }
        const PreviewGenerationRecipe& recipe = getCurrentRecipe();
        if (recipe.FactionPreset.has_value()) { return getFactionDisplayName(*recipe.FactionPreset); }
        if (m_SelectedFactionPresetId.has_value())
        {
            const RuntimeFactionPreset* preset = m_CustomPresetWorkspace.findFaction(*m_SelectedFactionPresetId);
            if (preset != nullptr) { return preset->Name; }
        }
        return "CUSTOM";
    }

    void ShipGeneratorPreviewApp::selectRuntimePalettePreset(RuntimeCustomPresetId id)
    {
        const RuntimePalettePreset* preset = m_CustomPresetWorkspace.findPalette(id);
        if (preset == nullptr) { return; }
        PreviewGenerationRecipe& recipe = getCurrentRecipe();
        recipe.PaletteConfiguration = preset->Configuration;
        m_SelectedBuiltInPalettePreset.reset();
        m_SelectedPalettePresetId = id;
        m_SelectedConfigurationBundleId.reset();
        regenerate();
        setStatusMessage("Selected runtime palette: " + preset->Name);
    }

    void ShipGeneratorPreviewApp::selectPaletteProfileEntry(const PaletteProfileSelectionEntry& entry)
    {
        PreviewGenerationRecipe& recipe = getCurrentRecipe();
        switch (entry.Kind)
        {
        case PaletteProfileSelectionKind::FACTION_DEFAULT:
            recipe.PaletteConfiguration = {};
            m_SelectedBuiltInPalettePreset.reset();
            m_SelectedPalettePresetId.reset();
            m_SelectedConfigurationBundleId.reset();
            regenerate();
            setStatusMessage("Selected faction-default palette language.");
            break;
        case PaletteProfileSelectionKind::BUILT_IN_GENERATED:
            recipe.PaletteConfiguration.Mode = SpectralShipGen::ShipPaletteSourceMode::EXPLICIT_GENERATED;
            recipe.PaletteConfiguration.Generated = SpectralShipGen::getBuiltInPalettePresetProfile(*entry.PalettePreset);
            m_SelectedBuiltInPalettePreset = *entry.PalettePreset;
            m_SelectedPalettePresetId.reset();
            m_SelectedConfigurationBundleId.reset();
            regenerate();
            setStatusMessage("Selected built-in palette language: " + entry.Label);
            break;
        case PaletteProfileSelectionKind::RUNTIME_CUSTOM:
            selectRuntimePalettePreset(entry.CustomPresetId);
            break;
        case PaletteProfileSelectionKind::ADD_PALETTE:
            enterPaletteConfigurationEditorDefault();
            break;
        default:
            break;
        }
    }

    std::string ShipGeneratorPreviewApp::getCurrentPaletteDisplayName() const
    {
        if (m_SelectedConfigurationBundleId.has_value())
        {
            const RuntimeConfigurationBundle* bundle = m_CustomPresetWorkspace.findConfigurationBundle(*m_SelectedConfigurationBundleId);
            if (bundle != nullptr && !bundle->Bundle.PaletteDisplayName.empty()) { return bundle->Bundle.PaletteDisplayName; }
        }
        const PreviewGenerationRecipe& recipe = getCurrentRecipe();
        if (recipe.PaletteConfiguration.Mode == SpectralShipGen::ShipPaletteSourceMode::FACTION_PROFILE_GENERATED) { return "FACTION DEFAULT"; }
        if (m_SelectedPalettePresetId.has_value())
        {
            const RuntimePalettePreset* preset = m_CustomPresetWorkspace.findPalette(*m_SelectedPalettePresetId);
            if (preset != nullptr) { return preset->Name; }
        }
        if (m_SelectedBuiltInPalettePreset.has_value()) { return std::string(SpectralShipGen::getBuiltInPalettePresetId(*m_SelectedBuiltInPalettePreset)); }
        return recipe.PaletteConfiguration.Mode == SpectralShipGen::ShipPaletteSourceMode::FIXED ? "CUSTOM FIXED" : "CUSTOM GENERATED";
    }

    std::string ShipGeneratorPreviewApp::getCurrentConfigurationBundleDisplayName() const
    {
        if (m_SelectedConfigurationBundleId.has_value())
        {
            const RuntimeConfigurationBundle* preset = m_CustomPresetWorkspace.findConfigurationBundle(*m_SelectedConfigurationBundleId);
            if (preset != nullptr) { return preset->Name; }
        }
        return "INDIVIDUAL COMPONENTS";
    }

    std::string ShipGeneratorPreviewApp::getProfilesItemDisplayName() const
    {
        switch (m_ProfilesSection)
        {
        case ProfilesSection::STRUCTURAL: return getCurrentStructuralProfileDisplayName();
        case ProfilesSection::FACTION: return getCurrentFactionProfileDisplayName();
        case ProfilesSection::PALETTE: return getCurrentPaletteDisplayName();
        case ProfilesSection::FULL_CONFIGURATION:
            if (m_ProfilesSelectedBundleId.has_value())
            {
                const RuntimeConfigurationBundle* preset = m_CustomPresetWorkspace.findConfigurationBundle(*m_ProfilesSelectedBundleId);
                if (preset != nullptr) { return preset->Name; }
            }
            return "NO SAVED BUNDLES";
        default: return "";
        }
    }

    void ShipGeneratorPreviewApp::applyRuntimeConfigurationBundle(RuntimeCustomPresetId id)
    {
        const RuntimeConfigurationBundle* preset = m_CustomPresetWorkspace.findConfigurationBundle(id);
        if (preset == nullptr) { return; }
        applyConfigurationBundle(preset->Bundle, getCurrentRecipe());
        m_SelectedStructuralPresetId.reset();
        m_SelectedFactionPresetId.reset();
        m_SelectedBuiltInPalettePreset.reset();
        m_SelectedPalettePresetId.reset();
        m_SelectedConfigurationBundleId = id;
        regenerate();
        setStatusMessage("Applied Full Configuration: " + preset->Name);
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
        return m_Collections.isFavorite(getCurrentRecipe());
    }

    bool ShipGeneratorPreviewApp::isDiagnosticImageViewActive() const
    {
        return hasCurrentShip() && m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::INSPECT && (m_Diagnostics.GenerationStageView || m_Diagnostics.ViewMode != DiagnosticViewMode::FINAL);
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

        const SpectralShipGen::Image diagnosticImage = createDiagnosticImage();
        const sf::Image image = SpectralShipGen::SFMLImageAdapter::createSFMLImage(diagnosticImage);

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

        if (m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::GENERATE && m_PreviewMode == PreviewMode::ANIMATION && m_GenerateIdleFrameIndex < m_GenerateIdleAnimationTextures.size())
        {
            m_PreviewSprite.setTexture(m_GenerateIdleAnimationTextures[m_GenerateIdleFrameIndex], true);
            return;
        }

        if ((m_PreviewMode == PreviewMode::ANIMATION || m_PreviewMode == PreviewMode::FRAME_INSPECTION) && m_AnimationSession.getFrameIndex() < m_AnimationTextures.size())
        {
            m_PreviewSprite.setTexture(m_AnimationTextures[m_AnimationSession.getFrameIndex()], true);
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

    void ShipGeneratorPreviewApp::toggleInspectionPresentation()
    {
        if (m_WorkspaceSession.getActiveWorkspace() != PreviewWorkspace::INSPECT || !hasCurrentShip()) { return; }
        m_Diagnostics.InspectionPresentation = m_Diagnostics.InspectionPresentation == PreviewInspectionPresentation::OVERLAY ? PreviewInspectionPresentation::ISOLATE : PreviewInspectionPresentation::OVERLAY;
        refreshDiagnosticTexture();
        refreshDisplayedTexture();
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
        if (m_WorkspaceSession.getActiveWorkspace() != PreviewWorkspace::INSPECT) { return; }

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
        if (m_WorkspaceSession.getActiveWorkspace() != PreviewWorkspace::INSPECT) { return; }

        const bool newValue = !m_Diagnostics.PaletteInspectorVisible;
        m_Diagnostics.HelpVisible = false;
        m_Diagnostics.GenerationInspectorVisible = false;
        m_Diagnostics.PaletteInspectorVisible = newValue;
    }

    void ShipGeneratorPreviewApp::update()
    {
        if (m_PreviewMode != PreviewMode::ANIMATION) { return; }

        const double elapsedMicroseconds = std::max(0.0, static_cast<double>(m_AnimationClock.restart().asMicroseconds()));
        if (m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::GENERATE)
        {
            const SpectralShipGen::ShipIdleAnimation& idleAnimation = m_AnimationSession.getIdleAnimation();
            if (idleAnimation.Frames.empty() || m_GenerateIdleAnimationTextures.empty()) { return; }
            const double frameDurationMicroseconds = std::max(1.0, idleAnimation.FrameDurationMilliseconds * 1000.0);
            m_GenerateIdlePlaybackAccumulatorMicroseconds += elapsedMicroseconds;
            if (m_GenerateIdlePlaybackAccumulatorMicroseconds < frameDurationMicroseconds) { return; }
            const uint32_t elapsedFrames = std::max(1u, static_cast<uint32_t>(m_GenerateIdlePlaybackAccumulatorMicroseconds / frameDurationMicroseconds));
            m_GenerateIdlePlaybackAccumulatorMicroseconds -= static_cast<double>(elapsedFrames) * frameDurationMicroseconds;
            m_GenerateIdleFrameIndex = (m_GenerateIdleFrameIndex + elapsedFrames) % static_cast<uint32_t>(idleAnimation.Frames.size());
            refreshDisplayedTexture();
            updateWindowTitle();
            return;
        }

        if (getActiveAnimationFrames().empty()) { return; }
        const PreviewAnimationAdvanceResult result = m_AnimationSession.advancePlayback(m_GeneratedShip, elapsedMicroseconds);
        if (result.ActiveFramesChanged && !refreshAnimationTextures()) { return; }

        if (result.ReturnToStatic)
        {
            m_PreviewMode = PreviewMode::FRAME_INSPECTION;
            if (getActiveAnimationFrames().empty()) { setDisplayedStaticFrame(); }
            else { setDisplayedAnimationFrame(m_AnimationSession.getFrameIndex()); }
        }
        else if (result.FrameChanged)
        {
            setDisplayedAnimationFrame(m_AnimationSession.getFrameIndex());
        }

        if (result.ActiveFramesChanged || result.FrameChanged || result.ReturnToStatic)
        {
            updateCommandPanelState();
            updateWindowTitle();
        }
    }

    void ShipGeneratorPreviewApp::updateLogicalWindowView(uint32_t physicalWidth, uint32_t physicalHeight)
    {
        m_LogicalViewport = calculatePreviewLogicalViewport(
            physicalWidth,
            physicalHeight,
            PreviewWindowWidth,
            PreviewWindowHeight);
        m_LogicalView.setViewport(sf::FloatRect(
            m_LogicalViewport.Left,
            m_LogicalViewport.Top,
            m_LogicalViewport.Width,
            m_LogicalViewport.Height));
        m_Window.setView(m_LogicalView);
    }

    void ShipGeneratorPreviewApp::updateCommandPanelState()
    {
        m_CommandPanel.updateState(createCommandPanelState());
    }

    void ShipGeneratorPreviewApp::setStatusMessage(const std::string& message)
    {
        m_StatusMessage = message;
    }

    void ShipGeneratorPreviewApp::switchWorkspace(PreviewWorkspace workspace)
    {
        if (workspace == m_WorkspaceSession.getActiveWorkspace()) { return; }
        if (workspace != PreviewWorkspace::FAVORITES) { m_PendingFavoriteRemovalIndex.reset(); }

        PreviewMode targetMode = m_WorkspaceSession.switchTo(workspace, m_PreviewMode);
        m_WorkspaceNavigation.setActiveWorkspace(workspace);

        if (workspace == PreviewWorkspace::PROFILES && targetMode == PreviewMode::CONFIGURATION_EDITOR && !m_ConfigurationEditor.isOpen())
        {
            targetMode = PreviewMode::STATIC;
        }
        if (workspace == PreviewWorkspace::REROLL)
        {
            if (!m_RerollStudio.Active || !(m_RerollStudio.BaseRecipe == getCurrentRecipe()))
            {
                beginAttributeRerollStudio(m_RerollStudio, getCurrentRecipe());
            }
            targetMode = PreviewMode::REROLL_STUDIO;
        }
        else if (workspace == PreviewWorkspace::FAVORITES)
        {
            targetMode = PreviewMode::FAVORITES;
            if (!m_FavoritesState.Grid.Items.empty())
            {
                m_FavoritesState.Grid.SelectedIndex = std::min(m_FavoritesState.Grid.SelectedIndex, static_cast<uint32_t>(m_FavoritesState.Grid.Items.size() - 1u));
            }
            m_FavoritesState.Grid.HoveredIndex = -1;
        }
        else if (workspace == PreviewWorkspace::ANIMATION)
        {
            if (targetMode != PreviewMode::ANIMATION && targetMode != PreviewMode::FRAME_INSPECTION) { targetMode = PreviewMode::FRAME_INSPECTION; }
            if (!hasCurrentShip()) { setStatusMessage("No current ship to animate. Return to Generate first."); }
        }
        else if (!isPreviewModeOwnedByWorkspace(workspace, targetMode))
        {
            targetMode = getDefaultPreviewMode(workspace);
        }

        m_PreviewMode = targetMode;
        if (m_PreviewMode == PreviewMode::ANIMATION) { m_AnimationClock.restart(); }
        if (workspace == PreviewWorkspace::INSPECT) { refreshDiagnosticTexture(); }
        refreshDisplayedTexture();
        updateCommandPanelState();
        updateWindowTitle();
    }

    void ShipGeneratorPreviewApp::updateWindowTitle()
    {
        if (m_PreviewMode == PreviewMode::CONFIGURATION_EDITOR)
        {
            const ConfigurationEditorProfileKind kind = m_ConfigurationEditor.getProfileKind();
            const std::size_t runtimeCount = kind == ConfigurationEditorProfileKind::FACTION ? m_CustomPresetWorkspace.getFactionPresets().size() : kind == ConfigurationEditorProfileKind::PALETTE ? m_CustomPresetWorkspace.getPalettePresets().size() : m_CustomPresetWorkspace.getStructuralPresets().size();
            const char* editorName = kind == ConfigurationEditorProfileKind::FACTION ? "Faction Editor" : kind == ConfigurationEditorProfileKind::PALETTE ? "Palette Editor" : "Structural Editor";
            m_Window.setTitle("SpectralShipGen Studio | " + std::string(editorName) + " | " + m_ConfigurationEditor.getName() + " | User presets: " + std::to_string(runtimeCount));
            return;
        }

        if (m_PreviewMode == PreviewMode::GALLERY)
        {
            const std::string title = "SpectralShipGen Studio | Gallery | BatchSeed: " + std::to_string(m_GalleryState.BatchSeed) + " | " + std::to_string(m_GalleryState.CandidateCount) + " candidates | Structure: " + getRecipeStructuralDisplayName(m_GalleryState.TemplateRecipe) + " | Faction: " + getRecipeFactionDisplayName(m_GalleryState.TemplateRecipe) + " | " + std::to_string(m_GalleryState.TemplateRecipe.Dimensions.Width) + "x" + std::to_string(m_GalleryState.TemplateRecipe.Dimensions.Height) + " | Selected " + std::to_string(m_GalleryState.Grid.SelectedIndex + 1u) + "/" + std::to_string(m_GalleryState.Grid.Items.size());
            m_Window.setTitle(title);
            return;
        }

        if (m_PreviewMode == PreviewMode::FAVORITES)
        {
            const std::string selected = m_FavoritesState.Grid.Items.empty() ? "0/0" : std::to_string(m_FavoritesState.Grid.SelectedIndex + 1u) + "/" + std::to_string(m_FavoritesState.Grid.Items.size());
            m_Window.setTitle("SpectralShipGen Studio | Favorites | Selected " + selected + " | Current seed: " + std::to_string(getCurrentRecipe().Seeds.Master));
            return;
        }

        if (m_PreviewMode == PreviewMode::REROLL_STUDIO)
        {
            const std::size_t selectedCount = static_cast<std::size_t>(std::count(m_RerollStudio.SelectedDomains.begin(), m_RerollStudio.SelectedDomains.end(), true));
            const std::string candidate = m_RerollStudio.CandidateValid ? ("Candidate " + std::to_string(m_RerollStudio.CandidateSequence)) : "No Candidate";
            m_Window.setTitle("SpectralShipGen Studio | Attribute Reroll Studio | Selected " + std::to_string(selectedCount) + " domains | " + candidate + " | Base seed: " + std::to_string(m_RerollStudio.BaseRecipe.Seeds.Master));
            return;
        }

        const PreviewGenerationRecipe& recipe = getCurrentRecipe();
        const std::string attachmentState = recipe.AttachmentsEnabled ? "ON" : "OFF";
        std::string title = "SpectralShipGen Studio | " + std::string(getPreviewWorkspaceName(m_WorkspaceSession.getActiveWorkspace())) + " | Seed: " + std::to_string(recipe.Seeds.Master) + " | " + std::to_string(recipe.Dimensions.Width) + "x" + std::to_string(recipe.Dimensions.Height) + " | Profile: " + getCurrentStructuralProfileDisplayName() + " | Faction: " + getCurrentFactionProfileDisplayName() + " | Palette: " + getCurrentPaletteDisplayName() + " | Attachments: " + attachmentState + " | Favorite: " + (isCurrentFavorite() ? "YES" : "NO") + " | Favorites: " + std::to_string(m_Collections.getFavorites().size()) + " | S:" + getLockDisplay(m_Locks.Structure) + " P:" + getLockDisplay(m_Locks.Palette) + " D:" + getLockDisplay(m_Locks.Details) + " A:" + getLockDisplay(m_Locks.Attachments) + " | History " + std::to_string(m_Collections.getHistoryIndex() + 1u) + "/" + std::to_string(m_Collections.getHistoryCount());

        if (m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::INSPECT && m_Comparison.ViewEnabled && m_Comparison.Pinned.Valid)
        {
            title += " | Compare: PIN " + std::to_string(m_Comparison.Pinned.Recipe.Seeds.Master);
        }

        const bool generateIdlePlayback = m_WorkspaceSession.getActiveWorkspace() == PreviewWorkspace::GENERATE && m_PreviewMode == PreviewMode::ANIMATION;
        if (generateIdlePlayback && !m_AnimationSession.getIdleAnimation().Frames.empty())
        {
            title += " | Anim: PLAY IDLE | Frame " + std::to_string(m_GenerateIdleFrameIndex + 1u) + "/" + std::to_string(m_AnimationSession.getIdleAnimation().Frames.size());
            title += " | AnimationSeed: " + std::to_string(m_AnimationSession.getIdleAnimation().Seed);
        }

        const std::vector<SpectralShipGen::Image>& animationFrames = getActiveAnimationFrames();
        if (!generateIdlePlayback && (m_PreviewMode == PreviewMode::ANIMATION || m_PreviewMode == PreviewMode::FRAME_INSPECTION) && !animationFrames.empty())
        {
            title += m_PreviewMode == PreviewMode::ANIMATION ? " | Anim: PLAY " : " | Anim: FRAME ";
            title += getAnimationTypeDisplayName(m_AnimationSession.getSelectedAnimationType());
            if (m_AnimationSession.getSelectedAnimationType() == SpectralShipGen::ShipAnimationType::FIRE)
            {
                title += " | Weapon: " + std::to_string(m_AnimationSession.getFiringAnimation().Target.WeaponComponentIndex);
                if (m_AnimationSession.isTransientStatePreviewActive())
                {
                    title += " | Underlying: " + getAnimationTypeDisplayName(m_AnimationSession.getRuntimeMovementType());
                }
            }
            else if (m_AnimationSession.getSelectedAnimationType() != SpectralShipGen::ShipAnimationType::IDLE)
            {
                title += " | Phase: " + getMovementPhaseDisplayName(m_AnimationSession.getMovementPhase());
                if (m_AnimationSession.isMovementTransitionPending())
                {
                    title += " | Next: " + getAnimationTypeDisplayName(m_AnimationSession.getPendingMovementType());
                }
            }
            title += " | Frame " + std::to_string(m_AnimationSession.getFrameIndex() + 1u) + "/" + std::to_string(animationFrames.size());
            title += " | t=" + std::to_string(m_AnimationSession.getActiveNormalizedTime());
            title += " | " + m_AnimationSession.getSemanticPhaseDisplay();
            title += " | Speed " + getPlaybackSpeedDisplay(m_AnimationSession.getPlaybackSpeed());
            title += " | AnimationSeed: " + std::to_string(getActiveAnimationSeed());
            title += " | FX: " + getAnimationEffectDisplay();
        }

        m_Window.setTitle(title);
    }

    PreviewGenerationRecipe& ShipGeneratorPreviewApp::getCurrentRecipe()
    {
        return m_Collections.getCurrentRecipe();
    }

    const PreviewGenerationRecipe& ShipGeneratorPreviewApp::getCurrentRecipe() const
    {
        return m_Collections.getCurrentRecipe();
    }
}
