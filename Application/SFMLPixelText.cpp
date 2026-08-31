#include "SFMLPixelText.h"

#include <array>
#include <cctype>

namespace
{
    std::array<uint8_t, 5u> getGlyph(char character)
    {
        const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
        switch (c)
        {
        case 'A': return { 2u, 5u, 7u, 5u, 5u };
        case 'B': return { 6u, 5u, 6u, 5u, 6u };
        case 'C': return { 3u, 4u, 4u, 4u, 3u };
        case 'D': return { 6u, 5u, 5u, 5u, 6u };
        case 'E': return { 7u, 4u, 6u, 4u, 7u };
        case 'F': return { 7u, 4u, 6u, 4u, 4u };
        case 'G': return { 3u, 4u, 5u, 5u, 3u };
        case 'H': return { 5u, 5u, 7u, 5u, 5u };
        case 'I': return { 7u, 2u, 2u, 2u, 7u };
        case 'J': return { 1u, 1u, 1u, 5u, 2u };
        case 'K': return { 5u, 5u, 6u, 5u, 5u };
        case 'L': return { 4u, 4u, 4u, 4u, 7u };
        case 'M': return { 5u, 7u, 7u, 5u, 5u };
        case 'N': return { 5u, 7u, 7u, 7u, 5u };
        case 'O': return { 2u, 5u, 5u, 5u, 2u };
        case 'P': return { 6u, 5u, 6u, 4u, 4u };
        case 'Q': return { 2u, 5u, 5u, 3u, 1u };
        case 'R': return { 6u, 5u, 6u, 5u, 5u };
        case 'S': return { 3u, 4u, 2u, 1u, 6u };
        case 'T': return { 7u, 2u, 2u, 2u, 2u };
        case 'U': return { 5u, 5u, 5u, 5u, 7u };
        case 'V': return { 5u, 5u, 5u, 5u, 2u };
        case 'W': return { 5u, 5u, 7u, 7u, 5u };
        case 'X': return { 5u, 5u, 2u, 5u, 5u };
        case 'Y': return { 5u, 5u, 2u, 2u, 2u };
        case 'Z': return { 7u, 1u, 2u, 4u, 7u };
        case '0': return { 7u, 5u, 5u, 5u, 7u };
        case '1': return { 2u, 6u, 2u, 2u, 7u };
        case '2': return { 6u, 1u, 2u, 4u, 7u };
        case '3': return { 6u, 1u, 2u, 1u, 6u };
        case '4': return { 5u, 5u, 7u, 1u, 1u };
        case '5': return { 7u, 4u, 6u, 1u, 6u };
        case '6': return { 3u, 4u, 6u, 5u, 2u };
        case '7': return { 7u, 1u, 2u, 2u, 2u };
        case '8': return { 2u, 5u, 2u, 5u, 2u };
        case '9': return { 2u, 5u, 3u, 1u, 6u };
        case ':': return { 0u, 2u, 0u, 2u, 0u };
        case '.': return { 0u, 0u, 0u, 0u, 2u };
        case ',': return { 0u, 0u, 0u, 2u, 4u };
        case '-': return { 0u, 0u, 7u, 0u, 0u };
        case '+': return { 0u, 2u, 7u, 2u, 0u };
        case '/': return { 1u, 1u, 2u, 4u, 4u };
        case '\\': return { 4u, 4u, 2u, 1u, 1u };
        case '[': return { 6u, 4u, 4u, 4u, 6u };
        case ']': return { 3u, 1u, 1u, 1u, 3u };
        case '(': return { 2u, 4u, 4u, 4u, 2u };
        case ')': return { 2u, 1u, 1u, 1u, 2u };
        case '=': return { 0u, 7u, 0u, 7u, 0u };
        case '_': return { 0u, 0u, 0u, 0u, 7u };
        case '#': return { 5u, 7u, 5u, 7u, 5u };
        case '%': return { 5u, 1u, 2u, 4u, 5u };
        case '?': return { 6u, 1u, 2u, 0u, 2u };
        case '|': return { 2u, 2u, 2u, 2u, 2u };
        case '<': return { 1u, 2u, 4u, 2u, 1u };
        case '>': return { 4u, 2u, 1u, 2u, 4u };
        default: return { 0u, 0u, 0u, 0u, 0u };
        }
    }

    void appendQuad(sf::VertexArray& vertices, float x, float y, float size, const sf::Color& color)
    {
        vertices.append(sf::Vertex(sf::Vector2f(x, y), color));
        vertices.append(sf::Vertex(sf::Vector2f(x + size, y), color));
        vertices.append(sf::Vertex(sf::Vector2f(x + size, y + size), color));
        vertices.append(sf::Vertex(sf::Vector2f(x, y + size), color));
    }
}

namespace SpectralShipGenStudioApplication
{
    void drawPixelText(sf::RenderTarget& target, const std::string& text, float x, float y, const sf::Color& color, uint32_t scale)
    {
        sf::VertexArray vertices(sf::Quads);
        float cursorX = x;
        float cursorY = y;
        const float pixelSize = static_cast<float>(scale);
        const float characterAdvance = static_cast<float>(4u * scale);
        const float lineAdvance = static_cast<float>(7u * scale);

        for (char character : text)
        {
            if (character == '\n')
            {
                cursorX = x;
                cursorY += lineAdvance;
                continue;
            }
            const std::array<uint8_t, 5u> glyph = getGlyph(character);
            for (uint32_t row = 0u; row < glyph.size(); ++row)
            {
                for (uint32_t column = 0u; column < 3u; ++column)
                {
                    if ((glyph[row] & (1u << (2u - column))) != 0u)
                    {
                        appendQuad(vertices, cursorX + static_cast<float>(column * scale), cursorY + static_cast<float>(row * scale), pixelSize, color);
                    }
                }
            }
            cursorX += characterAdvance;
        }
        target.draw(vertices);
    }

    float getPixelTextWidth(const std::string& text, uint32_t scale)
    {
        return static_cast<float>(text.size() * 4u * scale);
    }

    std::string wrapPixelText(const std::string& text, std::size_t maximumCharactersPerLine)
    {
        if (text.size() <= maximumCharactersPerLine) { return text; }
        std::string result;
        std::size_t lineLength = 0u;
        std::size_t wordStart = 0u;
        while (wordStart < text.size())
        {
            while (wordStart < text.size() && text[wordStart] == ' ') { ++wordStart; }
            if (wordStart >= text.size()) { break; }
            const std::size_t wordEnd = text.find(' ', wordStart);
            const std::size_t end = wordEnd == std::string::npos ? text.size() : wordEnd;
            const std::string word = text.substr(wordStart, end - wordStart);
            if (lineLength > 0u && lineLength + 1u + word.size() > maximumCharactersPerLine)
            {
                result += '\n';
                lineLength = 0u;
            }
            else if (lineLength > 0u)
            {
                result += ' ';
                ++lineLength;
            }
            result += word;
            lineLength += word.size();
            wordStart = end == text.size() ? end : end + 1u;
        }
        return result;
    }
}
