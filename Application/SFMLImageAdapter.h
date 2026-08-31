#pragma once

#include <SFML/Graphics/Image.hpp>

#include <PixelShipGenerator/Image.h>

namespace PixelShipGenerator
{
    class SFMLImageAdapter
    {
    public:
        static sf::Image createSFMLImage(const Image& image);
    };
} // namespace PixelShipGenerator
