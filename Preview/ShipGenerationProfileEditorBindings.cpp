#include "ShipGenerationProfileEditorBindings.h"

#include <algorithm>
#include <initializer_list>
#include <utility>

namespace SpectralShipGenStudioPreview
{
    namespace
    {
        using Profile = SpectralShipGen::ShipGenerationProfile;
        constexpr uint32_t WeightMaximum = 10000u;
        constexpr int32_t MultiplierMaximum = 2000;
        constexpr int32_t CountMaximum = 64;

        std::array<std::string, ConfigurationWeightGroupControl::MaximumRows> makeLabels(std::initializer_list<const char*> labels)
        {
            std::array<std::string, ConfigurationWeightGroupControl::MaximumRows> result = {};
            std::size_t index = 0u;
            for (const char* label : labels)
            {
                if (index >= result.size()) { break; }
                result[index++] = label;
            }
            return result;
        }

        template<typename T>
        T* findByPath(std::vector<StructuralProfileEditorSection>& sections, std::string_view path, std::vector<T> StructuralProfileEditorSection::* member)
        {
            for (StructuralProfileEditorSection& section : sections)
            {
                auto& fields = section.*member;
                const auto iterator = std::find_if(fields.begin(), fields.end(), [&](const T& field) { return field.Path == path; });
                if (iterator != fields.end()) { return &*iterator; }
            }
            return nullptr;
        }

        template<typename T>
        const T* findByPath(const std::vector<StructuralProfileEditorSection>& sections, std::string_view path, const std::vector<T> StructuralProfileEditorSection::* member)
        {
            for (const StructuralProfileEditorSection& section : sections)
            {
                const auto& fields = section.*member;
                const auto iterator = std::find_if(fields.begin(), fields.end(), [&](const T& field) { return field.Path == path; });
                if (iterator != fields.end()) { return &*iterator; }
            }
            return nullptr;
        }
    }

    ShipGenerationProfileEditorBindings::ShipGenerationProfileEditorBindings()
    {
        m_Sections.reserve(19u);
        const auto addSection = [&](const char* label) -> StructuralProfileEditorSection&
        {
            m_Sections.push_back({});
            m_Sections.back().Label = label;
            return m_Sections.back();
        };

        StructuralProfileEditorSection& visual = addSection("VISUAL HIERARCHY");
        StructuralProfileEditorSection& complexity = addSection("COMPLEXITY / SPATIAL BUDGET");
        StructuralProfileEditorSection& hull = addSection("HULL DIMENSIONS / PROPORTIONS");
        StructuralProfileEditorSection& wings = addSection("WINGS");
        StructuralProfileEditorSection& silhouette = addSection("SILHOUETTE GUIDANCE");
        StructuralProfileEditorSection& negativeSpace = addSection("STRUCTURAL NEGATIVE SPACE");
        StructuralProfileEditorSection& coreLayers = addSection("CORE TREATMENT / HULL LAYERS");
        StructuralProfileEditorSection& cockpit = addSection("COCKPIT");
        StructuralProfileEditorSection& engines = addSection("ENGINES");
        StructuralProfileEditorSection& majorFeatures = addSection("MAJOR FEATURES");
        StructuralProfileEditorSection& weapons = addSection("WEAPONS");
        StructuralProfileEditorSection& details = addSection("DETAIL DENSITY / MOTIFS");
        StructuralProfileEditorSection& attachments = addSection("ATTACHMENTS");
        StructuralProfileEditorSection& materials = addSection("MATERIAL COMPOSITION");
        StructuralProfileEditorSection& livery = addSection("LIVERY");
        StructuralProfileEditorSection& palette = addSection("PALETTE MODIFIERS");
        StructuralProfileEditorSection& macro = addSection("MACRO ASYMMETRY");
        StructuralProfileEditorSection& supplemental = addSection("SUPPLEMENTAL DETAILS");
        StructuralProfileEditorSection& animation = addSection("ANIMATION TRAITS");

        const auto addInteger = [](StructuralProfileEditorSection& section, const char* path, const char* label, ConfigurationNumericSemantic semantic, int32_t minimum, int32_t maximum, int32_t step,
            std::function<int32_t(const Profile&)> read, std::function<void(Profile&, int32_t)> write)
        {
            StructuralIntegerFieldBinding field;
            field.Path = path;
            field.Control.configure(label, semantic, minimum, maximum, step, 0);
            field.Read = std::move(read);
            field.Write = std::move(write);
            section.Integers.push_back(std::move(field));
        };

        const auto addRange = [](StructuralProfileEditorSection& section, const char* path, const char* label, int32_t minimum, int32_t maximum, int32_t step,
            std::function<SpectralShipGen::UIntRange(const Profile&)> read, std::function<void(Profile&, SpectralShipGen::UIntRange)> write)
        {
            StructuralRangeFieldBinding field;
            field.Path = path;
            field.Control.configure(label, minimum, maximum, step, 0, 0);
            field.Read = std::move(read);
            field.Write = std::move(write);
            section.Ranges.push_back(std::move(field));
        };

        const auto addToggle = [](StructuralProfileEditorSection& section, const char* path, const char* label,
            std::function<bool(const Profile&)> read, std::function<void(Profile&, bool)> write)
        {
            StructuralToggleFieldBinding field;
            field.Path = path;
            field.Control.configure(label, false);
            field.Read = std::move(read);
            field.Write = std::move(write);
            section.Toggles.push_back(std::move(field));
        };

        const auto addChoice = [](StructuralProfileEditorSection& section, const char* path, const char* label, std::vector<std::string> options,
            std::function<uint32_t(const Profile&)> read, std::function<void(Profile&, uint32_t)> write)
        {
            StructuralChoiceFieldBinding field;
            field.Path = path;
            field.Control.configure(label, std::move(options), 0u);
            field.Read = std::move(read);
            field.Write = std::move(write);
            section.Choices.push_back(std::move(field));
        };

        const auto addWeightGroup = [](StructuralProfileEditorSection& section, const char* path, const char* label, std::initializer_list<const char*> labels,
            std::function<std::array<uint32_t, ConfigurationWeightGroupControl::MaximumRows>(const Profile&)> read,
            std::function<void(Profile&, const std::array<uint32_t, ConfigurationWeightGroupControl::MaximumRows>&)> write)
        {
            StructuralWeightGroupBinding field;
            field.Path = path;
            const auto rowLabels = makeLabels(labels);
            const std::array<uint32_t, ConfigurationWeightGroupControl::MaximumRows> emptyWeights = {};
            field.Control.configure(label, rowLabels, emptyWeights, std::min(labels.size(), ConfigurationWeightGroupControl::MaximumRows), WeightMaximum);
            field.Read = std::move(read);
            field.Write = std::move(write);
            section.WeightGroups.push_back(std::move(field));
        };

#define ADD_UINT(section, member, label, semantic, minimum, maximum, step) \
        addInteger(section, #member, label, semantic, minimum, maximum, step, \
            [](const Profile& p) { return static_cast<int32_t>(p.member); }, \
            [](Profile& p, int32_t value) { p.member = static_cast<decltype(p.member)>(value); })
#define ADD_INT(section, member, label, semantic, minimum, maximum, step) ADD_UINT(section, member, label, semantic, minimum, maximum, step)
#define ADD_RANGE(section, member, label, minimum, maximum, step) \
        addRange(section, #member, label, minimum, maximum, step, \
            [](const Profile& p) { return p.member; }, \
            [](Profile& p, SpectralShipGen::UIntRange value) { p.member = value; })
#define ADD_TOGGLE(section, member, label) \
        addToggle(section, #member, label, [](const Profile& p) { return p.member; }, [](Profile& p, bool value) { p.member = value; })
#define ADD_CHOICE(section, member, label, options) \
        addChoice(section, #member, label, options, [](const Profile& p) { return static_cast<uint32_t>(p.member); }, \
            [](Profile& p, uint32_t value) { p.member = static_cast<decltype(p.member)>(value); })

        ADD_UINT(visual, VisualSecondaryAnchorChance, "SECONDARY ANCHOR CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_TOGGLE(visual, VisualHierarchyEnabled, "VISUAL HIERARCHY ENABLED");
        ADD_TOGGLE(visual, HullLayerHierarchyUsesWingRoot, "HULL LAYERS USE WING ROOT");
        ADD_TOGGLE(visual, WeaponHierarchyUsesWingRoot, "WEAPONS USE WING ROOT");
        addWeightGroup(visual, "VisualAnchorWeights", "VISUAL ANCHOR WEIGHTS", { "SILHOUETTE", "COCKPIT", "WINGS", "ENGINES", "WEAPONS", "MAJOR FEATURE", "HULL LAYERS", "CENTRAL CORE", "MACRO ASYMMETRY", "NEGATIVE SPACE" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.VisualAnchorWeights.Silhouette, p.VisualAnchorWeights.Cockpit, p.VisualAnchorWeights.Wings, p.VisualAnchorWeights.Engines, p.VisualAnchorWeights.Weapons, p.VisualAnchorWeights.MajorFeature, p.VisualAnchorWeights.HullLayers, p.VisualAnchorWeights.CentralCore, p.VisualAnchorWeights.MacroAsymmetry, p.VisualAnchorWeights.NegativeSpace }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.VisualAnchorWeights = { v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9] }; });

        ADD_UINT(complexity, ComplexityBudgetPercent, "COMPLEXITY BUDGET", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_INT(complexity, SpatialCapacityBias.Nose, "NOSE CAPACITY BIAS", ConfigurationNumericSemantic::SIGNED_OFFSET, -100, 100, 1);
        ADD_INT(complexity, SpatialCapacityBias.FrontFuselage, "FRONT FUSELAGE BIAS", ConfigurationNumericSemantic::SIGNED_OFFSET, -100, 100, 1);
        ADD_INT(complexity, SpatialCapacityBias.MidFuselage, "MID FUSELAGE BIAS", ConfigurationNumericSemantic::SIGNED_OFFSET, -100, 100, 1);
        ADD_INT(complexity, SpatialCapacityBias.RearFuselage, "REAR FUSELAGE BIAS", ConfigurationNumericSemantic::SIGNED_OFFSET, -100, 100, 1);
        ADD_INT(complexity, SpatialCapacityBias.WingRoot, "WING ROOT BIAS", ConfigurationNumericSemantic::SIGNED_OFFSET, -100, 100, 1);
        ADD_INT(complexity, SpatialCapacityBias.OuterWing, "OUTER WING BIAS", ConfigurationNumericSemantic::SIGNED_OFFSET, -100, 100, 1);
        addWeightGroup(complexity, "ComplexityCategoryWeights", "COMPLEXITY CATEGORY WEIGHTS", { "SILHOUETTE", "COCKPIT", "HULL LAYER", "MAJOR FEATURE", "LARGE WEAPON", "ATTACHMENT", "DETAIL" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ static_cast<uint32_t>(std::max(0, p.ComplexityCategoryWeights.Silhouette)), static_cast<uint32_t>(std::max(0, p.ComplexityCategoryWeights.CockpitStructure)), static_cast<uint32_t>(std::max(0, p.ComplexityCategoryWeights.HullLayer)), static_cast<uint32_t>(std::max(0, p.ComplexityCategoryWeights.MajorFeature)), static_cast<uint32_t>(std::max(0, p.ComplexityCategoryWeights.LargeWeapon)), static_cast<uint32_t>(std::max(0, p.ComplexityCategoryWeights.Attachment)), static_cast<uint32_t>(std::max(0, p.ComplexityCategoryWeights.Detail)), 0u, 0u, 0u }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.ComplexityCategoryWeights = { static_cast<int32_t>(v[0]), static_cast<int32_t>(v[1]), static_cast<int32_t>(v[2]), static_cast<int32_t>(v[3]), static_cast<int32_t>(v[4]), static_cast<int32_t>(v[5]), static_cast<int32_t>(v[6]) }; });
        ADD_RANGE(hull, HullVerticalPaddingPercent, "VERTICAL PADDING", 0, 49, 1);
        ADD_RANGE(hull, HullHorizontalPaddingPercent, "HORIZONTAL PADDING", 0, 49, 1);
        ADD_RANGE(hull, NoseEndPercent, "NOSE END", 0, 100, 1);
        ADD_RANGE(hull, UpperFuselageEndPercent, "UPPER FUSELAGE END", 0, 100, 1);
        ADD_RANGE(hull, MainBodyEndPercent, "MAIN BODY END", 0, 100, 1);
        ADD_RANGE(hull, RearFuselageStartPercent, "REAR FUSELAGE START", 0, 100, 1);
        ADD_RANGE(hull, NoseWidthPercent, "NOSE WIDTH", 0, 100, 1);
        ADD_RANGE(hull, UpperFuselageWidthPercent, "UPPER FUSELAGE WIDTH", 0, 100, 1);
        ADD_RANGE(hull, MainBodyWidthPercent, "MAIN BODY WIDTH", 0, 100, 1);
        ADD_RANGE(hull, RearFuselageWidthPercent, "REAR FUSELAGE WIDTH", 0, 100, 1);
        ADD_RANGE(hull, RearWidthPercent, "REAR WIDTH", 0, 100, 1);

        ADD_RANGE(wings, SmallWingIncreasePercent, "SMALL WING INCREASE", 0, 500, 1);
        ADD_RANGE(wings, SweptWingWidthPercent, "SWEPT WING WIDTH", 0, 100, 1);
        ADD_RANGE(wings, BroadWingWidthPercent, "BROAD WING WIDTH", 0, 100, 1);
        ADD_INT(wings, WingLongitudinalOffsetPercent, "LONGITUDINAL OFFSET", ConfigurationNumericSemantic::SIGNED_OFFSET, -100, 100, 1);
        ADD_UINT(wings, WingRootLengthPercent, "WING ROOT LENGTH", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(wings, WingRootWidthPercent, "WING ROOT WIDTH", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(wings, SmallWingTaperPercent, "SMALL WING TAPER", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, 500, 5);
        ADD_UINT(wings, SweptWingTaperPercent, "SWEPT WING TAPER", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, 500, 5);
        ADD_UINT(wings, BroadWingTaperPercent, "BROAD WING TAPER", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, 500, 5);
        addWeightGroup(wings, "WingWeights", "WING SHAPE WEIGHTS", { "NONE", "SMALL", "SWEPT", "BROAD" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.NoWingWeight, p.SmallWingWeight, p.SweptWingWeight, p.BroadWingWeight }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.NoWingWeight = v[0]; p.SmallWingWeight = v[1]; p.SweptWingWeight = v[2]; p.BroadWingWeight = v[3]; });

        ADD_UINT(silhouette, HullModifierChance, "HULL MODIFIER CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(silhouette, MaximumHullModifiers, "MAX HULL MODIFIERS", ConfigurationNumericSemantic::COUNT, 0, CountMaximum, 1);
        ADD_UINT(silhouette, MinimumSilhouetteWidthPercent, "MIN SILHOUETTE WIDTH", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(silhouette, MinimumSilhouetteHeightPercent, "MIN SILHOUETTE HEIGHT", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(silhouette, SilhouetteArticulationTarget, "ARTICULATION TARGET", ConfigurationNumericSemantic::COUNT, 0, CountMaximum, 1);
        ADD_UINT(silhouette, SilhouetteMaximumStableRunPercent, "MAX STABLE RUN", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(silhouette, SilhouetteConvexFillTriggerPercent, "CONVEX FILL TRIGGER", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(silhouette, TinySilhouetteExtraWidthRelaxationPercent, "TINY WIDTH RELAXATION", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_TOGGLE(silhouette, SilhouetteGuidanceEnabled, "GUIDANCE ENABLED");
        ADD_TOGGLE(silhouette, SilhouetteWeakArticulationGuidanceEnabled, "WEAK ARTICULATION GUIDANCE");
        ADD_TOGGLE(silhouette, SilhouetteProfileValidationEnabled, "PROFILE VALIDATION ENABLED");
        ADD_TOGGLE(silhouette, CleanAxialTaperArticulationExemption, "CLEAN AXIAL TAPER EXEMPTION");
        ADD_TOGGLE(silhouette, WingWedgeArticulationExemption, "WING WEDGE EXEMPTION");
        addWeightGroup(silhouette, "HullModifierWeights", "HULL MODIFIER WEIGHTS", { "BROAD SHOULDERS", "SIDE LOBES", "STEPPED WING", "NARROW WAIST", "WING CUTOUT", "SPLIT NOSE" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.BroaderShouldersModifierWeight, p.SideLobesModifierWeight, p.SteppedWingModifierWeight, p.NarrowWaistModifierWeight, p.WingCutoutModifierWeight, p.SplitNoseModifierWeight }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.BroaderShouldersModifierWeight = v[0]; p.SideLobesModifierWeight = v[1]; p.SteppedWingModifierWeight = v[2]; p.NarrowWaistModifierWeight = v[3]; p.WingCutoutModifierWeight = v[4]; p.SplitNoseModifierWeight = v[5]; });
        addWeightGroup(silhouette, "SilhouetteGuidanceWeights", "GUIDANCE WEIGHTS", { "BROAD SHOULDERS", "SIDE LOBES", "STEPPED WING" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.SilhouetteGuidanceWeights.BroaderShoulders, p.SilhouetteGuidanceWeights.SideLobes, p.SilhouetteGuidanceWeights.SteppedWingExtension }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.SilhouetteGuidanceWeights = { v[0], v[1], v[2] }; });

        ADD_UINT(negativeSpace, StructuralNegativeSpaceChance, "NEGATIVE SPACE CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(negativeSpace, MaximumStructuralNegativeSpaceStructures, "MAX VOID STRUCTURES", ConfigurationNumericSemantic::COUNT, 0, CountMaximum, 1);
        ADD_UINT(negativeSpace, StructuralNegativeSpaceScalePercent, "VOID SCALE", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_RANGE(negativeSpace, RearForkStartPercent, "REAR FORK START", 0, 100, 1);
        addWeightGroup(negativeSpace, "StructuralNegativeSpaceWeights", "VOID TYPE WEIGHTS", { "WING CHANNEL", "REAR FORK", "SHOULDER GAP", "OPEN FRAME BAY", "NACELLE CHANNEL" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.StructuralNegativeSpaceWeights.WingChannel, p.StructuralNegativeSpaceWeights.RearFork, p.StructuralNegativeSpaceWeights.ShoulderGap, p.StructuralNegativeSpaceWeights.OpenFrameBay, p.StructuralNegativeSpaceWeights.NacelleChannel }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.StructuralNegativeSpaceWeights = { v[0], v[1], v[2], v[3], v[4] }; });

        ADD_UINT(coreLayers, CoreTreatmentChance, "CORE TREATMENT CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(coreLayers, CoreRegionWidthBasePercent, "CORE WIDTH BASE", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, 500, 1);
        ADD_UINT(coreLayers, CoreRegionWidthHorizontalCapacityDivisor, "CORE CAPACITY DIVISOR", ConfigurationNumericSemantic::COUNT, 1, 64, 1);
        ADD_UINT(coreLayers, CoreRegionWidthMaximumPercent, "CORE WIDTH MAXIMUM", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(coreLayers, RaisedCorePlateWidthPercent, "RAISED CORE WIDTH", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(coreLayers, MaximumCoreTreatments, "MAX CORE TREATMENTS", ConfigurationNumericSemantic::COUNT, 0, CountMaximum, 1);
        ADD_UINT(coreLayers, HullLayerChance, "HULL LAYER CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(coreLayers, MaximumHullLayers, "MAX HULL LAYERS", ConfigurationNumericSemantic::COUNT, 0, CountMaximum, 1);
        addWeightGroup(coreLayers, "CoreTreatmentWeights", "CORE TREATMENT WEIGHTS", { "CENTRAL SPINE", "COCKPIT SURROUND", "RAISED PLATE", "LATERAL RECESSES", "ARMOR BAND", "CORE CHANNEL" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.CoreTreatmentWeights.CentralSpine, p.CoreTreatmentWeights.CockpitSurround, p.CoreTreatmentWeights.RaisedCorePlate, p.CoreTreatmentWeights.LateralRecesses, p.CoreTreatmentWeights.LongitudinalArmorBand, p.CoreTreatmentWeights.CoreChannel }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.CoreTreatmentWeights = { v[0], v[1], v[2], v[3], v[4], v[5] }; });
        addWeightGroup(coreLayers, "HullLayerWeights", "HULL LAYER WEIGHTS", { "DORSAL PLATE", "FORWARD ARMOR", "WING ARMOR", "SHOULDER ARMOR", "ENGINE COVER" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.HullLayerWeights.CentralDorsalPlate, p.HullLayerWeights.ForwardArmor, p.HullLayerWeights.WingArmor, p.HullLayerWeights.ShoulderArmor, p.HullLayerWeights.RearEngineCover }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.HullLayerWeights = { v[0], v[1], v[2], v[3], v[4] }; });

        ADD_RANGE(cockpit, CockpitStartPercent, "COCKPIT START", 0, 100, 1);
        ADD_UINT(cockpit, CockpitHeightPercent, "COCKPIT HEIGHT", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(cockpit, CockpitWidthPercent, "COCKPIT WIDTH", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(cockpit, MaximumCockpitHullPercent, "MAX COCKPIT HULL COVERAGE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        addWeightGroup(cockpit, "CockpitSizeWeights", "COCKPIT SIZE WEIGHTS", { "COMPACT", "STANDARD", "LARGE", "MASSIVE" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.CockpitSizeWeights.Compact, p.CockpitSizeWeights.Standard, p.CockpitSizeWeights.Large, p.CockpitSizeWeights.Massive }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.CockpitSizeWeights = { v[0], v[1], v[2], v[3] }; });
        addWeightGroup(cockpit, "CockpitShapeWeights", "COCKPIT SHAPE WEIGHTS", { "COMPACT CANOPY", "ELONGATED", "WIDE COMMAND", "SPLIT CANOPY", "DORSAL BRIDGE", "LAYERED BRIDGE" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.CockpitShapeWeights.CompactCanopy, p.CockpitShapeWeights.ElongatedCanopy, p.CockpitShapeWeights.WideCommandDeck, p.CockpitShapeWeights.SplitCanopy, p.CockpitShapeWeights.DorsalBridge, p.CockpitShapeWeights.LayeredBridge }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.CockpitShapeWeights = { v[0], v[1], v[2], v[3], v[4], v[5] }; });

        ADD_UINT(engines, EngineNacelleChance, "NACELLE CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        addWeightGroup(engines, "EngineLayoutWeights", "ENGINE LAYOUT WEIGHTS", { "CENTRAL", "TWIN", "QUAD", "CENTRAL + AUX", "ENGINE BANK" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.CentralEngineWeight, p.TwinEngineWeight, p.QuadEngineWeight, p.CentralAuxiliaryEngineWeight, p.EngineBankWeight }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.CentralEngineWeight = v[0]; p.TwinEngineWeight = v[1]; p.QuadEngineWeight = v[2]; p.CentralAuxiliaryEngineWeight = v[3]; p.EngineBankWeight = v[4]; });
        addWeightGroup(engines, "EngineSizeWeights", "ENGINE SIZE WEIGHTS", { "SMALL", "MEDIUM", "LARGE" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.SmallEngineSizeWeight, p.MediumEngineSizeWeight, p.LargeEngineSizeWeight }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.SmallEngineSizeWeight = v[0]; p.MediumEngineSizeWeight = v[1]; p.LargeEngineSizeWeight = v[2]; });

        ADD_UINT(majorFeatures, MajorFeatureChance, "MAJOR FEATURE CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(majorFeatures, MaximumMajorFeatures, "MAX MAJOR FEATURES", ConfigurationNumericSemantic::COUNT, 0, CountMaximum, 1);
        ADD_UINT(majorFeatures, MajorFeatureScalePercent, "MAJOR FEATURE SCALE", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        addWeightGroup(majorFeatures, "MajorFeatureWeights", "MAJOR FEATURE WEIGHTS", { "CENTRAL SPINE", "ARMOR PLATE", "RECESSED BAY", "VENT BANK", "WING PLATE", "TECH CORE" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.MajorFeatureWeights.CentralSpine, p.MajorFeatureWeights.ArmorPlate, p.MajorFeatureWeights.RecessedBay, p.MajorFeatureWeights.VentBank, p.MajorFeatureWeights.WingPlate, p.MajorFeatureWeights.TechCore }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.MajorFeatureWeights = { v[0], v[1], v[2], v[3], v[4], v[5] }; });

        ADD_UINT(weapons, LargeWeaponChance, "LARGE WEAPON CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(weapons, MaximumLargeWeaponGroups, "MAX WEAPON GROUPS", ConfigurationNumericSemantic::COUNT, 0, CountMaximum, 1);
        ADD_UINT(weapons, LargeWeaponSymmetryChance, "WEAPON SYMMETRY CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(weapons, LargeWeaponScalePercent, "LARGE WEAPON SCALE", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        addWeightGroup(weapons, "LargeWeaponWeights", "WEAPON TYPE WEIGHTS", { "SINGLE CANNON", "TWIN CANNON", "COMPACT TURRET", "RAIL WEAPON", "WEAPON POD" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.LargeWeaponWeights.SingleCannon, p.LargeWeaponWeights.TwinCannon, p.LargeWeaponWeights.CompactTurret, p.LargeWeaponWeights.RailWeapon, p.LargeWeaponWeights.WeaponPod }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.LargeWeaponWeights = { v[0], v[1], v[2], v[3], v[4] }; });
        addWeightGroup(weapons, "LargeWeaponHardpointWeights", "HARDPOINT WEIGHTS", { "CENTRAL NOSE", "FUSELAGE SIDE", "WING ROOT", "OUTER WING", "FORWARD SHOULDER", "CENTRAL BODY" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.LargeWeaponHardpointWeights.CentralNose, p.LargeWeaponHardpointWeights.ForwardFuselageSide, p.LargeWeaponHardpointWeights.WingRoot, p.LargeWeaponHardpointWeights.OuterWing, p.LargeWeaponHardpointWeights.ForwardShoulder, p.LargeWeaponHardpointWeights.CentralBody }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.LargeWeaponHardpointWeights = { v[0], v[1], v[2], v[3], v[4], v[5] }; });

        ADD_UINT(details, DetailDensityPercent, "DETAIL DENSITY", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(details, MechanicalPatternCountPercent, "MECHANICAL PATTERN COUNT", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(details, DetailMotifChance, "DETAIL MOTIF CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(details, SecondaryDetailMotifChance, "SECONDARY MOTIF CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(details, DetailMotifRepeatPercent, "MOTIF REPEAT", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(details, DetailMotifMirroringBonusPercent, "MIRRORING BONUS", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(details, HorizontalVentChance, "HORIZONTAL VENT CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_CHOICE(details, DetailMotifPlacementBias, "MOTIF PLACEMENT BIAS", (std::vector<std::string>{ "DEFAULT", "AXIAL", "WING SURFACE" }));
        ADD_CHOICE(details, DetailMotifOrientationBias, "MOTIF ORIENTATION", (std::vector<std::string>{ "DEFAULT", "LONGITUDINAL", "LATERAL" }));
        addWeightGroup(details, "DetailMotifWeights", "DETAIL MOTIF WEIGHTS", { "PAIRED VENTS", "TRIPLE VENTS", "PAIRED LIGHTS", "THREE LIGHTS", "PARALLEL SEAMS", "REPEATED DASHES", "RECESSED SLOT" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.DetailMotifWeights.PairedVents, p.DetailMotifWeights.TripleVentBank, p.DetailMotifWeights.PairedLights, p.DetailMotifWeights.ThreeNodeLights, p.DetailMotifWeights.ParallelSeams, p.DetailMotifWeights.RepeatedDashes, p.DetailMotifWeights.RecessedSlot }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.DetailMotifWeights = { v[0], v[1], v[2], v[3], v[4], v[5], v[6] }; });
        addWeightGroup(details, "AccentDetailWeights", "ACCENT DETAIL WEIGHTS", { "PANEL", "STRIPE", "ARMOR" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.AccentPanelWeight, p.AccentStripeWeight, p.AccentArmorWeight }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.AccentPanelWeight = v[0]; p.AccentStripeWeight = v[1]; p.AccentArmorWeight = v[2]; });

        ADD_UINT(attachments, AttachmentChance, "ATTACHMENT CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(attachments, MaximumAttachmentGroups, "MAX ATTACHMENT GROUPS", ConfigurationNumericSemantic::COUNT, 0, CountMaximum, 1);
        ADD_UINT(attachments, SymmetricAttachmentChance, "SYMMETRIC ATTACHMENT CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(attachments, AttachmentSizePercent, "ATTACHMENT SIZE", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        addWeightGroup(attachments, "AttachmentWeights", "ATTACHMENT WEIGHTS", { "WEAPON MOUNT", "SENSOR ARRAY", "AUXILIARY POD", "RADIATOR", "ARMOR FIN", "TECH NODE" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.AttachmentWeights.WeaponMount, p.AttachmentWeights.SensorArray, p.AttachmentWeights.AuxiliaryPod, p.AttachmentWeights.Radiator, p.AttachmentWeights.ArmorFin, p.AttachmentWeights.TechnologyNode }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.AttachmentWeights = { v[0], v[1], v[2], v[3], v[4], v[5] }; });

        ADD_UINT(materials, MaterialCompositionChance, "MATERIAL COMPOSITION CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(materials, MaximumMaterialZones, "MAX MATERIAL ZONES", ConfigurationNumericSemantic::COUNT, 0, CountMaximum, 1);
        ADD_UINT(materials, MaterialSecondaryContrastPercent, "SECONDARY CONTRAST", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_TOGGLE(materials, MaterialWingSurfaceUsesFullWing, "WING MATERIAL USES FULL WING");
        ADD_UINT(materials, MaterialAxialBandWidthPercent, "AXIAL BAND WIDTH", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        addWeightGroup(materials, "MaterialZoneWeights", "MATERIAL ZONE WEIGHTS", { "WING SURFACE", "SHOULDER", "AXIAL BAND", "REAR MECHANICAL", "COCKPIT COLLAR", "HARDPOINT" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.MaterialZoneWeights.WingSurface, p.MaterialZoneWeights.ShoulderSurface, p.MaterialZoneWeights.AxialBand, p.MaterialZoneWeights.RearMechanical, p.MaterialZoneWeights.CockpitCollar, p.MaterialZoneWeights.HardpointSurround }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.MaterialZoneWeights = { v[0], v[1], v[2], v[3], v[4], v[5] }; });

        ADD_UINT(livery, LiveryChance, "LIVERY CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(livery, MaximumLiveryMarkings, "MAX LIVERY MARKINGS", ConfigurationNumericSemantic::COUNT, 0, CountMaximum, 1);
        ADD_UINT(livery, SupportingLiveryChance, "SUPPORTING LIVERY CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(livery, LiveryAsymmetricChance, "ASYMMETRIC LIVERY CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(livery, MaximumLiveryCoveragePercent, "MAX LIVERY COVERAGE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(livery, MaximumLiveryConnectedCoveragePercent, "MAX CONNECTED COVERAGE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        addWeightGroup(livery, "LiveryWeights", "LIVERY WEIGHTS", { "CENTER STRIPE", "DOUBLE STRIPE", "WING BAND", "SHOULDER BLOCK", "NOSE BAND", "CHEVRON", "ID PANEL", "INSIGNIA" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.LiveryWeights.CenterStripe, p.LiveryWeights.DoubleCenterStripe, p.LiveryWeights.WingBand, p.LiveryWeights.ShoulderBlock, p.LiveryWeights.NoseBand, p.LiveryWeights.Chevron, p.LiveryWeights.IdPanel, p.LiveryWeights.GeometricInsignia }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.LiveryWeights = { v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7] }; });

        ADD_INT(palette, PaletteHullValueOffset, "HULL VALUE OFFSET", ConfigurationNumericSemantic::SIGNED_OFFSET, -255, 255, 1);
        ADD_UINT(palette, PaletteContrastPercent, "PALETTE CONTRAST", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(palette, PaletteHullSaturationPercent, "HULL SATURATION", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(palette, PaletteAccentSaturationPercent, "ACCENT SATURATION", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(palette, PaletteEmissiveValuePercent, "EMISSIVE VALUE", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(palette, SecondaryHullToneCoveragePercent, "SECONDARY TONE COVERAGE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_CHOICE(palette, CoreRaisedSurfaceTone, "RAISED CORE TONE", (std::vector<std::string>{ "BASE", "SECONDARY", "HIGHLIGHT" }));
        ADD_CHOICE(palette, CentralDorsalPlateTone, "DORSAL PLATE TONE", (std::vector<std::string>{ "BASE", "SECONDARY", "HIGHLIGHT" }));
        ADD_TOGGLE(palette, AxialRidgeUsesEdgeHighlight, "AXIAL RIDGE EDGE HIGHLIGHT");

        ADD_UINT(macro, MacroAsymmetryChance, "MACRO ASYMMETRY CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(macro, MacroAsymmetryOuterRegionChance, "OUTER REGION CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(macro, MacroAsymmetryWingRootRegionChance, "WING ROOT REGION CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(macro, MacroAsymmetryVisualWeightPercent, "ASYMMETRY VISUAL WEIGHT", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        addWeightGroup(macro, "MacroAsymmetryCategoryWeights", "ASYMMETRY CATEGORY WEIGHTS", { "HULL LAYER", "LARGE WEAPON", "ATTACHMENT" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.MacroAsymmetryCategoryWeights.HullLayer, p.MacroAsymmetryCategoryWeights.LargeWeapon, p.MacroAsymmetryCategoryWeights.Attachment }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.MacroAsymmetryCategoryWeights = { v[0], v[1], v[2] }; });

        addWeightGroup(supplemental, "SupplementalDetailWeights", "SUPPLEMENTAL DETAIL WEIGHTS", { "PANEL SEAM", "GEOMETRIC MARKING", "MECHANICAL EXPOSURE", "REPEATING MOTIF", "IDENTIFICATION", "LUMINOUS CHANNEL" },
            [](const Profile& p) { return std::array<uint32_t, 10u>{ p.SupplementalDetailWeights.PanelSeam, p.SupplementalDetailWeights.GeometricMarking, p.SupplementalDetailWeights.MechanicalExposure, p.SupplementalDetailWeights.RepeatingMotif, p.SupplementalDetailWeights.IdentificationMarking, p.SupplementalDetailWeights.LuminousChannel }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.SupplementalDetailWeights = { v[0], v[1], v[2], v[3], v[4], v[5] }; });

        ADD_UINT(animation, AnimationTraits.Idle.EnginePulseStrength, "IDLE ENGINE PULSE STRENGTH", ConfigurationNumericSemantic::COUNT, 0, 32, 1);
        ADD_UINT(animation, AnimationTraits.Idle.ExhaustAmplitudePercent, "IDLE EXHAUST AMPLITUDE", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(animation, AnimationTraits.Idle.EngineMechanicalChance, "IDLE ENGINE MECHANICAL CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(animation, AnimationTraits.Idle.WeaponMechanicalChance, "IDLE WEAPON MECHANICAL CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(animation, AnimationTraits.Idle.VentActivityChance, "IDLE VENT ACTIVITY CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_TOGGLE(animation, AnimationTraits.Idle.SynchronizeEngines, "IDLE SYNCHRONIZE ENGINES");
        ADD_TOGGLE(animation, AnimationTraits.Idle.AsynchronousEngines, "IDLE ASYNCHRONOUS ENGINES");
        ADD_TOGGLE(animation, AnimationTraits.Idle.AlternateEnginePhases, "IDLE ALTERNATE ENGINE PHASES");
        ADD_TOGGLE(animation, AnimationTraits.Idle.SlowMechanicalCycle, "IDLE SLOW MECHANICAL CYCLE");
        ADD_UINT(animation, AnimationTraits.LateralMovement.ResponseStrengthPercent, "LATERAL RESPONSE STRENGTH", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(animation, AnimationTraits.LateralMovement.EngineTravelLimit, "LATERAL ENGINE TRAVEL", ConfigurationNumericSemantic::COUNT, 0, 32, 1);
        ADD_UINT(animation, AnimationTraits.LateralMovement.WeaponTravelLimit, "LATERAL WEAPON TRAVEL", ConfigurationNumericSemantic::COUNT, 0, 32, 1);
        ADD_UINT(animation, AnimationTraits.LateralMovement.AttachmentTravelLimit, "LATERAL ATTACHMENT TRAVEL", ConfigurationNumericSemantic::COUNT, 0, 32, 1);
        ADD_TOGGLE(animation, AnimationTraits.LateralMovement.Synchronized, "LATERAL SYNCHRONIZED");
        ADD_TOGGLE(animation, AnimationTraits.LateralMovement.Staggered, "LATERAL STAGGERED");
        ADD_TOGGLE(animation, AnimationTraits.LateralMovement.HeavyResponse, "LATERAL HEAVY RESPONSE");
        ADD_TOGGLE(animation, AnimationTraits.LateralMovement.Responsive, "LATERAL RESPONSIVE");
        ADD_UINT(animation, AnimationTraits.LongitudinalMovement.ResponseStrengthPercent, "LONGITUDINAL RESPONSE STRENGTH", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(animation, AnimationTraits.LongitudinalMovement.AccelerationExtensionPercent, "ACCELERATION EXTENSION", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(animation, AnimationTraits.LongitudinalMovement.BrakingContractionPercent, "BRAKING CONTRACTION", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(animation, AnimationTraits.LongitudinalMovement.ExhaustVariationLimit, "EXHAUST VARIATION LIMIT", ConfigurationNumericSemantic::COUNT, 0, 32, 1);
        ADD_UINT(animation, AnimationTraits.LongitudinalMovement.WeaponTravelLimit, "LONGITUDINAL WEAPON TRAVEL", ConfigurationNumericSemantic::COUNT, 0, 32, 1);
        ADD_UINT(animation, AnimationTraits.LongitudinalMovement.AttachmentTravelLimit, "LONGITUDINAL ATTACHMENT TRAVEL", ConfigurationNumericSemantic::COUNT, 0, 32, 1);
        ADD_UINT(animation, AnimationTraits.LongitudinalMovement.BrakingTravelLimit, "BRAKING TRAVEL LIMIT", ConfigurationNumericSemantic::COUNT, 0, 32, 1);
        ADD_TOGGLE(animation, AnimationTraits.LongitudinalMovement.Synchronized, "LONGITUDINAL SYNCHRONIZED");
        ADD_TOGGLE(animation, AnimationTraits.LongitudinalMovement.Staggered, "LONGITUDINAL STAGGERED");
        ADD_TOGGLE(animation, AnimationTraits.LongitudinalMovement.HeavyResponse, "LONGITUDINAL HEAVY RESPONSE");
        ADD_TOGGLE(animation, AnimationTraits.LongitudinalMovement.Responsive, "LONGITUDINAL RESPONSIVE");
        ADD_UINT(animation, AnimationTraits.Firing.ResponseStrengthPercent, "FIRING RESPONSE STRENGTH", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(animation, AnimationTraits.Firing.DurationAdditionMilliseconds, "FIRING DURATION ADDITION MS", ConfigurationNumericSemantic::COUNT, 0, 5000, 10);
        ADD_UINT(animation, AnimationTraits.Firing.AdditionalRecoilLimit, "ADDITIONAL RECOIL LIMIT", ConfigurationNumericSemantic::COUNT, 0, 32, 1);
        ADD_UINT(animation, AnimationTraits.Firing.RailWeaponAdditionalRecoilLimit, "RAIL RECOIL ADDITION", ConfigurationNumericSemantic::COUNT, 0, 32, 1);
        ADD_UINT(animation, AnimationTraits.Firing.MaximumRecoilLimit, "MAXIMUM RECOIL LIMIT", ConfigurationNumericSemantic::COUNT, 0, 32, 1);
        ADD_UINT(animation, AnimationTraits.Firing.MinimumPreFireExtensionLimit, "MIN PREFIRE EXTENSION", ConfigurationNumericSemantic::COUNT, 0, 32, 1);
        ADD_TOGGLE(animation, AnimationTraits.Firing.HeavyResponse, "FIRING HEAVY RESPONSE");
        ADD_TOGGLE(animation, AnimationTraits.Firing.Responsive, "FIRING RESPONSIVE");

#undef ADD_CHOICE
#undef ADD_TOGGLE
#undef ADD_RANGE
#undef ADD_INT
#undef ADD_UINT
    }

    void ShipGenerationProfileEditorBindings::load(const Profile& profile)
    {
        for (StructuralProfileEditorSection& section : m_Sections)
        {
            for (StructuralIntegerFieldBinding& field : section.Integers) { field.Control.setValue(field.Read(profile)); }
            for (StructuralRangeFieldBinding& field : section.Ranges)
            {
                const SpectralShipGen::UIntRange value = field.Read(profile);
                field.Control.setValues(static_cast<int32_t>(value.Min), static_cast<int32_t>(value.Max));
            }
            for (StructuralToggleFieldBinding& field : section.Toggles) { field.Control.Value = field.Read(profile); }
            for (StructuralChoiceFieldBinding& field : section.Choices) { field.Control.setValue(field.Read(profile)); }
            for (StructuralWeightGroupBinding& field : section.WeightGroups)
            {
                const auto values = field.Read(profile);
                auto& rows = field.Control.getRows();
                for (std::size_t index = 0u; index < field.Control.getRowCount(); ++index) { rows[index].Control.setValue(static_cast<int32_t>(values[index])); }
                field.Control.refreshProbabilities();
            }
        }
    }

    void ShipGenerationProfileEditorBindings::write(Profile& profile) const
    {
        for (const StructuralProfileEditorSection& section : m_Sections)
        {
            for (const StructuralIntegerFieldBinding& field : section.Integers) { field.Write(profile, field.Control.Value); }
            for (const StructuralRangeFieldBinding& field : section.Ranges) { field.Write(profile, { static_cast<uint32_t>(field.Control.MinimumValue), static_cast<uint32_t>(field.Control.MaximumValue) }); }
            for (const StructuralToggleFieldBinding& field : section.Toggles) { field.Write(profile, field.Control.Value); }
            for (const StructuralChoiceFieldBinding& field : section.Choices) { field.Write(profile, field.Control.Value); }
            for (const StructuralWeightGroupBinding& field : section.WeightGroups)
            {
                std::array<uint32_t, ConfigurationWeightGroupControl::MaximumRows> values = {};
                const auto& rows = field.Control.getRows();
                for (std::size_t index = 0u; index < field.Control.getRowCount(); ++index) { values[index] = static_cast<uint32_t>(rows[index].Control.Value); }
                field.Write(profile, values);
            }
        }
    }

    bool ShipGenerationProfileEditorBindings::equivalent(const Profile& first, const Profile& second) const
    {
        for (const StructuralProfileEditorSection& section : m_Sections)
        {
            for (const StructuralIntegerFieldBinding& field : section.Integers) { if (field.Read(first) != field.Read(second)) { return false; } }
            for (const StructuralRangeFieldBinding& field : section.Ranges)
            {
                const auto a = field.Read(first);
                const auto b = field.Read(second);
                if (a.Min != b.Min || a.Max != b.Max) { return false; }
            }
            for (const StructuralToggleFieldBinding& field : section.Toggles) { if (field.Read(first) != field.Read(second)) { return false; } }
            for (const StructuralChoiceFieldBinding& field : section.Choices) { if (field.Read(first) != field.Read(second)) { return false; } }
            for (const StructuralWeightGroupBinding& field : section.WeightGroups) { if (field.Read(first) != field.Read(second)) { return false; } }
        }
        return true;
    }

    std::vector<StructuralProfileEditorSection>& ShipGenerationProfileEditorBindings::getSections() { return m_Sections; }
    const std::vector<StructuralProfileEditorSection>& ShipGenerationProfileEditorBindings::getSections() const { return m_Sections; }

    StructuralIntegerFieldBinding* ShipGenerationProfileEditorBindings::findInteger(std::string_view path) { return findByPath(m_Sections, path, &StructuralProfileEditorSection::Integers); }
    StructuralRangeFieldBinding* ShipGenerationProfileEditorBindings::findRange(std::string_view path) { return findByPath(m_Sections, path, &StructuralProfileEditorSection::Ranges); }
    StructuralToggleFieldBinding* ShipGenerationProfileEditorBindings::findToggle(std::string_view path) { return findByPath(m_Sections, path, &StructuralProfileEditorSection::Toggles); }
    StructuralChoiceFieldBinding* ShipGenerationProfileEditorBindings::findChoice(std::string_view path) { return findByPath(m_Sections, path, &StructuralProfileEditorSection::Choices); }
    StructuralWeightGroupBinding* ShipGenerationProfileEditorBindings::findWeightGroup(std::string_view path) { return findByPath(m_Sections, path, &StructuralProfileEditorSection::WeightGroups); }
    const StructuralIntegerFieldBinding* ShipGenerationProfileEditorBindings::findInteger(std::string_view path) const { return findByPath(m_Sections, path, &StructuralProfileEditorSection::Integers); }
    const StructuralRangeFieldBinding* ShipGenerationProfileEditorBindings::findRange(std::string_view path) const { return findByPath(m_Sections, path, &StructuralProfileEditorSection::Ranges); }
    const StructuralToggleFieldBinding* ShipGenerationProfileEditorBindings::findToggle(std::string_view path) const { return findByPath(m_Sections, path, &StructuralProfileEditorSection::Toggles); }
    const StructuralChoiceFieldBinding* ShipGenerationProfileEditorBindings::findChoice(std::string_view path) const { return findByPath(m_Sections, path, &StructuralProfileEditorSection::Choices); }
    const StructuralWeightGroupBinding* ShipGenerationProfileEditorBindings::findWeightGroup(std::string_view path) const { return findByPath(m_Sections, path, &StructuralProfileEditorSection::WeightGroups); }

    std::size_t ShipGenerationProfileEditorBindings::getBoundValueCount() const
    {
        std::size_t total = 0u;
        for (const StructuralProfileEditorSection& section : m_Sections)
        {
            total += section.Integers.size() + section.Ranges.size() + section.Toggles.size() + section.Choices.size();
            for (const StructuralWeightGroupBinding& group : section.WeightGroups) { total += group.Control.getRowCount(); }
        }
        return total;
    }
}
