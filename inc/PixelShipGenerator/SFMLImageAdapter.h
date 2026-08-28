#pragma once

#include <SFML/Graphics/Image.hpp>

#include "Image.h"

namespace PixelShipGenerator
{
    class SFMLImageAdapter
    {
    public:
        static sf::Image createSFMLImage(const Image& image);
    };
} // namespace PixelShipGenerator
