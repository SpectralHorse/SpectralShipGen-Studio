#include "SFMLCharts.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "SFMLPixelText.h"

namespace
{
    constexpr uint32_t ChartTextScale = 2u;
    constexpr uint32_t AxisTextScale = 2u;

    std::string formatValue(double value)
    {
        std::ostringstream stream;
        const double magnitude = std::abs(value);
        if (magnitude >= 100.0) { stream << std::fixed << std::setprecision(0); }
        else if (magnitude >= 10.0) { stream << std::fixed << std::setprecision(1); }
        else { stream << std::fixed << std::setprecision(2); }
        stream << value;
        return stream.str();
    }

    double maximumValue(const std::vector<PixelShipGeneratorApplication::ChartSeries>& series)
    {
        double maximum = 0.0;
        for (const auto& item : series) { for (const auto& value : item.Values) { maximum = std::max(maximum, value.Value); } }
        return maximum <= 0.0 ? 1.0 : maximum;
    }

    void drawRect(sf::RenderTarget& target, const sf::FloatRect& bounds, const sf::Color& fill, const sf::Color& outline)
    {
        sf::RectangleShape shape({ bounds.width, bounds.height });
        shape.setPosition(bounds.left, bounds.top);
        shape.setFillColor(fill);
        shape.setOutlineThickness(1.0f);
        shape.setOutlineColor(outline);
        target.draw(shape);
    }

    sf::Color seriesColor(std::size_t index, const PixelShipGeneratorApplication::ChartColors& colors)
    {
        return index == 0u ? colors.Primary : colors.Secondary;
    }
}

namespace PixelShipGeneratorApplication
{
    std::vector<ChartHitRegion> drawBarChart(sf::RenderTarget& target,
                                              const sf::FloatRect& bounds,
                                              const std::string& title,
                                              const std::vector<ChartSeries>& series,
                                              const std::string& unit,
                                              const ChartColors& colors,
                                              std::size_t selectedSeries,
                                              std::size_t selectedValue,
                                              bool horizontal)
    {
        std::vector<ChartHitRegion> hits;
        drawRect(target, bounds, colors.Background, colors.Grid);
        drawPixelText(target, title, bounds.left + 10.0f, bounds.top + 8.0f, colors.Text, ChartTextScale);
        if (series.empty() || series.front().Values.empty())
        {
            drawPixelText(target, "NO DATA FOR CURRENT FILTER", bounds.left + 10.0f, bounds.top + 40.0f, colors.Muted, ChartTextScale);
            return hits;
        }
        const double maximum = maximumValue(series);
        const float titleHeight = 32.0f;
        const float leftPad = horizontal ? 190.0f : 70.0f;
        const float rightPad = 18.0f;
        const float top = bounds.top + titleHeight + 14.0f;
        const float bottomPad = horizontal ? 28.0f : 54.0f;
        const sf::FloatRect plot{ bounds.left + leftPad, top, bounds.width - leftPad - rightPad, bounds.height - titleHeight - 14.0f - bottomPad };
        if (plot.width <= 1.0f || plot.height <= 1.0f) { return hits; }

        for (uint32_t tick = 0u; tick <= 4u; ++tick)
        {
            const float fraction = static_cast<float>(tick) / 4.0f;
            if (!horizontal)
            {
                const float y = plot.top + plot.height * (1.0f - fraction);
                sf::Vertex line[] = { sf::Vertex({ plot.left, y }, colors.Grid), sf::Vertex({ plot.left + plot.width, y }, colors.Grid) };
                target.draw(line, 2u, sf::Lines);
                drawPixelText(target, formatValue(maximum * fraction), bounds.left + 6.0f, y - 7.0f, colors.Muted, AxisTextScale);
            }
            else
            {
                const float x = plot.left + plot.width * fraction;
                sf::Vertex line[] = { sf::Vertex({ x, plot.top }, colors.Grid), sf::Vertex({ x, plot.top + plot.height }, colors.Grid) };
                target.draw(line, 2u, sf::Lines);
            }
        }

        const std::size_t count = series.front().Values.size();
        const std::size_t seriesCount = series.size();
        if (!horizontal)
        {
            const float groupWidth = plot.width / static_cast<float>(std::max<std::size_t>(1u, count));
            const float barWidth = std::max(3.0f, (groupWidth - 8.0f) / static_cast<float>(std::max<std::size_t>(1u, seriesCount)));
            for (std::size_t valueIndex = 0u; valueIndex < count; ++valueIndex)
            {
                for (std::size_t seriesIndex = 0u; seriesIndex < seriesCount; ++seriesIndex)
                {
                    if (valueIndex >= series[seriesIndex].Values.size()) { continue; }
                    const double value = series[seriesIndex].Values[valueIndex].Value;
                    const float height = static_cast<float>(std::max(0.0, value) / maximum) * plot.height;
                    sf::FloatRect bar{ plot.left + groupWidth * static_cast<float>(valueIndex) + 4.0f + barWidth * static_cast<float>(seriesIndex), plot.top + plot.height - height, barWidth - 2.0f, height };
                    sf::Color color = seriesColor(seriesIndex, colors);
                    if (seriesIndex == selectedSeries && valueIndex == selectedValue) { color = colors.Highlight; }
                    drawRect(target, bar, color, color);
                    hits.push_back({ bar, seriesIndex, valueIndex });
                }
                const std::string& label = series.front().Values[valueIndex].Label;
                const float textWidth = getPixelTextWidth(label, AxisTextScale);
                drawPixelText(target, label, plot.left + groupWidth * (static_cast<float>(valueIndex) + 0.5f) - textWidth * 0.5f, plot.top + plot.height + 8.0f, colors.Muted, AxisTextScale);
            }
        }
        else
        {
            const float rowHeight = plot.height / static_cast<float>(std::max<std::size_t>(1u, count));
            const float barHeight = std::max(3.0f, (rowHeight - 4.0f) / static_cast<float>(std::max<std::size_t>(1u, seriesCount)));
            for (std::size_t valueIndex = 0u; valueIndex < count; ++valueIndex)
            {
                drawPixelText(target, series.front().Values[valueIndex].Label, bounds.left + 8.0f, plot.top + rowHeight * static_cast<float>(valueIndex) + 3.0f, colors.Muted, AxisTextScale);
                for (std::size_t seriesIndex = 0u; seriesIndex < seriesCount; ++seriesIndex)
                {
                    if (valueIndex >= series[seriesIndex].Values.size()) { continue; }
                    const double value = series[seriesIndex].Values[valueIndex].Value;
                    const float width = static_cast<float>(std::max(0.0, value) / maximum) * plot.width;
                    sf::FloatRect bar{ plot.left, plot.top + rowHeight * static_cast<float>(valueIndex) + barHeight * static_cast<float>(seriesIndex), width, barHeight - 1.0f };
                    sf::Color color = seriesColor(seriesIndex, colors);
                    if (seriesIndex == selectedSeries && valueIndex == selectedValue) { color = colors.Highlight; }
                    drawRect(target, bar, color, color);
                    hits.push_back({ bar, seriesIndex, valueIndex });
                }
            }
        }
        drawPixelText(target, unit, bounds.left + bounds.width - getPixelTextWidth(unit, AxisTextScale) - 8.0f, bounds.top + 10.0f, colors.Muted, AxisTextScale);
        if (series.size() > 1u)
        {
            float x = bounds.left + 230.0f;
            for (std::size_t i = 0u; i < series.size(); ++i)
            {
                sf::RectangleShape swatch({ 10.0f, 10.0f }); swatch.setPosition(x, bounds.top + 10.0f); swatch.setFillColor(seriesColor(i, colors)); target.draw(swatch);
                drawPixelText(target, series[i].Label, x + 14.0f, bounds.top + 8.0f, colors.Text, AxisTextScale); x += 100.0f;
            }
        }
        return hits;
    }

    std::vector<ChartHitRegion> drawLineChart(sf::RenderTarget& target,
                                               const sf::FloatRect& bounds,
                                               const std::string& title,
                                               const std::vector<ChartSeries>& series,
                                               const std::string& unit,
                                               const ChartColors& colors,
                                               std::size_t selectedSeries,
                                               std::size_t selectedValue)
    {
        std::vector<ChartHitRegion> hits;
        drawRect(target, bounds, colors.Background, colors.Grid);
        drawPixelText(target, title, bounds.left + 10.0f, bounds.top + 8.0f, colors.Text, ChartTextScale);
        if (series.empty() || series.front().Values.empty())
        {
            drawPixelText(target, "NO DATA FOR CURRENT FILTER", bounds.left + 10.0f, bounds.top + 40.0f, colors.Muted, ChartTextScale);
            return hits;
        }
        const double maximum = maximumValue(series);
        const float leftPad = 70.0f;
        const float topPad = 50.0f;
        const float bottomPad = 54.0f;
        const sf::FloatRect plot{ bounds.left + leftPad, bounds.top + topPad, bounds.width - leftPad - 18.0f, bounds.height - topPad - bottomPad };
        for (uint32_t tick = 0u; tick <= 4u; ++tick)
        {
            const float fraction = static_cast<float>(tick) / 4.0f;
            const float y = plot.top + plot.height * (1.0f - fraction);
            sf::Vertex line[] = { sf::Vertex({ plot.left, y }, colors.Grid), sf::Vertex({ plot.left + plot.width, y }, colors.Grid) };
            target.draw(line, 2u, sf::Lines);
            drawPixelText(target, formatValue(maximum * fraction), bounds.left + 6.0f, y - 7.0f, colors.Muted, AxisTextScale);
        }
        const std::size_t count = series.front().Values.size();
        for (std::size_t seriesIndex = 0u; seriesIndex < series.size(); ++seriesIndex)
        {
            sf::VertexArray line(sf::LineStrip);
            for (std::size_t valueIndex = 0u; valueIndex < series[seriesIndex].Values.size(); ++valueIndex)
            {
                const float fractionX = count <= 1u ? 0.5f : static_cast<float>(valueIndex) / static_cast<float>(count - 1u);
                const float x = plot.left + plot.width * fractionX;
                const float y = plot.top + plot.height * (1.0f - static_cast<float>(std::max(0.0, series[seriesIndex].Values[valueIndex].Value) / maximum));
                line.append(sf::Vertex({ x, y }, seriesColor(seriesIndex, colors)));
                sf::CircleShape point(4.0f); point.setOrigin(4.0f, 4.0f); point.setPosition(x, y); point.setFillColor(seriesIndex == selectedSeries && valueIndex == selectedValue ? colors.Highlight : seriesColor(seriesIndex, colors)); target.draw(point);
                hits.push_back({ { x - 8.0f, y - 8.0f, 16.0f, 16.0f }, seriesIndex, valueIndex });
                if (seriesIndex == 0u)
                {
                    const std::string& label = series[seriesIndex].Values[valueIndex].Label;
                    drawPixelText(target, label, x - getPixelTextWidth(label, AxisTextScale) * 0.5f, plot.top + plot.height + 8.0f, colors.Muted, AxisTextScale);
                }
            }
            target.draw(line);
        }
        drawPixelText(target, unit, bounds.left + bounds.width - getPixelTextWidth(unit, AxisTextScale) - 8.0f, bounds.top + 10.0f, colors.Muted, AxisTextScale);
        if (series.size() > 1u)
        {
            float x = bounds.left + 230.0f;
            for (std::size_t i = 0u; i < series.size(); ++i)
            {
                sf::RectangleShape swatch({ 10.0f, 10.0f }); swatch.setPosition(x, bounds.top + 10.0f); swatch.setFillColor(seriesColor(i, colors)); target.draw(swatch);
                drawPixelText(target, series[i].Label, x + 14.0f, bounds.top + 8.0f, colors.Text, AxisTextScale); x += 100.0f;
            }
        }
        return hits;
    }
}
