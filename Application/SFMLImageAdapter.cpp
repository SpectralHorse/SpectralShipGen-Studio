#include <PixelShipGenerator/SFMLImageAdapter.h>

namespace PixelShipGenerator
{
    sf::Image SFMLImageAdapter::createSFMLImage(const Image& image)
    {
        sf::Image sfmlImage;
        sfmlImage.create(image.getWidth(), image.getHeight(), sf::Color::Transparent);

        for (uint32_t y = 0; y < image.getHeight(); ++y)
        {
            for (uint32_t x = 0; x < image.getWidth(); ++x)
            {
                const Color& pixel = image.getPixel(x, y);
                sfmlImage.setPixel(x, y, sf::Color(pixel.R, pixel.G, pixel.B, pixel.A));
            }
        }

        return sfmlImage;
    }
} // namespace PixelShipGenerator
