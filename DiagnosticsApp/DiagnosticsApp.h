#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "DiagnosticsAppController.h"
#include "ShipFactionType.h"
#include "ShipGenerationProfile.h"

namespace PixelShipGeneratorDiagnosticsApp
{
    struct DiagnosticsAppLaunchOptions
    {
        bool AutomatedSmoke = false;
        bool CancelSmoke = false;
        std::filesystem::path ScreenshotPath;
        std::filesystem::path SmokeCsvPath;
    };

    class DiagnosticsApp
    {
    public:
        explicit DiagnosticsApp(DiagnosticsAppLaunchOptions options = {});
        ~DiagnosticsApp();

        int run();

    private:
        enum class UiActionType : uint32_t
        {
            TOGGLE_DIMENSION = 0u,
            DIMENSIONS_SELECT_ALL,
            DIMENSIONS_CLEAR_ALL,
            CUSTOM_WIDTH_DECREASE,
            CUSTOM_WIDTH_INCREASE,
            CUSTOM_HEIGHT_DECREASE,
            CUSTOM_HEIGHT_INCREASE,
            ADD_CUSTOM_DIMENSION,
            TOGGLE_STYLE,
            STYLES_SELECT_ALL,
            STYLES_CLEAR_ALL,
            TOGGLE_FACTION,
            FACTIONS_SELECT_ALL,
            FACTIONS_CLEAR_ALL,
            SAMPLES_DECREASE_SMALL,
            SAMPLES_INCREASE_SMALL,
            SAMPLES_DECREASE_LARGE,
            SAMPLES_INCREASE_LARGE,
            SEED_DECREASE,
            SEED_INCREASE,
            TOGGLE_DETAILED_TIMING,
            START,
            CANCEL,
            NEW_RUN,
            EXPORT_CSV
        };

        struct UiControl
        {
            sf::FloatRect Bounds;
            UiActionType Action = UiActionType::START;
            uint32_t Value = 0u;
            bool Enabled = true;
        };

        void processEvents();
        void handleKeyPressed(sf::Keyboard::Key key);
        void handleClick(sf::Vector2f position);
        void executeAction(UiActionType action, uint32_t value);
        void update();
        void render();
        void rebuildControls(const DiagnosticsAppSnapshot& snapshot);
        void drawConfigurationPanel(const DiagnosticsAppSnapshot& snapshot);
        void drawRunPanel(const DiagnosticsAppSnapshot& snapshot);
        void drawSummaryPanel(const DiagnosticsAppSnapshot& snapshot);
        void drawButton(const sf::FloatRect& bounds, const std::string& label, bool active, bool enabled);
        void drawSectionTitle(const std::string& label, float x, float y);
        void drawLabelValue(const std::string& label, const std::string& value, float x, float y, bool emphasize = false);
        void drawProgressBar(const DiagnosticsAppSnapshot& snapshot, float x, float y, float width, float height);
        PixelShipGeneratorDiagnostics::DiagnosticsRunConfiguration createConfiguration() const;
        void startDiagnostics();
        void exportCsv();
        void captureScreenshot(const std::filesystem::path& path);
        void configureAutomatedSmoke();
        void updateAutomatedSmoke(const DiagnosticsAppSnapshot& snapshot);

        static std::string styleName(PixelShipGenerator::ShipStyle style);
        static std::string factionName(PixelShipGenerator::ShipFactionType faction);
        static std::string dimensionsName(PixelShipGenerator::ShipDimensions dimensions);
        static std::string formatDuration(uint64_t nanoseconds, bool available = true);
        static uint32_t clampDimension(uint32_t value);

    private:
        sf::RenderWindow m_Window;
        DiagnosticsAppController m_Controller;
        DiagnosticsAppLaunchOptions m_Options;
        std::vector<PixelShipGenerator::ShipDimensions> m_Dimensions = {
            { 24u, 24u }, { 32u, 32u }, { 44u, 44u }, { 48u, 64u }, { 64u, 48u },
            { 64u, 64u }, { 96u, 96u }, { 128u, 128u }, { 160u, 160u }
        };
        std::vector<bool> m_DimensionSelected;
        std::array<bool, static_cast<std::size_t>(PixelShipGenerator::ShipStyle::SHIP_STYLE_END)> m_StyleSelected = {};
        std::array<bool, static_cast<std::size_t>(PixelShipGenerator::ShipFactionType::SHIP_FACTION_TYPE_END)> m_FactionSelected = {};
        uint32_t m_CustomWidth = 48u;
        uint32_t m_CustomHeight = 64u;
        uint64_t m_SamplesPerCombination = 100u;
        uint64_t m_DiagnosticSeed = 0x6A09E667F3BCC909ull;
        bool m_DetailedTiming = false;
        std::vector<UiControl> m_Controls;
        sf::Vector2f m_MousePosition;
        std::string m_LocalStatus;
        bool m_AutomatedStarted = false;
        bool m_AutomatedCancelRequested = false;
        bool m_AutomatedFinalized = false;
    };
}
