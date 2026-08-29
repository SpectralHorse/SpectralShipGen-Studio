#pragma once

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace PixelShipGeneratorApplication
{
    struct ChartDatum
    {
        std::string Label;
        double Value = 0.0;
    };

    struct ChartSeries
    {
        std::string Label;
        std::vector<ChartDatum> Values;
    };

    struct ChartHitRegion
    {
        sf::FloatRect Bounds;
        std::size_t SeriesIndex = 0u;
        std::size_t ValueIndex = 0u;
    };

    struct ChartColors
    {
        sf::Color Background;
        sf::Color Grid;
        sf::Color Text;
        sf::Color Muted;
        sf::Color Primary;
        sf::Color Secondary;
        sf::Color Highlight;
    };

    std::vector<ChartHitRegion> drawBarChart(sf::RenderTarget& target,
                                              const sf::FloatRect& bounds,
                                              const std::string& title,
                                              const std::vector<ChartSeries>& series,
                                              const std::string& unit,
                                              const ChartColors& colors,
                                              std::size_t selectedSeries,
                                              std::size_t selectedValue,
                                              bool horizontal = false);

    std::vector<ChartHitRegion> drawLineChart(sf::RenderTarget& target,
                                               const sf::FloatRect& bounds,
                                               const std::string& title,
                                               const std::vector<ChartSeries>& series,
                                               const std::string& unit,
                                               const ChartColors& colors,
                                               std::size_t selectedSeries,
                                               std::size_t selectedValue);
}
