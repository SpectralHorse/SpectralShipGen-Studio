#include "PreviewPreferences.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <utility>

#include "PreviewResolution.h"

namespace
{
    using namespace SpectralShipGenStudioPreview;
    using SpectralShipGen::ShipDimensions;

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
        std::size_t colonPosition = skipWhitespace(text, keyPosition + quotedKey.size());
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

    bool extractUInt32Array(const std::string& text, const std::string& key, std::vector<uint32_t>& values)
    {
        std::size_t position = 0u;
        if (!findKeyValueStart(text, key, position) || text[position] != '[') { return false; }
        ++position;
        values.clear();

        while (true)
        {
            position = skipWhitespace(text, position);
            if (position >= text.size()) { return false; }
            if (text[position] == ']') { return true; }
            if (!std::isdigit(static_cast<unsigned char>(text[position]))) { return false; }

            std::size_t end = position;
            while (end < text.size() && std::isdigit(static_cast<unsigned char>(text[end]))) { ++end; }
            try
            {
                const unsigned long long parsed = std::stoull(text.substr(position, end - position));
                if (parsed > std::numeric_limits<uint32_t>::max()) { return false; }
                values.push_back(static_cast<uint32_t>(parsed));
            }
            catch (...) { return false; }

            position = skipWhitespace(text, end);
            if (position >= text.size()) { return false; }
            if (text[position] == ']') { return true; }
            if (text[position] != ',') { return false; }
            ++position;
        }
    }

    bool extractObjectArray(const std::string& text, const std::string& key, std::vector<std::string>& objects)
    {
        std::size_t position = 0u;
        if (!findKeyValueStart(text, key, position) || text[position] != '[') { return false; }
        ++position;
        objects.clear();

        while (true)
        {
            position = skipWhitespace(text, position);
            if (position >= text.size()) { return false; }
            if (text[position] == ']') { return true; }
            if (text[position] != '{') { return false; }

            const std::size_t start = position;
            uint32_t depth = 0u;
            bool inString = false;
            bool escaped = false;
            bool foundEnd = false;

            for (; position < text.size(); ++position)
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
                if (character == '{') { ++depth; }
                else if (character == '}')
                {
                    if (depth == 0u) { return false; }
                    --depth;
                    if (depth == 0u)
                    {
                        objects.push_back(text.substr(start, position - start + 1u));
                        ++position;
                        foundEnd = true;
                        break;
                    }
                }
            }

            if (!foundEnd) { return false; }
            position = skipWhitespace(text, position);
            if (position >= text.size()) { return false; }
            if (text[position] == ']') { return true; }
            if (text[position] != ',') { return false; }
            ++position;
        }
    }

    bool dimensionsLess(const ShipDimensions& first, const ShipDimensions& second)
    {
        if (first.Width != second.Width) { return first.Width < second.Width; }
        return first.Height < second.Height;
    }

    PreviewPreferences sanitizePreferences(PreviewPreferences preferences)
    {
        auto& bookmarks = preferences.ResolutionBookmarks;
        bookmarks.erase(std::remove_if(bookmarks.begin(), bookmarks.end(), [](const ShipDimensions& dimensions) { return !isSelectablePreviewDimensions(dimensions); }), bookmarks.end());
        std::sort(bookmarks.begin(), bookmarks.end(), dimensionsLess);
        bookmarks.erase(std::unique(bookmarks.begin(), bookmarks.end()), bookmarks.end());
        if (bookmarks.size() > MaximumResolutionBookmarks) { bookmarks.resize(MaximumResolutionBookmarks); }
        return preferences;
    }
}

namespace SpectralShipGenStudioPreview
{
    std::string serializePreviewPreferences(const PreviewPreferences& preferences)
    {
        const PreviewPreferences sanitized = sanitizePreferences(preferences);
        std::ostringstream stream;
        stream << "{\n";
        stream << "  \"format_version\": " << PreviewPreferencesFormatVersion << ",\n";
        stream << "  \"dimension_bookmarks\": [";
        for (std::size_t index = 0u; index < sanitized.ResolutionBookmarks.size(); ++index)
        {
            if (index > 0u) { stream << ", "; }
            const ShipDimensions& dimensions = sanitized.ResolutionBookmarks[index];
            stream << "{ \"width\": " << dimensions.Width << ", \"height\": " << dimensions.Height << " }";
        }
        stream << "]\n";
        stream << "}\n";
        return stream.str();
    }

    PreviewPreferencesLoadResult deserializePreviewPreferences(const std::string& jsonText)
    {
        PreviewPreferencesLoadResult result;
        uint32_t version = 0u;
        if (!extractUInt32(jsonText, "format_version", version)) { result.Error = "Missing or invalid field: format_version."; return result; }

        if (version == PreviewPreferencesFormatVersion)
        {
            std::vector<std::string> objects;
            if (!extractObjectArray(jsonText, "dimension_bookmarks", objects)) { result.Error = "Missing or invalid field: dimension_bookmarks."; return result; }
            for (const std::string& object : objects)
            {
                ShipDimensions dimensions;
                if (!extractUInt32(object, "width", dimensions.Width) || !extractUInt32(object, "height", dimensions.Height)) { result.Error = "Invalid dimension bookmark."; return result; }
                result.Preferences.ResolutionBookmarks.push_back(dimensions);
            }
        }
        else
        {
            result.Error = "Unsupported PreviewApp preferences format version: " + std::to_string(version) + ".";
            return result;
        }

        result.Preferences = sanitizePreferences(std::move(result.Preferences));
        result.Success = true;
        return result;
    }

    bool savePreviewPreferences(const PreviewPreferences& preferences, const std::filesystem::path& path, std::string& error)
    {
        std::ofstream stream(path, std::ios::binary);
        if (!stream) { error = "Failed to open PreviewApp preferences for writing: " + path.string(); return false; }
        stream << serializePreviewPreferences(preferences);
        if (!stream) { error = "Failed while writing PreviewApp preferences: " + path.string(); return false; }
        return true;
    }

    PreviewPreferencesLoadResult loadPreviewPreferences(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream)
        {
            PreviewPreferencesLoadResult result;
            result.Success = true;
            return result;
        }
        std::ostringstream buffer;
        buffer << stream.rdbuf();
        if (!stream.good() && !stream.eof())
        {
            PreviewPreferencesLoadResult result;
            result.Error = "Failed while reading PreviewApp preferences: " + path.string();
            return result;
        }
        return deserializePreviewPreferences(buffer.str());
    }
}
