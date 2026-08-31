#include "ShipFactionProfileEditorBindings.h"

#include <algorithm>
#include <initializer_list>
#include <utility>

namespace SpectralShipGenStudioPreview
{
    namespace
    {
        using Profile = SpectralShipGen::ShipFactionProfile;
        constexpr uint32_t WeightMaximum = 10000u;
        constexpr int32_t MultiplierMaximum = 2000;
        constexpr int32_t SafeOffsetMaximum = 1000000;
        constexpr int32_t CountMaximum = 64;
        constexpr int32_t ScaleMaximum = 1000;

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
        T* findByPath(std::vector<FactionProfileEditorSection>& sections, std::string_view path, std::vector<T> FactionProfileEditorSection::* member)
        {
            for (FactionProfileEditorSection& section : sections)
            {
                auto& fields = section.*member;
                const auto iterator = std::find_if(fields.begin(), fields.end(), [&](const T& field) { return field.Path == path; });
                if (iterator != fields.end()) { return &*iterator; }
            }
            return nullptr;
        }

        template<typename T>
        const T* findByPath(const std::vector<FactionProfileEditorSection>& sections, std::string_view path, const std::vector<T> FactionProfileEditorSection::* member)
        {
            for (const FactionProfileEditorSection& section : sections)
            {
                const auto& fields = section.*member;
                const auto iterator = std::find_if(fields.begin(), fields.end(), [&](const T& field) { return field.Path == path; });
                if (iterator != fields.end()) { return &*iterator; }
            }
            return nullptr;
        }
    }

    ShipFactionProfileEditorBindings::ShipFactionProfileEditorBindings()
    {
        m_Sections.reserve(16u);
        const auto addSection = [&](const char* label) -> FactionProfileEditorSection&
        {
            m_Sections.push_back({});
            m_Sections.back().Label = label;
            return m_Sections.back();
        };

        FactionProfileEditorSection& palette = addSection("PALETTE / FINISH LANGUAGE");
        FactionProfileEditorSection& surface = addSection("SURFACE DETAILS / MOTIFS");
        FactionProfileEditorSection& attachments = addSection("ATTACHMENTS");
        FactionProfileEditorSection& engines = addSection("ENGINES");
        FactionProfileEditorSection& weapons = addSection("WEAPONS");
        FactionProfileEditorSection& major = addSection("MAJOR FEATURES");
        FactionProfileEditorSection& cockpit = addSection("COCKPIT / TECHNOLOGY");
        FactionProfileEditorSection& hull = addSection("HULL / NEGATIVE SPACE");
        FactionProfileEditorSection& layers = addSection("HULL LAYERS");
        FactionProfileEditorSection& materials = addSection("MATERIALS");
        FactionProfileEditorSection& livery = addSection("LIVERY");
        FactionProfileEditorSection& hierarchy = addSection("HIERARCHY / ASYMMETRY / COMPLEXITY");
        FactionProfileEditorSection& finish = addSection("PAINTING / FINISH ROLES");
        FactionProfileEditorSection& idle = addSection("ANIMATION / IDLE");
        FactionProfileEditorSection& movement = addSection("ANIMATION / MOVEMENT");
        FactionProfileEditorSection& firing = addSection("ANIMATION / FIRING");

        const auto addInteger = [](FactionProfileEditorSection& section, const char* path, const char* label, ConfigurationNumericSemantic semantic, int32_t minimum, int32_t maximum, int32_t step,
            std::function<int32_t(const Profile&)> read, std::function<void(Profile&, int32_t)> write)
        {
            FactionIntegerFieldBinding field;
            field.Path = path;
            field.Control.configure(label, semantic, minimum, maximum, step, 0);
            field.Read = std::move(read);
            field.Write = std::move(write);
            section.Integers.push_back(std::move(field));
        };

        const auto addRange = [](FactionProfileEditorSection& section, const char* path, const char* label, int32_t minimum, int32_t maximum, int32_t step,
            std::function<FactionRangeValue(const Profile&)> read, std::function<void(Profile&, FactionRangeValue)> write)
        {
            FactionRangeFieldBinding field;
            field.Path = path;
            field.Control.configure(label, minimum, maximum, step, 0, 0);
            field.Read = std::move(read);
            field.Write = std::move(write);
            section.Ranges.push_back(std::move(field));
        };

        const auto addToggle = [](FactionProfileEditorSection& section, const char* path, const char* label,
            std::function<bool(const Profile&)> read, std::function<void(Profile&, bool)> write)
        {
            FactionToggleFieldBinding field;
            field.Path = path;
            field.Control.configure(label, false);
            field.Read = std::move(read);
            field.Write = std::move(write);
            section.Toggles.push_back(std::move(field));
        };

        const auto addChoice = [](FactionProfileEditorSection& section, const char* path, const char* label, std::vector<std::string> options,
            std::function<uint32_t(const Profile&)> read, std::function<void(Profile&, uint32_t)> write)
        {
            FactionChoiceFieldBinding field;
            field.Path = path;
            field.Control.configure(label, std::move(options), 0u);
            field.Read = std::move(read);
            field.Write = std::move(write);
            section.Choices.push_back(std::move(field));
        };

        const auto addWeightGroup = [](FactionProfileEditorSection& section, const char* path, const char* label, std::initializer_list<const char*> labels,
            std::function<std::array<uint32_t, ConfigurationWeightGroupControl::MaximumRows>(const Profile&)> read,
            std::function<void(Profile&, const std::array<uint32_t, ConfigurationWeightGroupControl::MaximumRows>&)> write)
        {
            FactionWeightGroupBinding field;
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
#define ADD_TOGGLE(section, member, label) \
        addToggle(section, #member, label, [](const Profile& p) { return p.member; }, [](Profile& p, bool value) { p.member = value; })
#define ADD_CHOICE(section, member, label, options) \
        addChoice(section, #member, label, options, [](const Profile& p) { return static_cast<uint32_t>(p.member); }, \
            [](Profile& p, uint32_t value) { p.member = static_cast<decltype(p.member)>(value); })
#define ADD_UINT_RANGE(section, member, label, minimum, maximum, step) \
        addRange(section, #member, label, minimum, maximum, step, \
            [](const Profile& p) { return FactionRangeValue{ static_cast<int32_t>(p.member.Min), static_cast<int32_t>(p.member.Max) }; }, \
            [](Profile& p, FactionRangeValue value) { p.member = { static_cast<uint32_t>(value.Min), static_cast<uint32_t>(value.Max) }; })
#define ADD_INT_RANGE(section, member, label, minimum, maximum, step) \
        addRange(section, #member, label, minimum, maximum, step, \
            [](const Profile& p) { return FactionRangeValue{ p.member.Min, p.member.Max }; }, \
            [](Profile& p, FactionRangeValue value) { p.member = { value.Min, value.Max }; })
#define ADD_SCALE(section, member, label) \
        ADD_UINT(section, member.Numerator, label " NUMERATOR", ConfigurationNumericSemantic::COUNT, 0, ScaleMaximum, 1); \
        ADD_UINT(section, member.Denominator, label " DENOMINATOR", ConfigurationNumericSemantic::COUNT, 1, ScaleMaximum, 1)

        ADD_UINT_RANGE(palette, Palette.HullHue, "HULL HUE", 0, 359, 1);
        ADD_UINT_RANGE(palette, Palette.HullSaturation, "HULL SATURATION", 0, 100, 1);
        ADD_UINT_RANGE(palette, Palette.HullValue, "HULL VALUE", 0, 100, 1);
        ADD_INT_RANGE(palette, Palette.Accent.HueOffset, "ACCENT HUE OFFSET", -360, 360, 1);
        ADD_UINT_RANGE(palette, Palette.Accent.Saturation, "ACCENT SATURATION", 0, 100, 1);
        ADD_UINT_RANGE(palette, Palette.Accent.Value, "ACCENT VALUE", 0, 100, 1);
        ADD_INT_RANGE(palette, Palette.Cockpit.HueOffset, "COCKPIT HUE OFFSET", -360, 360, 1);
        ADD_UINT_RANGE(palette, Palette.Cockpit.Saturation, "COCKPIT SATURATION", 0, 100, 1);
        ADD_UINT_RANGE(palette, Palette.Cockpit.Value, "COCKPIT VALUE", 0, 100, 1);
        ADD_INT_RANGE(palette, Palette.Light.HueOffset, "LIGHT HUE OFFSET", -360, 360, 1);
        ADD_UINT_RANGE(palette, Palette.Light.Saturation, "LIGHT SATURATION", 0, 100, 1);
        ADD_UINT_RANGE(palette, Palette.Light.Value, "LIGHT VALUE", 0, 100, 1);
        ADD_INT_RANGE(palette, Palette.Exhaust.HueOffset, "EXHAUST HUE OFFSET", -360, 360, 1);
        ADD_UINT_RANGE(palette, Palette.Exhaust.Saturation, "EXHAUST SATURATION", 0, 100, 1);
        ADD_UINT_RANGE(palette, Palette.Exhaust.Value, "EXHAUST VALUE", 0, 100, 1);
        ADD_UINT_RANGE(palette, Palette.MechanicalSaturation, "MECHANICAL SATURATION", 0, 100, 1);
        ADD_UINT_RANGE(palette, Palette.MechanicalValue, "MECHANICAL VALUE", 0, 100, 1);
        ADD_CHOICE(palette, PaletteBehavior.HullValueMode, "HULL VALUE MODE", (std::vector<std::string>{ "PROFILE RANGE", "ALTERNATING BRIGHT/DARK" }));
        ADD_UINT_RANGE(palette, PaletteBehavior.BrightHullValue, "BRIGHT HULL VALUE", 0, 100, 1);
        ADD_UINT_RANGE(palette, PaletteBehavior.DarkHullValue, "DARK HULL VALUE", 0, 100, 1);
        ADD_CHOICE(palette, PaletteBehavior.SecondaryToneDirection, "SECONDARY TONE DIRECTION", (std::vector<std::string>{ "RANDOM", "DARKER", "LIGHTER", "CONTRAST FROM MIDPOINT" }));
        ADD_UINT(palette, PaletteBehavior.MinimumAccentHueDistance, "MIN ACCENT HUE DISTANCE", ConfigurationNumericSemantic::COUNT, 0, 180, 1);
        ADD_INT(palette, PaletteBehavior.AccentHueSeparationShiftA, "ACCENT HUE SHIFT A", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1);
        ADD_INT(palette, PaletteBehavior.AccentHueSeparationShiftB, "ACCENT HUE SHIFT B", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1);

        ADD_UINT(surface, SurfaceDetails.DetailDensityPercent, "DETAIL DENSITY", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(surface, SurfaceDetails.MechanicalPatternCountPercent, "MECHANICAL PATTERN COUNT", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(surface, SurfaceDetails.LightPatternCountPercent, "LIGHT PATTERN COUNT", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(surface, SurfaceDetails.AccentPanelWeightPercent, "ACCENT PANEL WEIGHT", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(surface, SurfaceDetails.AccentStripeWeightPercent, "ACCENT STRIPE WEIGHT", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(surface, SurfaceDetails.AccentArmorWeightPercent, "ACCENT ARMOR WEIGHT", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(surface, SurfaceDetails.HorizontalVentChancePercent, "HORIZONTAL VENT CHANCE MULTIPLIER", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(surface, SurfaceDetails.MotifRepeatPercent, "MOTIF REPEAT", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_INT(surface, SurfaceDetails.AsymmetricDetailChanceOffset, "ASYMMETRIC DETAIL CHANCE OFFSET", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1);
        ADD_UINT(surface, SurfaceDetails.LuminousChannelCoreRegionBiasChance, "LUMINOUS CORE BIAS CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        addWeightGroup(surface, "SurfaceDetails.SupplementalWeightMultipliersPercent", "SUPPLEMENTAL DETAIL MULTIPLIERS", { "PANEL SEAM", "GEOMETRIC MARKING", "MECHANICAL EXPOSURE", "REPEATING MOTIF", "IDENTIFICATION", "LUMINOUS CHANNEL" },
            [](const Profile& p) { const auto& w = p.SurfaceDetails.SupplementalWeightMultipliersPercent; return std::array<uint32_t, 10u>{ w.PanelSeam, w.GeometricMarking, w.MechanicalExposure, w.RepeatingMotif, w.IdentificationMarking, w.LuminousChannel }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.SurfaceDetails.SupplementalWeightMultipliersPercent = { v[0], v[1], v[2], v[3], v[4], v[5] }; });
        addWeightGroup(surface, "SurfaceDetails.MotifWeightMultipliersPercent", "MOTIF WEIGHT MULTIPLIERS", { "PAIRED VENTS", "TRIPLE VENTS", "PAIRED LIGHTS", "THREE LIGHTS", "PARALLEL SEAMS", "REPEATED DASHES", "RECESSED SLOT" },
            [](const Profile& p) { const auto& w = p.SurfaceDetails.MotifWeightMultipliersPercent; return std::array<uint32_t, 10u>{ w.PairedVents, w.TripleVentBank, w.PairedLights, w.ThreeNodeLights, w.ParallelSeams, w.RepeatedDashes, w.RecessedSlot }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.SurfaceDetails.MotifWeightMultipliersPercent = { v[0], v[1], v[2], v[3], v[4], v[5], v[6] }; });

        ADD_UINT(attachments, Attachments.AttachmentChancePercent, "ATTACHMENT CHANCE MULTIPLIER", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_INT(attachments, Attachments.SymmetryChanceOffset, "SYMMETRY CHANCE OFFSET", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1);
        addWeightGroup(attachments, "Attachments.WeightMultipliersPercent", "ATTACHMENT WEIGHT MULTIPLIERS", { "WEAPON MOUNT", "SENSOR ARRAY", "AUXILIARY POD", "RADIATOR", "ARMOR FIN", "TECH NODE" },
            [](const Profile& p) { const auto& w = p.Attachments.WeightMultipliersPercent; return std::array<uint32_t, 10u>{ w.WeaponMount, w.SensorArray, w.AuxiliaryPod, w.Radiator, w.ArmorFin, w.TechnologyNode }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.Attachments.WeightMultipliersPercent = { v[0], v[1], v[2], v[3], v[4], v[5] }; });

        ADD_UINT(engines, Engines.NacelleChancePercent, "NACELLE CHANCE MULTIPLIER", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(engines, Engines.ExternalHeightPercent, "EXTERNAL HEIGHT MULTIPLIER", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        addWeightGroup(engines, "Engines.LayoutWeightMultipliersPercent", "ENGINE LAYOUT MULTIPLIERS", { "CENTRAL", "TWIN", "QUAD", "CENTRAL + AUX", "WIDE BANK" },
            [](const Profile& p) { const auto& w = p.Engines.LayoutWeightMultipliersPercent; return std::array<uint32_t, 10u>{ w.Central, w.Twin, w.Quad, w.CentralAuxiliary, w.WideBank }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.Engines.LayoutWeightMultipliersPercent = { v[0], v[1], v[2], v[3], v[4] }; });
        addWeightGroup(engines, "Engines.SizeWeightMultipliersPercent", "ENGINE SIZE MULTIPLIERS", { "SMALL", "MEDIUM", "LARGE" },
            [](const Profile& p) { const auto& w = p.Engines.SizeWeightMultipliersPercent; return std::array<uint32_t, 10u>{ w.Small, w.Medium, w.Large }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.Engines.SizeWeightMultipliersPercent = { v[0], v[1], v[2] }; });

        ADD_UINT(weapons, Weapons.ChancePercent, "LARGE WEAPON CHANCE MULTIPLIER", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_INT(weapons, Weapons.SymmetryChanceOffset, "WEAPON SYMMETRY OFFSET", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1);
        ADD_UINT(weapons, Weapons.EmissiveChance, "EMISSIVE WEAPON CHANCE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        addWeightGroup(weapons, "Weapons.WeightMultipliersPercent", "WEAPON TYPE MULTIPLIERS", { "SINGLE CANNON", "TWIN CANNON", "COMPACT TURRET", "RAIL WEAPON", "WEAPON POD" },
            [](const Profile& p) { const auto& w = p.Weapons.WeightMultipliersPercent; return std::array<uint32_t, 10u>{ w.SingleCannon, w.TwinCannon, w.CompactTurret, w.RailWeapon, w.WeaponPod }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.Weapons.WeightMultipliersPercent = { v[0], v[1], v[2], v[3], v[4] }; });

        ADD_UINT(major, MajorFeatures.ChancePercent, "MAJOR FEATURE CHANCE MULTIPLIER", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        addWeightGroup(major, "MajorFeatures.WeightMultipliersPercent", "MAJOR FEATURE MULTIPLIERS", { "CENTRAL SPINE", "ARMOR PLATE", "RECESSED BAY", "VENT BANK", "WING PLATE", "TECH CORE" },
            [](const Profile& p) { const auto& w = p.MajorFeatures.WeightMultipliersPercent; return std::array<uint32_t, 10u>{ w.CentralSpine, w.ArmorPlate, w.RecessedBay, w.VentBank, w.WingPlate, w.TechCore }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.MajorFeatures.WeightMultipliersPercent = { v[0], v[1], v[2], v[3], v[4], v[5] }; });

        addWeightGroup(cockpit, "Cockpit.SizeWeightMultipliersPercent", "COCKPIT SIZE MULTIPLIERS", { "COMPACT", "STANDARD", "LARGE", "MASSIVE" },
            [](const Profile& p) { const auto& w = p.Cockpit.SizeWeightMultipliersPercent; return std::array<uint32_t, 10u>{ w.Compact, w.Standard, w.Large, w.Massive }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.Cockpit.SizeWeightMultipliersPercent = { v[0], v[1], v[2], v[3] }; });
        addWeightGroup(cockpit, "Cockpit.ShapeWeightMultipliersPercent", "COCKPIT SHAPE MULTIPLIERS", { "COMPACT CANOPY", "ELONGATED", "WIDE COMMAND", "SPLIT CANOPY", "DORSAL BRIDGE", "LAYERED BRIDGE" },
            [](const Profile& p) { const auto& w = p.Cockpit.ShapeWeightMultipliersPercent; return std::array<uint32_t, 10u>{ w.CompactCanopy, w.ElongatedCanopy, w.WideCommandDeck, w.SplitCanopy, w.DorsalBridge, w.LayeredBridge }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.Cockpit.ShapeWeightMultipliersPercent = { v[0], v[1], v[2], v[3], v[4], v[5] }; });
        ADD_UINT(cockpit, CoreTreatment.ChancePercent, "CORE TREATMENT CHANCE MULTIPLIER", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_INT(cockpit, CoreTreatment.WeightOffsets.CentralSpine, "CORE CENTRAL SPINE OFFSET", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1);
        ADD_INT(cockpit, CoreTreatment.WeightOffsets.CockpitSurround, "COCKPIT SURROUND OFFSET", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1);
        ADD_INT(cockpit, CoreTreatment.WeightOffsets.RaisedCorePlate, "RAISED CORE PLATE OFFSET", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1);
        ADD_INT(cockpit, CoreTreatment.WeightOffsets.LateralRecesses, "LATERAL RECESSES OFFSET", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1);
        ADD_INT(cockpit, CoreTreatment.WeightOffsets.LongitudinalArmorBand, "ARMOR BAND OFFSET", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1);
        ADD_INT(cockpit, CoreTreatment.WeightOffsets.CoreChannel, "CORE CHANNEL OFFSET", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1);
        ADD_CHOICE(cockpit, CoreTreatment.CoreChannelLuminousPattern, "CORE CHANNEL LUMINOUS PATTERN", (std::vector<std::string>{ "NONE", "EVERY THIRD ROW", "EXCEPT EVERY THIRD ROW" }));

        ADD_UINT(hull, Hull.NegativeSpaceChancePercent, "NEGATIVE SPACE CHANCE MULTIPLIER", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_TOGGLE(hull, Hull.PreferAlternateArticulationOrder, "PREFER ALTERNATE ARTICULATION ORDER");
        addWeightGroup(hull, "Hull.NegativeSpaceWeightMultipliersPercent", "NEGATIVE SPACE MULTIPLIERS", { "WING CHANNEL", "REAR FORK", "SHOULDER GAP", "OPEN FRAME BAY", "NACELLE CHANNEL" },
            [](const Profile& p) { const auto& w = p.Hull.NegativeSpaceWeightMultipliersPercent; return std::array<uint32_t, 10u>{ w.WingChannel, w.RearFork, w.ShoulderGap, w.OpenFrameBay, w.NacelleChannel }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.Hull.NegativeSpaceWeightMultipliersPercent = { v[0], v[1], v[2], v[3], v[4] }; });

        ADD_UINT(layers, HullLayers.ChancePercent, "HULL LAYER CHANCE MULTIPLIER", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(layers, HullLayers.MaximumLayerCount, "MAXIMUM LAYER COUNT / 0=INHERIT", ConfigurationNumericSemantic::COUNT, 0, CountMaximum, 1);
#define ADD_LAYER_ADJUSTMENT(member, label) \
        ADD_INT(layers, HullLayers.WeightAdjustments.member.Offset, label " OFFSET", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1); \
        ADD_SCALE(layers, HullLayers.WeightAdjustments.member.Scale, label " SCALE")
        ADD_LAYER_ADJUSTMENT(CentralDorsalPlate, "DORSAL PLATE");
        ADD_LAYER_ADJUSTMENT(ForwardArmor, "FORWARD ARMOR");
        ADD_LAYER_ADJUSTMENT(WingArmor, "WING ARMOR");
        ADD_LAYER_ADJUSTMENT(ShoulderArmor, "SHOULDER ARMOR");
        ADD_LAYER_ADJUSTMENT(RearEngineCover, "ENGINE COVER");
#undef ADD_LAYER_ADJUSTMENT

        addWeightGroup(materials, "Materials.ZoneWeightMultipliersPercent", "MATERIAL ZONE MULTIPLIERS", { "WING SURFACE", "SHOULDER", "AXIAL BAND", "REAR MECHANICAL", "COCKPIT COLLAR", "HARDPOINT" },
            [](const Profile& p) { const auto& w = p.Materials.ZoneWeightMultipliersPercent; return std::array<uint32_t, 10u>{ w.WingSurface, w.ShoulderSurface, w.AxialBand, w.RearMechanical, w.CockpitCollar, w.HardpointSurround }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.Materials.ZoneWeightMultipliersPercent = { v[0], v[1], v[2], v[3], v[4], v[5] }; });

        ADD_UINT(livery, Livery.ChancePercent, "LIVERY CHANCE MULTIPLIER", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_INT(livery, Livery.AsymmetricChanceOffset, "ASYMMETRIC CHANCE OFFSET", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1);
        ADD_UINT(livery, Livery.AsymmetricChanceDivisor, "ASYMMETRIC CHANCE DIVISOR", ConfigurationNumericSemantic::COUNT, 1, 1000, 1);
        ADD_TOGGLE(livery, Livery.AllowAsymmetricGeometricInsignia, "ALLOW ASYMMETRIC INSIGNIA");
        addWeightGroup(livery, "Livery.WeightMultipliersPercent", "LIVERY TYPE MULTIPLIERS", { "CENTER STRIPE", "DOUBLE STRIPE", "WING BAND", "SHOULDER BLOCK", "NOSE BAND", "CHEVRON", "ID PANEL", "INSIGNIA" },
            [](const Profile& p) { const auto& w = p.Livery.WeightMultipliersPercent; return std::array<uint32_t, 10u>{ w.CenterStripe, w.DoubleCenterStripe, w.WingBand, w.ShoulderBlock, w.NoseBand, w.Chevron, w.IdPanel, w.GeometricInsignia }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.Livery.WeightMultipliersPercent = { v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7] }; });

        ADD_UINT(hierarchy, MacroAsymmetry.ChancePercent, "MACRO ASYMMETRY CHANCE MULTIPLIER", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        ADD_UINT(hierarchy, Complexity.TotalBudgetPercent, "TOTAL COMPLEXITY BUDGET", ConfigurationNumericSemantic::MULTIPLIER_PERCENT, 0, MultiplierMaximum, 5);
        addWeightGroup(hierarchy, "VisualHierarchy.AnchorWeightMultipliersPercent", "VISUAL ANCHOR MULTIPLIERS", { "SILHOUETTE", "COCKPIT", "WINGS", "ENGINES", "WEAPONS", "MAJOR FEATURE", "HULL LAYERS", "CENTRAL CORE", "MACRO ASYMMETRY", "NEGATIVE SPACE" },
            [](const Profile& p) { const auto& w = p.VisualHierarchy.AnchorWeightMultipliersPercent; return std::array<uint32_t, 10u>{ w.Silhouette, w.Cockpit, w.Wings, w.Engines, w.Weapons, w.MajorFeature, w.HullLayers, w.CentralCore, w.MacroAsymmetry, w.NegativeSpace }; },
            [](Profile& p, const std::array<uint32_t, 10u>& v) { p.VisualHierarchy.AnchorWeightMultipliersPercent = { v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9] }; });
#define ADD_COMPLEXITY_OFFSETS(prefix, label) \
        ADD_INT(hierarchy, Complexity.prefix.Silhouette, label " SILHOUETTE", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1); \
        ADD_INT(hierarchy, Complexity.prefix.CockpitStructure, label " COCKPIT", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1); \
        ADD_INT(hierarchy, Complexity.prefix.HullLayer, label " HULL LAYER", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1); \
        ADD_INT(hierarchy, Complexity.prefix.MajorFeature, label " MAJOR FEATURE", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1); \
        ADD_INT(hierarchy, Complexity.prefix.LargeWeapon, label " LARGE WEAPON", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1); \
        ADD_INT(hierarchy, Complexity.prefix.Attachment, label " ATTACHMENT", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1); \
        ADD_INT(hierarchy, Complexity.prefix.Detail, label " DETAIL", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1)
        ADD_COMPLEXITY_OFFSETS(LegacyCategoryOffsets, "LEGACY OFFSET");
        ADD_COMPLEXITY_OFFSETS(CategoryOffsets, "CATEGORY OFFSET");
#undef ADD_COMPLEXITY_OFFSETS

        const std::vector<std::string> paintRoles = { "PROFILE DEFAULT", "HULL BASE", "HULL SECONDARY", "HULL HIGHLIGHT", "HULL ACCENT", "ACCENT HIGHLIGHT", "MECHANICAL BASE", "ENGINE BASE", "ENGINE HIGHLIGHT", "ENGINE HOT CORE", "LIGHT BASE", "LIGHT HIGHLIGHT" };
        ADD_CHOICE(finish, Finish.WeaponMuzzleRole, "WEAPON MUZZLE ROLE", paintRoles);
        ADD_CHOICE(finish, Finish.WeaponBodyRole, "WEAPON BODY ROLE", paintRoles);
        ADD_CHOICE(finish, Finish.WeaponRaisedHighlightRole, "WEAPON RAISED HIGHLIGHT ROLE", paintRoles);
        ADD_CHOICE(finish, Finish.CoreSecondaryMaterialRole, "CORE SECONDARY MATERIAL ROLE", paintRoles);
        ADD_CHOICE(finish, Finish.CoreRaisedRole, "CORE RAISED ROLE", paintRoles);
        ADD_CHOICE(finish, Finish.CoreLuminousRole, "CORE LUMINOUS ROLE", paintRoles);
        ADD_CHOICE(finish, Finish.CoreLuminousHighlightRole, "CORE LUMINOUS HIGHLIGHT ROLE", paintRoles);
        ADD_CHOICE(finish, Finish.CentralDorsalPlateRole, "DORSAL PLATE ROLE", paintRoles);
        ADD_CHOICE(finish, Finish.CockpitBaseRole, "COCKPIT BASE ROLE", paintRoles);
        ADD_CHOICE(finish, Finish.CockpitFrameRole, "COCKPIT FRAME ROLE", paintRoles);
        ADD_CHOICE(finish, Finish.EngineHotCoreRole, "ENGINE HOT CORE ROLE", paintRoles);
        ADD_CHOICE(finish, Finish.EngineInteriorHighlightRole, "ENGINE INTERIOR HIGHLIGHT ROLE", paintRoles);
        ADD_TOGGLE(finish, Finish.ForceAxialRidgeEdgeHighlight, "FORCE AXIAL RIDGE EDGE HIGHLIGHT");

        const std::vector<std::string> overrideOptions = { "INHERIT", "ENABLE", "DISABLE" };
        ADD_INT(idle, Animation.Idle.EngineMechanicalChanceOffset, "ENGINE MECHANICAL CHANCE OFFSET", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1);
        ADD_UINT(idle, Animation.Idle.EngineMechanicalChanceMaximum, "ENGINE MECHANICAL CHANCE MAX / 0=NONE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(idle, Animation.Idle.EngineMechanicalChanceMinimum, "ENGINE MECHANICAL CHANCE MIN", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(idle, Animation.Idle.EnginePulseStrengthMinimum, "ENGINE PULSE STRENGTH MIN", ConfigurationNumericSemantic::COUNT, 0, 255, 1);
        ADD_SCALE(idle, Animation.Idle.ExhaustAmplitudeScale, "EXHAUST AMPLITUDE");
        ADD_INT(idle, Animation.Idle.WeaponMechanicalChanceOffset, "WEAPON MECHANICAL CHANCE OFFSET", ConfigurationNumericSemantic::SIGNED_OFFSET, -SafeOffsetMaximum, SafeOffsetMaximum, 1);
        ADD_SCALE(idle, Animation.Idle.WeaponMechanicalChanceScale, "WEAPON MECHANICAL CHANCE");
        ADD_UINT(idle, Animation.Idle.WeaponMechanicalChanceMinimum, "WEAPON MECHANICAL CHANCE MIN", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_UINT(idle, Animation.Idle.WeaponMechanicalChanceMaximum, "WEAPON MECHANICAL CHANCE MAX / 0=NONE", ConfigurationNumericSemantic::PROBABILITY, 0, 100, 1);
        ADD_SCALE(idle, Animation.Idle.VentActivityChanceScale, "VENT ACTIVITY CHANCE");
        ADD_UINT(idle, Animation.Idle.TechPulseStrength, "TECH PULSE STRENGTH", ConfigurationNumericSemantic::COUNT, 0, 255, 1);
        ADD_CHOICE(idle, Animation.Idle.SynchronizeEngines, "SYNCHRONIZE ENGINES", overrideOptions);
        ADD_CHOICE(idle, Animation.Idle.AsynchronousEngines, "ASYNCHRONOUS ENGINES", overrideOptions);
        ADD_CHOICE(idle, Animation.Idle.AlternateEnginePhases, "ALTERNATE ENGINE PHASES", overrideOptions);
        ADD_CHOICE(idle, Animation.Idle.AlternateWeaponPhases, "ALTERNATE WEAPON PHASES", overrideOptions);
        ADD_CHOICE(idle, Animation.Idle.SlowMechanicalCycle, "SLOW MECHANICAL CYCLE", overrideOptions);
        ADD_CHOICE(idle, Animation.Idle.IrregularEngineCycle, "IRREGULAR ENGINE CYCLE", overrideOptions);
        ADD_TOGGLE(idle, Animation.Idle.RandomizeSymmetricWeaponAlternatePhase, "RANDOMIZE SYMMETRIC WEAPON PHASE");
        ADD_TOGGLE(idle, Animation.Idle.AlternateTechCorePhases, "ALTERNATE TECH CORE PHASES");

#define ADD_MOVEMENT(prefix, label) \
        ADD_SCALE(movement, Animation.prefix.ResponseStrengthScale, label " RESPONSE STRENGTH"); \
        ADD_CHOICE(movement, Animation.prefix.Synchronized, label " SYNCHRONIZED", overrideOptions); \
        ADD_CHOICE(movement, Animation.prefix.Staggered, label " STAGGERED", overrideOptions); \
        ADD_CHOICE(movement, Animation.prefix.HeavyResponse, label " HEAVY RESPONSE", overrideOptions); \
        ADD_TOGGLE(movement, Animation.prefix.SquareTransitionInput, label " SQUARE TRANSITION INPUT"); \
        ADD_UINT(movement, Animation.prefix.MinimumExhaustVariationLimit, label " MIN EXHAUST VARIATION", ConfigurationNumericSemantic::COUNT, 0, CountMaximum, 1)
        ADD_MOVEMENT(LateralMovement, "LATERAL");
        ADD_MOVEMENT(LongitudinalMovement, "LONGITUDINAL");
#undef ADD_MOVEMENT

        ADD_SCALE(firing, Animation.Firing.DurationScale, "DURATION");
        ADD_INT(firing, Animation.Firing.DurationAdditionMilliseconds, "DURATION ADDITION MS", ConfigurationNumericSemantic::SIGNED_OFFSET, -10000, 10000, 10);
        ADD_SCALE(firing, Animation.Firing.ResponseStrengthScale, "RESPONSE STRENGTH");
        ADD_UINT(firing, Animation.Firing.MaximumPreFireExtensionLimit, "MAX PREFIRE EXTENSION / 0=NONE", ConfigurationNumericSemantic::COUNT, 0, CountMaximum, 1);
        ADD_CHOICE(firing, Animation.Firing.HeavyResponse, "HEAVY RESPONSE", overrideOptions);

#undef ADD_SCALE
#undef ADD_INT_RANGE
#undef ADD_UINT_RANGE
#undef ADD_CHOICE
#undef ADD_TOGGLE
#undef ADD_INT
#undef ADD_UINT
    }

    void ShipFactionProfileEditorBindings::load(const Profile& profile)
    {
        for (FactionProfileEditorSection& section : m_Sections)
        {
            for (FactionIntegerFieldBinding& field : section.Integers) { field.Control.setValue(field.Read(profile)); }
            for (FactionRangeFieldBinding& field : section.Ranges)
            {
                const FactionRangeValue value = field.Read(profile);
                field.Control.setValues(value.Min, value.Max);
            }
            for (FactionToggleFieldBinding& field : section.Toggles) { field.Control.Value = field.Read(profile); }
            for (FactionChoiceFieldBinding& field : section.Choices) { field.Control.setValue(field.Read(profile)); }
            for (FactionWeightGroupBinding& field : section.WeightGroups)
            {
                const auto values = field.Read(profile);
                auto& rows = field.Control.getRows();
                for (std::size_t index = 0u; index < field.Control.getRowCount(); ++index) { rows[index].Control.setValue(static_cast<int32_t>(values[index])); }
                field.Control.refreshProbabilities();
            }
        }
    }

    void ShipFactionProfileEditorBindings::write(Profile& profile) const
    {
        for (const FactionProfileEditorSection& section : m_Sections)
        {
            for (const FactionIntegerFieldBinding& field : section.Integers) { field.Write(profile, field.Control.Value); }
            for (const FactionRangeFieldBinding& field : section.Ranges) { field.Write(profile, { field.Control.MinimumValue, field.Control.MaximumValue }); }
            for (const FactionToggleFieldBinding& field : section.Toggles) { field.Write(profile, field.Control.Value); }
            for (const FactionChoiceFieldBinding& field : section.Choices) { field.Write(profile, field.Control.Value); }
            for (const FactionWeightGroupBinding& field : section.WeightGroups)
            {
                std::array<uint32_t, ConfigurationWeightGroupControl::MaximumRows> values = {};
                const auto& rows = field.Control.getRows();
                for (std::size_t index = 0u; index < field.Control.getRowCount(); ++index) { values[index] = static_cast<uint32_t>(rows[index].Control.Value); }
                field.Write(profile, values);
            }
        }
    }

    bool ShipFactionProfileEditorBindings::equivalent(const Profile& first, const Profile& second) const
    {
        for (const FactionProfileEditorSection& section : m_Sections)
        {
            for (const FactionIntegerFieldBinding& field : section.Integers) { if (field.Read(first) != field.Read(second)) { return false; } }
            for (const FactionRangeFieldBinding& field : section.Ranges) { if (field.Read(first) != field.Read(second)) { return false; } }
            for (const FactionToggleFieldBinding& field : section.Toggles) { if (field.Read(first) != field.Read(second)) { return false; } }
            for (const FactionChoiceFieldBinding& field : section.Choices) { if (field.Read(first) != field.Read(second)) { return false; } }
            for (const FactionWeightGroupBinding& field : section.WeightGroups) { if (field.Read(first) != field.Read(second)) { return false; } }
        }
        return true;
    }

    std::vector<FactionProfileEditorSection>& ShipFactionProfileEditorBindings::getSections() { return m_Sections; }
    const std::vector<FactionProfileEditorSection>& ShipFactionProfileEditorBindings::getSections() const { return m_Sections; }

    FactionIntegerFieldBinding* ShipFactionProfileEditorBindings::findInteger(std::string_view path) { return findByPath(m_Sections, path, &FactionProfileEditorSection::Integers); }
    FactionRangeFieldBinding* ShipFactionProfileEditorBindings::findRange(std::string_view path) { return findByPath(m_Sections, path, &FactionProfileEditorSection::Ranges); }
    FactionToggleFieldBinding* ShipFactionProfileEditorBindings::findToggle(std::string_view path) { return findByPath(m_Sections, path, &FactionProfileEditorSection::Toggles); }
    FactionChoiceFieldBinding* ShipFactionProfileEditorBindings::findChoice(std::string_view path) { return findByPath(m_Sections, path, &FactionProfileEditorSection::Choices); }
    FactionWeightGroupBinding* ShipFactionProfileEditorBindings::findWeightGroup(std::string_view path) { return findByPath(m_Sections, path, &FactionProfileEditorSection::WeightGroups); }
    const FactionIntegerFieldBinding* ShipFactionProfileEditorBindings::findInteger(std::string_view path) const { return findByPath(m_Sections, path, &FactionProfileEditorSection::Integers); }
    const FactionRangeFieldBinding* ShipFactionProfileEditorBindings::findRange(std::string_view path) const { return findByPath(m_Sections, path, &FactionProfileEditorSection::Ranges); }
    const FactionToggleFieldBinding* ShipFactionProfileEditorBindings::findToggle(std::string_view path) const { return findByPath(m_Sections, path, &FactionProfileEditorSection::Toggles); }
    const FactionChoiceFieldBinding* ShipFactionProfileEditorBindings::findChoice(std::string_view path) const { return findByPath(m_Sections, path, &FactionProfileEditorSection::Choices); }
    const FactionWeightGroupBinding* ShipFactionProfileEditorBindings::findWeightGroup(std::string_view path) const { return findByPath(m_Sections, path, &FactionProfileEditorSection::WeightGroups); }

    std::size_t ShipFactionProfileEditorBindings::getBoundValueCount() const
    {
        std::size_t total = 0u;
        for (const FactionProfileEditorSection& section : m_Sections)
        {
            total += section.Integers.size() + section.Toggles.size() + section.Choices.size();
            total += section.Ranges.size() * 2u;
            for (const FactionWeightGroupBinding& group : section.WeightGroups) { total += group.Control.getRowCount(); }
        }
        return total;
    }
}
