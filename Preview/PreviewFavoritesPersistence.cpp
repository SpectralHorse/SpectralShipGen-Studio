#include "PreviewFavoritesPersistence.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <fstream>
#include <limits>
#include <sstream>
#include <system_error>

#include "ShipGenerationRecipeSerializer.h"

namespace
{
    using namespace SpectralShipGenStudioPreview;

    std::size_t skipWhitespace(const std::string& text, std::size_t position)
    {
        while (position < text.size() && std::isspace(static_cast<unsigned char>(text[position]))) { ++position; }
        return position;
    }

    bool findKeyValueStart(const std::string& text, const std::string& key, std::size_t& valueStart)
    {
        const std::string quotedKey = "\"" + key + "\"";
        const std::size_t keyPosition = text.find(quotedKey);
        if (keyPosition == std::string::npos) { return false; }
        const std::size_t colonPosition = skipWhitespace(text, keyPosition + quotedKey.size());
        if (colonPosition >= text.size() || text[colonPosition] != ':') { return false; }
        valueStart = skipWhitespace(text, colonPosition + 1u);
        return valueStart < text.size();
    }

    bool extractUInt32(const std::string& text, const std::string& key, uint32_t& value)
    {
        std::size_t start = 0u;
        if (!findKeyValueStart(text, key, start) || !std::isdigit(static_cast<unsigned char>(text[start]))) { return false; }
        std::size_t end = start;
        while (end < text.size() && std::isdigit(static_cast<unsigned char>(text[end]))) { ++end; }
        try
        {
            const unsigned long long parsed = std::stoull(text.substr(start, end - start));
            if (parsed > std::numeric_limits<uint32_t>::max()) { return false; }
            value = static_cast<uint32_t>(parsed);
            return true;
        }
        catch (...) { return false; }
    }

    bool extractBalancedValue(const std::string& text, std::size_t start, char openCharacter, char closeCharacter, std::string& value, std::size_t& nextPosition)
    {
        if (start >= text.size() || text[start] != openCharacter) { return false; }
        uint32_t depth = 0u;
        bool inString = false;
        bool escaped = false;
        for (std::size_t position = start; position < text.size(); ++position)
        {
            const char character = text[position];
            if (inString)
            {
                if (escaped) { escaped = false; continue; }
                if (character == '\\') { escaped = true; continue; }
                if (character == '"') { inString = false; }
                continue;
            }
            if (character == '"') { inString = true; continue; }
            if (character == openCharacter) { ++depth; }
            else if (character == closeCharacter)
            {
                if (depth == 0u) { return false; }
                --depth;
                if (depth == 0u)
                {
                    value = text.substr(start, position - start + 1u);
                    nextPosition = position + 1u;
                    return true;
                }
            }
        }
        return false;
    }

    bool extractObject(const std::string& text, const std::string& key, std::string& object)
    {
        std::size_t start = 0u;
        if (!findKeyValueStart(text, key, start)) { return false; }
        std::size_t nextPosition = 0u;
        return extractBalancedValue(text, start, '{', '}', object, nextPosition);
    }

    bool skipJsonValue(const std::string& text, std::size_t& position)
    {
        position = skipWhitespace(text, position);
        if (position >= text.size()) { return false; }
        if (text[position] == '{' || text[position] == '[')
        {
            std::string ignored;
            std::size_t nextPosition = 0u;
            const char openCharacter = text[position];
            const char closeCharacter = openCharacter == '{' ? '}' : ']';
            if (!extractBalancedValue(text, position, openCharacter, closeCharacter, ignored, nextPosition)) { return false; }
            position = nextPosition;
            return true;
        }
        if (text[position] == '"')
        {
            bool escaped = false;
            for (++position; position < text.size(); ++position)
            {
                const char character = text[position];
                if (escaped) { escaped = false; continue; }
                if (character == '\\') { escaped = true; continue; }
                if (character == '"') { ++position; return true; }
            }
            return false;
        }

        const std::size_t start = position;
        while (position < text.size() && text[position] != ',' && text[position] != ']') { ++position; }
        return skipWhitespace(text, start) < position;
    }

    std::vector<PreviewGenerationRecipe> uniqueFavorites(const std::vector<PreviewGenerationRecipe>& favorites)
    {
        std::vector<PreviewGenerationRecipe> unique;
        unique.reserve(favorites.size());
        for (const PreviewGenerationRecipe& recipe : favorites)
        {
            if (std::find(unique.begin(), unique.end(), recipe) == unique.end()) { unique.push_back(recipe); }
        }
        return unique;
    }

    std::string indentJson(std::string text, const std::string& indentation)
    {
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) { text.pop_back(); }
        std::string result;
        result.reserve(text.size() + 64u);
        for (char character : text)
        {
            result.push_back(character);
            if (character == '\n') { result += indentation; }
        }
        return result;
    }

    bool writeTextFile(const std::filesystem::path& path, const std::string& text, std::string& error)
    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        if (!stream) { error = "Failed to open Favorites file for writing: " + path.string(); return false; }
        stream << text;
        stream.flush();
        if (!stream) { error = "Failed while writing Favorites file: " + path.string(); return false; }
        stream.close();
        if (!stream) { error = "Failed while closing Favorites file: " + path.string(); return false; }
        return true;
    }

    void removeIfExists(const std::filesystem::path& path)
    {
        std::error_code error;
        std::filesystem::remove(path, error);
    }
}

namespace SpectralShipGenStudioPreview
{
    std::string serializePreviewFavorites(const std::vector<PreviewGenerationRecipe>& favorites)
    {
        const std::vector<PreviewGenerationRecipe> unique = uniqueFavorites(favorites);
        std::ostringstream stream;
        stream << "{\n";
        stream << "  \"format_version\": " << PreviewFavoritesFormatVersion << ",\n";
        stream << "  \"favorites\": [";
        if (!unique.empty()) { stream << '\n'; }

        for (std::size_t index = 0u; index < unique.size(); ++index)
        {
            ShipGenerationRecipeDocument document;
            document.Recipe = unique[index];
            stream << "    {\n";
            stream << "      \"recipe\": " << indentJson(serializeShipGenerationRecipe(document), "      ") << '\n';
            stream << "    }";
            if (index + 1u < unique.size()) { stream << ','; }
            stream << '\n';
        }

        stream << "  ]\n";
        stream << "}\n";
        return stream.str();
    }

    PreviewFavoritesLoadResult deserializePreviewFavorites(const std::string& jsonText)
    {
        PreviewFavoritesLoadResult result;
        const std::size_t first = skipWhitespace(jsonText, 0u);
        std::size_t last = jsonText.size();
        while (last > first && std::isspace(static_cast<unsigned char>(jsonText[last - 1u]))) { --last; }
        if (first >= last || jsonText[first] != '{' || jsonText[last - 1u] != '}')
        {
            result.Error = "Failed to parse Favorites JSON object.";
            return result;
        }

        uint32_t formatVersion = 0u;
        if (!extractUInt32(jsonText, "format_version", formatVersion))
        {
            result.Error = "Missing or invalid Favorites field: format_version.";
            return result;
        }
        if (formatVersion != PreviewFavoritesFormatVersion)
        {
            result.Error = "Unsupported Favorites format version: " + std::to_string(formatVersion) + ".";
            return result;
        }

        std::size_t position = 0u;
        if (!findKeyValueStart(jsonText, "favorites", position) || jsonText[position] != '[')
        {
            result.Error = "Missing or invalid Favorites field: favorites.";
            return result;
        }
        ++position;

        while (true)
        {
            position = skipWhitespace(jsonText, position);
            if (position >= jsonText.size())
            {
                result.Error = "Favorites array is not terminated.";
                return result;
            }
            if (jsonText[position] == ']')
            {
                result.Success = true;
                return result;
            }

            if (jsonText[position] != '{')
            {
                if (!skipJsonValue(jsonText, position))
                {
                    result.Error = "Malformed Favorites array entry.";
                    return result;
                }
                ++result.SkippedEntryCount;
            }
            else
            {
                std::string entryObject;
                std::size_t nextPosition = 0u;
                if (!extractBalancedValue(jsonText, position, '{', '}', entryObject, nextPosition))
                {
                    result.Error = "Malformed Favorites object entry.";
                    return result;
                }
                position = nextPosition;

                std::string recipeObject;
                if (!extractObject(entryObject, "recipe", recipeObject))
                {
                    ++result.SkippedEntryCount;
                }
                else
                {
                    const ShipGenerationRecipeLoadResult recipeResult = deserializeShipGenerationRecipe(recipeObject);
                    if (!recipeResult.Success)
                    {
                        ++result.SkippedEntryCount;
                    }
                    else if (std::find(result.Favorites.begin(), result.Favorites.end(), recipeResult.Document.Recipe) != result.Favorites.end())
                    {
                        ++result.DuplicateEntryCount;
                    }
                    else
                    {
                        result.Favorites.push_back(recipeResult.Document.Recipe);
                    }
                }
            }

            position = skipWhitespace(jsonText, position);
            if (position >= jsonText.size())
            {
                result.Error = "Favorites array is not terminated.";
                return result;
            }
            if (jsonText[position] == ']')
            {
                result.Success = true;
                return result;
            }
            if (jsonText[position] != ',')
            {
                result.Error = "Malformed Favorites array separator.";
                return result;
            }
            ++position;
        }
    }

    bool savePreviewFavorites(const std::vector<PreviewGenerationRecipe>& favorites, const std::filesystem::path& path, std::string& error)
    {
        error.clear();
        const std::filesystem::path temporaryPath = path.string() + ".tmp";
        const std::filesystem::path backupPath = path.string() + ".bak";
        removeIfExists(temporaryPath);
        removeIfExists(backupPath);

        std::string serialized;
        try
        {
            serialized = serializePreviewFavorites(favorites);
        }
        catch (const std::exception& exception)
        {
            error = std::string("Failed to serialize Favorites: ") + exception.what();
            return false;
        }

        if (!writeTextFile(temporaryPath, serialized, error))
        {
            removeIfExists(temporaryPath);
            return false;
        }

        std::error_code filesystemError;
        const bool destinationExists = std::filesystem::exists(path, filesystemError);
        if (filesystemError)
        {
            error = "Failed to inspect existing Favorites file: " + path.string() + ".";
            removeIfExists(temporaryPath);
            return false;
        }

        if (destinationExists)
        {
            std::filesystem::rename(path, backupPath, filesystemError);
            if (filesystemError)
            {
                error = "Failed to prepare existing Favorites file for replacement: " + path.string() + ".";
                removeIfExists(temporaryPath);
                removeIfExists(backupPath);
                return false;
            }
        }

        filesystemError.clear();
        std::filesystem::rename(temporaryPath, path, filesystemError);
        if (filesystemError)
        {
            error = "Failed to replace Favorites file: " + path.string() + ".";
            removeIfExists(temporaryPath);
            if (destinationExists)
            {
                std::error_code restoreError;
                std::filesystem::rename(backupPath, path, restoreError);
                if (restoreError) { error += " Previous Favorites file could not be restored."; }
            }
            return false;
        }

        removeIfExists(backupPath);
        return true;
    }

    PreviewFavoritesLoadResult loadPreviewFavorites(const std::filesystem::path& path)
    {
        std::error_code filesystemError;
        if (!std::filesystem::exists(path, filesystemError))
        {
            PreviewFavoritesLoadResult result;
            if (filesystemError)
            {
                result.Error = "Failed to inspect Favorites file: " + path.string() + ".";
                return result;
            }
            result.Success = true;
            return result;
        }

        std::ifstream stream(path, std::ios::binary);
        if (!stream)
        {
            PreviewFavoritesLoadResult result;
            result.Error = "Failed to open Favorites file: " + path.string();
            return result;
        }
        std::ostringstream buffer;
        buffer << stream.rdbuf();
        if (!stream.good() && !stream.eof())
        {
            PreviewFavoritesLoadResult result;
            result.Error = "Failed while reading Favorites file: " + path.string();
            return result;
        }
        return deserializePreviewFavorites(buffer.str());
    }
}
