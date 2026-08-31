#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <SpectralShipGen/GeneratedShip.h>
#include <SpectralShipGen/GenerationTuningProfile.h>
#include <SpectralShipGen/ShipGenerationDebugInfo.h>
#include <SpectralShipGen/ShipGenerator.h>
#include <SpectralShipGen/Diagnostics/GenerationStatistics.h>

#include "PreviewGenerationRecipe.h"

namespace SpectralShipGenStudioPreview
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
        std::optional<SpectralShipGen::ShipStyle> Style;
        std::optional<SpectralShipGen::ShipFactionType> Faction;
        CalibrationDimensionBucket DimensionBucket = CalibrationDimensionBucket::ANY;
    };

    struct CalibrationComparisonRecord
    {
        uint64_t PairIndex = 0u;
        SpectralShipGen::GenerationWeightGroup Group = SpectralShipGen::GenerationWeightGroup::ENGINE_LAYOUT;
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
        SpectralShipGen::GenerationWeightGroup Group = SpectralShipGen::GenerationWeightGroup::ENGINE_LAYOUT;
        std::vector<CalibrationOptionStatistics> Options;
        uint32_t UsefulComparisonCount = 0u;
        CalibrationEvidenceLevel Evidence = CalibrationEvidenceLevel::INSUFFICIENT;
    };

    struct GenerationCalibrationSession
    {
        uint32_t FormatVersion = 1u;
        uint64_t RootSeed = 0xD6E8FEB86659FD93ull;
        SpectralShipGen::GenerationTuningProfile DefaultProfile;
        SpectralShipGen::GenerationTuningProfile TunedProfile;
        std::array<uint64_t, static_cast<std::size_t>(SpectralShipGen::GenerationWeightGroup::GENERATION_WEIGHT_GROUP_END)> PairSequenceIndices = {};
        std::vector<CalibrationComparisonRecord> Records;
    };


    struct CalibrationObjectiveBatch
    {
        uint32_t SampleCount = 0u;
        SpectralShipGenDiagnostics::GenerationStatistics Production;
        SpectralShipGenDiagnostics::GenerationStatistics Tuned;
        bool Valid = false;
    };

    struct CalibrationCandidatePair
    {
        uint64_t PairIndex = 0u;
        SpectralShipGen::GenerationWeightGroup Group = SpectralShipGen::GenerationWeightGroup::ENGINE_LAYOUT;
        uint32_t OptionA = 0u;
        uint32_t OptionB = 1u;
        bool DisplayAOnLeft = true;
        PreviewGenerationRecipe Recipe;
        SpectralShipGen::GeneratedShip ShipA;
        SpectralShipGen::GeneratedShip ShipB;
        SpectralShipGen::ShipGenerationDebugInfo DebugA;
        SpectralShipGen::ShipGenerationDebugInfo DebugB;
        std::string IsolationNote;
        bool Valid = false;
    };

    GenerationCalibrationSession createGenerationCalibrationSession(uint64_t rootSeed);
    CalibrationObjectiveBatch collectCalibrationObjectiveBatch(SpectralShipGen::ShipGenerator& generator, const GenerationCalibrationSession& session, const PreviewGenerationRecipe& contextRecipe, uint32_t sampleCount = 12u);
    CalibrationCandidatePair generateNextCalibrationPair(SpectralShipGen::ShipGenerator& generator, GenerationCalibrationSession& session, const PreviewGenerationRecipe& contextRecipe, SpectralShipGen::GenerationWeightGroup group);
    void recordCalibrationPreference(GenerationCalibrationSession& session, const CalibrationCandidatePair& pair, CalibrationPreferenceResult result);
    CalibrationGroupStatistics calculateCalibrationGroupStatistics(const GenerationCalibrationSession& session, SpectralShipGen::GenerationWeightGroup group, const CalibrationContextFilter& filter = {});
    std::vector<uint32_t> calculateSuggestedGroupWeights(const GenerationCalibrationSession& session, SpectralShipGen::ShipStyle style, SpectralShipGen::GenerationWeightGroup group, const CalibrationContextFilter& filter = {});
    void applySuggestedGroupWeights(GenerationCalibrationSession& session, SpectralShipGen::ShipStyle style, SpectralShipGen::GenerationWeightGroup group, const CalibrationContextFilter& filter = {});
    void resetCalibrationGroup(GenerationCalibrationSession& session, SpectralShipGen::ShipStyle style, SpectralShipGen::GenerationWeightGroup group);
    void resetAllCalibrationTuning(GenerationCalibrationSession& session);

    const char* getCalibrationGroupName(SpectralShipGen::GenerationWeightGroup group);
    const char* getCalibrationOptionName(SpectralShipGen::GenerationWeightGroup group, uint32_t optionIndex);
    const char* getCalibrationEvidenceName(CalibrationEvidenceLevel level);
    const char* getCalibrationDimensionBucketName(CalibrationDimensionBucket bucket);
    CalibrationDimensionBucket getCalibrationDimensionBucket(const SpectralShipGen::ShipDimensions& dimensions);
    bool calibrationRecordMatchesFilter(const CalibrationComparisonRecord& record, const CalibrationContextFilter& filter);
}
