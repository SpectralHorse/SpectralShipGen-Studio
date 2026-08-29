#include "DiagnosticsApp.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

#include "Application/SFMLPixelText.h"
#include "ShipGenerationPerformance.h"

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
        return std::max(0.0f, PixelShipGeneratorApplication::getPixelTextWidth(text, scale) - static_cast<float>(scale));
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

    void drawPanel(sf::RenderTarget& target, float x, float y, float width, float height)
    {
        sf::RectangleShape panel(sf::Vector2f(width, height));
        panel.setPosition(x, y);
        panel.setFillColor(PanelFill);
        panel.setOutlineThickness(1.0f);
        panel.setOutlineColor(PanelOutline);
        target.draw(panel);
    }
}

namespace PixelShipGeneratorDiagnosticsApp
{
    DiagnosticsApp::DiagnosticsApp(DiagnosticsAppLaunchOptions options)
        : m_Window(sf::VideoMode(WindowWidth, WindowHeight), "Pixel Ship Generator Diagnostics", sf::Style::Titlebar | sf::Style::Close),
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
        else if (key == sf::Keyboard::Enter && snapshot.State == DiagnosticsAppRunState::READY)
        {
            startDiagnostics();
        }
        else if (key == sf::Keyboard::E && snapshot.HasResult)
        {
            exportCsv();
        }
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
    }

    void DiagnosticsApp::executeAction(UiActionType action, uint32_t value)
    {
        switch (action)
        {
        case UiActionType::TOGGLE_DIMENSION:
            if (value < m_DimensionSelected.size()) { m_DimensionSelected[value] = !m_DimensionSelected[value]; }
            break;
        case UiActionType::DIMENSIONS_SELECT_ALL: std::fill(m_DimensionSelected.begin(), m_DimensionSelected.end(), true); break;
        case UiActionType::DIMENSIONS_CLEAR_ALL: std::fill(m_DimensionSelected.begin(), m_DimensionSelected.end(), false); break;
        case UiActionType::CUSTOM_WIDTH_DECREASE: m_CustomWidth = clampDimension(m_CustomWidth > 2u ? m_CustomWidth - 2u : 24u); break;
        case UiActionType::CUSTOM_WIDTH_INCREASE: m_CustomWidth = clampDimension(m_CustomWidth + 2u); break;
        case UiActionType::CUSTOM_HEIGHT_DECREASE: m_CustomHeight = clampDimension(m_CustomHeight > 2u ? m_CustomHeight - 2u : 24u); break;
        case UiActionType::CUSTOM_HEIGHT_INCREASE: m_CustomHeight = clampDimension(m_CustomHeight + 2u); break;
        case UiActionType::ADD_CUSTOM_DIMENSION:
        {
            const PixelShipGenerator::ShipDimensions dimensions{ m_CustomWidth, m_CustomHeight };
            const auto found = std::find(m_Dimensions.begin(), m_Dimensions.end(), dimensions);
            if (found != m_Dimensions.end())
            {
                m_DimensionSelected[static_cast<std::size_t>(std::distance(m_Dimensions.begin(), found))] = true;
            }
            else if (m_Dimensions.size() < 10u)
            {
                m_Dimensions.push_back(dimensions);
                m_DimensionSelected.push_back(true);
            }
            else
            {
                m_Dimensions.back() = dimensions;
                m_DimensionSelected.back() = true;
            }
            break;
        }
        case UiActionType::TOGGLE_STYLE:
            if (value < m_StyleSelected.size()) { m_StyleSelected[value] = !m_StyleSelected[value]; }
            break;
        case UiActionType::STYLES_SELECT_ALL: m_StyleSelected.fill(true); break;
        case UiActionType::STYLES_CLEAR_ALL: m_StyleSelected.fill(false); break;
        case UiActionType::TOGGLE_FACTION:
            if (value < m_FactionSelected.size()) { m_FactionSelected[value] = !m_FactionSelected[value]; }
            break;
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
        case UiActionType::NEW_RUN: m_Controller.reset(); m_LocalStatus = "Ready for a new diagnostics run."; break;
        case UiActionType::EXPORT_CSV: exportCsv(); break;
        }
    }

    void DiagnosticsApp::update()
    {
        const DiagnosticsAppSnapshot snapshot = m_Controller.getSnapshot();
        rebuildControls(snapshot);
        if (m_Options.AutomatedSmoke) { updateAutomatedSmoke(snapshot); }
    }

    void DiagnosticsApp::render()
    {
        const DiagnosticsAppSnapshot snapshot = m_Controller.getSnapshot();
        m_Window.clear(Background);
        drawConfigurationPanel(snapshot);
        drawRunPanel(snapshot);
        drawSummaryPanel(snapshot);
        PixelShipGeneratorApplication::drawPixelText(m_Window, "PIXEL SHIP GENERATOR - DIAGNOSTICS", Margin, 8.0f, Highlight, TextScale);
        const std::string status = !m_LocalStatus.empty() ? m_LocalStatus : snapshot.StatusMessage;
        if (!status.empty())
        {
            PixelShipGeneratorApplication::drawPixelText(m_Window, PixelShipGeneratorApplication::wrapPixelText(status, 160u), Margin, static_cast<float>(WindowHeight) - 36.0f, snapshot.State == DiagnosticsAppRunState::ERROR ? Negative : Muted, SmallTextScale);
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

        const float bottomY = 805.0f;
        if (snapshot.State == DiagnosticsAppRunState::READY)
        {
            m_Controls.push_back({ { startX, bottomY, 608.0f, 42.0f }, UiActionType::START, 0u, true });
        }
        else if (snapshot.State == DiagnosticsAppRunState::RUNNING || snapshot.State == DiagnosticsAppRunState::CANCELLING)
        {
            m_Controls.push_back({ { startX, bottomY, 608.0f, 42.0f }, UiActionType::CANCEL, 0u, snapshot.State == DiagnosticsAppRunState::RUNNING });
        }
        else
        {
            m_Controls.push_back({ { startX, bottomY, 294.0f, 42.0f }, UiActionType::NEW_RUN, 0u, true });
            m_Controls.push_back({ { startX + 314.0f, bottomY, 294.0f, 42.0f }, UiActionType::EXPORT_CSV, 0u, snapshot.HasResult });
        }
    }

    void DiagnosticsApp::drawConfigurationPanel(const DiagnosticsAppSnapshot& snapshot)
    {
        drawPanel(m_Window, Margin, 40.0f, ConfigWidth, 820.0f);
        drawSectionTitle("RUN CONFIGURATION", Margin + 14.0f, 50.0f);
        const bool editable = snapshot.State == DiagnosticsAppRunState::READY;
        float y = 90.0f;
        PixelShipGeneratorApplication::drawPixelText(m_Window, "DIMENSIONS", Margin + 14.0f, y - 16.0f, Accent, SmallTextScale);
        for (std::size_t index = 0u; index < m_Dimensions.size(); ++index)
        {
            const UiControl& control = m_Controls[index];
            drawButton(control.Bounds, dimensionsName(m_Dimensions[index]), m_DimensionSelected[index], editable);
        }
        y = 156.0f;
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

        PixelShipGeneratorApplication::drawPixelText(m_Window, "CUSTOM W", Margin + 14.0f, customLabelY, Muted, SmallTextScale);
        PixelShipGeneratorApplication::drawPixelText(
            m_Window,
            customWidthText,
            getCenteredPixelTextXBetween(widthDecreaseBounds, widthIncreaseBounds, customWidthText, TextScale),
            customValueY,
            Text,
            TextScale);
        PixelShipGeneratorApplication::drawPixelText(m_Window, "H", Margin + 248.0f, customLabelY, Muted, SmallTextScale);
        PixelShipGeneratorApplication::drawPixelText(
            m_Window,
            customHeightText,
            getCenteredPixelTextXBetween(heightDecreaseBounds, heightIncreaseBounds, customHeightText, TextScale),
            customValueY,
            Text,
            TextScale);

        std::size_t cursor = customControlsStart;
        for (std::size_t index = 0u; index < 5u; ++index) { drawButton(m_Controls[cursor + index].Bounds, index == 4u ? "ADD CUSTOM" : (index % 2u == 0u ? "-" : "+"), false, editable); }
        cursor += 5u;

        drawSectionTitle("STYLES", Margin + 14.0f, 238.0f);
        for (std::size_t index = 0u; index < m_StyleSelected.size(); ++index)
        {
            drawButton(m_Controls[cursor++].Bounds, styleName(static_cast<PixelShipGenerator::ShipStyle>(index)), m_StyleSelected[index], editable);
        }
        drawButton(m_Controls[cursor++].Bounds, "SELECT ALL", false, editable);
        drawButton(m_Controls[cursor++].Bounds, "CLEAR ALL", false, editable);

        drawSectionTitle("FACTIONS", Margin + 14.0f, 374.0f);
        for (std::size_t index = 0u; index < m_FactionSelected.size(); ++index)
        {
            drawButton(m_Controls[cursor++].Bounds, factionName(static_cast<PixelShipGenerator::ShipFactionType>(index)), m_FactionSelected[index], editable);
        }
        drawButton(m_Controls[cursor++].Bounds, "SELECT ALL", false, editable);
        drawButton(m_Controls[cursor++].Bounds, "CLEAR ALL", false, editable);

        drawSectionTitle("SAMPLING", Margin + 14.0f, 510.0f);
        drawLabelValue("SAMPLES / COMBO", std::to_string(m_SamplesPerCombination), Margin + 14.0f, 570.0f, true);
        drawButton(m_Controls[cursor++].Bounds, "-100", false, editable);
        drawButton(m_Controls[cursor++].Bounds, "-10", false, editable);
        drawButton(m_Controls[cursor++].Bounds, "+10", false, editable);
        drawButton(m_Controls[cursor++].Bounds, "+100", false, editable);
        drawLabelValue("DIAGNOSTIC SEED", std::to_string(m_DiagnosticSeed), Margin + 14.0f, 602.0f);
        drawButton(m_Controls[cursor++].Bounds, "-1", false, editable);
        drawButton(m_Controls[cursor++].Bounds, "+1", false, editable);
        drawLabelValue("DETAILED STAGE TIMING", m_DetailedTiming ? "ON" : "OFF", Margin + 14.0f, 634.0f, m_DetailedTiming);
        drawButton(m_Controls[cursor++].Bounds, m_DetailedTiming ? "DETAILED TIMING: ON" : "DETAILED TIMING: OFF", m_DetailedTiming, editable);

        const uint64_t dimensionsCount = static_cast<uint64_t>(std::count(m_DimensionSelected.begin(), m_DimensionSelected.end(), true));
        const uint64_t stylesCount = static_cast<uint64_t>(std::count(m_StyleSelected.begin(), m_StyleSelected.end(), true));
        const uint64_t factionsCount = static_cast<uint64_t>(std::count(m_FactionSelected.begin(), m_FactionSelected.end(), true));
        const uint64_t totalSamples = dimensionsCount * stylesCount * factionsCount * m_SamplesPerCombination;
        drawLabelValue("CONFIGURED WORK", std::to_string(totalSamples) + " SAMPLES", Margin + 14.0f, 690.0f, true);

        const UiControl& action = m_Controls.back();
        const bool running = snapshot.State == DiagnosticsAppRunState::RUNNING || snapshot.State == DiagnosticsAppRunState::CANCELLING;
        const std::string actionLabel = snapshot.State == DiagnosticsAppRunState::READY ? "START DIAGNOSTICS [ENTER]" : (running ? "CANCEL [ESC]" : "EXPORT CSV / NEW RUN");
        if (snapshot.State == DiagnosticsAppRunState::READY || running)
        {
            drawButton(action.Bounds, actionLabel, snapshot.State == DiagnosticsAppRunState::RUNNING, action.Enabled);
        }
        else
        {
            drawButton(m_Controls[m_Controls.size() - 2u].Bounds, "NEW RUN", false, true);
            drawButton(m_Controls.back().Bounds, "EXPORT CSV [E]", false, snapshot.HasResult);
        }
    }

    void DiagnosticsApp::drawRunPanel(const DiagnosticsAppSnapshot& snapshot)
    {
        drawPanel(m_Window, RightX, 40.0f, RightWidth, 390.0f);
        drawSectionTitle("LIVE RUN", RightX + 14.0f, 50.0f);
        drawLabelValue("STATE", getDiagnosticsAppRunStateName(snapshot.State), RightX + 14.0f, 82.0f, true);
        drawProgressBar(snapshot, RightX + 14.0f, 118.0f, RightWidth - 28.0f, 24.0f);
        drawLabelValue("PROGRESS", formatDouble(snapshot.Progress.ProgressPercent, 1) + "%", RightX + 14.0f, 154.0f);
        drawLabelValue("SAMPLES", std::to_string(snapshot.CompletedSamples) + " / " + std::to_string(snapshot.ScheduledSamples), RightX + 14.0f, 182.0f);
        drawLabelValue("ELAPSED", formatDuration(snapshot.Progress.ElapsedNanoseconds), RightX + 14.0f, 210.0f);
        drawLabelValue("ETA", formatDuration(snapshot.Progress.EstimatedRemainingNanoseconds, snapshot.Progress.EstimatedRemainingAvailable), RightX + 14.0f, 238.0f);
        if (snapshot.ScheduledSamples > 0u)
        {
            drawLabelValue("CURRENT", std::to_string(snapshot.Progress.CurrentWidth) + "X" + std::to_string(snapshot.Progress.CurrentHeight), RightX + 14.0f, 276.0f);
            drawLabelValue("STYLE", styleName(snapshot.Progress.CurrentStyle), RightX + 14.0f, 304.0f);
            drawLabelValue("FACTION", factionName(snapshot.Progress.CurrentFaction), RightX + 14.0f, 332.0f);
            const std::string stage = snapshot.Progress.CurrentStage == PixelShipGenerator::ShipGenerationPerformanceStage::SHIP_GENERATION_PERFORMANCE_STAGE_END ? "SAMPLE COMPLETE" : PixelShipGenerator::getShipGenerationPerformanceStageName(snapshot.Progress.CurrentStage);
            drawLabelValue("STAGE", stage, RightX + 14.0f, 360.0f);
        }
    }

    void DiagnosticsApp::drawSummaryPanel(const DiagnosticsAppSnapshot& snapshot)
    {
        drawPanel(m_Window, RightX, 448.0f, RightWidth, 412.0f);
        drawSectionTitle("NUMBERS", RightX + 14.0f, 458.0f);
        const bool final = snapshot.HasResult;
        const auto& summary = snapshot.FinalSummary;
        float y = 492.0f;
        drawLabelValue("SAMPLE COUNT", std::to_string(snapshot.CompletedSamples), RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("GEN AVG", formatDouble(final ? summary.GenerationTimeMilliseconds.Mean : snapshot.LiveSummary.AverageGenerationMilliseconds) + " MS", RightX + 14.0f, y, true); y += RowHeight;
        drawLabelValue("GEN MEDIAN", final ? formatDouble(summary.GenerationTimeMilliseconds.Median) + " MS" : "COLLECTING...", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("GEN P95", final ? formatDouble(summary.GenerationTimeMilliseconds.P95) + " MS" : "COLLECTING...", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("GEN MAX", final ? formatDouble(summary.GenerationTimeMilliseconds.Maximum) + " MS" : "COLLECTING...", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("HULL AVG ATTEMPTS", formatDouble(final ? summary.HullAttempts.Mean : snapshot.LiveSummary.AverageHullAttempts), RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("HULL RETRY / 100", formatDouble(final ? summary.HullRetryRatePercent : snapshot.LiveSummary.HullRetryRatePercent), RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("NEG SPACE ATTEMPT", formatDouble(final ? summary.StructuralNegativeSpaceAttemptRatePercent : snapshot.LiveSummary.NegativeSpaceAttemptRatePercent) + "%", RightX + 14.0f, y); y += RowHeight;
        drawLabelValue("NEG SPACE SUCCESS", formatDouble(final ? summary.StructuralNegativeSpaceSuccessRatePercent : snapshot.LiveSummary.NegativeSpaceSuccessRatePercent) + "%", RightX + 14.0f, y); y += RowHeight;
        if (final)
        {
            drawLabelValue("MATERIAL ZONES AVG", formatDouble(summary.MaterialZoneCount.Mean), RightX + 14.0f, y); y += RowHeight;
            drawLabelValue("RESULT", snapshot.ResultCancelled ? "PARTIAL / CANCELLED" : (snapshot.ResultCompleted ? "COMPLETE" : "INCOMPLETE"), RightX + 14.0f, y, true);
        }
    }

    void DiagnosticsApp::drawButton(const sf::FloatRect& bounds, const std::string& label, bool active, bool enabled)
    {
        const bool hovered = contains(bounds, m_MousePosition);
        sf::RectangleShape shape(sf::Vector2f(bounds.width, bounds.height));
        shape.setPosition(bounds.left, bounds.top);
        shape.setFillColor(!enabled ? sf::Color(43u, 46u, 57u) : (active ? sf::Color(47u, 92u, 116u) : (hovered ? sf::Color(55u, 62u, 79u) : sf::Color(42u, 47u, 60u))));
        shape.setOutlineThickness(1.0f);
        shape.setOutlineColor(active ? Accent : PanelOutline);
        m_Window.draw(shape);
        const float textWidth = PixelShipGeneratorApplication::getPixelTextWidth(label, SmallTextScale);
        const float textY = bounds.height >= 40.0f ? getCenteredPixelTextY(bounds, SmallTextScale) : bounds.top + 7.0f;
        PixelShipGeneratorApplication::drawPixelText(m_Window, label, bounds.left + std::max(5.0f, (bounds.width - textWidth) * 0.5f), textY, enabled ? (active ? sf::Color::White : Text) : sf::Color(90u, 94u, 105u), SmallTextScale);
    }

    void DiagnosticsApp::drawSectionTitle(const std::string& label, float x, float y)
    {
        PixelShipGeneratorApplication::drawPixelText(m_Window, label, x, y, Accent, TextScale);
    }

    void DiagnosticsApp::drawLabelValue(const std::string& label, const std::string& value, float x, float y, bool emphasize)
    {
        PixelShipGeneratorApplication::drawPixelText(m_Window, label, x, y, Muted, SmallTextScale);
        PixelShipGeneratorApplication::drawPixelText(m_Window, value, x + 190.0f, y, emphasize ? Highlight : Text, SmallTextScale);
    }

    void DiagnosticsApp::drawProgressBar(const DiagnosticsAppSnapshot& snapshot, float x, float y, float width, float height)
    {
        sf::RectangleShape background(sf::Vector2f(width, height));
        background.setPosition(x, y);
        background.setFillColor(sf::Color(18u, 20u, 27u));
        background.setOutlineThickness(1.0f);
        background.setOutlineColor(PanelOutline);
        m_Window.draw(background);
        const float fraction = static_cast<float>(std::clamp(snapshot.Progress.ProgressPercent / 100.0, 0.0, 1.0));
        sf::RectangleShape fill(sf::Vector2f((width - 2.0f) * fraction, height - 2.0f));
        fill.setPosition(x + 1.0f, y + 1.0f);
        fill.setFillColor(snapshot.State == DiagnosticsAppRunState::CANCELLING ? Highlight : (snapshot.State == DiagnosticsAppRunState::ERROR ? Negative : Accent));
        m_Window.draw(fill);
    }

    PixelShipGeneratorDiagnostics::DiagnosticsRunConfiguration DiagnosticsApp::createConfiguration() const
    {
        PixelShipGeneratorDiagnostics::DiagnosticsRunConfiguration configuration;
        configuration.Dimensions.clear();
        for (std::size_t index = 0u; index < m_Dimensions.size(); ++index) { if (m_DimensionSelected[index]) { configuration.Dimensions.push_back(m_Dimensions[index]); } }
        configuration.Styles.clear();
        for (std::size_t index = 0u; index < m_StyleSelected.size(); ++index) { if (m_StyleSelected[index]) { configuration.Styles.push_back(static_cast<PixelShipGenerator::ShipStyle>(index)); } }
        configuration.Factions.clear();
        for (std::size_t index = 0u; index < m_FactionSelected.size(); ++index) { if (m_FactionSelected[index]) { configuration.Factions.push_back(static_cast<PixelShipGenerator::ShipFactionType>(index)); } }
        configuration.SamplesPerConfiguration = m_SamplesPerCombination;
        configuration.DiagnosticSeed = m_DiagnosticSeed;
        configuration.DetailedPerformanceInstrumentation = m_DetailedTiming;
        configuration.DetailLevel = PixelShipGeneratorDiagnostics::DiagnosticsDetailLevel::RAW_SAMPLES_AND_SUMMARY;
#ifdef PIXEL_SHIP_GENERATOR_BUILD_CONFIGURATION
        configuration.BuildConfiguration = PIXEL_SHIP_GENERATOR_BUILD_CONFIGURATION;
#endif
        return configuration;
    }

    void DiagnosticsApp::startDiagnostics()
    {
        std::string error;
        if (m_Controller.start(createConfiguration(), error)) { m_LocalStatus = "Diagnostics running on one sequential worker thread."; }
        else { m_LocalStatus = error; }
    }

    void DiagnosticsApp::exportCsv()
    {
        const std::filesystem::path outputPath = "pixel_ship_generator_diagnostics.csv";
        std::string error;
        if (m_Controller.exportCsv(outputPath, error)) { m_LocalStatus = "Exported CSV: " + outputPath.string(); }
        else { m_LocalStatus = error; }
    }

    void DiagnosticsApp::captureScreenshot(const std::filesystem::path& path)
    {
        if (path.empty()) { return; }
        sf::Texture texture;
        if (!texture.create(WindowWidth, WindowHeight)) { return; }
        texture.update(m_Window);
        const sf::Image image = texture.copyToImage();
        image.saveToFile(path.string());
    }

    void DiagnosticsApp::configureAutomatedSmoke()
    {
        std::fill(m_DimensionSelected.begin(), m_DimensionSelected.end(), false);
        m_DimensionSelected[5u] = true;
        m_StyleSelected.fill(false);
        m_StyleSelected[static_cast<std::size_t>(PixelShipGenerator::ShipStyle::FIGHTER)] = true;
        m_StyleSelected[static_cast<std::size_t>(PixelShipGenerator::ShipStyle::INDUSTRIAL)] = true;
        m_FactionSelected.fill(false);
        m_FactionSelected[static_cast<std::size_t>(PixelShipGenerator::ShipFactionType::MILITARY)] = true;
        m_FactionSelected[static_cast<std::size_t>(PixelShipGenerator::ShipFactionType::CORPORATE)] = true;
        m_SamplesPerCombination = m_Options.CancelSmoke ? 250u : 12u;
        m_DetailedTiming = true;
    }

    void DiagnosticsApp::updateAutomatedSmoke(const DiagnosticsAppSnapshot& snapshot)
    {
        if (!m_AutomatedStarted)
        {
            m_AutomatedStarted = true;
            startDiagnostics();
            return;
        }
        if (m_Options.CancelSmoke && !m_AutomatedCancelRequested && snapshot.State == DiagnosticsAppRunState::RUNNING && snapshot.CompletedSamples >= 3u)
        {
            m_AutomatedCancelRequested = true;
            m_Controller.requestCancel();
            return;
        }
        if (m_AutomatedFinalized) { return; }
        if (snapshot.State == DiagnosticsAppRunState::COMPLETED || snapshot.State == DiagnosticsAppRunState::CANCELLED || snapshot.State == DiagnosticsAppRunState::ERROR)
        {
            m_AutomatedFinalized = true;
            if (!m_Options.SmokeCsvPath.empty())
            {
                std::string error;
                m_Controller.exportCsv(m_Options.SmokeCsvPath, error);
            }
            render();
            captureScreenshot(m_Options.ScreenshotPath);
            m_Window.close();
        }
    }

    std::string DiagnosticsApp::styleName(PixelShipGenerator::ShipStyle style)
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

    std::string DiagnosticsApp::factionName(PixelShipGenerator::ShipFactionType faction)
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

    std::string DiagnosticsApp::dimensionsName(PixelShipGenerator::ShipDimensions dimensions)
    {
        return std::to_string(dimensions.Width) + "X" + std::to_string(dimensions.Height);
    }

    std::string DiagnosticsApp::formatDuration(uint64_t nanoseconds, bool available)
    {
        if (!available) { return "ESTIMATING..."; }
        uint64_t totalSeconds = nanoseconds / 1000000000ull;
        const uint64_t hours = totalSeconds / 3600u;
        totalSeconds %= 3600u;
        const uint64_t minutes = totalSeconds / 60u;
        const uint64_t seconds = totalSeconds % 60u;
        std::ostringstream stream;
        if (hours > 0u) { stream << hours << ':' << std::setfill('0') << std::setw(2) << minutes << ':' << std::setw(2) << seconds; }
        else { stream << std::setfill('0') << std::setw(2) << minutes << ':' << std::setw(2) << seconds; }
        return stream.str();
    }

    uint32_t DiagnosticsApp::clampDimension(uint32_t value)
    {
        return std::clamp<uint32_t>(value, 24u, 256u);
    }
}
