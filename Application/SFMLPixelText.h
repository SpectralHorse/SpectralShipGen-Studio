#pragma once

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

namespace PixelShipGeneratorApplication
{
    void drawPixelText(sf::RenderTarget& target, const std::string& text, float x, float y, const sf::Color& color, uint32_t scale = 2u);
    float getPixelTextWidth(const std::string& text, uint32_t scale = 2u);
    std::string wrapPixelText(const std::string& text, std::size_t maximumCharactersPerLine);
}
