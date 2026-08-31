#pragma once

#include <SFML/Graphics/Image.hpp>

#include <SpectralShipGen/Image.h>

namespace SpectralShipGen
{
    class SFMLImageAdapter
    {
    public:
        static sf::Image createSFMLImage(const Image& image);
    };
} // namespace SpectralShipGen
