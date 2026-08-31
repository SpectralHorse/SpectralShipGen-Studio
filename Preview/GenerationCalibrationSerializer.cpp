#include "GenerationCalibrationSerializer.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <vector>

#include <SpectralShipGen/ShipGenerationRecipeSerializer.h>

namespace
{
    using namespace SpectralShipGen;
    using namespace SpectralShipGenStudioPreview;

    std::size_t skipWhitespace(const std::string& text, std::size_t position)
    {
        while (position < text.size() && std::isspace(static_cast<unsigned char>(text[position]))) { ++position; }
        return position;
    }

    bool findKeyValueStart(const std::string& text, const std::string& key, std::size_t& valueStart)
    {
        const std::string quoted = "\"" + key + "\"";
        const std::size_t keyPosition = text.find(quoted);
        if (keyPosition == std::string::npos) { return false; }
        std::size_t colon = skipWhitespace(text, keyPosition + quoted.size());
        if (colon >= text.size() || text[colon] != ':') { return false; }
        valueStart = skipWhitespace(text, colon + 1u);
        return valueStart < text.size();
    }

    bool extractUnsigned(const std::string& text, const std::string& key, uint64_t& value)
    {
        std::size_t start = 0u;
        if (!findKeyValueStart(text, key, start) || start >= text.size() || !std::isdigit(static_cast<unsigned char>(text[start]))) { return false; }
        std::size_t end = start;
        while (end < text.size() && std::isdigit(static_cast<unsigned char>(text[end]))) { ++end; }
        try { value = static_cast<uint64_t>(std::stoull(text.substr(start, end - start))); return true; }
        catch (...) { return false; }
    }

    bool extractUInt32(const std::string& text, const std::string& key, uint32_t& value)
    {
        uint64_t parsed = 0u;
        if (!extractUnsigned(text, key, parsed) || parsed > std::numeric_limits<uint32_t>::max()) { return false; }
        value = static_cast<uint32_t>(parsed);
        return true;
    }

    bool extractBool(const std::string& text, const std::string& key, bool& value)
    {
        std::size_t start = 0u;
        if (!findKeyValueStart(text, key, start)) { return false; }
        if (text.compare(start, 4u, "true") == 0) { value = true; return true; }
        if (text.compare(start, 5u, "false") == 0) { value = false; return true; }
        return false;
    }

    bool extractObject(const std::string& text, const std::string& key, std::string& object)
    {
        std::size_t position = 0u;
        if (!findKeyValueStart(text, key, position) || text[position] != '{') { return false; }
        const std::size_t start = position;
        uint32_t depth = 0u;
        bool inString = false;
        bool escaped = false;
        for (; position < text.size(); ++position)
        {
            const char c = text[position];
            if (inString)
            {
                if (escaped) { escaped = false; continue; }
                if (c == '\\') { escaped = true; continue; }
                if (c == '"') { inString = false; }
                continue;
            }
            if (c == '"') { inString = true; continue; }
            if (c == '{') { ++depth; }
            else if (c == '}')
            {
                if (depth == 0u) { return false; }
                --depth;
                if (depth == 0u) { object = text.substr(start, position - start + 1u); return true; }
            }
        }
        return false;
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
            bool complete = false;
            for (; position < text.size(); ++position)
            {
                const char c = text[position];
                if (inString)
                {
                    if (escaped) { escaped = false; continue; }
                    if (c == '\\') { escaped = true; continue; }
                    if (c == '"') { inString = false; }
                    continue;
                }
                if (c == '"') { inString = true; continue; }
                if (c == '{') { ++depth; }
                else if (c == '}')
                {
                    --depth;
                    if (depth == 0u) { objects.push_back(text.substr(start, position - start + 1u)); ++position; complete = true; break; }
                }
            }
            if (!complete) { return false; }
            position = skipWhitespace(text, position);
            if (position >= text.size()) { return false; }
            if (text[position] == ']') { return true; }
            if (text[position] != ',') { return false; }
            ++position;
        }
    }

    bool extractUIntArray(const std::string& text, const std::string& key, std::vector<uint64_t>& values)
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
            try { values.push_back(static_cast<uint64_t>(std::stoull(text.substr(position, end - position)))); }
            catch (...) { return false; }
            position = skipWhitespace(text, end);
            if (position >= text.size()) { return false; }
            if (text[position] == ']') { return true; }
            if (text[position] != ',') { return false; }
            ++position;
        }
    }

    const char* styleKey(ShipStyle style)
    {
        switch (style)
        {
        case ShipStyle::SLEEK: return "SLEEK";
        case ShipStyle::FIGHTER: return "FIGHTER";
        case ShipStyle::HEAVY: return "HEAVY";
        case ShipStyle::INDUSTRIAL: return "INDUSTRIAL";
        case ShipStyle::SPEARHEAD: return "SPEARHEAD";
        case ShipStyle::DELTA: return "DELTA";
        default: return "UNKNOWN";
        }
    }

    const char* groupKey(GenerationWeightGroup group)
    {
        switch (group)
        {
        case GenerationWeightGroup::ENGINE_LAYOUT: return "ENGINE_LAYOUT";
        case GenerationWeightGroup::ENGINE_SIZE: return "ENGINE_SIZE";
        case GenerationWeightGroup::HULL_MODIFIER: return "HULL_MODIFIER";
        case GenerationWeightGroup::MAJOR_FEATURE_TYPE: return "MAJOR_FEATURE_TYPE";
        case GenerationWeightGroup::ATTACHMENT_TYPE: return "ATTACHMENT_TYPE";
        case GenerationWeightGroup::LARGE_WEAPON_TYPE: return "LARGE_WEAPON_TYPE";
        case GenerationWeightGroup::ENGINE_NACELLE_PRESENCE: return "ENGINE_NACELLE_PRESENCE";
        case GenerationWeightGroup::MAJOR_FEATURE_PRESENCE: return "MAJOR_FEATURE_PRESENCE";
        case GenerationWeightGroup::ATTACHMENT_PRESENCE: return "ATTACHMENT_PRESENCE";
        case GenerationWeightGroup::LARGE_WEAPON_PRESENCE: return "LARGE_WEAPON_PRESENCE";
        default: return "UNKNOWN";
        }
    }

    void writeProfile(std::ostringstream& stream, const GenerationTuningProfile& profile, const char* key, const std::string& indent)
    {
        stream << indent << "\"" << key << "\": {\n";
        for (uint32_t styleIndex = 0u; styleIndex < static_cast<uint32_t>(ShipStyle::SHIP_STYLE_END); ++styleIndex)
        {
            const ShipStyle style = static_cast<ShipStyle>(styleIndex);
            stream << indent << "  \"" << styleKey(style) << "\": {\n";
            for (uint32_t groupIndex = 0u; groupIndex < static_cast<uint32_t>(GenerationWeightGroup::GENERATION_WEIGHT_GROUP_END); ++groupIndex)
            {
                const GenerationWeightGroup group = static_cast<GenerationWeightGroup>(groupIndex);
                stream << indent << "    \"" << groupKey(group) << "\": [";
                const uint32_t count = getGenerationWeightOptionCount(group);
                for (uint32_t option = 0u; option < count; ++option)
                {
                    if (option > 0u) { stream << ", "; }
                    stream << getGenerationTuningWeight(profile, style, group, option);
                }
                stream << "]" << (groupIndex + 1u < static_cast<uint32_t>(GenerationWeightGroup::GENERATION_WEIGHT_GROUP_END) ? "," : "") << "\n";
            }
            stream << indent << "  }" << (styleIndex + 1u < static_cast<uint32_t>(ShipStyle::SHIP_STYLE_END) ? "," : "") << "\n";
        }
        stream << indent << "}";
    }

    bool readProfile(const std::string& root, const char* key, GenerationTuningProfile& profile)
    {
        std::string profileObject;
        if (!extractObject(root, key, profileObject)) { return false; }
        for (uint32_t styleIndex = 0u; styleIndex < static_cast<uint32_t>(ShipStyle::SHIP_STYLE_END); ++styleIndex)
        {
            const ShipStyle style = static_cast<ShipStyle>(styleIndex);
            std::string styleObject;
            if (!extractObject(profileObject, styleKey(style), styleObject)) { return false; }
            for (uint32_t groupIndex = 0u; groupIndex < static_cast<uint32_t>(GenerationWeightGroup::GENERATION_WEIGHT_GROUP_END); ++groupIndex)
            {
                const GenerationWeightGroup group = static_cast<GenerationWeightGroup>(groupIndex);
                std::vector<uint64_t> values;
                if (!extractUIntArray(styleObject, groupKey(group), values) || values.size() != getGenerationWeightOptionCount(group)) { return false; }
                for (uint32_t option = 0u; option < values.size(); ++option)
                {
                    if (values[option] > std::numeric_limits<uint32_t>::max()) { return false; }
                    setGenerationTuningWeight(profile, style, group, option, static_cast<uint32_t>(values[option]));
                }
            }
        }
        return true;
    }

    const char* resultKey(CalibrationPreferenceResult result)
    {
        switch (result)
        {
        case CalibrationPreferenceResult::PREFER_A: return "PREFER_A";
        case CalibrationPreferenceResult::PREFER_B: return "PREFER_B";
        case CalibrationPreferenceResult::NO_PREFERENCE: return "NO_PREFERENCE";
        case CalibrationPreferenceResult::SKIP: return "SKIP";
        default: return "SKIP";
        }
    }
}

namespace SpectralShipGenStudioPreview
{
    std::string serializeGenerationCalibrationSession(const GenerationCalibrationSession& session)
    {
        std::ostringstream stream;
        stream << "{\n";
        stream << "  \"format_version\": " << session.FormatVersion << ",\n";
        stream << "  \"root_seed\": " << session.RootSeed << ",\n";
        stream << "  \"pair_sequence_indices\": [";
        for (std::size_t index = 0u; index < session.PairSequenceIndices.size(); ++index) { if (index > 0u) { stream << ", "; } stream << session.PairSequenceIndices[index]; }
        stream << "],\n";
        writeProfile(stream, session.DefaultProfile, "default_profile", "  ");
        stream << ",\n";
        writeProfile(stream, session.TunedProfile, "tuned_profile", "  ");
        stream << ",\n  \"records\": [\n";
        for (std::size_t index = 0u; index < session.Records.size(); ++index)
        {
            const CalibrationComparisonRecord& record = session.Records[index];
            ShipGenerationRecipeDocument recipeDocument;
            recipeDocument.Recipe = record.Recipe;
            stream << "    { \"pair_index\": " << record.PairIndex << ", \"group\": " << static_cast<uint32_t>(record.Group) << ", \"option_a\": " << record.OptionA << ", \"option_b\": " << record.OptionB << ", \"display_a_on_left\": " << (record.DisplayAOnLeft ? "true" : "false") << ", \"result\": " << static_cast<uint32_t>(record.Result) << ", \"recipe\": " << serializeShipGenerationRecipe(recipeDocument) << " }";
            if (index + 1u < session.Records.size()) { stream << ','; }
            stream << '\n';
        }
        stream << "  ]\n}\n";
        return stream.str();
    }

    GenerationCalibrationSessionLoadResult deserializeGenerationCalibrationSession(const std::string& text)
    {
        GenerationCalibrationSessionLoadResult result;
        uint32_t version = 0u;
        if (!extractUInt32(text, "format_version", version) || version != 2u) { result.Error = "Unsupported or missing calibration format_version."; return result; }
        result.Session.FormatVersion = version;
        if (!extractUnsigned(text, "root_seed", result.Session.RootSeed)) { result.Error = "Missing root_seed."; return result; }
        std::vector<uint64_t> sequence;
        if (!extractUIntArray(text, "pair_sequence_indices", sequence) || sequence.size() != result.Session.PairSequenceIndices.size()) { result.Error = "Invalid pair_sequence_indices."; return result; }
        std::copy(sequence.begin(), sequence.end(), result.Session.PairSequenceIndices.begin());
        if (!readProfile(text, "default_profile", result.Session.DefaultProfile) || !readProfile(text, "tuned_profile", result.Session.TunedProfile)) { result.Error = "Invalid calibration tuning profile snapshot."; return result; }

        std::vector<std::string> records;
        if (!extractObjectArray(text, "records", records)) { result.Error = "Invalid records array."; return result; }
        for (const std::string& object : records)
        {
            CalibrationComparisonRecord record;
            uint32_t group = 0u, resultValue = 0u;
            if (!extractUnsigned(object, "pair_index", record.PairIndex) || !extractUInt32(object, "group", group) || !extractUInt32(object, "option_a", record.OptionA) || !extractUInt32(object, "option_b", record.OptionB) || !extractBool(object, "display_a_on_left", record.DisplayAOnLeft) || !extractUInt32(object, "result", resultValue)) { result.Error = "Invalid calibration record header."; return result; }
            if (group >= static_cast<uint32_t>(GenerationWeightGroup::GENERATION_WEIGHT_GROUP_END) || resultValue > static_cast<uint32_t>(CalibrationPreferenceResult::SKIP)) { result.Error = "Calibration record enum value out of range."; return result; }
            record.Group = static_cast<GenerationWeightGroup>(group);
            record.Result = static_cast<CalibrationPreferenceResult>(resultValue);
            std::string recipeJson;
            if (!extractObject(object, "recipe", recipeJson)) { result.Error = "Missing calibration record recipe."; return result; }
            const ShipGenerationRecipeLoadResult recipeLoad = deserializeShipGenerationRecipe(recipeJson);
            if (!recipeLoad.Success) { result.Error = "Invalid calibration record recipe: " + recipeLoad.Error; return result; }
            record.Recipe = recipeLoad.Document.Recipe;
            result.Session.Records.push_back(record);
        }
        result.Success = true;
        return result;
    }

    bool saveGenerationCalibrationSession(const GenerationCalibrationSession& session, const std::filesystem::path& path, std::string& error)
    {
        std::ofstream stream(path, std::ios::binary);
        if (!stream) { error = "Failed to open calibration session for writing: " + path.string(); return false; }
        stream << serializeGenerationCalibrationSession(session);
        if (!stream) { error = "Failed while writing calibration session: " + path.string(); return false; }
        return true;
    }

    GenerationCalibrationSessionLoadResult loadGenerationCalibrationSession(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream) { GenerationCalibrationSessionLoadResult result; result.Error = "Failed to open calibration session: " + path.string(); return result; }
        std::ostringstream buffer; buffer << stream.rdbuf();
        return deserializeGenerationCalibrationSession(buffer.str());
    }

    bool exportGenerationCalibrationCsv(const GenerationCalibrationSession& session, const std::filesystem::path& path, std::string& error)
    {
        std::ofstream stream(path, std::ios::binary);
        if (!stream) { error = "Failed to open calibration CSV for writing: " + path.string(); return false; }
        stream << "pair_index,group,option_a,option_b,display_a_on_left,result,width,height,structural_preset,faction_preset,dimension_bucket,master_seed,structure_seed,palette_seed,details_seed,attachments_seed\n";
        for (const CalibrationComparisonRecord& record : session.Records)
        {
            stream << record.PairIndex << ',' << groupKey(record.Group) << ',' << getCalibrationOptionName(record.Group, record.OptionA) << ',' << getCalibrationOptionName(record.Group, record.OptionB) << ',' << (record.DisplayAOnLeft ? 1 : 0) << ',' << resultKey(record.Result) << ',' << record.Recipe.Dimensions.Width << ',' << record.Recipe.Dimensions.Height << ',' << (record.Recipe.StructuralPreset.has_value() ? styleKey(*record.Recipe.StructuralPreset) : "CUSTOM") << ',' << (record.Recipe.FactionPreset.has_value() ? std::to_string(static_cast<uint32_t>(*record.Recipe.FactionPreset)) : "CUSTOM") << ',' << getCalibrationDimensionBucketName(getCalibrationDimensionBucket(record.Recipe.Dimensions)) << ',' << record.Recipe.Seeds.Master << ',' << record.Recipe.Seeds.Structure << ',' << record.Recipe.Seeds.Palette << ',' << record.Recipe.Seeds.Details << ',' << record.Recipe.Seeds.Attachments << '\n';
        }
        if (!stream) { error = "Failed while writing calibration CSV: " + path.string(); return false; }
        return true;
    }

    bool exportGenerationTuningProfile(const SpectralShipGen::GenerationTuningProfile& profile, const std::filesystem::path& path, std::string& error)
    {
        std::ofstream stream(path, std::ios::binary);
        if (!stream) { error = "Failed to open tuning profile for writing: " + path.string(); return false; }
        std::ostringstream document;
        document << "{\n  \"format_version\": 1,\n";
        writeProfile(document, profile, "tuning_profile", "  ");
        document << "\n}\n";
        stream << document.str();
        if (!stream) { error = "Failed while writing tuning profile: " + path.string(); return false; }
        return true;
    }
}
