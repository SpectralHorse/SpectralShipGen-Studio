#include "GenerationCalibration.h"

#include <SpectralShipGen/GenerationScaleTraits.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

#include <SpectralShipGen/ShipGenerationSeeds.h>
#include <SpectralShipGen/ShipGenerationSettings.h>

namespace
{
    using namespace SpectralShipGen;
    using namespace SpectralShipGenStudioPreview;

    constexpr uint32_t MaximumPairGenerationAttempts = 24u;
    constexpr double ScorePrior = 1.0;
    constexpr uint32_t FullSuggestionEvidence = 25u;

    std::vector<std::pair<uint32_t, uint32_t>> createBalancedPairs(uint32_t optionCount)
    {
        std::vector<std::pair<uint32_t, uint32_t>> result;
        for (uint32_t first = 0u; first < optionCount; ++first)
        {
            for (uint32_t second = first + 1u; second < optionCount; ++second)
            {
                result.emplace_back(first, second);
            }
        }
        return result;
    }

    uint64_t derivePairMasterSeed(uint64_t rootSeed, GenerationWeightGroup group, uint64_t sequenceIndex, uint32_t attempt)
    {
        const uint64_t groupValue = static_cast<uint64_t>(group) * 0x9E3779B97F4A7C15ull;
        const uint64_t sequenceValue = sequenceIndex * 0xBF58476D1CE4E5B9ull;
        const uint64_t attemptValue = static_cast<uint64_t>(attempt) * 0x94D049BB133111EBull;
        return mixGenerationSeed64(rootSeed ^ 0xC6BC279692B5CC83ull ^ groupValue ^ sequenceValue ^ attemptValue);
    }

    PreviewGenerationRecipe createPairRecipe(const PreviewGenerationRecipe& contextRecipe, uint64_t masterSeed)
    {
        PreviewGenerationRecipe result = contextRecipe;
        result.Seeds = deriveShipGenerationSeeds(masterSeed);
        result.DomainSeedOverrides.clearAll();
        result.RandomStreamMode = GenerationRandomStreamMode::DOMAIN_SUBSTREAMS;
        return result;
    }

    ShipGenerationSettings createSettings(const PreviewGenerationRecipe& recipe)
    {
        ShipGenerationSettings settings;
        settings.Seed = recipe.Seeds.Master;
        settings.Dimensions = recipe.Dimensions;
        settings.Style = recipe.Style;
        settings.Faction = recipe.Faction;
        settings.DetailDensity = recipe.DetailDensity;
        settings.AsymmetricDetailChance = recipe.AsymmetricDetailChance;
        settings.AttachmentsEnabled = recipe.AttachmentsEnabled;
        settings.SeedOverrides.Structure = recipe.Seeds.Structure;
        settings.SeedOverrides.Palette = recipe.Seeds.Palette;
        settings.SeedOverrides.Details = recipe.Seeds.Details;
        settings.SeedOverrides.Attachments = recipe.Seeds.Attachments;
        settings.DomainSeedOverrides = recipe.DomainSeedOverrides;
        settings.RandomStreamMode = recipe.RandomStreamMode;
        return settings;
    }

    GenerationCalibrationOverrides createForcedOverride(GenerationWeightGroup group, uint32_t optionIndex)
    {
        GenerationCalibrationOverrides result;
        switch (group)
        {
        case GenerationWeightGroup::ENGINE_LAYOUT: result.ForcedEngineLayout = static_cast<EngineLayoutType>(optionIndex); break;
        case GenerationWeightGroup::ENGINE_SIZE: result.ForcedEngineSize = static_cast<EngineSizeClass>(optionIndex); break;
        case GenerationWeightGroup::HULL_MODIFIER: result.ForcedHullModifier = static_cast<HullModifierType>(optionIndex); break;
        case GenerationWeightGroup::MAJOR_FEATURE_TYPE: result.ForcedMajorFeatureType = static_cast<ShipMajorFeatureType>(optionIndex); break;
        case GenerationWeightGroup::ATTACHMENT_TYPE: result.ForcedAttachmentType = static_cast<ShipAttachmentType>(optionIndex); break;
        case GenerationWeightGroup::LARGE_WEAPON_TYPE: result.ForcedLargeWeaponType = static_cast<ShipWeaponType>(optionIndex); break;
        case GenerationWeightGroup::ENGINE_NACELLE_PRESENCE: result.ForcedEngineNacellePresence = optionIndex != 0u; break;
        case GenerationWeightGroup::MAJOR_FEATURE_PRESENCE: result.ForcedMajorFeaturePresence = optionIndex != 0u; break;
        case GenerationWeightGroup::ATTACHMENT_PRESENCE: result.ForcedAttachmentPresence = optionIndex != 0u; break;
        case GenerationWeightGroup::LARGE_WEAPON_PRESENCE: result.ForcedLargeWeaponPresence = optionIndex != 0u; break;
        default: break;
        }
        return result;
    }

    void applySharedPairControls(GenerationWeightGroup group, const ShipGenerationDebugInfo& reference, GenerationCalibrationOverrides& first, GenerationCalibrationOverrides& second)
    {
        switch (group)
        {
        case GenerationWeightGroup::ENGINE_LAYOUT:
            if (!reference.EngineUnits.empty())
            {
                first.ForcedEngineSize = reference.EngineUnits.front().SizeClass;
                second.ForcedEngineSize = reference.EngineUnits.front().SizeClass;
            }
            break;
        case GenerationWeightGroup::ENGINE_SIZE:
            if (reference.EngineLayout != EngineLayoutType::ENGINE_LAYOUT_TYPE_END)
            {
                first.ForcedEngineLayout = reference.EngineLayout;
                second.ForcedEngineLayout = reference.EngineLayout;
            }
            break;
        case GenerationWeightGroup::ENGINE_NACELLE_PRESENCE:
            if (reference.EngineLayout != EngineLayoutType::ENGINE_LAYOUT_TYPE_END)
            {
                first.ForcedEngineLayout = reference.EngineLayout;
                second.ForcedEngineLayout = reference.EngineLayout;
            }
            if (!reference.EngineUnits.empty())
            {
                first.ForcedEngineSize = reference.EngineUnits.front().SizeClass;
                second.ForcedEngineSize = reference.EngineUnits.front().SizeClass;
            }
            break;
        case GenerationWeightGroup::MAJOR_FEATURE_TYPE:
            first.ForcedMajorFeaturePresence = true;
            second.ForcedMajorFeaturePresence = true;
            break;
        case GenerationWeightGroup::ATTACHMENT_TYPE:
            first.ForcedAttachmentPresence = true;
            second.ForcedAttachmentPresence = true;
            break;
        case GenerationWeightGroup::LARGE_WEAPON_TYPE:
            first.ForcedLargeWeaponPresence = true;
            second.ForcedLargeWeaponPresence = true;
            break;
        default:
            break;
        }
    }

    bool engineControlsMatch(GenerationWeightGroup group, const ShipGenerationDebugInfo& first, const ShipGenerationDebugInfo& second)
    {
        if (group == GenerationWeightGroup::ENGINE_LAYOUT)
        {
            if (first.EngineUnits.empty() || second.EngineUnits.empty()) { return false; }
            return first.EngineUnits.front().SizeClass == second.EngineUnits.front().SizeClass;
        }
        if (group == GenerationWeightGroup::ENGINE_SIZE)
        {
            return first.EngineLayout == second.EngineLayout;
        }
        if (group == GenerationWeightGroup::ENGINE_NACELLE_PRESENCE)
        {
            if (first.EngineUnits.empty() || second.EngineUnits.empty()) { return false; }
            return first.EngineLayout == second.EngineLayout && first.EngineUnits.front().SizeClass == second.EngineUnits.front().SizeClass;
        }
        return true;
    }

    bool masksEqual(const PixelMask& first, const PixelMask& second)
    {
        if (first.getWidth() != second.getWidth() || first.getHeight() != second.getHeight()) { return false; }
        for (uint32_t y = 0u; y < first.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < first.getWidth(); ++x)
            {
                if (first.get(x, y) != second.get(x, y)) { return false; }
            }
        }
        return true;
    }

    const PixelMask* findDebugStageMask(const ShipGenerationDebugInfo& info, ShipGenerationDebugStageType type)
    {
        for (const ShipGenerationDebugStage& stage : info.HullStages)
        {
            if (stage.Type == type) { return &stage.HullMask; }
        }
        return nullptr;
    }

    bool forcedChoicePresent(GenerationWeightGroup group, uint32_t option, const GeneratedShip& ship, const ShipGenerationDebugInfo& debugInfo)
    {
        switch (group)
        {
        case GenerationWeightGroup::ENGINE_LAYOUT:
            return debugInfo.EngineLayout == static_cast<EngineLayoutType>(option);
        case GenerationWeightGroup::ENGINE_SIZE:
            return std::any_of(debugInfo.EngineUnits.begin(), debugInfo.EngineUnits.end(), [option](const EngineUnitDebugInfo& unit) { return unit.SizeClass == static_cast<EngineSizeClass>(option); });
        case GenerationWeightGroup::HULL_MODIFIER:
            return std::find(debugInfo.AppliedHullModifiers.begin(), debugInfo.AppliedHullModifiers.end(), static_cast<HullModifierType>(option)) != debugInfo.AppliedHullModifiers.end();
        case GenerationWeightGroup::MAJOR_FEATURE_TYPE:
            return option < debugInfo.MajorFeatureTypeCounts.size() && debugInfo.MajorFeatureTypeCounts[option] > 0u;
        case GenerationWeightGroup::ATTACHMENT_TYPE:
            return std::any_of(ship.AttachmentPlacements.begin(), ship.AttachmentPlacements.end(), [option](const ShipAttachmentPlacement& placement) { return placement.Type == static_cast<ShipAttachmentType>(option); });
        case GenerationWeightGroup::LARGE_WEAPON_TYPE:
            return option < debugInfo.WeaponTypeCounts.size() && debugInfo.WeaponTypeCounts[option] > 0u;
        case GenerationWeightGroup::ENGINE_NACELLE_PRESENCE:
        {
            const bool present = std::any_of(debugInfo.EngineUnits.begin(), debugInfo.EngineUnits.end(), [](const EngineUnitDebugInfo& unit) { return unit.Nacelle; });
            return present == (option != 0u);
        }
        case GenerationWeightGroup::MAJOR_FEATURE_PRESENCE: return (debugInfo.MajorFeatureCount > 0u) == (option != 0u);
        case GenerationWeightGroup::ATTACHMENT_PRESENCE: return (!ship.AttachmentPlacements.empty()) == (option != 0u);
        case GenerationWeightGroup::LARGE_WEAPON_PRESENCE: return (debugInfo.WeaponCount > 0u) == (option != 0u);
        default: return false;
        }
    }

    bool pairHasControlledBase(const CalibrationCandidatePair& pair)
    {
        const PixelMask* cleanA = findDebugStageMask(pair.DebugA, ShipGenerationDebugStageType::CLEANED_BASE_HULL);
        const PixelMask* cleanB = findDebugStageMask(pair.DebugB, ShipGenerationDebugStageType::CLEANED_BASE_HULL);
        if (cleanA == nullptr || cleanB == nullptr || !masksEqual(*cleanA, *cleanB)) { return false; }
        if (pair.Group != GenerationWeightGroup::HULL_MODIFIER && !masksEqual(pair.ShipA.HullMask, pair.ShipB.HullMask)) { return false; }
        if (pair.Group != GenerationWeightGroup::HULL_MODIFIER && !masksEqual(pair.ShipA.CockpitMask, pair.ShipB.CockpitMask)) { return false; }
        return true;
    }

    const char* getIsolationNote(GenerationWeightGroup group)
    {
        switch (group)
        {
        case GenerationWeightGroup::HULL_MODIFIER: return "Base hull RNG is isolated; the forced modifier may legitimately affect cockpit and all downstream geometry.";
        case GenerationWeightGroup::ENGINE_LAYOUT:
        case GenerationWeightGroup::ENGINE_SIZE:
        case GenerationWeightGroup::ENGINE_NACELLE_PRESENCE:
            return "Engine RNG is isolated; non-tested layout/size controls are shared where feasible. Downstream placement can still react to engine occupancy.";
        case GenerationWeightGroup::MAJOR_FEATURE_TYPE:
        case GenerationWeightGroup::MAJOR_FEATURE_PRESENCE:
            return "Major-feature RNG is isolated; downstream weapon/attachment placement can react to occupied feature pixels.";
        case GenerationWeightGroup::LARGE_WEAPON_TYPE:
        case GenerationWeightGroup::LARGE_WEAPON_PRESENCE:
            return "Weapon RNG is isolated; downstream attachment/detail placement can react to weapon occupancy.";
        case GenerationWeightGroup::ATTACHMENT_TYPE:
        case GenerationWeightGroup::ATTACHMENT_PRESENCE:
            return "Attachment RNG is isolated. Details are generated afterward and may react to attachment occupancy.";
        default: return "Targeted subsystem uses a deterministic calibration substream.";
        }
    }

    CalibrationEvidenceLevel evidenceLevel(uint32_t usefulComparisons)
    {
        if (usefulComparisons < 10u) { return CalibrationEvidenceLevel::INSUFFICIENT; }
        if (usefulComparisons < 25u) { return CalibrationEvidenceLevel::EARLY; }
        if (usefulComparisons < 100u) { return CalibrationEvidenceLevel::MODERATE; }
        return CalibrationEvidenceLevel::STRONGER;
    }
}

namespace SpectralShipGenStudioPreview
{
    GenerationCalibrationSession createGenerationCalibrationSession(uint64_t rootSeed)
    {
        GenerationCalibrationSession result;
        result.RootSeed = rootSeed;
        result.DefaultProfile = SpectralShipGen::createDefaultGenerationTuningProfile();
        result.TunedProfile = result.DefaultProfile;
        return result;
    }

    CalibrationObjectiveBatch collectCalibrationObjectiveBatch(SpectralShipGen::ShipGenerator& generator, const GenerationCalibrationSession& session, const PreviewGenerationRecipe& contextRecipe, uint32_t sampleCount)
    {
        CalibrationObjectiveBatch result;
        result.SampleCount = sampleCount;
        if (sampleCount == 0u) { return result; }

        SpectralShipGenDiagnostics::DiagnosticGenerationConfiguration configuration;
        configuration.Width = contextRecipe.Dimensions.Width;
        configuration.Height = contextRecipe.Dimensions.Height;
        configuration.Style = contextRecipe.Style;
        configuration.Faction = contextRecipe.Faction;
        configuration.DetailDensity = contextRecipe.DetailDensity;
        configuration.AsymmetricDetailChance = contextRecipe.AsymmetricDetailChance;
        configuration.AttachmentsEnabled = contextRecipe.AttachmentsEnabled;
        configuration.Samples = sampleCount;
        configuration.DiagnosticSeed = SpectralShipGen::mixGenerationSeed64(session.RootSeed ^ 0x94D049BB133111EBull);

        for (uint32_t index = 0u; index < sampleCount; ++index)
        {
            const uint64_t masterSeed = SpectralShipGenDiagnostics::deriveDiagnosticSampleSeed(configuration.DiagnosticSeed, index);
            PreviewGenerationRecipe recipe = contextRecipe;
            recipe.Seeds = SpectralShipGen::deriveShipGenerationSeeds(masterSeed);
            const SpectralShipGen::ShipGenerationSettings settings = createSettings(recipe);

            SpectralShipGen::ShipGenerationDebugInfo productionDebug;
            try
            {
                const SpectralShipGen::GeneratedShip productionShip = generator.generate(settings, &productionDebug);
                result.Production.recordSuccess(productionShip, productionDebug, configuration);
            }
            catch (const std::exception&)
            {
                result.Production.recordFailure(productionDebug);
            }

            SpectralShipGen::ShipGenerationDebugInfo tunedDebug;
            SpectralShipGen::GenerationCalibrationSettings calibration;
            calibration.TuningProfile = &session.TunedProfile;
            try
            {
                const SpectralShipGen::GeneratedShip tunedShip = generator.generateCalibrated(settings, calibration, &tunedDebug);
                result.Tuned.recordSuccess(tunedShip, tunedDebug, configuration);
            }
            catch (const std::exception&)
            {
                result.Tuned.recordFailure(tunedDebug);
            }
        }

        result.Valid = result.Production.SuccessfulGenerations > 0u && result.Tuned.SuccessfulGenerations > 0u;
        return result;
    }

    CalibrationCandidatePair generateNextCalibrationPair(SpectralShipGen::ShipGenerator& generator, GenerationCalibrationSession& session, const PreviewGenerationRecipe& contextRecipe, SpectralShipGen::GenerationWeightGroup group)
    {
        CalibrationCandidatePair result;
        result.Group = group;
        const uint32_t optionCount = SpectralShipGen::getGenerationWeightOptionCount(group);
        const std::vector<std::pair<uint32_t, uint32_t>> pairs = createBalancedPairs(optionCount);
        if (pairs.empty()) { return result; }

        const std::size_t groupIndex = static_cast<std::size_t>(group);
        const uint64_t sequenceIndex = session.PairSequenceIndices[groupIndex];
        const uint64_t pairOffset = SpectralShipGen::mixGenerationSeed64(session.RootSeed ^ (static_cast<uint64_t>(group) << 32u)) % pairs.size();
        result.PairIndex = sequenceIndex;
        result.DisplayAOnLeft = (SpectralShipGen::mixGenerationSeed64(session.RootSeed ^ sequenceIndex ^ (static_cast<uint64_t>(group) * 0xD1B54A32D192ED03ull)) & 1ull) == 0ull;
        result.IsolationNote = getIsolationNote(group);

        for (uint32_t pairAdvance = 0u; pairAdvance < pairs.size(); ++pairAdvance)
        {
            const std::pair<uint32_t, uint32_t> options = pairs[(sequenceIndex + pairOffset + pairAdvance) % pairs.size()];
            result.OptionA = options.first;
            result.OptionB = options.second;

            for (uint32_t attempt = 0u; attempt < MaximumPairGenerationAttempts; ++attempt)
            {
                const uint32_t deterministicAttempt = pairAdvance * MaximumPairGenerationAttempts + attempt;
                const uint64_t masterSeed = derivePairMasterSeed(session.RootSeed, group, sequenceIndex, deterministicAttempt);
                result.Recipe = createPairRecipe(contextRecipe, masterSeed);
                const SpectralShipGen::ShipGenerationSettings settings = createSettings(result.Recipe);

                SpectralShipGen::GenerationCalibrationSettings referenceCalibration;
                referenceCalibration.TuningProfile = &session.TunedProfile;
                referenceCalibration.IsolatedGroup = group;
                referenceCalibration.IsolationSalt = SpectralShipGen::mixGenerationSeed64(masterSeed ^ 0xA24BAED4963EE407ull);

                SpectralShipGen::ShipGenerationDebugInfo referenceDebug;
                SpectralShipGen::GenerationCalibrationSettings calibrationA = referenceCalibration;
                SpectralShipGen::GenerationCalibrationSettings calibrationB = referenceCalibration;
                calibrationA.Overrides = createForcedOverride(group, result.OptionA);
                calibrationB.Overrides = createForcedOverride(group, result.OptionB);

                try
                {
                    generator.generateCalibrated(settings, referenceCalibration, &referenceDebug);
                    applySharedPairControls(group, referenceDebug, calibrationA.Overrides, calibrationB.Overrides);
                    result.ShipA = generator.generateCalibrated(settings, calibrationA, &result.DebugA);
                    result.ShipB = generator.generateCalibrated(settings, calibrationB, &result.DebugB);
                }
                catch (const std::exception&)
                {
                    continue;
                }

                if (!forcedChoicePresent(group, result.OptionA, result.ShipA, result.DebugA) || !forcedChoicePresent(group, result.OptionB, result.ShipB, result.DebugB)) { continue; }
                if (!engineControlsMatch(group, result.DebugA, result.DebugB)) { continue; }
                if (!pairHasControlledBase(result)) { continue; }
                result.Valid = true;
                session.PairSequenceIndices[groupIndex] += static_cast<uint64_t>(pairAdvance) + 1u;
                if (pairAdvance > 0u)
                {
                    result.IsolationNote += " Scheduled option pair was unavailable for this geometry and was deterministically advanced to the next feasible pair.";
                }
                return result;
            }
        }

        session.PairSequenceIndices[groupIndex] += static_cast<uint64_t>(pairs.size());
        return result;
    }

    void recordCalibrationPreference(GenerationCalibrationSession& session, const CalibrationCandidatePair& pair, CalibrationPreferenceResult result)
    {
        if (!pair.Valid) { return; }
        CalibrationComparisonRecord record;
        record.PairIndex = pair.PairIndex;
        record.Group = pair.Group;
        record.OptionA = pair.OptionA;
        record.OptionB = pair.OptionB;
        record.DisplayAOnLeft = pair.DisplayAOnLeft;
        record.Result = result;
        record.Recipe = pair.Recipe;
        session.Records.push_back(record);
    }

    CalibrationGroupStatistics calculateCalibrationGroupStatistics(const GenerationCalibrationSession& session, SpectralShipGen::GenerationWeightGroup group, const CalibrationContextFilter& filter)
    {
        CalibrationGroupStatistics result;
        result.Group = group;
        result.Options.resize(SpectralShipGen::getGenerationWeightOptionCount(group));

        for (const CalibrationComparisonRecord& record : session.Records)
        {
            if (record.Group != group || !calibrationRecordMatchesFilter(record, filter)) { continue; }
            if (record.OptionA >= result.Options.size() || record.OptionB >= result.Options.size()) { continue; }
            CalibrationOptionStatistics& first = result.Options[record.OptionA];
            CalibrationOptionStatistics& second = result.Options[record.OptionB];

            if (record.Result == CalibrationPreferenceResult::SKIP)
            {
                ++first.Skips;
                ++second.Skips;
                continue;
            }

            ++result.UsefulComparisonCount;
            ++first.Comparisons;
            ++second.Comparisons;
            if (record.Result == CalibrationPreferenceResult::PREFER_A) { ++first.Wins; ++second.Losses; }
            else if (record.Result == CalibrationPreferenceResult::PREFER_B) { ++second.Wins; ++first.Losses; }
            else { ++first.Ties; ++second.Ties; }
        }

        for (CalibrationOptionStatistics& option : result.Options)
        {
            const double useful = static_cast<double>(option.Wins + option.Losses + option.Ties);
            option.PreferenceScore = (static_cast<double>(option.Wins) + 0.5 * static_cast<double>(option.Ties) + ScorePrior) / (useful + 2.0 * ScorePrior);
        }
        result.Evidence = evidenceLevel(result.UsefulComparisonCount);
        return result;
    }

    std::vector<uint32_t> calculateSuggestedGroupWeights(const GenerationCalibrationSession& session, SpectralShipGen::ShipStyle style, SpectralShipGen::GenerationWeightGroup group, const CalibrationContextFilter& filter)
    {
        const CalibrationGroupStatistics statistics = calculateCalibrationGroupStatistics(session, group, filter);
        const uint32_t optionCount = SpectralShipGen::getGenerationWeightOptionCount(group);
        std::vector<uint32_t> result(optionCount, 0u);
        if (optionCount == 0u) { return result; }

        const uint32_t currentTotal = SpectralShipGen::getGenerationTuningGroupTotalWeight(session.TunedProfile, style, group);
        double scoreTotal = 0.0;
        for (const CalibrationOptionStatistics& option : statistics.Options) { scoreTotal += option.PreferenceScore; }
        const double evidence = std::min(1.0, static_cast<double>(statistics.UsefulComparisonCount) / static_cast<double>(FullSuggestionEvidence));
        const uint32_t outputTotal = SpectralShipGen::getGenerationWeightGroupKind(group) == SpectralShipGen::GenerationWeightGroupKind::BINARY_PROBABILITY ? 100u : std::max(1u, currentTotal);

        uint32_t assigned = 0u;
        for (uint32_t index = 0u; index < optionCount; ++index)
        {
            const double currentShare = currentTotal == 0u ? 1.0 / optionCount : static_cast<double>(SpectralShipGen::getGenerationTuningWeight(session.TunedProfile, style, group, index)) / currentTotal;
            const double preferenceShare = scoreTotal <= 0.0 ? 1.0 / optionCount : statistics.Options[index].PreferenceScore / scoreTotal;
            const double blended = currentShare * (1.0 - evidence) + preferenceShare * evidence;
            result[index] = static_cast<uint32_t>(std::lround(blended * outputTotal));
            assigned += result[index];
        }

        if (!result.empty() && assigned != outputTotal)
        {
            const int64_t difference = static_cast<int64_t>(outputTotal) - static_cast<int64_t>(assigned);
            const int64_t adjusted = std::max<int64_t>(0, static_cast<int64_t>(result.back()) + difference);
            result.back() = static_cast<uint32_t>(adjusted);
        }
        return result;
    }

    void applySuggestedGroupWeights(GenerationCalibrationSession& session, SpectralShipGen::ShipStyle style, SpectralShipGen::GenerationWeightGroup group, const CalibrationContextFilter& filter)
    {
        const std::vector<uint32_t> suggested = calculateSuggestedGroupWeights(session, style, group, filter);
        for (uint32_t index = 0u; index < suggested.size(); ++index)
        {
            SpectralShipGen::setGenerationTuningWeight(session.TunedProfile, style, group, index, suggested[index]);
        }
    }

    void resetCalibrationGroup(GenerationCalibrationSession& session, SpectralShipGen::ShipStyle style, SpectralShipGen::GenerationWeightGroup group)
    {
        const uint32_t optionCount = SpectralShipGen::getGenerationWeightOptionCount(group);
        for (uint32_t index = 0u; index < optionCount; ++index)
        {
            SpectralShipGen::setGenerationTuningWeight(session.TunedProfile, style, group, index, SpectralShipGen::getGenerationTuningWeight(session.DefaultProfile, style, group, index));
        }
    }

    void resetAllCalibrationTuning(GenerationCalibrationSession& session)
    {
        session.TunedProfile = session.DefaultProfile;
    }

    const char* getCalibrationGroupName(SpectralShipGen::GenerationWeightGroup group)
    {
        using SpectralShipGen::GenerationWeightGroup;
        switch (group)
        {
        case GenerationWeightGroup::ENGINE_LAYOUT: return "ENGINE LAYOUT";
        case GenerationWeightGroup::ENGINE_SIZE: return "ENGINE SIZE";
        case GenerationWeightGroup::HULL_MODIFIER: return "HULL MODIFIER";
        case GenerationWeightGroup::MAJOR_FEATURE_TYPE: return "MAJOR FEATURE";
        case GenerationWeightGroup::ATTACHMENT_TYPE: return "ATTACHMENT TYPE";
        case GenerationWeightGroup::LARGE_WEAPON_TYPE: return "LARGE WEAPON";
        case GenerationWeightGroup::ENGINE_NACELLE_PRESENCE: return "ENGINE NACELLE";
        case GenerationWeightGroup::MAJOR_FEATURE_PRESENCE: return "MAJOR FEATURE PRESENCE";
        case GenerationWeightGroup::ATTACHMENT_PRESENCE: return "ATTACHMENT PRESENCE";
        case GenerationWeightGroup::LARGE_WEAPON_PRESENCE: return "LARGE WEAPON PRESENCE";
        default: return "UNKNOWN";
        }
    }

    const char* getCalibrationOptionName(SpectralShipGen::GenerationWeightGroup group, uint32_t optionIndex)
    {
        using namespace SpectralShipGen;
        if (getGenerationWeightGroupKind(group) == GenerationWeightGroupKind::BINARY_PROBABILITY) { return optionIndex == 0u ? "OFF" : "ON"; }
        switch (group)
        {
        case GenerationWeightGroup::ENGINE_LAYOUT:
        {
            static constexpr const char* Names[] = { "CENTRAL", "TWIN", "QUAD", "CENTRAL + AUX", "WIDE BANK" };
            return optionIndex < 5u ? Names[optionIndex] : "?";
        }
        case GenerationWeightGroup::ENGINE_SIZE:
        {
            static constexpr const char* Names[] = { "SMALL", "MEDIUM", "LARGE" };
            return optionIndex < 3u ? Names[optionIndex] : "?";
        }
        case GenerationWeightGroup::HULL_MODIFIER:
        {
            static constexpr const char* Names[] = { "BROADER SHOULDERS", "SIDE LOBES", "STEPPED WING", "NARROW WAIST", "WING CUTOUT", "SPLIT NOSE" };
            return optionIndex < 6u ? Names[optionIndex] : "?";
        }
        case GenerationWeightGroup::MAJOR_FEATURE_TYPE:
        {
            static constexpr const char* Names[] = { "CENTRAL SPINE", "ARMOR PLATE", "RECESSED BAY", "VENT BANK", "WING PLATE", "TECH CORE" };
            return optionIndex < 6u ? Names[optionIndex] : "?";
        }
        case GenerationWeightGroup::ATTACHMENT_TYPE:
        {
            static constexpr const char* Names[] = { "WEAPON MOUNT", "SENSOR ARRAY", "AUXILIARY POD", "RADIATOR", "ARMOR FIN", "TECH NODE" };
            return optionIndex < 6u ? Names[optionIndex] : "?";
        }
        case GenerationWeightGroup::LARGE_WEAPON_TYPE:
        {
            static constexpr const char* Names[] = { "SINGLE CANNON", "TWIN CANNON", "COMPACT TURRET", "RAIL WEAPON", "WEAPON POD" };
            return optionIndex < 5u ? Names[optionIndex] : "?";
        }
        default: return "?";
        }
    }

    const char* getCalibrationEvidenceName(CalibrationEvidenceLevel level)
    {
        switch (level)
        {
        case CalibrationEvidenceLevel::INSUFFICIENT: return "INSUFFICIENT";
        case CalibrationEvidenceLevel::EARLY: return "EARLY";
        case CalibrationEvidenceLevel::MODERATE: return "MODERATE";
        case CalibrationEvidenceLevel::STRONGER: return "STRONGER";
        default: return "UNKNOWN";
        }
    }

    const char* getCalibrationDimensionBucketName(CalibrationDimensionBucket bucket)
    {
        switch (bucket)
        {
        case CalibrationDimensionBucket::SMALL: return "SMALL";
        case CalibrationDimensionBucket::MEDIUM: return "MEDIUM";
        case CalibrationDimensionBucket::LARGE: return "LARGE";
        case CalibrationDimensionBucket::ANY: return "ANY";
        default: return "UNKNOWN";
        }
    }

    CalibrationDimensionBucket getCalibrationDimensionBucket(const SpectralShipGen::ShipDimensions& dimensions)
    {
        const SpectralShipGen::GenerationScaleTraits scaleTraits = SpectralShipGen::GenerationScaleTraits::fromDimensions(dimensions);
        if (scaleTraits.Tier == SpectralShipGen::GenerationScaleTier::TINY || scaleTraits.Tier == SpectralShipGen::GenerationScaleTier::SMALL) { return CalibrationDimensionBucket::SMALL; }
        if (scaleTraits.Tier == SpectralShipGen::GenerationScaleTier::MEDIUM) { return CalibrationDimensionBucket::MEDIUM; }
        return CalibrationDimensionBucket::LARGE;
    }

    bool calibrationRecordMatchesFilter(const CalibrationComparisonRecord& record, const CalibrationContextFilter& filter)
    {
        if (filter.Style.has_value() && record.Recipe.Style != *filter.Style) { return false; }
        if (filter.Faction.has_value() && record.Recipe.Faction != *filter.Faction) { return false; }
        if (filter.DimensionBucket != CalibrationDimensionBucket::ANY && getCalibrationDimensionBucket(record.Recipe.Dimensions) != filter.DimensionBucket) { return false; }
        return true;
    }
}
