#include "ShipGenerationRecipeSerializer.h"

#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace
{
    using namespace PixelShipGeneratorPreview;

    std::size_t skipWhitespace(const std::string& text, std::size_t position)
    {
        while (position < text.size() && std::isspace(static_cast<unsigned char>(text[position]))) { ++position; }
        return position;
    }

    bool findKeyValueStart(const std::string& objectText, const std::string& key, std::size_t& valueStart)
    {
        const std::string quotedKey = "\"" + key + "\"";
        const std::size_t keyPosition = objectText.find(quotedKey);
        if (keyPosition == std::string::npos) { return false; }
        std::size_t colonPosition = skipWhitespace(objectText, keyPosition + quotedKey.size());
        if (colonPosition >= objectText.size() || objectText[colonPosition] != ':') { return false; }
        valueStart = skipWhitespace(objectText, colonPosition + 1u);
        return valueStart < objectText.size();
    }

    bool extractObject(const std::string& objectText, const std::string& key, std::string& outObject)
    {
        std::size_t start = 0u;
        if (!findKeyValueStart(objectText, key, start) || objectText[start] != '{') { return false; }
        uint32_t depth = 0u;
        bool inString = false;
        bool escaped = false;
        for (std::size_t index = start; index < objectText.size(); ++index)
        {
            const char character = objectText[index];
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
                if (depth == 0u) { outObject = objectText.substr(start, index - start + 1u); return true; }
            }
        }
        return false;
    }

    bool extractString(const std::string& objectText, const std::string& key, std::string& value)
    {
        std::size_t start = 0u;
        if (!findKeyValueStart(objectText, key, start) || objectText[start] != '"') { return false; }
        std::string parsed;
        bool escaped = false;
        for (std::size_t index = start + 1u; index < objectText.size(); ++index)
        {
            const char character = objectText[index];
            if (escaped)
            {
                switch (character)
                {
                case '"': parsed.push_back('"'); break;
                case '\\': parsed.push_back('\\'); break;
                case 'n': parsed.push_back('\n'); break;
                case 'r': parsed.push_back('\r'); break;
                case 't': parsed.push_back('\t'); break;
                default: return false;
                }
                escaped = false;
                continue;
            }
            if (character == '\\') { escaped = true; continue; }
            if (character == '"') { value = parsed; return true; }
            parsed.push_back(character);
        }
        return false;
    }

    bool isValueDelimiter(const std::string& text, std::size_t position)
    {
        position = skipWhitespace(text, position);
        return position >= text.size() || text[position] == ',' || text[position] == '}' || text[position] == ']';
    }

    bool extractUnsigned(const std::string& objectText, const std::string& key, uint64_t& value)
    {
        std::size_t start = 0u;
        if (!findKeyValueStart(objectText, key, start) || start >= objectText.size() || !std::isdigit(static_cast<unsigned char>(objectText[start]))) { return false; }
        std::size_t end = start;
        while (end < objectText.size() && std::isdigit(static_cast<unsigned char>(objectText[end]))) { ++end; }
        if (!isValueDelimiter(objectText, end)) { return false; }
        try
        {
            std::size_t parsed = 0u;
            const unsigned long long result = std::stoull(objectText.substr(start, end - start), &parsed, 10);
            if (parsed != end - start) { return false; }
            value = static_cast<uint64_t>(result);
            return true;
        }
        catch (...) { return false; }
    }

    bool extractUInt32(const std::string& objectText, const std::string& key, uint32_t& value)
    {
        uint64_t parsed = 0u;
        if (!extractUnsigned(objectText, key, parsed) || parsed > std::numeric_limits<uint32_t>::max()) { return false; }
        value = static_cast<uint32_t>(parsed);
        return true;
    }

    bool extractBool(const std::string& objectText, const std::string& key, bool& value)
    {
        std::size_t start = 0u;
        if (!findKeyValueStart(objectText, key, start)) { return false; }
        if (objectText.compare(start, 4u, "true") == 0 && isValueDelimiter(objectText, start + 4u)) { value = true; return true; }
        if (objectText.compare(start, 5u, "false") == 0 && isValueDelimiter(objectText, start + 5u)) { value = false; return true; }
        return false;
    }

    bool extractOptionalUInt64(const std::string& objectText, const std::string& key, std::optional<uint64_t>& value)
    {
        std::size_t start = 0u;
        if (!findKeyValueStart(objectText, key, start)) { return false; }
        if (objectText.compare(start, 4u, "null") == 0 && isValueDelimiter(objectText, start + 4u)) { value.reset(); return true; }
        uint64_t parsed = 0u;
        if (!extractUnsigned(objectText, key, parsed)) { return false; }
        value = parsed;
        return true;
    }


    bool extractOptionalOverrideUInt64(const std::string& objectText, const std::string& key, std::optional<uint64_t>& value)
    {
        std::size_t start = 0u;
        if (!findKeyValueStart(objectText, key, start)) { value.reset(); return true; }
        uint64_t parsed = 0u;
        if (!extractUnsigned(objectText, key, parsed)) { return false; }
        value = parsed;
        return true;
    }

    const char* generationDomainRecipeKey(PixelShipGenerator::GenerationDomain domain)
    {
        using PixelShipGenerator::GenerationDomain;
        switch (domain)
        {
        case GenerationDomain::HULL: return "hull";
        case GenerationDomain::WINGS: return "wings";
        case GenerationDomain::COCKPIT: return "cockpit";
        case GenerationDomain::ENGINES: return "engines";
        case GenerationDomain::HULL_LAYERS: return "hull_layers";
        case GenerationDomain::MAJOR_FEATURES: return "major_features";
        case GenerationDomain::MACRO_ASYMMETRY: return "macro_asymmetry";
        case GenerationDomain::WEAPONS: return "weapons";
        case GenerationDomain::ATTACHMENTS: return "attachments";
        case GenerationDomain::PALETTE: return "palette";
        case GenerationDomain::DETAILS: return "details";
        default: return "unknown";
        }
    }

    const char* randomStreamModeRecipeString(PixelShipGenerator::GenerationRandomStreamMode mode)
    {
        return mode == PixelShipGenerator::GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS ? "LEGACY_TOP_LEVEL_STREAMS" : "DOMAIN_SUBSTREAMS";
    }

    bool randomStreamModeFromRecipeString(const std::string& value, PixelShipGenerator::GenerationRandomStreamMode& mode)
    {
        if (value == "DOMAIN_SUBSTREAMS") { mode = PixelShipGenerator::GenerationRandomStreamMode::DOMAIN_SUBSTREAMS; return true; }
        if (value == "LEGACY_TOP_LEVEL_STREAMS") { mode = PixelShipGenerator::GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS; return true; }
        return false;
    }

    const char* animationSamplingModeRecipeString(PixelShipGenerator::AnimationSamplingMode mode)
    {
        return mode == PixelShipGenerator::AnimationSamplingMode::EXACT_FRAME_COUNT ? "EXACT_FRAME_COUNT" : "ADAPTIVE";
    }

    bool animationSamplingModeFromRecipeString(const std::string& value, PixelShipGenerator::AnimationSamplingMode& mode)
    {
        if (value == "ADAPTIVE") { mode = PixelShipGenerator::AnimationSamplingMode::ADAPTIVE; return true; }
        if (value == "EXACT_FRAME_COUNT") { mode = PixelShipGenerator::AnimationSamplingMode::EXACT_FRAME_COUNT; return true; }
        return false;
    }

    std::string boolString(bool value) { return value ? "true" : "false"; }

    ShipGenerationRecipeLoadResult errorResult(const std::string& error)
    {
        ShipGenerationRecipeLoadResult result;
        result.Error = error;
        return result;
    }

    bool validateRecipe(const PreviewGenerationRecipe& recipe, std::string& error)
    {
        if (recipe.Dimensions.Width == 0u || recipe.Dimensions.Height == 0u) { error = "Resolution dimensions must be greater than zero."; return false; }
        if (recipe.Dimensions.Width > 4096u || recipe.Dimensions.Height > 4096u) { error = "Resolution dimensions are unreasonably large."; return false; }
        if (recipe.DetailDensity > 100u) { error = "detail_density must be in the range 0-100."; return false; }
        if (recipe.AsymmetricDetailChance > 100u) { error = "asymmetric_detail_chance must be in the range 0-100."; return false; }
        return true;
    }

    bool validateAnimation(const PixelShipGenerator::ShipIdleAnimationSettings& settings, std::string& error)
    {
        if (settings.AnimationDurationMilliseconds == 0u || settings.AnimationDurationMilliseconds > 120000u) { error = "animation.duration_milliseconds must be in the range 1-120000."; return false; }
        if (settings.FrameCount == 0u || settings.FrameCount > 1000u) { error = "animation.exact_frame_count must be in the range 1-1000."; return false; }
        if (settings.MinimumFrameCount == 0u || settings.MinimumFrameCount > 1000u) { error = "animation.minimum_frame_count must be in the range 1-1000."; return false; }
        if (settings.MaximumFrameCount == 0u || settings.MaximumFrameCount > 1000u) { error = "animation.maximum_frame_count must be in the range 1-1000."; return false; }
        if (settings.MinimumFrameCount > settings.MaximumFrameCount) { error = "animation.minimum_frame_count must not exceed animation.maximum_frame_count."; return false; }
        if (settings.SamplingMode != PixelShipGenerator::AnimationSamplingMode::ADAPTIVE && settings.SamplingMode != PixelShipGenerator::AnimationSamplingMode::EXACT_FRAME_COUNT) { error = "animation.sampling_mode is invalid."; return false; }
        if (settings.SamplingMode == PixelShipGenerator::AnimationSamplingMode::EXACT_FRAME_COUNT && (settings.FrameCount < settings.MinimumFrameCount || settings.FrameCount > settings.MaximumFrameCount)) { error = "animation.exact_frame_count must be within animation frame limits in EXACT_FRAME_COUNT mode."; return false; }
        return true;
    }
}

namespace PixelShipGeneratorPreview
{
    std::string shipStyleToRecipeString(PixelShipGenerator::ShipStyle style)
    {
        switch (style)
        {
        case PixelShipGenerator::ShipStyle::SLEEK: return "SLEEK";
        case PixelShipGenerator::ShipStyle::FIGHTER: return "FIGHTER";
        case PixelShipGenerator::ShipStyle::HEAVY: return "HEAVY";
        case PixelShipGenerator::ShipStyle::INDUSTRIAL: return "INDUSTRIAL";
        case PixelShipGenerator::ShipStyle::SPEARHEAD: return "SPEARHEAD";
        case PixelShipGenerator::ShipStyle::DELTA: return "DELTA";
        default: throw std::runtime_error("Cannot serialize unknown ShipStyle value.");
        }
    }

    std::string shipFactionToRecipeString(PixelShipGenerator::ShipFactionType faction)
    {
        switch (faction)
        {
        case PixelShipGenerator::ShipFactionType::FRONTIER: return "FRONTIER";
        case PixelShipGenerator::ShipFactionType::MILITARY: return "MILITARY";
        case PixelShipGenerator::ShipFactionType::ASCENDANT: return "ASCENDANT";
        case PixelShipGenerator::ShipFactionType::XENO: return "XENO";
        case PixelShipGenerator::ShipFactionType::CORPORATE: return "CORPORATE";
        case PixelShipGenerator::ShipFactionType::RELIC: return "RELIC";
        default: throw std::runtime_error("Cannot serialize unknown ShipFactionType value.");
        }
    }

    bool shipStyleFromRecipeString(const std::string& value, PixelShipGenerator::ShipStyle& style)
    {
        if (value == "SLEEK") { style = PixelShipGenerator::ShipStyle::SLEEK; return true; }
        if (value == "FIGHTER") { style = PixelShipGenerator::ShipStyle::FIGHTER; return true; }
        if (value == "HEAVY") { style = PixelShipGenerator::ShipStyle::HEAVY; return true; }
        if (value == "INDUSTRIAL") { style = PixelShipGenerator::ShipStyle::INDUSTRIAL; return true; }
        if (value == "SPEARHEAD") { style = PixelShipGenerator::ShipStyle::SPEARHEAD; return true; }
        if (value == "DELTA") { style = PixelShipGenerator::ShipStyle::DELTA; return true; }
        return false;
    }

    bool shipFactionFromRecipeString(const std::string& value, PixelShipGenerator::ShipFactionType& faction)
    {
        if (value == "FRONTIER") { faction = PixelShipGenerator::ShipFactionType::FRONTIER; return true; }
        if (value == "MILITARY") { faction = PixelShipGenerator::ShipFactionType::MILITARY; return true; }
        if (value == "ASCENDANT") { faction = PixelShipGenerator::ShipFactionType::ASCENDANT; return true; }
        if (value == "XENO") { faction = PixelShipGenerator::ShipFactionType::XENO; return true; }
        if (value == "CORPORATE") { faction = PixelShipGenerator::ShipFactionType::CORPORATE; return true; }
        if (value == "RELIC") { faction = PixelShipGenerator::ShipFactionType::RELIC; return true; }
        return false;
    }

    std::string serializeShipGenerationRecipe(const ShipGenerationRecipeDocument& document)
    {
        const PreviewGenerationRecipe& recipe = document.Recipe;
        std::ostringstream stream;
        stream << "{\n";
        stream << "  \"format_version\": " << ShipGenerationRecipeFormatVersion << ",\n";
        stream << "  \"ship\": {\n";
        stream << "    \"dimensions\": { \"width\": " << recipe.Dimensions.Width << ", \"height\": " << recipe.Dimensions.Height << " },\n";
        stream << "    \"style\": \"" << shipStyleToRecipeString(recipe.Style) << "\",\n";
        stream << "    \"faction\": \"" << shipFactionToRecipeString(recipe.Faction) << "\",\n";
        stream << "    \"seeds\": {\n";
        stream << "      \"master\": " << recipe.Seeds.Master << ",\n";
        stream << "      \"structure\": " << recipe.Seeds.Structure << ",\n";
        stream << "      \"palette\": " << recipe.Seeds.Palette << ",\n";
        stream << "      \"details\": " << recipe.Seeds.Details << ",\n";
        stream << "      \"attachments\": " << recipe.Seeds.Attachments << ",\n";
        stream << "      \"rng_mode\": \"" << randomStreamModeRecipeString(recipe.RandomStreamMode) << "\"";
        if (recipe.DomainSeedOverrides.hasAny())
        {
            stream << ",\n      \"domains\": {\n";
            bool firstDomain = true;
            for (std::size_t index = 0u; index < PixelShipGenerator::GenerationDomainCount; ++index)
            {
                const std::optional<uint64_t>& overrideSeed = recipe.DomainSeedOverrides.Values[index];
                if (!overrideSeed.has_value()) { continue; }
                if (!firstDomain) { stream << ",\n"; }
                firstDomain = false;
                const auto domain = static_cast<PixelShipGenerator::GenerationDomain>(index);
                stream << "        \"" << generationDomainRecipeKey(domain) << "\": " << *overrideSeed;
            }
            stream << "\n      }";
        }
        stream << "\n    },\n";
        stream << "    \"settings\": {\n";
        stream << "      \"detail_density\": " << recipe.DetailDensity << ",\n";
        stream << "      \"asymmetric_detail_chance\": " << recipe.AsymmetricDetailChance << ",\n";
        stream << "      \"attachments_enabled\": " << boolString(recipe.AttachmentsEnabled) << "\n";
        stream << "    }\n";
        stream << "  }";

        if (document.AnimationSettings.has_value())
        {
            const PixelShipGenerator::ShipIdleAnimationSettings& animation = *document.AnimationSettings;
            stream << ",\n  \"animation\": {\n";
            stream << "    \"seed\": ";
            if (animation.Seed.has_value()) { stream << *animation.Seed; }
            else { stream << "null"; }
            stream << ",\n";
            stream << "    \"duration_milliseconds\": " << animation.AnimationDurationMilliseconds << ",\n";
            stream << "    \"exact_frame_count\": " << animation.FrameCount << ",\n";
            stream << "    \"minimum_frame_count\": " << animation.MinimumFrameCount << ",\n";
            stream << "    \"maximum_frame_count\": " << animation.MaximumFrameCount << ",\n";
            stream << "    \"sampling_mode\": \"" << animationSamplingModeRecipeString(animation.SamplingMode) << "\",\n";
            stream << "    \"engine_flicker\": " << boolString(animation.EngineFlicker) << ",\n";
            stream << "    \"light_blinking\": " << boolString(animation.LightBlinking) << ",\n";
            stream << "    \"mechanical_micro_movement\": " << boolString(animation.MechanicalMicroMovement) << ",\n";
            stream << "    \"hover_offset\": " << boolString(animation.HoverOffset) << ",\n";
            stream << "    \"small_detail_variation\": " << boolString(animation.SmallDetailVariation) << "\n";
            stream << "  }";
        }

        stream << "\n}\n";
        return stream.str();
    }

    ShipGenerationRecipeLoadResult deserializeShipGenerationRecipe(const std::string& jsonText)
    {
        try
        {
            const std::size_t first = skipWhitespace(jsonText, 0u);
            std::size_t last = jsonText.size();
            while (last > first && std::isspace(static_cast<unsigned char>(jsonText[last - 1u]))) { --last; }
            if (first >= last || jsonText[first] != '{' || jsonText[last - 1u] != '}') { return errorResult("Failed to parse JSON object."); }
            uint32_t formatVersion = 0u;
            if (!extractUInt32(jsonText, "format_version", formatVersion)) { return errorResult("Missing or invalid field: format_version."); }
            if (formatVersion != 1u && formatVersion != 2u && formatVersion != 3u && formatVersion != ShipGenerationRecipeFormatVersion) { return errorResult("Unsupported format version: " + std::to_string(formatVersion) + "."); }

            std::string shipObject;
            if (!extractObject(jsonText, "ship", shipObject)) { return errorResult("Missing or invalid object: ship."); }
            std::string dimensionsObject;
            std::string seedsObject;
            std::string settingsObject;
            if (!extractObject(shipObject, "seeds", seedsObject)) { return errorResult("Missing or invalid object: ship.seeds."); }
            if (!extractObject(shipObject, "settings", settingsObject)) { return errorResult("Missing or invalid object: ship.settings."); }

            ShipGenerationRecipeDocument document;
            PreviewGenerationRecipe& recipe = document.Recipe;

            if (formatVersion >= 2u)
            {
                if (!extractObject(shipObject, "dimensions", dimensionsObject)) { return errorResult("Missing or invalid object: ship.dimensions."); }
                if (!extractUInt32(dimensionsObject, "width", recipe.Dimensions.Width)) { return errorResult("Missing or invalid field: ship.dimensions.width."); }
                if (!extractUInt32(dimensionsObject, "height", recipe.Dimensions.Height)) { return errorResult("Missing or invalid field: ship.dimensions.height."); }
            }
            else if (extractObject(shipObject, "resolution", dimensionsObject))
            {
                if (!extractUInt32(dimensionsObject, "width", recipe.Dimensions.Width)) { return errorResult("Missing or invalid field: ship.resolution.width."); }
                if (!extractUInt32(dimensionsObject, "height", recipe.Dimensions.Height)) { return errorResult("Missing or invalid field: ship.resolution.height."); }
            }
            else
            {
                uint32_t legacyResolution = 0u;
                if (!extractUInt32(shipObject, "resolution", legacyResolution)) { return errorResult("Missing or invalid field: ship.resolution."); }
                recipe.Dimensions = { legacyResolution, legacyResolution };
            }

            std::string styleString;
            std::string factionString;
            if (!extractString(shipObject, "style", styleString)) { return errorResult("Missing or invalid field: ship.style."); }
            if (!shipStyleFromRecipeString(styleString, recipe.Style)) { return errorResult("Unknown style: " + styleString + "."); }
            if (!extractString(shipObject, "faction", factionString)) { return errorResult("Missing or invalid field: ship.faction."); }
            if (!shipFactionFromRecipeString(factionString, recipe.Faction)) { return errorResult("Unknown faction: " + factionString + "."); }

            if (!extractUnsigned(seedsObject, "master", recipe.Seeds.Master)) { return errorResult("Missing or invalid field: ship.seeds.master."); }
            if (!extractUnsigned(seedsObject, "structure", recipe.Seeds.Structure)) { return errorResult("Missing or invalid field: ship.seeds.structure."); }
            if (!extractUnsigned(seedsObject, "palette", recipe.Seeds.Palette)) { return errorResult("Missing or invalid field: ship.seeds.palette."); }
            if (!extractUnsigned(seedsObject, "details", recipe.Seeds.Details)) { return errorResult("Missing or invalid field: ship.seeds.details."); }
            if (!extractUnsigned(seedsObject, "attachments", recipe.Seeds.Attachments)) { return errorResult("Missing or invalid field: ship.seeds.attachments."); }

            if (formatVersion >= 3u)
            {
                std::string rngMode;
                if (!extractString(seedsObject, "rng_mode", rngMode)) { return errorResult("Missing or invalid field: ship.seeds.rng_mode."); }
                if (!randomStreamModeFromRecipeString(rngMode, recipe.RandomStreamMode)) { return errorResult("Unknown ship.seeds.rng_mode: " + rngMode + "."); }

                std::string domainsObject;
                if (seedsObject.find("\"domains\"") != std::string::npos)
                {
                    if (!extractObject(seedsObject, "domains", domainsObject)) { return errorResult("Invalid object: ship.seeds.domains."); }
                    for (std::size_t index = 0u; index < PixelShipGenerator::GenerationDomainCount; ++index)
                    {
                        const auto domain = static_cast<PixelShipGenerator::GenerationDomain>(index);
                        if (!extractOptionalOverrideUInt64(domainsObject, generationDomainRecipeKey(domain), recipe.DomainSeedOverrides.Values[index]))
                        {
                            return errorResult(std::string("Invalid domain seed override: ship.seeds.domains.") + generationDomainRecipeKey(domain) + ".");
                        }
                    }
                }
            }
            else
            {
                recipe.RandomStreamMode = PixelShipGenerator::GenerationRandomStreamMode::LEGACY_TOP_LEVEL_STREAMS;
                recipe.DomainSeedOverrides.clearAll();
            }
            if (!extractUInt32(settingsObject, "detail_density", recipe.DetailDensity)) { return errorResult("Missing or invalid field: ship.settings.detail_density."); }
            if (!extractUInt32(settingsObject, "asymmetric_detail_chance", recipe.AsymmetricDetailChance)) { return errorResult("Missing or invalid field: ship.settings.asymmetric_detail_chance."); }
            if (!extractBool(settingsObject, "attachments_enabled", recipe.AttachmentsEnabled)) { return errorResult("Missing or invalid field: ship.settings.attachments_enabled."); }

            std::string validationError;
            if (!validateRecipe(recipe, validationError)) { return errorResult(validationError); }

            std::string animationObject;
            const bool animationFieldPresent = jsonText.find("\"animation\"") != std::string::npos;
            if (animationFieldPresent && !extractObject(jsonText, "animation", animationObject)) { return errorResult("Invalid object: animation."); }
            if (animationFieldPresent)
            {
                PixelShipGenerator::ShipIdleAnimationSettings animation;
                if (!extractOptionalUInt64(animationObject, "seed", animation.Seed)) { return errorResult("Missing or invalid field: animation.seed."); }
                if (formatVersion >= 4u)
                {
                    std::string samplingMode;
                    if (!extractUInt32(animationObject, "duration_milliseconds", animation.AnimationDurationMilliseconds)) { return errorResult("Missing or invalid field: animation.duration_milliseconds."); }
                    if (!extractUInt32(animationObject, "exact_frame_count", animation.FrameCount)) { return errorResult("Missing or invalid field: animation.exact_frame_count."); }
                    if (!extractUInt32(animationObject, "minimum_frame_count", animation.MinimumFrameCount)) { return errorResult("Missing or invalid field: animation.minimum_frame_count."); }
                    if (!extractUInt32(animationObject, "maximum_frame_count", animation.MaximumFrameCount)) { return errorResult("Missing or invalid field: animation.maximum_frame_count."); }
                    if (!extractString(animationObject, "sampling_mode", samplingMode)) { return errorResult("Missing or invalid field: animation.sampling_mode."); }
                    if (!animationSamplingModeFromRecipeString(samplingMode, animation.SamplingMode)) { return errorResult("Unknown animation.sampling_mode: " + samplingMode + "."); }
                }
                else
                {
                    if (!extractUInt32(animationObject, "frame_count", animation.FrameCount)) { return errorResult("Missing or invalid field: animation.frame_count."); }
                    animation.AnimationDurationMilliseconds = animation.FrameCount * 100u;
                    animation.MinimumFrameCount = animation.FrameCount;
                    animation.MaximumFrameCount = animation.FrameCount;
                    animation.SamplingMode = PixelShipGenerator::AnimationSamplingMode::EXACT_FRAME_COUNT;
                }
                if (!extractBool(animationObject, "engine_flicker", animation.EngineFlicker)) { return errorResult("Missing or invalid field: animation.engine_flicker."); }
                if (!extractBool(animationObject, "light_blinking", animation.LightBlinking)) { return errorResult("Missing or invalid field: animation.light_blinking."); }
                if (!extractBool(animationObject, "mechanical_micro_movement", animation.MechanicalMicroMovement)) { return errorResult("Missing or invalid field: animation.mechanical_micro_movement."); }
                if (!extractBool(animationObject, "hover_offset", animation.HoverOffset)) { return errorResult("Missing or invalid field: animation.hover_offset."); }
                if (!extractBool(animationObject, "small_detail_variation", animation.SmallDetailVariation)) { return errorResult("Missing or invalid field: animation.small_detail_variation."); }
                if (!validateAnimation(animation, validationError)) { return errorResult(validationError); }
                document.AnimationSettings = animation;
            }

            ShipGenerationRecipeLoadResult result;
            result.Success = true;
            result.Document = document;
            return result;
        }
        catch (const std::exception& exception)
        {
            return errorResult(std::string("Failed to parse recipe JSON: ") + exception.what());
        }
    }

    bool saveShipGenerationRecipe(const ShipGenerationRecipeDocument& document, const std::filesystem::path& path, std::string& error)
    {
        try
        {
            std::ofstream stream(path, std::ios::binary);
            if (!stream) { error = "Failed to open recipe file for writing: " + path.string(); return false; }
            stream << serializeShipGenerationRecipe(document);
            if (!stream) { error = "Failed while writing recipe file: " + path.string(); return false; }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = std::string("Recipe export failed: ") + exception.what();
            return false;
        }
    }

    ShipGenerationRecipeLoadResult loadShipGenerationRecipe(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream) { return errorResult("Failed to open recipe file: " + path.string()); }
        std::ostringstream buffer;
        buffer << stream.rdbuf();
        if (!stream.good() && !stream.eof()) { return errorResult("Failed while reading recipe file: " + path.string()); }
        return deserializeShipGenerationRecipe(buffer.str());
    }
}
