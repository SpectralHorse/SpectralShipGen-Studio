#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "GeneratedShip.h"
#include "GenerationTuningProfile.h"
#include "ShipGenerationDebugInfo.h"
#include "ShipGenerator.h"
#include "GenerationStatistics.h"

#include "PreviewGenerationRecipe.h"

namespace PixelShipGeneratorPreview
{
    enum class CalibrationPreferenceResult : uint32_t
    {
        PREFER_A = 0u,
        PREFER_B,
        NO_PREFERENCE,
        SKIP
    };

    enum class CalibrationEvidenceLevel : uint32_t
    {
        INSUFFICIENT = 0u,
        EARLY,
        MODERATE,
        STRONGER
    };

    enum class CalibrationDimensionBucket : uint32_t
    {
        SMALL = 0u,
        MEDIUM,
        LARGE,
        ANY
    };

    struct CalibrationContextFilter
    {
        std::optional<PixelShipGenerator::ShipStyle> Style;
        std::optional<PixelShipGenerator::ShipFactionType> Faction;
        CalibrationDimensionBucket DimensionBucket = CalibrationDimensionBucket::ANY;
    };

    struct CalibrationComparisonRecord
    {
        uint64_t PairIndex = 0u;
        PixelShipGenerator::GenerationWeightGroup Group = PixelShipGenerator::GenerationWeightGroup::ENGINE_LAYOUT;
        uint32_t OptionA = 0u;
        uint32_t OptionB = 1u;
        bool DisplayAOnLeft = true;
        CalibrationPreferenceResult Result = CalibrationPreferenceResult::SKIP;
        PreviewGenerationRecipe Recipe;
    };

    struct CalibrationOptionStatistics
    {
        uint32_t Comparisons = 0u;
        uint32_t Wins = 0u;
        uint32_t Losses = 0u;
        uint32_t Ties = 0u;
        uint32_t Skips = 0u;
        double PreferenceScore = 0.5;
    };

    struct CalibrationGroupStatistics
    {
        PixelShipGenerator::GenerationWeightGroup Group = PixelShipGenerator::GenerationWeightGroup::ENGINE_LAYOUT;
        std::vector<CalibrationOptionStatistics> Options;
        uint32_t UsefulComparisonCount = 0u;
        CalibrationEvidenceLevel Evidence = CalibrationEvidenceLevel::INSUFFICIENT;
    };

    struct GenerationCalibrationSession
    {
        uint32_t FormatVersion = 1u;
        uint64_t RootSeed = 0xD6E8FEB86659FD93ull;
        PixelShipGenerator::GenerationTuningProfile DefaultProfile;
        PixelShipGenerator::GenerationTuningProfile TunedProfile;
        std::array<uint64_t, static_cast<std::size_t>(PixelShipGenerator::GenerationWeightGroup::GENERATION_WEIGHT_GROUP_END)> PairSequenceIndices = {};
        std::vector<CalibrationComparisonRecord> Records;
    };


    struct CalibrationObjectiveBatch
    {
        uint32_t SampleCount = 0u;
        PixelShipGeneratorDiagnostics::GenerationStatistics Production;
        PixelShipGeneratorDiagnostics::GenerationStatistics Tuned;
        bool Valid = false;
    };

    struct CalibrationCandidatePair
    {
        uint64_t PairIndex = 0u;
        PixelShipGenerator::GenerationWeightGroup Group = PixelShipGenerator::GenerationWeightGroup::ENGINE_LAYOUT;
        uint32_t OptionA = 0u;
        uint32_t OptionB = 1u;
        bool DisplayAOnLeft = true;
        PreviewGenerationRecipe Recipe;
        PixelShipGenerator::GeneratedShip ShipA;
        PixelShipGenerator::GeneratedShip ShipB;
        PixelShipGenerator::ShipGenerationDebugInfo DebugA;
        PixelShipGenerator::ShipGenerationDebugInfo DebugB;
        std::string IsolationNote;
        bool Valid = false;
    };

    GenerationCalibrationSession createGenerationCalibrationSession(uint64_t rootSeed);
    CalibrationObjectiveBatch collectCalibrationObjectiveBatch(PixelShipGenerator::ShipGenerator& generator, const GenerationCalibrationSession& session, const PreviewGenerationRecipe& contextRecipe, uint32_t sampleCount = 12u);
    CalibrationCandidatePair generateNextCalibrationPair(PixelShipGenerator::ShipGenerator& generator, GenerationCalibrationSession& session, const PreviewGenerationRecipe& contextRecipe, PixelShipGenerator::GenerationWeightGroup group);
    void recordCalibrationPreference(GenerationCalibrationSession& session, const CalibrationCandidatePair& pair, CalibrationPreferenceResult result);
    CalibrationGroupStatistics calculateCalibrationGroupStatistics(const GenerationCalibrationSession& session, PixelShipGenerator::GenerationWeightGroup group, const CalibrationContextFilter& filter = {});
    std::vector<uint32_t> calculateSuggestedGroupWeights(const GenerationCalibrationSession& session, PixelShipGenerator::ShipStyle style, PixelShipGenerator::GenerationWeightGroup group, const CalibrationContextFilter& filter = {});
    void applySuggestedGroupWeights(GenerationCalibrationSession& session, PixelShipGenerator::ShipStyle style, PixelShipGenerator::GenerationWeightGroup group, const CalibrationContextFilter& filter = {});
    void resetCalibrationGroup(GenerationCalibrationSession& session, PixelShipGenerator::ShipStyle style, PixelShipGenerator::GenerationWeightGroup group);
    void resetAllCalibrationTuning(GenerationCalibrationSession& session);

    const char* getCalibrationGroupName(PixelShipGenerator::GenerationWeightGroup group);
    const char* getCalibrationOptionName(PixelShipGenerator::GenerationWeightGroup group, uint32_t optionIndex);
    const char* getCalibrationEvidenceName(CalibrationEvidenceLevel level);
    const char* getCalibrationDimensionBucketName(CalibrationDimensionBucket bucket);
    CalibrationDimensionBucket getCalibrationDimensionBucket(const PixelShipGenerator::ShipDimensions& dimensions);
    bool calibrationRecordMatchesFilter(const CalibrationComparisonRecord& record, const CalibrationContextFilter& filter);
}
