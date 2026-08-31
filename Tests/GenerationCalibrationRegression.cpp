#include "PreviewRegressionSuites.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>

#include "GenerationCalibration.h"
#include "GenerationCalibrationSerializer.h"
#include <SpectralShipGen/GenerationTuningProfile.h>
#include <SpectralShipGen/ShipGenerationSeeds.h>
#include <SpectralShipGen/ShipGenerator.h>

namespace
{
    using namespace SpectralShipGen;
    using namespace SpectralShipGenStudioPreview;

    bool imagesEqual(const Image& first, const Image& second)
    {
        return first.getWidth() == second.getWidth() && first.getHeight() == second.getHeight() && first.getPixels() == second.getPixels();
    }

    PreviewGenerationRecipe createRecipe()
    {
        PreviewGenerationRecipe recipe;
        recipe.Seeds = deriveShipGenerationSeeds(0x4F1BBCDCBFA54001ull);
        recipe.Dimensions = { 64u, 64u };
        recipe.StructuralPreset = ShipStyle::HEAVY;
        recipe.FactionPreset = ShipFactionType::MILITARY;
        recipe.DetailDensity = 50u;
        recipe.AsymmetricDetailChance = 10u;
        recipe.AttachmentsEnabled = true;
        return recipe;
    }

    ShipGenerationSettings createSettings(const PreviewGenerationRecipe& recipe)
    {
        ShipGenerationSettings settings;
        settings.Seed = recipe.Seeds.Master;
        settings.Dimensions = recipe.Dimensions;
        settings.Style = *recipe.StructuralPreset;
        settings.Faction = *recipe.FactionPreset;
        settings.DetailDensity = recipe.DetailDensity;
        settings.AsymmetricDetailChance = recipe.AsymmetricDetailChance;
        settings.AttachmentsEnabled = recipe.AttachmentsEnabled;
        settings.SeedOverrides.Structure = recipe.Seeds.Structure;
        settings.SeedOverrides.Palette = recipe.Seeds.Palette;
        settings.SeedOverrides.Details = recipe.Seeds.Details;
        settings.SeedOverrides.Attachments = recipe.Seeds.Attachments;
        settings.DomainSeedOverrides = recipe.DomainSeedOverrides;
        return settings;
    }
}

int SpectralShipGenStudioTests::runGenerationCalibrationRegression()
{
    using namespace SpectralShipGen;
    using namespace SpectralShipGenStudioPreview;

    bool success = true;
    ShipGenerator generator;
    const PreviewGenerationRecipe recipe = createRecipe();
    const ShipGenerationSettings settings = createSettings(recipe);
    GenerationCalibrationSession session = createGenerationCalibrationSession(0x123456789ABCDEF0ull);

    if (getCalibrationDimensionBucket({ 24u, 24u }) != CalibrationDimensionBucket::SMALL || getCalibrationDimensionBucket({ 64u, 64u }) != CalibrationDimensionBucket::MEDIUM || getCalibrationDimensionBucket({ 160u, 160u }) != CalibrationDimensionBucket::LARGE || getCalibrationDimensionBucket({ 64u, 32u }) != CalibrationDimensionBucket::SMALL)
    {
        std::cerr << "Calibration dimension buckets do not follow centralized generation scale traits.\n";
        success = false;
    }

    // Default tuning must be a non-destructive view over existing production weights.
    const ShipGenerationProfile productionHeavy = getShipGenerationProfile(ShipStyle::HEAVY);
    if (getGenerationTuningWeight(session.DefaultProfile, ShipStyle::HEAVY, GenerationWeightGroup::ENGINE_LAYOUT, 1u) != productionHeavy.TwinEngineWeight)
    {
        std::cerr << "Default tuning profile does not mirror production engine weights.\n";
        success = false;
    }

    const ShipGenerationProfile productionSpearhead = getShipGenerationProfile(ShipStyle::SPEARHEAD);
    const ShipGenerationProfile productionDelta = getShipGenerationProfile(ShipStyle::DELTA);
    if (getGenerationTuningWeight(session.DefaultProfile, ShipStyle::SPEARHEAD, GenerationWeightGroup::LARGE_WEAPON_TYPE, 3u) != productionSpearhead.LargeWeaponWeights.RailWeapon ||
        getGenerationTuningWeight(session.DefaultProfile, ShipStyle::DELTA, GenerationWeightGroup::ENGINE_LAYOUT, 4u) != productionDelta.EngineBankWeight)
    {
        std::cerr << "Calibration default profile does not include the Task 54 styles.\n";
        success = false;
    }

    const GeneratedShip normal = generator.generate(settings);
    GenerationCalibrationSettings defaultCalibration;
    defaultCalibration.TuningProfile = &session.TunedProfile;
    const GeneratedShip calibratedDefault = generator.generateCalibrated(settings, defaultCalibration);
    if (!imagesEqual(normal.FinalImage, calibratedDefault.FinalImage))
    {
        std::cerr << "Applying untouched tuning profile changed normal generation output.\n";
        success = false;
    }

    const uint32_t defaultTwin = getGenerationTuningWeight(session.DefaultProfile, ShipStyle::HEAVY, GenerationWeightGroup::ENGINE_LAYOUT, 1u);
    setGenerationTuningWeight(session.TunedProfile, ShipStyle::HEAVY, GenerationWeightGroup::ENGINE_LAYOUT, 1u, defaultTwin + 25u);
    if (getGenerationTuningWeight(session.DefaultProfile, ShipStyle::HEAVY, GenerationWeightGroup::ENGINE_LAYOUT, 1u) != defaultTwin)
    {
        std::cerr << "Editing tuned profile mutated default snapshot.\n";
        success = false;
    }
    resetCalibrationGroup(session, ShipStyle::HEAVY, GenerationWeightGroup::ENGINE_LAYOUT);

    // Balanced schedule must cover every unordered engine-layout pair before repeating.
    std::set<std::pair<uint32_t, uint32_t>> seenPairs;
    constexpr uint32_t engineOptionCount = static_cast<uint32_t>(EngineLayoutType::ENGINE_LAYOUT_TYPE_END);
    const uint32_t pairCount = engineOptionCount * (engineOptionCount - 1u) / 2u;
    GenerationCalibrationSession scheduleSession = createGenerationCalibrationSession(0x1020304050607080ull);
    for (uint32_t index = 0u; index < pairCount; ++index)
    {
        CalibrationCandidatePair pair = generateNextCalibrationPair(generator, scheduleSession, recipe, GenerationWeightGroup::ENGINE_LAYOUT);
        if (!pair.Valid)
        {
            std::cerr << "Failed to generate controlled engine-layout calibration pair.\n";
            success = false;
            break;
        }
        seenPairs.emplace(std::min(pair.OptionA, pair.OptionB), std::max(pair.OptionA, pair.OptionB));
    }
    if (seenPairs.size() != pairCount)
    {
        std::cerr << "Balanced pair scheduler did not cover every engine-layout pairing.\n";
        success = false;
    }

    // Every initially exposed group must be able to construct a controlled pair in a representative context.
    GenerationCalibrationSession groupSmokeSession = createGenerationCalibrationSession(0x6A09E667F3BCC909ull);
    for (uint32_t groupValue = 0u; groupValue < static_cast<uint32_t>(GenerationWeightGroup::GENERATION_WEIGHT_GROUP_END); ++groupValue)
    {
        const GenerationWeightGroup group = static_cast<GenerationWeightGroup>(groupValue);
        const CalibrationCandidatePair pair = generateNextCalibrationPair(generator, groupSmokeSession, recipe, group);
        if (!pair.Valid)
        {
            std::cerr << "Failed to generate controlled pair for calibration group " << getCalibrationGroupName(group) << ".\n";
            success = false;
        }
    }

    // Final Task-43 isolation refinement: non-tested controls should be shared where feasible,
    // and type comparisons must force feature presence on both sides.
    {
        GenerationCalibrationSession isolationSession = createGenerationCalibrationSession(0x3141592653589793ull);

        const CalibrationCandidatePair layoutPair = generateNextCalibrationPair(generator, isolationSession, recipe, GenerationWeightGroup::ENGINE_LAYOUT);
        if (!layoutPair.Valid || layoutPair.DebugA.EngineUnits.empty() || layoutPair.DebugB.EngineUnits.empty() || layoutPair.DebugA.EngineUnits.front().SizeClass != layoutPair.DebugB.EngineUnits.front().SizeClass)
        {
            std::cerr << "Engine-layout calibration did not preserve a shared feasible engine size.\n";
            success = false;
        }

        const CalibrationCandidatePair sizePair = generateNextCalibrationPair(generator, isolationSession, recipe, GenerationWeightGroup::ENGINE_SIZE);
        if (!sizePair.Valid || sizePair.DebugA.EngineLayout != sizePair.DebugB.EngineLayout)
        {
            std::cerr << "Engine-size calibration did not preserve a shared feasible engine layout.\n";
            success = false;
        }

        const CalibrationCandidatePair nacellePair = generateNextCalibrationPair(generator, isolationSession, recipe, GenerationWeightGroup::ENGINE_NACELLE_PRESENCE);
        if (!nacellePair.Valid || nacellePair.DebugA.EngineUnits.empty() || nacellePair.DebugB.EngineUnits.empty() || nacellePair.DebugA.EngineLayout != nacellePair.DebugB.EngineLayout || nacellePair.DebugA.EngineUnits.front().SizeClass != nacellePair.DebugB.EngineUnits.front().SizeClass)
        {
            std::cerr << "Nacelle calibration did not preserve shared layout and engine size.\n";
            success = false;
        }

        const CalibrationCandidatePair featurePair = generateNextCalibrationPair(generator, isolationSession, recipe, GenerationWeightGroup::MAJOR_FEATURE_TYPE);
        if (!featurePair.Valid || featurePair.DebugA.MajorFeatureCount == 0u || featurePair.DebugB.MajorFeatureCount == 0u || featurePair.OptionA >= featurePair.DebugA.MajorFeatureTypeCounts.size() || featurePair.OptionB >= featurePair.DebugB.MajorFeatureTypeCounts.size() || featurePair.DebugA.MajorFeatureTypeCounts[featurePair.OptionA] == 0u || featurePair.DebugB.MajorFeatureTypeCounts[featurePair.OptionB] == 0u)
        {
            std::cerr << "Major-feature type calibration did not force feature presence on both candidates.\n";
            success = false;
        }

        const CalibrationCandidatePair attachmentPair = generateNextCalibrationPair(generator, isolationSession, recipe, GenerationWeightGroup::ATTACHMENT_TYPE);
        const auto hasAttachmentType = [](const GeneratedShip& ship, uint32_t option)
            {
                return std::any_of(ship.AttachmentPlacements.begin(), ship.AttachmentPlacements.end(), [option](const ShipAttachmentPlacement& placement) { return placement.Type == static_cast<ShipAttachmentType>(option); });
            };
        if (!attachmentPair.Valid || attachmentPair.ShipA.AttachmentPlacements.empty() || attachmentPair.ShipB.AttachmentPlacements.empty() || !hasAttachmentType(attachmentPair.ShipA, attachmentPair.OptionA) || !hasAttachmentType(attachmentPair.ShipB, attachmentPair.OptionB))
        {
            std::cerr << "Attachment type calibration did not force attachment presence on both candidates.\n";
            success = false;
        }

        const CalibrationCandidatePair weaponPair = generateNextCalibrationPair(generator, isolationSession, recipe, GenerationWeightGroup::LARGE_WEAPON_TYPE);
        if (!weaponPair.Valid || weaponPair.DebugA.WeaponCount == 0u || weaponPair.DebugB.WeaponCount == 0u || weaponPair.OptionA >= weaponPair.DebugA.WeaponTypeCounts.size() || weaponPair.OptionB >= weaponPair.DebugB.WeaponTypeCounts.size() || weaponPair.DebugA.WeaponTypeCounts[weaponPair.OptionA] == 0u || weaponPair.DebugB.WeaponTypeCounts[weaponPair.OptionB] == 0u)
        {
            std::cerr << "Large-weapon type calibration did not force weapon presence on both candidates.\n";
            success = false;
        }
    }

    // Same session seed/context must regenerate the same first pair, including display swap and pixels.
    GenerationCalibrationSession deterministicA = createGenerationCalibrationSession(0x8877665544332211ull);
    GenerationCalibrationSession deterministicB = createGenerationCalibrationSession(0x8877665544332211ull);
    const CalibrationCandidatePair pairA = generateNextCalibrationPair(generator, deterministicA, recipe, GenerationWeightGroup::ENGINE_LAYOUT);
    const CalibrationCandidatePair pairB = generateNextCalibrationPair(generator, deterministicB, recipe, GenerationWeightGroup::ENGINE_LAYOUT);
    if (!pairA.Valid || !pairB.Valid || pairA.OptionA != pairB.OptionA || pairA.OptionB != pairB.OptionB || pairA.DisplayAOnLeft != pairB.DisplayAOnLeft || !imagesEqual(pairA.ShipA.FinalImage, pairB.ShipA.FinalImage) || !imagesEqual(pairA.ShipB.FinalImage, pairB.ShipB.FinalImage))
    {
        std::cerr << "Calibration candidate generation is not deterministic.\n";
        success = false;
    }

    // Calibration overrides/substreams must not leak into later ordinary production generation.
    const GeneratedShip normalAfterCalibration = generator.generate(settings);
    if (!imagesEqual(normal.FinalImage, normalAfterCalibration.FinalImage))
    {
        std::cerr << "Calibration candidate generation leaked state into ordinary generation.\n";
        success = false;
    }

    // A saved/resumed session must continue with the exact same deterministic next pair.
    GenerationCalibrationSession resumeOriginal = createGenerationCalibrationSession(0xDEADBEEF10293847ull);
    const CalibrationCandidatePair resumeFirst = generateNextCalibrationPair(generator, resumeOriginal, recipe, GenerationWeightGroup::ENGINE_LAYOUT);
    const GenerationCalibrationSessionLoadResult resumeLoadedResult = deserializeGenerationCalibrationSession(serializeGenerationCalibrationSession(resumeOriginal));
    if (!resumeFirst.Valid || !resumeLoadedResult.Success)
    {
        std::cerr << "Calibration resume setup failed.\n";
        success = false;
    }
    else
    {
        GenerationCalibrationSession resumed = resumeLoadedResult.Session;
        const CalibrationCandidatePair uninterruptedNext = generateNextCalibrationPair(generator, resumeOriginal, recipe, GenerationWeightGroup::ENGINE_LAYOUT);
        const CalibrationCandidatePair resumedNext = generateNextCalibrationPair(generator, resumed, recipe, GenerationWeightGroup::ENGINE_LAYOUT);
        if (!uninterruptedNext.Valid || !resumedNext.Valid || uninterruptedNext.PairIndex != resumedNext.PairIndex || uninterruptedNext.OptionA != resumedNext.OptionA || uninterruptedNext.OptionB != resumedNext.OptionB || uninterruptedNext.DisplayAOnLeft != resumedNext.DisplayAOnLeft || !imagesEqual(uninterruptedNext.ShipA.FinalImage, resumedNext.ShipA.FinalImage) || !imagesEqual(uninterruptedNext.ShipB.FinalImage, resumedNext.ShipB.FinalImage))
        {
            std::cerr << "Saved calibration session did not resume deterministic future pair generation.\n";
            success = false;
        }
    }

    if (deserializeGenerationCalibrationSession("{").Success)
    {
        std::cerr << "Malformed calibration session JSON was accepted.\n";
        success = false;
    }

    // Preference semantics and SKIP handling.
    GenerationCalibrationSession statsSession = createGenerationCalibrationSession(1234u);
    CalibrationCandidatePair fake;
    fake.Valid = true;
    fake.Group = GenerationWeightGroup::ENGINE_SIZE;
    fake.OptionA = 0u;
    fake.OptionB = 1u;
    fake.Recipe = recipe;
    recordCalibrationPreference(statsSession, fake, CalibrationPreferenceResult::PREFER_A);
    recordCalibrationPreference(statsSession, fake, CalibrationPreferenceResult::NO_PREFERENCE);
    recordCalibrationPreference(statsSession, fake, CalibrationPreferenceResult::SKIP);
    CalibrationGroupStatistics stats = calculateCalibrationGroupStatistics(statsSession, GenerationWeightGroup::ENGINE_SIZE);
    if (stats.UsefulComparisonCount != 2u || stats.Options[0u].Wins != 1u || stats.Options[1u].Losses != 1u || stats.Options[0u].Ties != 1u || stats.Options[0u].Skips != 1u)
    {
        std::cerr << "Preference statistics incorrectly count wins/ties/skips.\n";
        success = false;
    }

    // Suggestions are recommendations until explicitly applied.
    const uint32_t beforeSuggested = getGenerationTuningWeight(statsSession.TunedProfile, ShipStyle::HEAVY, GenerationWeightGroup::ENGINE_SIZE, 0u);
    const std::vector<uint32_t> suggested = calculateSuggestedGroupWeights(statsSession, ShipStyle::HEAVY, GenerationWeightGroup::ENGINE_SIZE);
    if (getGenerationTuningWeight(statsSession.TunedProfile, ShipStyle::HEAVY, GenerationWeightGroup::ENGINE_SIZE, 0u) != beforeSuggested || suggested.size() != 3u)
    {
        std::cerr << "Suggested weights were applied implicitly or malformed.\n";
        success = false;
    }
    applySuggestedGroupWeights(statsSession, ShipStyle::HEAVY, GenerationWeightGroup::ENGINE_SIZE);
    if (getGenerationTuningGroupTotalWeight(statsSession.TunedProfile, ShipStyle::HEAVY, GenerationWeightGroup::ENGINE_SIZE) != getGenerationTuningGroupTotalWeight(statsSession.DefaultProfile, ShipStyle::HEAVY, GenerationWeightGroup::ENGINE_SIZE))
    {
        std::cerr << "Applying suggested relative weights did not preserve group total.\n";
        success = false;
    }

    // Task-33 objective statistics remain a separate deterministic view of production vs temporary tuning.
    GenerationCalibrationSession objectiveSession = createGenerationCalibrationSession(0xCAFEBABE12345678ull);
    const CalibrationObjectiveBatch objectiveA = collectCalibrationObjectiveBatch(generator, objectiveSession, recipe, 4u);
    const CalibrationObjectiveBatch objectiveB = collectCalibrationObjectiveBatch(generator, objectiveSession, recipe, 4u);
    if (!objectiveA.Valid || !objectiveB.Valid || objectiveA.Production.deterministicSignature() != objectiveA.Tuned.deterministicSignature() || objectiveA.Production.deterministicSignature() != objectiveB.Production.deterministicSignature())
    {
        std::cerr << "Calibration objective batch is not deterministic or diverges with untouched tuning.\n";
        success = false;
    }

    // Context filtering and session serialization.
    CalibrationContextFilter heavyOnly;
    heavyOnly.Style = ShipStyle::HEAVY;
    if (calculateCalibrationGroupStatistics(statsSession, GenerationWeightGroup::ENGINE_SIZE, heavyOnly).UsefulComparisonCount != 2u)
    {
        std::cerr << "Style filter rejected matching calibration records.\n";
        success = false;
    }
    CalibrationContextFilter sleekOnly;
    sleekOnly.Style = ShipStyle::SLEEK;
    if (calculateCalibrationGroupStatistics(statsSession, GenerationWeightGroup::ENGINE_SIZE, sleekOnly).UsefulComparisonCount != 0u)
    {
        std::cerr << "Style filter accepted non-matching calibration records.\n";
        success = false;
    }

    GenerationCalibrationSession factionFilterSession = createGenerationCalibrationSession(0x55CA11B4A7100001ull);
    CalibrationCandidatePair corporateComparison = fake;
    corporateComparison.Recipe.FactionPreset = ShipFactionType::CORPORATE;
    recordCalibrationPreference(factionFilterSession, corporateComparison, CalibrationPreferenceResult::PREFER_A);
    CalibrationCandidatePair relicComparison = fake;
    relicComparison.Recipe.FactionPreset = ShipFactionType::RELIC;
    recordCalibrationPreference(factionFilterSession, relicComparison, CalibrationPreferenceResult::PREFER_B);

    CalibrationContextFilter corporateOnly;
    corporateOnly.Faction = ShipFactionType::CORPORATE;
    CalibrationContextFilter relicOnly;
    relicOnly.Faction = ShipFactionType::RELIC;
    if (calculateCalibrationGroupStatistics(factionFilterSession, GenerationWeightGroup::ENGINE_SIZE, corporateOnly).UsefulComparisonCount != 1u ||
        calculateCalibrationGroupStatistics(factionFilterSession, GenerationWeightGroup::ENGINE_SIZE, relicOnly).UsefulComparisonCount != 1u)
    {
        std::cerr << "Task 55 faction calibration filters do not recognize CORPORATE / RELIC.\n";
        success = false;
    }

    const GenerationCalibrationSessionLoadResult factionFilterLoaded = deserializeGenerationCalibrationSession(serializeGenerationCalibrationSession(factionFilterSession));
    if (!factionFilterLoaded.Success || factionFilterLoaded.Session.Records.size() != 2u ||
        factionFilterLoaded.Session.Records[0].Recipe.FactionPreset != ShipFactionType::CORPORATE || factionFilterLoaded.Session.Records[1].Recipe.FactionPreset != ShipFactionType::RELIC)
    {
        std::cerr << "Task 55 calibration faction context did not round-trip.\n";
        success = false;
    }

    const std::string serialized = serializeGenerationCalibrationSession(statsSession);
    const GenerationCalibrationSessionLoadResult loaded = deserializeGenerationCalibrationSession(serialized);
    if (!loaded.Success || loaded.Session.RootSeed != statsSession.RootSeed || loaded.Session.Records.size() != statsSession.Records.size() || getGenerationTuningWeight(loaded.Session.TunedProfile, ShipStyle::HEAVY, GenerationWeightGroup::ENGINE_SIZE, 0u) != getGenerationTuningWeight(statsSession.TunedProfile, ShipStyle::HEAVY, GenerationWeightGroup::ENGINE_SIZE, 0u))
    {
        std::cerr << "Calibration session JSON round-trip failed.\n";
        success = false;
    }

    const std::filesystem::path temporaryDirectory = std::filesystem::temp_directory_path();
    const std::filesystem::path sessionPath = temporaryDirectory / "generation_calibration_regression_session.json";
    const std::filesystem::path csvPath = temporaryDirectory / "generation_calibration_regression.csv";
    const std::filesystem::path tuningPath = temporaryDirectory / "generation_calibration_regression_tuning.json";
    std::string exportError;
    if (!saveGenerationCalibrationSession(statsSession, sessionPath, exportError) || !exportGenerationCalibrationCsv(statsSession, csvPath, exportError) || !exportGenerationTuningProfile(statsSession.TunedProfile, tuningPath, exportError))
    {
        std::cerr << "Calibration session/report export failed: " << exportError << "\n";
        success = false;
    }
    else
    {
        const GenerationCalibrationSessionLoadResult diskLoaded = loadGenerationCalibrationSession(sessionPath);
        if (!diskLoaded.Success || diskLoaded.Session.Records.size() != statsSession.Records.size())
        {
            std::cerr << "Saved calibration session could not be resumed.\n";
            success = false;
        }
    }
    std::error_code removeError;
    std::filesystem::remove(sessionPath, removeError);
    std::filesystem::remove(csvPath, removeError);
    std::filesystem::remove(tuningPath, removeError);

    // Verify deterministic left/right randomization actually uses both presentations over a short sequence.
    GenerationCalibrationSession sideSession = createGenerationCalibrationSession(0xAABBCCDDEEFF0011ull);
    bool sawAOnLeft = false;
    bool sawBOnLeft = false;
    for (uint32_t index = 0u; index < 12u; ++index)
    {
        CalibrationCandidatePair pair = generateNextCalibrationPair(generator, sideSession, recipe, GenerationWeightGroup::ENGINE_SIZE);
        if (!pair.Valid) { continue; }
        sawAOnLeft |= pair.DisplayAOnLeft;
        sawBOnLeft |= !pair.DisplayAOnLeft;
    }
    if (!sawAOnLeft || !sawBOnLeft)
    {
        std::cerr << "Calibration presentation did not exercise both left/right orientations.\n";
        success = false;
    }

    return success ? 0 : 1;
}
