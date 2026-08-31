#include "DiagnosticsApp.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

#include "SFMLPixelText.h"
#include <SpectralShipGen/Diagnostics/DiagnosticsResultSerializer.h>
#include <SpectralShipGen/ShipGenerationPerformance.h>

namespace
{
    constexpr uint32_t WindowWidth = 1480u;
    constexpr uint32_t WindowHeight = 920u;
    constexpr float Margin = 18.0f;
    constexpr float ConfigWidth = 690.0f;
    constexpr float ColumnGap = 18.0f;
    constexpr float RightX = Margin + ConfigWidth + ColumnGap;
    constexpr float RightWidth = static_cast<float>(WindowWidth) - RightX - Margin;
    constexpr uint32_t TextScale = 3u;
    constexpr uint32_t SmallTextScale = 2u;
    constexpr float RowHeight = 30.0f;

    const sf::Color Background(20u, 23u, 31u);
    const sf::Color PanelFill(31u, 35u, 46u);
    const sf::Color PanelOutline(67u, 74u, 92u);
    const sf::Color Accent(90u, 184u, 225u);
    const sf::Color SecondaryAccent(182u, 125u, 223u);
    const sf::Color Highlight(236u, 210u, 98u);
    const sf::Color Text(224u, 228u, 236u);
    const sf::Color Muted(145u, 151u, 166u);
    const sf::Color Positive(118u, 214u, 150u);
    const sf::Color Negative(224u, 115u, 120u);

    bool contains(const sf::FloatRect& bounds, sf::Vector2f point)
    {
        return bounds.contains(point);
    }

    float getPixelTextVisualWidth(const std::string& text, uint32_t scale)
    {
        if (text.empty()) { return 0.0f; }
        return std::max(0.0f, SpectralShipGenStudioApplication::getPixelTextWidth(text, scale) - static_cast<float>(scale));
    }

    float getPixelTextHeight(uint32_t scale)
    {
        return static_cast<float>(5u * scale);
    }

    float getCenteredPixelTextY(const sf::FloatRect& bounds, uint32_t scale)
    {
        return bounds.top + std::floor((bounds.height - getPixelTextHeight(scale)) * 0.5f);
    }

    float getCenteredPixelTextXBetween(const sf::FloatRect& leftBounds, const sf::FloatRect& rightBounds, const std::string& text, uint32_t scale)
    {
        const float gapLeft = leftBounds.left + leftBounds.width;
        const float gapWidth = rightBounds.left - gapLeft;
        return gapLeft + std::floor((gapWidth - getPixelTextVisualWidth(text, scale)) * 0.5f);
    }

    std::string formatDouble(double value, int precision = 2)
    {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(precision) << value;
        return stream.str();
    }

    std::string signedDouble(double value, int precision = 2)
    {
        std::ostringstream stream;
        if (value >= 0.0) { stream << '+'; }
        stream << std::fixed << std::setprecision(precision) << value;
        return stream.str();
    }

    void drawPanel(sf::RenderTarget& target, float x, float y, float width, float height)
    {
        sf::RectangleShape panel(sf::Vector2f(width, height));
        panel.setPosition(x, y);
        panel.setFillColor(PanelFill);
        panel.setOutlineThickness(1.0f);
        panel.setOutlineColor(PanelOutline);
        target.draw(panel);
    }

    SpectralShipGenStudioApplication::ChartColors chartColors()
    {
        return { sf::Color(25u, 28u, 37u), PanelOutline, Text, Muted, Accent, SecondaryAccent, Highlight };
    }

    SpectralShipGenStudioApplication::ChartSeries toChartSeries(const SpectralShipGenDiagnostics::DiagnosticsChartSeries& source, const std::string& labelOverride = {})
    {
        SpectralShipGenStudioApplication::ChartSeries result;
        result.Label = labelOverride.empty() ? source.Label : labelOverride;
        result.Values.reserve(source.Points.size());
        for (const auto& point : source.Points) { result.Values.push_back({ point.Label, point.Value }); }
        return result;
    }

    std::pair<SpectralShipGenStudioApplication::ChartSeries, SpectralShipGenStudioApplication::ChartSeries> alignComparisonSeries(
        const SpectralShipGenDiagnostics::DiagnosticsChartSeries& baseline,
        const SpectralShipGenDiagnostics::DiagnosticsChartSeries& current)
    {
        SpectralShipGenStudioApplication::ChartSeries baselineSeries{ "BASELINE", {} };
        SpectralShipGenStudioApplication::ChartSeries currentSeries{ "CURRENT", {} };
        for (const auto& currentPoint : current.Points)
        {
            const auto found = std::find_if(baseline.Points.begin(), baseline.Points.end(), [&](const auto& point) { return point.Label == currentPoint.Label; });
            if (found == baseline.Points.end()) { continue; }
            baselineSeries.Values.push_back({ currentPoint.Label, found->Value });
            currentSeries.Values.push_back({ currentPoint.Label, currentPoint.Value });
        }
        return { std::move(baselineSeries), std::move(currentSeries) };
    }
}

namespace SpectralShipGenStudioDiagnosticsApp
{
    DiagnosticsApp::DiagnosticsApp(DiagnosticsAppLaunchOptions options)
        : m_Window(sf::VideoMode(WindowWidth, WindowHeight), "SpectralShipGen Studio Diagnostics", sf::Style::Titlebar | sf::Style::Close),
        m_Options(std::move(options)),
        m_DimensionSelected(m_Dimensions.size(), false)
    {
        m_Window.setFramerateLimit(30u);
        if (m_DimensionSelected.size() > 5u)
        {
            m_DimensionSelected[2u] = true;
            m_DimensionSelected[5u] = true;
        }
        m_StyleSelected.fill(true);
        m_FactionSelected.fill(true);
        if (m_Options.AutomatedSmoke) { configureAutomatedSmoke(); }
    }

    DiagnosticsApp::~DiagnosticsApp()
    {
        m_Controller.requestCancel();
        m_Controller.wait();
    }

    int DiagnosticsApp::run()
    {
        while (m_Window.isOpen())
        {
            processEvents();
            if (!m_Window.isOpen()) { break; }
            update();
            if (!m_Window.isOpen()) { break; }
            render();
        }
        m_Controller.requestCancel();
        m_Controller.wait();
        return 0;
    }

    void DiagnosticsApp::processEvents()
    {
        sf::Event event;
        while (m_Window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                m_Controller.requestCancel();
                m_Controller.wait();
                m_Window.close();
            }
            else if (event.type == sf::Event::MouseMoved)
            {
                m_MousePosition = sf::Vector2f(static_cast<float>(event.mouseMove.x), static_cast<float>(event.mouseMove.y));
            }
            else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
            {
                handleClick(sf::Vector2f(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y)));
            }
            else if (event.type == sf::Event::KeyPressed)
            {
                handleKeyPressed(event.key.code);
            }
        }
    }

    void DiagnosticsApp::handleKeyPressed(sf::Keyboard::Key key)
    {
        const DiagnosticsAppSnapshot snapshot = m_Controller.getSnapshot();
        if (key == sf::Keyboard::Escape)
        {
            if (snapshot.State == DiagnosticsAppRunState::RUNNING || snapshot.State == DiagnosticsAppRunState::CANCELLING) { m_Controller.requestCancel(); }
            else { m_Window.close(); }
        }
        else if (key == sf::Keyboard::Enter && snapshot.State == DiagnosticsAppRunState::READY) { startDiagnostics(); }
        else if (key == sf::Keyboard::E && snapshot.HasResult) { exportCsv(); }
        else if (key == sf::Keyboard::S && snapshot.HasResult) { saveRun(); }
        else if (key == sf::Keyboard::L && snapshot.State == DiagnosticsAppRunState::READY) { loadRun(); }
        else if (key == sf::Keyboard::B && snapshot.HasResult) { loadBaseline(); }
    }

    void DiagnosticsApp::handleClick(sf::Vector2f position)
    {
        for (const UiControl& control : m_Controls)
        {
            if (control.Enabled && contains(control.Bounds, position))
            {
                executeAction(control.Action, control.Value);
                return;
            }
        }
        for (const DashboardChartHit& hit : m_ChartHits)
        {
            if (contains(hit.Bounds, position))
            {
                m_SelectedChartId = hit.ChartId;
                m_SelectedChartSeries = hit.SeriesIndex;
                m_SelectedChartValue = hit.ValueIndex;
                return;
            }
        }
    }

    void DiagnosticsApp::executeAction(UiActionType action, uint32_t value)
    {
        switch (action)
        {
        case UiActionType::TOGGLE_DIMENSION: if (value < m_DimensionSelected.size()) { m_DimensionSelected[value] = !m_DimensionSelected[value]; } break;
        case UiActionType::DIMENSIONS_SELECT_ALL: std::fill(m_DimensionSelected.begin(), m_DimensionSelected.end(), true); break;
        case UiActionType::DIMENSIONS_CLEAR_ALL: std::fill(m_DimensionSelected.begin(), m_DimensionSelected.end(), false); break;
        case UiActionType::CUSTOM_WIDTH_DECREASE: m_CustomWidth = clampDimension(m_CustomWidth > 2u ? m_CustomWidth - 2u : 24u); break;
        case UiActionType::CUSTOM_WIDTH_INCREASE: m_CustomWidth = clampDimension(m_CustomWidth + 2u); break;
        case UiActionType::CUSTOM_HEIGHT_DECREASE: m_CustomHeight = clampDimension(m_CustomHeight > 2u ? m_CustomHeight - 2u : 24u); break;
        case UiActionType::CUSTOM_HEIGHT_INCREASE: m_CustomHeight = clampDimension(m_CustomHeight + 2u); break;
        case UiActionType::ADD_CUSTOM_DIMENSION:
        {
            const SpectralShipGen::ShipDimensions dimensions{ m_CustomWidth, m_CustomHeight };
            const auto found = std::find(m_Dimensions.begin(), m_Dimensions.end(), dimensions);
            if (found != m_Dimensions.end()) { m_DimensionSelected[static_cast<std::size_t>(std::distance(m_Dimensions.begin(), found))] = true; }
            else if (m_Dimensions.size() < 10u) { m_Dimensions.push_back(dimensions); m_DimensionSelected.push_back(true); }
            else { m_Dimensions.back() = dimensions; m_DimensionSelected.back() = true; }
            break;
        }
        case UiActionType::TOGGLE_STYLE: if (value < m_StyleSelected.size()) { m_StyleSelected[value] = !m_StyleSelected[value]; } break;
        case UiActionType::STYLES_SELECT_ALL: m_StyleSelected.fill(true); break;
        case UiActionType::STYLES_CLEAR_ALL: m_StyleSelected.fill(false); break;
        case UiActionType::TOGGLE_FACTION: if (value < m_FactionSelected.size()) { m_FactionSelected[value] = !m_FactionSelected[value]; } break;
        case UiActionType::FACTIONS_SELECT_ALL: m_FactionSelected.fill(true); break;
        case UiActionType::FACTIONS_CLEAR_ALL: m_FactionSelected.fill(false); break;
        case UiActionType::SAMPLES_DECREASE_SMALL: m_SamplesPerCombination = std::max<uint64_t>(1u, m_SamplesPerCombination > 10u ? m_SamplesPerCombination - 10u : 1u); break;
        case UiActionType::SAMPLES_INCREASE_SMALL: m_SamplesPerCombination = std::min<uint64_t>(1000000u, m_SamplesPerCombination + 10u); break;
        case UiActionType::SAMPLES_DECREASE_LARGE: m_SamplesPerCombination = std::max<uint64_t>(1u, m_SamplesPerCombination > 100u ? m_SamplesPerCombination - 100u : 1u); break;
        case UiActionType::SAMPLES_INCREASE_LARGE: m_SamplesPerCombination = std::min<uint64_t>(1000000u, m_SamplesPerCombination + 100u); break;
        case UiActionType::SEED_DECREASE: --m_DiagnosticSeed; break;
        case UiActionType::SEED_INCREASE: ++m_DiagnosticSeed; break;
        case UiActionType::TOGGLE_DETAILED_TIMING: m_DetailedTiming = !m_DetailedTiming; break;
        case UiActionType::START: startDiagnostics(); break;
        case UiActionType::CANCEL: m_Controller.requestCancel(); break;
        case UiActionType::NEW_RUN: m_Controller.reset(); m_LocalStatus = "Ready for a new diagnostics run."; markAnalysisDirty(); break;
        case UiActionType::EXPORT_CSV: exportCsv(); break;
        case UiActionType::SAVE_RUN: saveRun(); break;
        case UiActionType::LOAD_RUN: loadRun(); break;
        case UiActionType::LOAD_BASELINE: loadBaseline(); break;
        case UiActionType::NAV_OVERVIEW: m_DashboardView = DashboardView::OVERVIEW; m_SelectedChartId = std::numeric_limits<uint32_t>::max(); break;
        case UiActionType::NAV_PERFORMANCE: m_DashboardView = DashboardView::PERFORMANCE; m_SelectedChartId = std::numeric_limits<uint32_t>::max(); break;
        case UiActionType::NAV_RETRIES: m_DashboardView = DashboardView::RETRIES; m_SelectedChartId = std::numeric_limits<uint32_t>::max(); break;
        case UiActionType::NAV_COMPOSITION: m_DashboardView = DashboardView::COMPOSITION; m_SelectedChartId = std::numeric_limits<uint32_t>::max(); break;
        case UiActionType::NAV_NUMBERS: m_DashboardView = DashboardView::NUMBERS; m_SelectedChartId = std::numeric_limits<uint32_t>::max(); break;
        case UiActionType::NAV_COMPARISON: m_DashboardView = DashboardView::COMPARISON; m_SelectedChartId = std::numeric_limits<uint32_t>::max(); break;
        case UiActionType::FILTER_DIMENSION_PREVIOUS: cycleDimensionFilter(-1); break;
        case UiActionType::FILTER_DIMENSION_NEXT: cycleDimensionFilter(1); break;
        case UiActionType::FILTER_STYLE_PREVIOUS: cycleStyleFilter(-1); break;
        case UiActionType::FILTER_STYLE_NEXT: cycleStyleFilter(1); break;
        case UiActionType::FILTER_FACTION_PREVIOUS: cycleFactionFilter(-1); break;
        case UiActionType::FILTER_FACTION_NEXT: cycleFactionFilter(1); break;
        case UiActionType::METRIC_PREVIOUS: cycleMetric(-1); break;
        case UiActionType::METRIC_NEXT: cycleMetric(1); break;
        }
    }

    void DiagnosticsApp::update()
    {
        const DiagnosticsAppSnapshot snapshot = m_Controller.getSnapshot();
        rebuildControls(snapshot);
        updateAnalysisCache();
        if (m_Options.AutomatedSmoke) { updateAutomatedSmoke(snapshot); }
    }

    void DiagnosticsApp::render()
    {
        const DiagnosticsAppSnapshot snapshot = m_Controller.getSnapshot();
        m_ChartHits.clear();
        m_Window.clear(Background);
        drawConfigurationPanel(snapshot);
        drawDashboardPanel(snapshot);
        SpectralShipGenStudioApplication::drawPixelText(m_Window, "SPECTRALSHIPGEN STUDIO - DIAGNOSTICS", Margin, 8.0f, Highlight, TextScale);
        const std::string status = !m_LocalStatus.empty() ? m_LocalStatus : snapshot.StatusMessage;
        if (!status.empty())
        {
            SpectralShipGenStudioApplication::drawPixelText(m_Window, SpectralShipGenStudioApplication::wrapPixelText(status, 160u), Margin, static_cast<float>(WindowHeight) - 36.0f, snapshot.State == DiagnosticsAppRunState::ERROR ? Negative : Muted, SmallTextScale);
        }
        m_Window.display();
    }

    void DiagnosticsApp::rebuildControls(const DiagnosticsAppSnapshot& snapshot)
    {
        m_Controls.clear();
        const bool editable = snapshot.State == DiagnosticsAppRunState::READY;
        float y = 90.0f;
        const float buttonHeight = 25.0f;
        const float buttonGap = 6.0f;
        const float cellWidth = 122.0f;
        const float startX = Margin + 14.0f;

        for (std::size_t index = 0u; index < m_Dimensions.size(); ++index)
        {
            const float x = startX + static_cast<float>(index % 5u) * (cellWidth + buttonGap);
            const float rowY = y + static_cast<float>(index / 5u) * (buttonHeight + buttonGap);
            m_Controls.push_back({ { x, rowY, cellWidth, buttonHeight }, UiActionType::TOGGLE_DIMENSION, static_cast<uint32_t>(index), editable });
        }
        y += 2.0f * (buttonHeight + buttonGap) + 4.0f;
        m_Controls.push_back({ { startX, y, 110.0f, buttonHeight }, UiActionType::DIMENSIONS_SELECT_ALL, 0u, editable });
        m_Controls.push_back({ { startX + 118.0f, y, 110.0f, buttonHeight }, UiActionType::DIMENSIONS_CLEAR_ALL, 0u, editable });
        y += buttonHeight + 8.0f;
        m_Controls.push_back({ { startX + 85.0f, y, 30.0f, buttonHeight }, UiActionType::CUSTOM_WIDTH_DECREASE, 0u, editable });
        m_Controls.push_back({ { startX + 180.0f, y, 30.0f, buttonHeight }, UiActionType::CUSTOM_WIDTH_INCREASE, 0u, editable });
        m_Controls.push_back({ { startX + 303.0f, y, 30.0f, buttonHeight }, UiActionType::CUSTOM_HEIGHT_DECREASE, 0u, editable });
        m_Controls.push_back({ { startX + 398.0f, y, 30.0f, buttonHeight }, UiActionType::CUSTOM_HEIGHT_INCREASE, 0u, editable });
        m_Controls.push_back({ { startX + 448.0f, y, 160.0f, buttonHeight }, UiActionType::ADD_CUSTOM_DIMENSION, 0u, editable });

        y += buttonHeight + 45.0f;
        for (std::size_t index = 0u; index < m_StyleSelected.size(); ++index)
        {
            const float x = startX + static_cast<float>(index % 3u) * 196.0f;
            const float rowY = y + static_cast<float>(index / 3u) * (buttonHeight + buttonGap);
            m_Controls.push_back({ { x, rowY, 184.0f, buttonHeight }, UiActionType::TOGGLE_STYLE, static_cast<uint32_t>(index), editable });
        }
        y += 2.0f * (buttonHeight + buttonGap) + 4.0f;
        m_Controls.push_back({ { startX, y, 110.0f, buttonHeight }, UiActionType::STYLES_SELECT_ALL, 0u, editable });
        m_Controls.push_back({ { startX + 118.0f, y, 110.0f, buttonHeight }, UiActionType::STYLES_CLEAR_ALL, 0u, editable });

        y += buttonHeight + 45.0f;
        for (std::size_t index = 0u; index < m_FactionSelected.size(); ++index)
        {
            const float x = startX + static_cast<float>(index % 3u) * 196.0f;
            const float rowY = y + static_cast<float>(index / 3u) * (buttonHeight + buttonGap);
            m_Controls.push_back({ { x, rowY, 184.0f, buttonHeight }, UiActionType::TOGGLE_FACTION, static_cast<uint32_t>(index), editable });
        }
        y += 2.0f * (buttonHeight + buttonGap) + 4.0f;
        m_Controls.push_back({ { startX, y, 110.0f, buttonHeight }, UiActionType::FACTIONS_SELECT_ALL, 0u, editable });
        m_Controls.push_back({ { startX + 118.0f, y, 110.0f, buttonHeight }, UiActionType::FACTIONS_CLEAR_ALL, 0u, editable });

        y += buttonHeight + 80.0f;
        m_Controls.push_back({ { startX + 350.0f, y, 52.0f, buttonHeight }, UiActionType::SAMPLES_DECREASE_LARGE, 0u, editable });
        m_Controls.push_back({ { startX + 408.0f, y, 44.0f, buttonHeight }, UiActionType::SAMPLES_DECREASE_SMALL, 0u, editable });
        m_Controls.push_back({ { startX + 458.0f, y, 44.0f, buttonHeight }, UiActionType::SAMPLES_INCREASE_SMALL, 0u, editable });
        m_Controls.push_back({ { startX + 508.0f, y, 52.0f, buttonHeight }, UiActionType::SAMPLES_INCREASE_LARGE, 0u, editable });
        y += buttonHeight + 7.0f;
        m_Controls.push_back({ { startX + 350.0f, y, 52.0f, buttonHeight }, UiActionType::SEED_DECREASE, 0u, editable });
        m_Controls.push_back({ { startX + 508.0f, y, 52.0f, buttonHeight }, UiActionType::SEED_INCREASE, 0u, editable });
        y += buttonHeight + 7.0f;
        m_Controls.push_back({ { startX + 350.0f, y, 210.0f, buttonHeight }, UiActionType::TOGGLE_DETAILED_TIMING, 0u, editable });

        m_ConfigActionStartIndex = m_Controls.size();
        const float bottomY = 805.0f;
        if (snapshot.State == DiagnosticsAppRunState::READY)
        {
            m_Controls.push_back({ { startX, bottomY, 410.0f, 42.0f }, UiActionType::START, 0u, true });
            m_Controls.push_back({ { startX + 420.0f, bottomY, 188.0f, 42.0f }, UiActionType::LOAD_RUN, 0u, true });
        }
        else if (snapshot.State == DiagnosticsAppRunState::RUNNING || snapshot.State == DiagnosticsAppRunState::CANCELLING)
        {
            m_Controls.push_back({ { startX, bottomY, 608.0f, 42.0f }, UiActionType::CANCEL, 0u, snapshot.State == DiagnosticsAppRunState::RUNNING });
        }
        else
        {
            const float actionWidth = 145.0f;
            m_Controls.push_back({ { startX, bottomY, actionWidth, 42.0f }, UiActionType::NEW_RUN, 0u, true });
            m_Controls.push_back({ { startX + 154.0f, bottomY, actionWidth, 42.0f }, UiActionType::EXPORT_CSV, 0u, snapshot.HasResult });
            m_Controls.push_back({ { startX + 308.0f, bottomY, actionWidth, 42.0f }, UiActionType::SAVE_RUN, 0u, snapshot.HasResult });
            m_Controls.push_back({ { startX + 462.0f, bottomY, actionWidth, 42.0f }, UiActionType::LOAD_BASELINE, 0u, snapshot.HasResult });
        }

        m_DashboardControlStartIndex = m_Controls.size();
        const bool analysisAvailable = snapshot.HasResult;
        const float tabY = 76.0f;
        const float tabX = RightX + 14.0f;
        const float tabWidth = 112.0f;
        const float tabGap = 5.0f;
        const UiActionType tabs[] = { UiActionType::NAV_OVERVIEW, UiActionType::NAV_PERFORMANCE, UiActionType::NAV_RETRIES, UiActionType::NAV_COMPOSITION, UiActionType::NAV_NUMBERS, UiActionType::NAV_COMPARISON };
        for (uint32_t index = 0u; index < 6u; ++index) { m_Controls.push_back({ { tabX + index * (tabWidth + tabGap), tabY, tabWidth, 25.0f }, tabs[index], index, true }); }

        const float filterLeft = RightX + 14.0f;
        const float filterRight = RightX + RightWidth * 0.5f + 4.0f;
        const float arrowWidth = 28.0f;
        const float blockWidth = RightWidth * 0.5f - 25.0f;
        const float filterY1 = 112.0f;
        const float filterY2 = 143.0f;
        m_Controls.push_back({ { filterLeft, filterY1, arrowWidth, 25.0f }, UiActionType::FILTER_DIMENSION_PREVIOUS, 0u, analysisAvailable });
        m_Controls.push_back({ { filterLeft + blockWidth - arrowWidth, filterY1, arrowWidth, 25.0f }, UiActionType::FILTER_DIMENSION_NEXT, 0u, analysisAvailable });
        m_Controls.push_back({ { filterRight, filterY1, arrowWidth, 25.0f }, UiActionType::FILTER_STYLE_PREVIOUS, 0u, analysisAvailable });
        m_Controls.push_back({ { filterRight + blockWidth - arrowWidth, filterY1, arrowWidth, 25.0f }, UiActionType::FILTER_STYLE_NEXT, 0u, analysisAvailable });
        m_Controls.push_back({ { filterLeft, filterY2, arrowWidth, 25.0f }, UiActionType::FILTER_FACTION_PREVIOUS, 0u, analysisAvailable });
        m_Controls.push_back({ { filterLeft + blockWidth - arrowWidth, filterY2, arrowWidth, 25.0f }, UiActionType::FILTER_FACTION_NEXT, 0u, analysisAvailable });
        m_Controls.push_back({ { filterRight, filterY2, arrowWidth, 25.0f }, UiActionType::METRIC_PREVIOUS, 0u, analysisAvailable });
        m_Controls.push_back({ { filterRight + blockWidth - arrowWidth, filterY2, arrowWidth, 25.0f }, UiActionType::METRIC_NEXT, 0u, analysisAvailable });
    }

    void DiagnosticsApp::drawConfigurationPanel(const DiagnosticsAppSnapshot& snapshot)
    {
        drawPanel(m_Window, Margin, 40.0f, ConfigWidth, 820.0f);
        drawSectionTitle("RUN CONFIGURATION", Margin + 14.0f, 50.0f);
        const bool editable = snapshot.State == DiagnosticsAppRunState::READY;
        float y = 90.0f;
        SpectralShipGenStudioApplication::drawPixelText(m_Window, "DIMENSIONS", Margin + 14.0f, y - 16.0f, Accent, SmallTextScale);
        for (std::size_t index = 0u; index < m_Dimensions.size(); ++index) { drawButton(m_Controls[index].Bounds, dimensionsName(m_Dimensions[index]), m_DimensionSelected[index], editable); }
        drawButton(m_Controls[m_Dimensions.size()].Bounds, "SELECT ALL", false, editable);
        drawButton(m_Controls[m_Dimensions.size() + 1u].Bounds, "CLEAR ALL", false, editable);
        const std::size_t customControlsStart = m_Dimensions.size() + 2u;
        const sf::FloatRect& widthDecreaseBounds = m_Controls[customControlsStart].Bounds;
        const sf::FloatRect& widthIncreaseBounds = m_Controls[customControlsStart + 1u].Bounds;
        const sf::FloatRect& heightDecreaseBounds = m_Controls[customControlsStart + 2u].Bounds;
        const sf::FloatRect& heightIncreaseBounds = m_Controls[customControlsStart + 3u].Bounds;
        const float customLabelY = getCenteredPixelTextY(widthDecreaseBounds, SmallTextScale);
        const float customValueY = getCenteredPixelTextY(widthDecreaseBounds, TextScale);
        const std::string customWidthText = std::to_string(m_CustomWidth);
        const std::string customHeightText = std::to_string(m_CustomHeight);
        SpectralShipGenStudioApplication::drawPixelText(m_Window, "CUSTOM W", Margin + 14.0f, customLabelY, Muted, SmallTextScale);
        SpectralShipGenStudioApplication::drawPixelText(m_Window, customWidthText, getCenteredPixelTextXBetween(widthDecreaseBounds, widthIncreaseBounds, customWidthText, TextScale), customValueY, Text, TextScale);
        SpectralShipGenStudioApplication::drawPixelText(m_Window, "H", Margin + 248.0f, customLabelY, Muted, SmallTextScale);
        SpectralShipGenStudioApplication::drawPixelText(m_Window, customHeightText, getCenteredPixelTextXBetween(heightDecreaseBounds, heightIncreaseBounds, customHeightText, TextScale), customValueY, Text, TextScale);

        std::size_t cursor = customControlsStart;
        for (std::size_t index = 0u; index < 5u; ++index) { drawButton(m_Controls[cursor + index].Bounds, index == 4u ? "ADD CUSTOM" : (index % 2u == 0u ? "-" : "+"), false, editable); }
        cursor += 5u;
        drawSectionTitle("STYLES", Margin + 14.0f, 238.0f);
        for (std::size_t index = 0u; index < m_StyleSelected.size(); ++index) { drawButton(m_Controls[cursor++].Bounds, styleName(static_cast<SpectralShipGen::ShipStyle>(index)), m_StyleSelected[index], editable); }
        drawButton(m_Controls[cursor++].Bounds, "SELECT ALL", false, editable); drawButton(m_Controls[cursor++].Bounds, "CLEAR ALL", false, editable);
        drawSectionTitle("FACTIONS", Margin + 14.0f, 374.0f);
        for (std::size_t index = 0u; index < m_FactionSelected.size(); ++index) { drawButton(m_Controls[cursor++].Bounds, factionName(static_cast<SpectralShipGen::ShipFactionType>(index)), m_FactionSelected[index], editable); }
        drawButton(m_Controls[cursor++].Bounds, "SELECT ALL", false, editable); drawButton(m_Controls[cursor++].Bounds, "CLEAR ALL", false, editable);
        drawSectionTitle("SAMPLING", Margin + 14.0f, 510.0f);
        drawLabelValue("SAMPLES / COMBO", std::to_string(m_SamplesPerCombination), Margin + 14.0f, 570.0f, true);
        drawButton(m_Controls[cursor++].Bounds, "-100", false, editable); drawButton(m_Controls[cursor++].Bounds, "-10", false, editable); drawButton(m_Controls[cursor++].Bounds, "+10", false, editable); drawButton(m_Controls[cursor++].Bounds, "+100", false, editable);
        drawLabelValue("DIAGNOSTIC SEED", std::to_string(m_DiagnosticSeed), Margin + 14.0f, 602.0f);
        drawButton(m_Controls[cursor++].Bounds, "-1", false, editable); drawButton(m_Controls[cursor++].Bounds, "+1", false, editable);
        drawLabelValue("DETAILED STAGE TIMING", m_DetailedTiming ? "ON" : "OFF", Margin + 14.0f, 634.0f, m_DetailedTiming);
        drawButton(m_Controls[cursor++].Bounds, m_DetailedTiming ? "DETAILED TIMING: ON" : "DETAILED TIMING: OFF", m_DetailedTiming, editable);
        const uint64_t dimensionsCount = static_cast<uint64_t>(std::count(m_DimensionSelected.begin(), m_DimensionSelected.end(), true));
        const uint64_t stylesCount = static_cast<uint64_t>(std::count(m_StyleSelected.begin(), m_StyleSelected.end(), true));
        const uint64_t factionsCount = static_cast<uint64_t>(std::count(m_FactionSelected.begin(), m_FactionSelected.end(), true));
        drawLabelValue("CONFIGURED WORK", std::to_string(dimensionsCount * stylesCount * factionsCount * m_SamplesPerCombination) + " SAMPLES", Margin + 14.0f, 690.0f, true);
        for (std::size_t index = m_ConfigActionStartIndex; index < m_DashboardControlStartIndex; ++index)
        {
            const UiControl& control = m_Controls[index];
            drawButton(control.Bounds, actionLabel(control.Action, snapshot), control.Action == UiActionType::CANCEL && snapshot.State == DiagnosticsAppRunState::RUNNING, control.Enabled);
        }
    }

    void DiagnosticsApp::drawDashboardPanel(const DiagnosticsAppSnapshot& snapshot)
    {
        drawPanel(m_Window, RightX, 40.0f, RightWidth, 820.0f);
        drawDashboardHeader(snapshot);
        if (!snapshot.HasResult)
        {
            drawLiveDashboard(snapshot);
            return;
        }
        const auto result = m_Controller.getResult();
        if (!result) { drawLiveDashboard(snapshot); return; }
        switch (m_DashboardView)
        {
        case DashboardView::OVERVIEW: drawOverviewView(*result, snapshot); break;
        case DashboardView::PERFORMANCE: drawPerformanceView(*result); break;
        case DashboardView::RETRIES: drawRetriesView(*result); break;
        case DashboardView::COMPOSITION: drawCompositionView(*result); break;
        case DashboardView::NUMBERS: drawNumbersView(*result, snapshot); break;
        case DashboardView::COMPARISON: drawComparisonView(*result); break;
        default: break;
        }
    }

    void DiagnosticsApp::drawDashboardHeader(const DiagnosticsAppSnapshot& snapshot)
    {
        drawSectionTitle("ANALYSIS DASHBOARD", RightX + 14.0f, 50.0f);
        const char* tabLabels[] = { "OVERVIEW", "PERF", "RETRIES", "COMPOSE", "NUMBERS", "COMPARE" };
        for (std::size_t i = 0u; i < 6u; ++i)
        {
            const UiControl& control = m_Controls[m_DashboardControlStartIndex + i];
            drawButton(control.Bounds, tabLabels[i], static_cast<uint32_t>(m_DashboardView) == i, true);
        }
        const std::size_t filterStart = m_DashboardControlStartIndex + 6u;
        for (std::size_t i = 0u; i < 8u; ++i) { drawButton(m_Controls[filterStart + i].Bounds, i % 2u == 0u ? "<" : ">", false, m_Controls[filterStart + i].Enabled); }
        const float left = RightX + 50.0f;
        const float right = RightX + RightWidth * 0.5f + 40.0f;
        SpectralShipGenStudioApplication::drawPixelText(m_Window, "DIM: " + dimensionFilterName(), left, 118.0f, snapshot.HasResult ? Text : Muted, SmallTextScale);
        SpectralShipGenStudioApplication::drawPixelText(m_Window, "STYLE: " + styleFilterName(), right, 118.0f, snapshot.HasResult ? Text : Muted, SmallTextScale);
        SpectralShipGenStudioApplication::drawPixelText(m_Window, "FACTION: " + factionFilterName(), left, 149.0f, snapshot.HasResult ? Text : Muted, SmallTextScale);
        SpectralShipGenStudioApplication::drawPixelText(m_Window, "METRIC: " + std::string(SpectralShipGenDiagnostics::getDiagnosticsMetricName(m_SelectedMetric)), right, 149.0f, snapshot.HasResult ? Highlight : Muted, SmallTextScale);
    }

    void DiagnosticsApp::drawLiveDashboard(const DiagnosticsAppSnapshot& snapshot)
    {
        float y = 205.0f;
        drawSectionTitle("LIVE RUN", RightX + 14.0f, y); y += 42.0f;
        drawLabelValue("STATE", getDiagnosticsAppRunStateName(snapshot.State), RightX + 14.0f, y, true); y += 38.0f;
        drawProgressBar(snapshot, RightX + 14.0f, y, RightWidth - 28.0f, 26.0f); y += 42.0f;
        drawLabelValue("PROGRESS", formatDouble(snapshot.Progress.ProgressPercent, 1) + "%", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("SAMPLES", std::to_string(snapshot.CompletedSamples) + " / " + std::to_string(snapshot.ScheduledSamples), RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("ELAPSED", formatDuration(snapshot.Progress.ElapsedNanoseconds), RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("ETA", formatDuration(snapshot.Progress.EstimatedRemainingNanoseconds, snapshot.Progress.EstimatedRemainingAvailable), RightX + 14.0f, y); y += 40.0f;
        if (snapshot.ScheduledSamples > 0u)
        {
            drawLabelValue("CURRENT", std::to_string(snapshot.Progress.CurrentWidth) + "X" + std::to_string(snapshot.Progress.CurrentHeight), RightX + 14.0f, y); y += RowHeight;
            drawLabelValue("STYLE", styleName(snapshot.Progress.CurrentStyle), RightX + 14.0f, y); y += RowHeight;
            drawLabelValue("FACTION", factionName(snapshot.Progress.CurrentFaction), RightX + 14.0f, y); y += RowHeight;
            drawLabelValue("STAGE", snapshot.Progress.CurrentStage == SpectralShipGen::ShipGenerationPerformanceStage::SHIP_GENERATION_PERFORMANCE_STAGE_END ? "SAMPLE COMPLETE" : SpectralShipGen::getShipGenerationPerformanceStageName(snapshot.Progress.CurrentStage), RightX + 14.0f, y);
        }
        if (snapshot.CompletedSamples > 0u)
        {
            y = 650.0f;
            drawSectionTitle("LIVE NUMBERS", RightX + 14.0f, y); y += 42.0f;
            drawLabelValue("GEN AVG", formatDouble(snapshot.LiveSummary.AverageGenerationMilliseconds) + " MS", RightX + 14.0f, y, true); y += RowHeight;
            drawLabelValue("HULL AVG ATTEMPTS", formatDouble(snapshot.LiveSummary.AverageHullAttempts), RightX + 14.0f, y); y += RowHeight;
            drawLabelValue("NEG SPACE SUCCESS", formatDouble(snapshot.LiveSummary.NegativeSpaceSuccessRatePercent) + "%", RightX + 14.0f, y);
        }
        else if (snapshot.State == DiagnosticsAppRunState::READY)
        {
            SpectralShipGenStudioApplication::drawPixelText(m_Window, "START A RUN OR LOAD A SAVED .SHIPDIAG.JSON RESULT.", RightX + 14.0f, 260.0f, Muted, SmallTextScale);
        }
    }

    void DiagnosticsApp::drawOverviewView(const SpectralShipGenDiagnostics::DiagnosticsResult& result, const DiagnosticsAppSnapshot& snapshot)
    {
        const auto& summary = m_FilteredCache.Summary;
        float y = 198.0f;
        drawSectionTitle("OVERVIEW", RightX + 14.0f, y); y += 42.0f;
        drawLabelValue("RUN STATUS", snapshot.ResultCancelled ? "PARTIAL / CANCELLED" : (snapshot.ResultCompleted ? "COMPLETE" : "INCOMPLETE"), RightX + 14.0f, y, true); y += RowHeight;
        drawLabelValue("FILTERED SAMPLES", std::to_string(m_FilteredCache.Samples.size()), RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("RUN SAMPLES", std::to_string(result.CompletedWorkItems) + " / " + std::to_string(result.ScheduledWorkItems), RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("ELAPSED", formatDuration(result.ElapsedNanoseconds), RightX + 14.0f, y); y += 38.0f;
        drawLabelValue("GEN AVG", formatDouble(summary.GenerationTimeMilliseconds.Mean) + " MS", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("GEN MEDIAN", formatDouble(summary.GenerationTimeMilliseconds.Median) + " MS", RightX + 14.0f, y, true); y += RowHeight;
        drawLabelValue("GEN P95", formatDouble(summary.GenerationTimeMilliseconds.P95) + " MS", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("GEN MAX", formatDouble(summary.GenerationTimeMilliseconds.Maximum) + " MS", RightX + 14.0f, y); y += 38.0f;
        drawLabelValue("HULL AVG ATTEMPTS", formatDouble(summary.HullAttempts.Mean), RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("HULL RETRY RATE", formatDouble(summary.HullRetryRatePercent) + "%", RightX + 14.0f, y); y += 38.0f;
        drawLabelValue("SLOWEST DIM", m_ExpensiveDimension ? m_ExpensiveDimension->Label + " / " + formatDouble(m_ExpensiveDimension->Value) + " MS" : "N/A", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("SLOWEST STYLE", m_SlowestStyle ? m_SlowestStyle->Label + " / " + formatDouble(m_SlowestStyle->Value) + " MS" : "N/A", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("SLOWEST FACTION", m_SlowestFaction ? m_SlowestFaction->Label + " / " + formatDouble(m_SlowestFaction->Value) + " MS" : "N/A", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("COSTLIEST STAGE", m_ExpensiveStage ? m_ExpensiveStage->Label + " / " + formatDouble(m_ExpensiveStage->Value) + " MS" : "N/A", RightX + 14.0f, y, false, 190.0f);
    }

    void DiagnosticsApp::drawPerformanceView(const SpectralShipGenDiagnostics::DiagnosticsResult&)
    {
        const std::vector<SpectralShipGenStudioApplication::ChartSeries> resolution = { toChartSeries(m_ResolutionSeries, "CURRENT") };
        appendChartHits(0u, SpectralShipGenStudioApplication::drawLineChart(m_Window, { RightX + 14.0f, 190.0f, RightWidth - 28.0f, 245.0f }, "PERFORMANCE VS RESOLUTION", resolution, SpectralShipGenDiagnostics::getDiagnosticsMetricUnit(m_SelectedMetric), chartColors(), m_SelectedChartSeries, m_SelectedChartValue));

        const std::vector<SpectralShipGenStudioApplication::ChartSeries> styles = { toChartSeries(m_StyleSeries, "STYLES") };
        const std::vector<SpectralShipGenStudioApplication::ChartSeries> factions = { toChartSeries(m_FactionSeries, "FACTIONS") };
        const float halfWidth = (RightWidth - 38.0f) * 0.5f;
        appendChartHits(1u, SpectralShipGenStudioApplication::drawBarChart(m_Window, { RightX + 14.0f, 448.0f, halfWidth, 190.0f }, "STYLE COMPARISON", styles, SpectralShipGenDiagnostics::getDiagnosticsMetricUnit(m_SelectedMetric), chartColors(), m_SelectedChartSeries, m_SelectedChartValue, true));
        appendChartHits(2u, SpectralShipGenStudioApplication::drawBarChart(m_Window, { RightX + 24.0f + halfWidth, 448.0f, halfWidth, 190.0f }, "FACTION COMPARISON", factions, SpectralShipGenDiagnostics::getDiagnosticsMetricUnit(m_SelectedMetric), chartColors(), m_SelectedChartSeries, m_SelectedChartValue, true));

        SpectralShipGenDiagnostics::DiagnosticsChartSeries topStages = m_StageSeries;
        std::sort(topStages.Points.begin(), topStages.Points.end(), [](const auto& left, const auto& right) { return left.Value > right.Value; });
        if (topStages.Points.size() > 5u) { topStages.Points.resize(5u); }
        SpectralShipGenStudioApplication::drawBarChart(m_Window, { RightX + 14.0f, 651.0f, RightWidth - 28.0f, 142.0f }, "TOP STAGE MEAN TIME", { toChartSeries(topStages, "STAGES") }, "MS", chartColors(), std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max(), true);
        drawSelectedChartDetail(806.0f);
    }

    void DiagnosticsApp::drawRetriesView(const SpectralShipGenDiagnostics::DiagnosticsResult& result)
    {
        (void)result;
        appendChartHits(0u, SpectralShipGenStudioApplication::drawLineChart(m_Window, { RightX + 14.0f, 190.0f, RightWidth - 28.0f, 270.0f }, "HULL RETRY RATE VS RESOLUTION", { toChartSeries(m_RetryResolutionSeries, "RETRY RATE") }, "%", chartColors(), m_SelectedChartSeries, m_SelectedChartValue));
        appendChartHits(1u, SpectralShipGenStudioApplication::drawBarChart(m_Window, { RightX + 14.0f, 475.0f, RightWidth - 28.0f, 300.0f }, "TASK-56 REJECTION REASONS", { toChartSeries(m_RejectionSeries, "COUNT") }, "COUNT", chartColors(), m_SelectedChartSeries, m_SelectedChartValue, true));
        const auto& summary = m_FilteredCache.Summary;
        SpectralShipGenStudioApplication::drawPixelText(m_Window, "NEG SPACE ATTEMPT " + formatDouble(summary.StructuralNegativeSpaceAttemptRatePercent) + "%   SUCCESS " + formatDouble(summary.StructuralNegativeSpaceSuccessRatePercent) + "%", RightX + 14.0f, 798.0f, Text, SmallTextScale);
    }

    void DiagnosticsApp::drawCompositionView(const SpectralShipGenDiagnostics::DiagnosticsResult& result)
    {
        appendChartHits(0u, SpectralShipGenStudioApplication::drawBarChart(m_Window, { RightX + 14.0f, 190.0f, RightWidth - 28.0f, 285.0f }, "PRIMARY VISUAL ANCHOR FREQUENCY", { toChartSeries(m_AnchorSeries, "PRIMARY") }, "%", chartColors(), m_SelectedChartSeries, m_SelectedChartValue, false));
        (void)result;
        appendChartHits(1u, SpectralShipGenStudioApplication::drawBarChart(m_Window, { RightX + 14.0f, 490.0f, RightWidth - 28.0f, 285.0f }, "MATERIAL ZONES BY STYLE", { toChartSeries(m_MaterialStyleSeries, "ZONES") }, "AVG", chartColors(), m_SelectedChartSeries, m_SelectedChartValue, false));
        drawSelectedChartDetail(794.0f);
    }

    void DiagnosticsApp::drawNumbersView(const SpectralShipGenDiagnostics::DiagnosticsResult&, const DiagnosticsAppSnapshot& snapshot)
    {
        const auto& summary = m_FilteredCache.Summary;
        float y = 198.0f;
        drawSectionTitle("NUMBERS", RightX + 14.0f, y); y += 44.0f;
        drawLabelValue("FILTERED SAMPLES", std::to_string(m_FilteredCache.Samples.size()), RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("RESULT", snapshot.ResultCancelled ? "PARTIAL / CANCELLED" : "COMPLETE", RightX + 14.0f, y, true); y += 38.0f;
        drawLabelValue("GEN AVERAGE", formatDouble(summary.GenerationTimeMilliseconds.Mean) + " MS", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("GEN MEDIAN", formatDouble(summary.GenerationTimeMilliseconds.Median) + " MS", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("GEN P95", formatDouble(summary.GenerationTimeMilliseconds.P95) + " MS", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("GEN MAXIMUM", formatDouble(summary.GenerationTimeMilliseconds.Maximum) + " MS", RightX + 14.0f, y); y += 38.0f;
        drawLabelValue("HULL AVG ATTEMPTS", formatDouble(summary.HullAttempts.Mean), RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("HULL RETRY RATE", formatDouble(summary.HullRetryRatePercent) + "%", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("MOST COMMON REJECT", SpectralShipGen::getSilhouetteValidationFailureReasonName(summary.MostCommonSilhouetteRejection), RightX + 14.0f, y); y += 38.0f;
        drawLabelValue("NEG SPACE ATTEMPT", formatDouble(summary.StructuralNegativeSpaceAttemptRatePercent) + "%", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("NEG SPACE SUCCESS", formatDouble(summary.StructuralNegativeSpaceSuccessRatePercent) + "%", RightX + 14.0f, y); y += 38.0f;
        drawLabelValue("MATERIAL ZONES AVG", formatDouble(summary.MaterialZoneCount.Mean), RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("LIVERY COVERAGE AVG", formatDouble(summary.LiveryCoveragePercent.Mean) + "%", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("LIVERY COVERAGE MED", formatDouble(summary.LiveryCoveragePercent.Median) + "%", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("LIVERY COVERAGE P95", formatDouble(summary.LiveryCoveragePercent.P95) + "%", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("LIVERY CONNECTED P95", formatDouble(summary.LiveryLargestConnectedCoveragePercent.P95) + "%", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("MAJOR FEATURES AVG", formatDouble(summary.MajorFeatureCount.Mean), RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("WEAPONS AVG", formatDouble(summary.WeaponCount.Mean), RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("ENGINES AVG", formatDouble(summary.EngineCount.Mean), RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("COMPLEXITY USE", formatDouble(summary.ComplexityUtilizationPercent.Mean) + "%", RightX + 14.0f, y);
    }

    void DiagnosticsApp::drawComparisonView(const SpectralShipGenDiagnostics::DiagnosticsResult& result)
    {
        if (!m_BaselineResult)
        {
            drawSectionTitle("COMPARISON", RightX + 14.0f, 198.0f);
            SpectralShipGenStudioApplication::drawPixelText(m_Window, "LOAD spectral_ship_gen_baseline.shipdiag.json USING LOAD BASELINE.", RightX + 14.0f, 250.0f, Muted, SmallTextScale);
            return;
        }
        const auto aligned = alignComparisonSeries(m_BaselineResolutionSeries, m_ResolutionSeries);
        appendChartHits(0u, SpectralShipGenStudioApplication::drawLineChart(m_Window, { RightX + 14.0f, 190.0f, RightWidth - 28.0f, 320.0f }, "BASELINE VS CURRENT / RESOLUTION", { aligned.first, aligned.second }, SpectralShipGenDiagnostics::getDiagnosticsMetricUnit(m_SelectedMetric), chartColors(), m_SelectedChartSeries, m_SelectedChartValue));
        (void)result;
        const auto& delta = m_ComparisonDelta;
        float y = 535.0f;
        drawSectionTitle("DELTA", RightX + 14.0f, y); y += 44.0f;
        SpectralShipGenStudioApplication::drawPixelText(m_Window, m_ComparisonCompatibility.Message, RightX + 14.0f, y, m_ComparisonCompatibility.HasComparableData ? Muted : Negative, SmallTextScale); y += 36.0f;
        if (!delta.Available)
        {
            SpectralShipGenStudioApplication::drawPixelText(m_Window, "NO MATCHING DATA FOR CURRENT FILTER.", RightX + 14.0f, y, Negative, SmallTextScale);
            return;
        }
        drawLabelValue("BASELINE", formatMetric(delta.Baseline, m_SelectedMetric), RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("CURRENT", formatMetric(delta.Current, m_SelectedMetric), RightX + 14.0f, y, true); y += RowHeight;
        drawLabelValue("ABSOLUTE DELTA", signedDouble(delta.Absolute) + " " + SpectralShipGenDiagnostics::getDiagnosticsMetricUnit(m_SelectedMetric), RightX + 14.0f, y); y += RowHeight;
        if (delta.PercentagePointMetric) { drawLabelValue("POINT DELTA", signedDouble(delta.PercentagePointDelta) + " PP", RightX + 14.0f, y); y += RowHeight; }
        drawLabelValue("RELATIVE DELTA", delta.RelativeAvailable ? signedDouble(delta.RelativePercent) + "%" : "N/A", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("MATCHED SAMPLES", std::to_string(delta.BaselineSamples) + " / " + std::to_string(delta.CurrentSamples), RightX + 14.0f, y);
    }

    void DiagnosticsApp::drawSelectedChartDetail(float y)
    {
        if (m_SelectedChartId == std::numeric_limits<uint32_t>::max())
        {
            SpectralShipGenStudioApplication::drawPixelText(m_Window, "CLICK A POINT OR BAR FOR EXACT VALUES.", RightX + 14.0f, y, Muted, SmallTextScale);
            return;
        }
        const SpectralShipGenDiagnostics::DiagnosticsChartSeries* series = nullptr;
        if (m_DashboardView == DashboardView::PERFORMANCE)
        {
            if (m_SelectedChartId == 0u) { series = &m_ResolutionSeries; }
            else if (m_SelectedChartId == 1u) { series = &m_StyleSeries; }
            else if (m_SelectedChartId == 2u) { series = &m_FactionSeries; }
        }
        else if (m_DashboardView == DashboardView::COMPOSITION)
        {
            if (m_SelectedChartId == 0u) { series = &m_AnchorSeries; }
            else if (m_SelectedChartId == 1u) { series = &m_MaterialStyleSeries; }
        }
        if (!series || m_SelectedChartValue >= series->Points.size()) { return; }
        const auto& point = series->Points[m_SelectedChartValue];
        const std::string value = (m_DashboardView == DashboardView::COMPOSITION && m_SelectedChartId == 0u) ? formatDouble(point.Value) + " %" : formatMetric(point.Value, series->Metric);
        SpectralShipGenStudioApplication::drawPixelText(m_Window, point.Label + " = " + value + " / N=" + std::to_string(point.SampleCount), RightX + 14.0f, y, Highlight, SmallTextScale);
    }

    void DiagnosticsApp::drawButton(const sf::FloatRect& bounds, const std::string& label, bool active, bool enabled)
    {
        const bool hovered = contains(bounds, m_MousePosition);
        sf::RectangleShape shape(sf::Vector2f(bounds.width, bounds.height));
        shape.setPosition(bounds.left, bounds.top);
        shape.setFillColor(!enabled ? sf::Color(43u, 46u, 57u) : (active ? sf::Color(47u, 92u, 116u) : (hovered ? sf::Color(55u, 62u, 79u) : sf::Color(42u, 47u, 60u))));
        shape.setOutlineThickness(1.0f); shape.setOutlineColor(active ? Accent : PanelOutline); m_Window.draw(shape);
        const float textWidth = getPixelTextVisualWidth(label, SmallTextScale);
        const float textY = bounds.height >= 40.0f ? getCenteredPixelTextY(bounds, SmallTextScale) : bounds.top + 7.0f;
        SpectralShipGenStudioApplication::drawPixelText(m_Window, label, bounds.left + std::max(5.0f, (bounds.width - textWidth) * 0.5f), textY, enabled ? (active ? sf::Color::White : Text) : sf::Color(90u, 94u, 105u), SmallTextScale);
    }

    void DiagnosticsApp::drawSectionTitle(const std::string& label, float x, float y) { SpectralShipGenStudioApplication::drawPixelText(m_Window, label, x, y, Accent, TextScale); }

    void DiagnosticsApp::drawLabelValue(const std::string& label, const std::string& value, float x, float y, bool emphasize, float valueOffset)
    {
        SpectralShipGenStudioApplication::drawPixelText(m_Window, label, x, y, Muted, SmallTextScale);
        SpectralShipGenStudioApplication::drawPixelText(m_Window, value, x + valueOffset, y, emphasize ? Highlight : Text, SmallTextScale);
    }

    void DiagnosticsApp::drawProgressBar(const DiagnosticsAppSnapshot& snapshot, float x, float y, float width, float height)
    {
        sf::RectangleShape background(sf::Vector2f(width, height)); background.setPosition(x, y); background.setFillColor(sf::Color(18u, 20u, 27u)); background.setOutlineThickness(1.0f); background.setOutlineColor(PanelOutline); m_Window.draw(background);
        const float fraction = static_cast<float>(std::clamp(snapshot.Progress.ProgressPercent / 100.0, 0.0, 1.0));
        sf::RectangleShape fill(sf::Vector2f((width - 2.0f) * fraction, height - 2.0f)); fill.setPosition(x + 1.0f, y + 1.0f); fill.setFillColor(snapshot.State == DiagnosticsAppRunState::CANCELLING ? Highlight : (snapshot.State == DiagnosticsAppRunState::ERROR ? Negative : Accent)); m_Window.draw(fill);
    }

    void DiagnosticsApp::appendChartHits(uint32_t chartId, const std::vector<SpectralShipGenStudioApplication::ChartHitRegion>& hits)
    {
        for (const auto& hit : hits) { m_ChartHits.push_back({ hit.Bounds, chartId, hit.SeriesIndex, hit.ValueIndex }); }
    }

    SpectralShipGenDiagnostics::DiagnosticsRunConfiguration DiagnosticsApp::createConfiguration() const
    {
        SpectralShipGenDiagnostics::DiagnosticsRunConfiguration configuration;
        configuration.Dimensions.clear(); for (std::size_t index = 0u; index < m_Dimensions.size(); ++index) { if (m_DimensionSelected[index]) { configuration.Dimensions.push_back(m_Dimensions[index]); } }
        configuration.Styles.clear(); for (std::size_t index = 0u; index < m_StyleSelected.size(); ++index) { if (m_StyleSelected[index]) { configuration.Styles.push_back(static_cast<SpectralShipGen::ShipStyle>(index)); } }
        configuration.Factions.clear(); for (std::size_t index = 0u; index < m_FactionSelected.size(); ++index) { if (m_FactionSelected[index]) { configuration.Factions.push_back(static_cast<SpectralShipGen::ShipFactionType>(index)); } }
        configuration.SamplesPerConfiguration = m_SamplesPerCombination;
        configuration.DiagnosticSeed = m_DiagnosticSeed;
        configuration.DetailedPerformanceInstrumentation = m_DetailedTiming;
        configuration.DetailLevel = SpectralShipGenDiagnostics::DiagnosticsDetailLevel::RAW_SAMPLES_AND_SUMMARY;
#ifdef SPECTRAL_SHIP_GEN_BUILD_CONFIGURATION
        configuration.BuildConfiguration = SPECTRAL_SHIP_GEN_BUILD_CONFIGURATION;
#endif
        return configuration;
    }

    SpectralShipGenDiagnostics::DiagnosticsFilter DiagnosticsApp::currentFilter() const
    {
        SpectralShipGenDiagnostics::DiagnosticsFilter filter;
        if (!m_AnalysisResult) { return filter; }
        if (m_FilterDimensionIndex >= 0 && static_cast<std::size_t>(m_FilterDimensionIndex) < m_AnalysisResult->Configuration.Dimensions.size()) { filter.Dimensions = m_AnalysisResult->Configuration.Dimensions[static_cast<std::size_t>(m_FilterDimensionIndex)]; }
        if (m_FilterStyleIndex >= 0 && static_cast<std::size_t>(m_FilterStyleIndex) < m_AnalysisResult->Configuration.Styles.size()) { filter.Style = m_AnalysisResult->Configuration.Styles[static_cast<std::size_t>(m_FilterStyleIndex)]; }
        if (m_FilterFactionIndex >= 0 && static_cast<std::size_t>(m_FilterFactionIndex) < m_AnalysisResult->Configuration.Factions.size()) { filter.Faction = m_AnalysisResult->Configuration.Factions[static_cast<std::size_t>(m_FilterFactionIndex)]; }
        return filter;
    }

    void DiagnosticsApp::updateAnalysisCache()
    {
        const auto current = m_Controller.getResult();
        if (current.get() != m_AnalysisResult.get())
        {
            m_AnalysisResult = current;
            m_FilterDimensionIndex = -1; m_FilterStyleIndex = -1; m_FilterFactionIndex = -1;
            markAnalysisDirty();
        }
        if (!m_AnalysisDirty || !m_AnalysisResult) { return; }
        const auto filter = currentFilter();
        m_FilteredCache = SpectralShipGenDiagnostics::filterDiagnosticsResult(*m_AnalysisResult, filter);
        m_ResolutionSeries = SpectralShipGenDiagnostics::prepareResolutionSeries(*m_AnalysisResult, filter, m_SelectedMetric);
        m_StyleSeries = SpectralShipGenDiagnostics::prepareStyleSeries(*m_AnalysisResult, filter, m_SelectedMetric);
        m_FactionSeries = SpectralShipGenDiagnostics::prepareFactionSeries(*m_AnalysisResult, filter, m_SelectedMetric);
        m_StageSeries = SpectralShipGenDiagnostics::prepareStageSeries(*m_AnalysisResult, filter);
        m_RejectionSeries = SpectralShipGenDiagnostics::prepareSilhouetteRejectionSeries(*m_AnalysisResult, filter);
        m_AnchorSeries = SpectralShipGenDiagnostics::prepareVisualAnchorSeries(*m_AnalysisResult, filter);
        m_RetryResolutionSeries = SpectralShipGenDiagnostics::prepareResolutionSeries(*m_AnalysisResult, filter, SpectralShipGenDiagnostics::DiagnosticsMetric::HULL_RETRY_RATE_PERCENT);
        m_MaterialStyleSeries = SpectralShipGenDiagnostics::prepareStyleSeries(*m_AnalysisResult, filter, SpectralShipGenDiagnostics::DiagnosticsMetric::MATERIAL_ZONE_AVERAGE);
        m_ExpensiveDimension = SpectralShipGenDiagnostics::findMostExpensiveDimension(*m_AnalysisResult, filter);
        m_SlowestStyle = SpectralShipGenDiagnostics::findSlowestStyle(*m_AnalysisResult, filter);
        m_SlowestFaction = SpectralShipGenDiagnostics::findSlowestFaction(*m_AnalysisResult, filter);
        m_ExpensiveStage = SpectralShipGenDiagnostics::findMostExpensiveStage(*m_AnalysisResult, filter);
        if (m_BaselineResult)
        {
            m_BaselineResolutionSeries = SpectralShipGenDiagnostics::prepareResolutionSeries(*m_BaselineResult, filter, m_SelectedMetric);
            m_ComparisonCompatibility = SpectralShipGenDiagnostics::evaluateDiagnosticsCompatibility(*m_BaselineResult, *m_AnalysisResult);
            m_ComparisonDelta = SpectralShipGenDiagnostics::compareDiagnosticsMetric(*m_BaselineResult, *m_AnalysisResult, filter, m_SelectedMetric);
        }
        else { m_BaselineResolutionSeries = {}; m_ComparisonCompatibility = {}; m_ComparisonDelta = {}; }
        m_SelectedChartId = std::numeric_limits<uint32_t>::max();
        m_AnalysisDirty = false;
    }

    void DiagnosticsApp::markAnalysisDirty() { m_AnalysisDirty = true; }

    void DiagnosticsApp::cycleDimensionFilter(int direction)
    {
        if (!m_AnalysisResult || m_AnalysisResult->Configuration.Dimensions.empty()) { return; }
        const int count = static_cast<int>(m_AnalysisResult->Configuration.Dimensions.size());
        int value = m_FilterDimensionIndex + 1;
        value = (value + direction + count + 1) % (count + 1);
        m_FilterDimensionIndex = value - 1; markAnalysisDirty();
    }

    void DiagnosticsApp::cycleStyleFilter(int direction)
    {
        if (!m_AnalysisResult || m_AnalysisResult->Configuration.Styles.empty()) { return; }
        const int count = static_cast<int>(m_AnalysisResult->Configuration.Styles.size());
        int value = m_FilterStyleIndex + 1; value = (value + direction + count + 1) % (count + 1); m_FilterStyleIndex = value - 1; markAnalysisDirty();
    }

    void DiagnosticsApp::cycleFactionFilter(int direction)
    {
        if (!m_AnalysisResult || m_AnalysisResult->Configuration.Factions.empty()) { return; }
        const int count = static_cast<int>(m_AnalysisResult->Configuration.Factions.size());
        int value = m_FilterFactionIndex + 1; value = (value + direction + count + 1) % (count + 1); m_FilterFactionIndex = value - 1; markAnalysisDirty();
    }

    void DiagnosticsApp::cycleMetric(int direction)
    {
        const int count = static_cast<int>(SpectralShipGenDiagnostics::DiagnosticsMetric::COMPLEXITY_UTILIZATION_PERCENT) + 1;
        int value = static_cast<int>(m_SelectedMetric); value = (value + direction + count) % count; m_SelectedMetric = static_cast<SpectralShipGenDiagnostics::DiagnosticsMetric>(value); markAnalysisDirty();
    }

    std::string DiagnosticsApp::dimensionFilterName() const
    {
        if (!m_AnalysisResult || m_FilterDimensionIndex < 0 || static_cast<std::size_t>(m_FilterDimensionIndex) >= m_AnalysisResult->Configuration.Dimensions.size()) { return "ALL"; }
        return dimensionsName(m_AnalysisResult->Configuration.Dimensions[static_cast<std::size_t>(m_FilterDimensionIndex)]);
    }

    std::string DiagnosticsApp::styleFilterName() const
    {
        if (!m_AnalysisResult || m_FilterStyleIndex < 0 || static_cast<std::size_t>(m_FilterStyleIndex) >= m_AnalysisResult->Configuration.Styles.size()) { return "ALL"; }
        return styleName(m_AnalysisResult->Configuration.Styles[static_cast<std::size_t>(m_FilterStyleIndex)]);
    }

    std::string DiagnosticsApp::factionFilterName() const
    {
        if (!m_AnalysisResult || m_FilterFactionIndex < 0 || static_cast<std::size_t>(m_FilterFactionIndex) >= m_AnalysisResult->Configuration.Factions.size()) { return "ALL"; }
        return factionName(m_AnalysisResult->Configuration.Factions[static_cast<std::size_t>(m_FilterFactionIndex)]);
    }

    std::string DiagnosticsApp::viewName(DashboardView view)
    {
        switch (view)
        {
        case DashboardView::OVERVIEW: return "OVERVIEW";
        case DashboardView::PERFORMANCE: return "PERFORMANCE";
        case DashboardView::RETRIES: return "RETRIES";
        case DashboardView::COMPOSITION: return "COMPOSITION";
        case DashboardView::NUMBERS: return "NUMBERS";
        case DashboardView::COMPARISON: return "COMPARISON";
        default: return "UNKNOWN";
        }
    }

    std::string DiagnosticsApp::actionLabel(UiActionType action, const DiagnosticsAppSnapshot&)
    {
        switch (action)
        {
        case UiActionType::START: return "START DIAGNOSTICS [ENTER]";
        case UiActionType::LOAD_RUN: return "LOAD RUN [L]";
        case UiActionType::CANCEL: return "CANCEL [ESC]";
        case UiActionType::NEW_RUN: return "NEW RUN";
        case UiActionType::EXPORT_CSV: return "EXPORT CSV [E]";
        case UiActionType::SAVE_RUN: return "SAVE RUN [S]";
        case UiActionType::LOAD_BASELINE: return "LOAD BASELINE [B]";
        default: return "";
        }
    }

    void DiagnosticsApp::startDiagnostics()
    {
        std::string error;
        if (m_Controller.start(createConfiguration(), error)) { m_LocalStatus = "Diagnostics running on one sequential worker thread."; markAnalysisDirty(); }
        else { m_LocalStatus = error; }
    }

    void DiagnosticsApp::exportCsv()
    {
        const std::filesystem::path outputPath = "spectral_ship_gen_diagnostics.csv";
        std::string error;
        if (m_Controller.exportCsv(outputPath, error)) { m_LocalStatus = "Exported full CSV: " + outputPath.string(); }
        else { m_LocalStatus = error; }
    }

    void DiagnosticsApp::saveRun()
    {
        const std::filesystem::path outputPath = "spectral_ship_gen_diagnostics.shipdiag.json";
        std::string error;
        if (m_Controller.saveRun(outputPath, error)) { m_LocalStatus = "Saved diagnostics run: " + outputPath.string(); }
        else { m_LocalStatus = error; }
    }

    void DiagnosticsApp::loadRun()
    {
        const std::filesystem::path inputPath = "spectral_ship_gen_diagnostics.shipdiag.json";
        std::string error;
        if (m_Controller.loadRun(inputPath, error)) { m_LocalStatus = "Loaded diagnostics run: " + inputPath.string(); m_DashboardView = DashboardView::OVERVIEW; markAnalysisDirty(); }
        else { m_LocalStatus = error; }
    }

    void DiagnosticsApp::loadBaseline()
    {
        const std::filesystem::path inputPath = "spectral_ship_gen_baseline.shipdiag.json";
        auto loaded = SpectralShipGenDiagnostics::loadDiagnosticsResultJson(inputPath);
        if (!loaded.Success) { m_LocalStatus = loaded.Error; return; }
        m_BaselineResult = std::make_shared<SpectralShipGenDiagnostics::DiagnosticsResult>(std::move(loaded.Result));
        m_LocalStatus = "Loaded comparison baseline: " + inputPath.string();
        m_DashboardView = DashboardView::COMPARISON;
        markAnalysisDirty();
    }

    void DiagnosticsApp::captureScreenshot(const std::filesystem::path& path)
    {
        if (path.empty()) { return; }
        sf::Texture texture; if (!texture.create(WindowWidth, WindowHeight)) { return; } texture.update(m_Window); const sf::Image image = texture.copyToImage(); image.saveToFile(path.string());
    }

    void DiagnosticsApp::configureAutomatedSmoke()
    {
        std::fill(m_DimensionSelected.begin(), m_DimensionSelected.end(), false); m_DimensionSelected[5u] = true;
        m_StyleSelected.fill(false); m_StyleSelected[static_cast<std::size_t>(SpectralShipGen::ShipStyle::FIGHTER)] = true; m_StyleSelected[static_cast<std::size_t>(SpectralShipGen::ShipStyle::INDUSTRIAL)] = true;
        m_FactionSelected.fill(false); m_FactionSelected[static_cast<std::size_t>(SpectralShipGen::ShipFactionType::MILITARY)] = true; m_FactionSelected[static_cast<std::size_t>(SpectralShipGen::ShipFactionType::CORPORATE)] = true;
        m_SamplesPerCombination = m_Options.CancelSmoke ? 250u : 12u; m_DetailedTiming = true;
    }

    void DiagnosticsApp::updateAutomatedSmoke(const DiagnosticsAppSnapshot& snapshot)
    {
        if (!m_AutomatedStarted) { m_AutomatedStarted = true; startDiagnostics(); return; }
        if (m_Options.CancelSmoke && !m_AutomatedCancelRequested && snapshot.State == DiagnosticsAppRunState::RUNNING && snapshot.CompletedSamples >= 3u) { m_AutomatedCancelRequested = true; m_Controller.requestCancel(); return; }
        if (m_AutomatedFinalized) { return; }
        if (snapshot.State == DiagnosticsAppRunState::COMPLETED || snapshot.State == DiagnosticsAppRunState::CANCELLED || snapshot.State == DiagnosticsAppRunState::ERROR)
        {
            m_AutomatedFinalized = true;
            if (!m_Options.SmokeCsvPath.empty()) { std::string error; m_Controller.exportCsv(m_Options.SmokeCsvPath, error); }
            render(); captureScreenshot(m_Options.ScreenshotPath); m_Window.close();
        }
    }

    std::string DiagnosticsApp::styleName(SpectralShipGen::ShipStyle style)
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

    std::string DiagnosticsApp::factionName(SpectralShipGen::ShipFactionType faction)
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

    std::string DiagnosticsApp::dimensionsName(SpectralShipGen::ShipDimensions dimensions) { return std::to_string(dimensions.Width) + "X" + std::to_string(dimensions.Height); }

    std::string DiagnosticsApp::formatDuration(uint64_t nanoseconds, bool available)
    {
        if (!available) { return "ESTIMATING..."; }
        uint64_t totalSeconds = nanoseconds / 1000000000ull; const uint64_t hours = totalSeconds / 3600u; totalSeconds %= 3600u; const uint64_t minutes = totalSeconds / 60u; const uint64_t seconds = totalSeconds % 60u;
        std::ostringstream stream; if (hours > 0u) { stream << hours << ':' << std::setfill('0') << std::setw(2) << minutes << ':' << std::setw(2) << seconds; }
        else { stream << std::setfill('0') << std::setw(2) << minutes << ':' << std::setw(2) << seconds; } return stream.str();
    }

    std::string DiagnosticsApp::formatMetric(double value, SpectralShipGenDiagnostics::DiagnosticsMetric metric)
    {
        const std::string unit = SpectralShipGenDiagnostics::getDiagnosticsMetricUnit(metric);
        return formatDouble(value) + (unit[0] == '\0' ? "" : " " + unit);
    }

    uint32_t DiagnosticsApp::clampDimension(uint32_t value) { return std::clamp<uint32_t>(value, 24u, 256u); }
}
