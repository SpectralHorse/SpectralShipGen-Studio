#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "SFMLCharts.h"
#include <SpectralShipGen/Diagnostics/DiagnosticsAnalysis.h>
#include "DiagnosticsAppController.h"
#include <SpectralShipGen/ShipFactionType.h>
#include <SpectralShipGen/ShipGenerationProfile.h>

namespace SpectralShipGenStudioDiagnosticsApp
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
        enum class DashboardView : uint32_t
        {
            OVERVIEW = 0u,
            PERFORMANCE,
            RETRIES,
            COMPOSITION,
            NUMBERS,
            COMPARISON,
            DASHBOARD_VIEW_END
        };

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
            EXPORT_CSV,
            SAVE_RUN,
            LOAD_RUN,
            LOAD_BASELINE,
            NAV_OVERVIEW,
            NAV_PERFORMANCE,
            NAV_RETRIES,
            NAV_COMPOSITION,
            NAV_NUMBERS,
            NAV_COMPARISON,
            FILTER_DIMENSION_PREVIOUS,
            FILTER_DIMENSION_NEXT,
            FILTER_STYLE_PREVIOUS,
            FILTER_STYLE_NEXT,
            FILTER_FACTION_PREVIOUS,
            FILTER_FACTION_NEXT,
            METRIC_PREVIOUS,
            METRIC_NEXT
        };

        struct UiControl
        {
            sf::FloatRect Bounds;
            UiActionType Action = UiActionType::START;
            uint32_t Value = 0u;
            bool Enabled = true;
        };

        struct DashboardChartHit
        {
            sf::FloatRect Bounds;
            uint32_t ChartId = 0u;
            std::size_t SeriesIndex = 0u;
            std::size_t ValueIndex = 0u;
        };

        void processEvents();
        void handleKeyPressed(sf::Keyboard::Key key);
        void handleClick(sf::Vector2f position);
        void executeAction(UiActionType action, uint32_t value);
        void update();
        void render();
        void rebuildControls(const DiagnosticsAppSnapshot& snapshot);
        void drawConfigurationPanel(const DiagnosticsAppSnapshot& snapshot);
        void drawDashboardPanel(const DiagnosticsAppSnapshot& snapshot);
        void drawDashboardHeader(const DiagnosticsAppSnapshot& snapshot);
        void drawLiveDashboard(const DiagnosticsAppSnapshot& snapshot);
        void drawOverviewView(const SpectralShipGenDiagnostics::DiagnosticsResult& result, const DiagnosticsAppSnapshot& snapshot);
        void drawPerformanceView(const SpectralShipGenDiagnostics::DiagnosticsResult& result);
        void drawRetriesView(const SpectralShipGenDiagnostics::DiagnosticsResult& result);
        void drawCompositionView(const SpectralShipGenDiagnostics::DiagnosticsResult& result);
        void drawNumbersView(const SpectralShipGenDiagnostics::DiagnosticsResult& result, const DiagnosticsAppSnapshot& snapshot);
        void drawComparisonView(const SpectralShipGenDiagnostics::DiagnosticsResult& result);
        void drawSelectedChartDetail(float y);
        void drawButton(const sf::FloatRect& bounds, const std::string& label, bool active, bool enabled);
        void drawSectionTitle(const std::string& label, float x, float y);
        void drawLabelValue(const std::string& label, const std::string& value, float x, float y, bool emphasize = false, float valueOffset = 190.0f);
        void drawProgressBar(const DiagnosticsAppSnapshot& snapshot, float x, float y, float width, float height);
        void appendChartHits(uint32_t chartId, const std::vector<SpectralShipGenStudioApplication::ChartHitRegion>& hits);

        SpectralShipGenDiagnostics::DiagnosticsRunConfiguration createConfiguration() const;
        SpectralShipGenDiagnostics::DiagnosticsFilter currentFilter() const;
        void updateAnalysisCache();
        void markAnalysisDirty();
        void cycleDimensionFilter(int direction);
        void cycleStyleFilter(int direction);
        void cycleFactionFilter(int direction);
        void cycleMetric(int direction);
        std::string dimensionFilterName() const;
        std::string styleFilterName() const;
        std::string factionFilterName() const;
        static std::string viewName(DashboardView view);
        static std::string actionLabel(UiActionType action, const DiagnosticsAppSnapshot& snapshot);

        void startDiagnostics();
        void exportCsv();
        void saveRun();
        void loadRun();
        void loadBaseline();
        void captureScreenshot(const std::filesystem::path& path);
        void configureAutomatedSmoke();
        void updateAutomatedSmoke(const DiagnosticsAppSnapshot& snapshot);

        static std::string styleName(SpectralShipGen::ShipStyle style);
        static std::string factionName(SpectralShipGen::ShipFactionType faction);
        static std::string dimensionsName(SpectralShipGen::ShipDimensions dimensions);
        static std::string formatDuration(uint64_t nanoseconds, bool available = true);
        static std::string formatMetric(double value, SpectralShipGenDiagnostics::DiagnosticsMetric metric);
        static uint32_t clampDimension(uint32_t value);

    private:
        sf::RenderWindow m_Window;
        DiagnosticsAppController m_Controller;
        DiagnosticsAppLaunchOptions m_Options;
        std::vector<SpectralShipGen::ShipDimensions> m_Dimensions = {
            { 24u, 24u }, { 32u, 32u }, { 44u, 44u }, { 48u, 64u }, { 64u, 48u },
            { 64u, 64u }, { 96u, 96u }, { 128u, 128u }, { 160u, 160u }
        };
        std::vector<bool> m_DimensionSelected;
        std::array<bool, static_cast<std::size_t>(SpectralShipGen::ShipStyle::SHIP_STYLE_END)> m_StyleSelected = {};
        std::array<bool, static_cast<std::size_t>(SpectralShipGen::ShipFactionType::SHIP_FACTION_TYPE_END)> m_FactionSelected = {};
        uint32_t m_CustomWidth = 48u;
        uint32_t m_CustomHeight = 64u;
        uint64_t m_SamplesPerCombination = 100u;
        uint64_t m_DiagnosticSeed = 0x6A09E667F3BCC909ull;
        bool m_DetailedTiming = false;
        std::vector<UiControl> m_Controls;
        std::size_t m_ConfigActionStartIndex = 0u;
        std::size_t m_DashboardControlStartIndex = 0u;
        sf::Vector2f m_MousePosition;
        std::string m_LocalStatus;

        DashboardView m_DashboardView = DashboardView::OVERVIEW;
        int m_FilterDimensionIndex = -1;
        int m_FilterStyleIndex = -1;
        int m_FilterFactionIndex = -1;
        SpectralShipGenDiagnostics::DiagnosticsMetric m_SelectedMetric = SpectralShipGenDiagnostics::DiagnosticsMetric::GENERATION_MEDIAN_MS;
        bool m_AnalysisDirty = true;
        std::shared_ptr<const SpectralShipGenDiagnostics::DiagnosticsResult> m_AnalysisResult;
        std::shared_ptr<SpectralShipGenDiagnostics::DiagnosticsResult> m_BaselineResult;
        SpectralShipGenDiagnostics::DiagnosticsFilteredResult m_FilteredCache;
        SpectralShipGenDiagnostics::DiagnosticsChartSeries m_ResolutionSeries;
        SpectralShipGenDiagnostics::DiagnosticsChartSeries m_StyleSeries;
        SpectralShipGenDiagnostics::DiagnosticsChartSeries m_FactionSeries;
        SpectralShipGenDiagnostics::DiagnosticsChartSeries m_StageSeries;
        SpectralShipGenDiagnostics::DiagnosticsChartSeries m_RejectionSeries;
        SpectralShipGenDiagnostics::DiagnosticsChartSeries m_AnchorSeries;
        SpectralShipGenDiagnostics::DiagnosticsChartSeries m_BaselineResolutionSeries;
        SpectralShipGenDiagnostics::DiagnosticsChartSeries m_RetryResolutionSeries;
        SpectralShipGenDiagnostics::DiagnosticsChartSeries m_MaterialStyleSeries;
        SpectralShipGenDiagnostics::DiagnosticsComparisonCompatibility m_ComparisonCompatibility;
        SpectralShipGenDiagnostics::DiagnosticsMetricDelta m_ComparisonDelta;
        std::optional<SpectralShipGenDiagnostics::DiagnosticsChartPoint> m_ExpensiveDimension;
        std::optional<SpectralShipGenDiagnostics::DiagnosticsChartPoint> m_SlowestStyle;
        std::optional<SpectralShipGenDiagnostics::DiagnosticsChartPoint> m_SlowestFaction;
        std::optional<SpectralShipGenDiagnostics::DiagnosticsChartPoint> m_ExpensiveStage;
        std::vector<DashboardChartHit> m_ChartHits;
        uint32_t m_SelectedChartId = std::numeric_limits<uint32_t>::max();
        std::size_t m_SelectedChartSeries = 0u;
        std::size_t m_SelectedChartValue = 0u;

        bool m_AutomatedStarted = false;
        bool m_AutomatedCancelRequested = false;
        bool m_AutomatedFinalized = false;
    };
}
