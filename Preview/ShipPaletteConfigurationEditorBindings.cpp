#include "ShipPaletteConfigurationEditorBindings.h"

#include <algorithm>
#include <utility>

namespace SpectralShipGenStudioPreview
{
    namespace
    {
        using Configuration = SpectralShipGen::ShipPaletteConfiguration;

        template<typename T>
        T* findByPath(std::vector<PaletteProfileEditorSection>& sections, std::string_view path, std::vector<T> PaletteProfileEditorSection::* member)
        {
            for (PaletteProfileEditorSection& section : sections)
            {
                auto& fields = section.*member;
                const auto iterator = std::find_if(fields.begin(), fields.end(), [&](const T& field) { return field.Path == path; });
                if (iterator != fields.end()) { return &*iterator; }
            }
            return nullptr;
        }

        template<typename T>
        const T* findByPath(const std::vector<PaletteProfileEditorSection>& sections, std::string_view path, const std::vector<T> PaletteProfileEditorSection::* member)
        {
            for (const PaletteProfileEditorSection& section : sections)
            {
                const auto& fields = section.*member;
                const auto iterator = std::find_if(fields.begin(), fields.end(), [&](const T& field) { return field.Path == path; });
                if (iterator != fields.end()) { return &*iterator; }
            }
            return nullptr;
        }
    }

    ShipPaletteConfigurationEditorBindings::ShipPaletteConfigurationEditorBindings()
    {
        m_Sections.reserve(10u);
        const auto addSection = [&](const char* label, PaletteEditorSectionMode mode) -> PaletteProfileEditorSection&
            {
                m_Sections.push_back({});
                m_Sections.back().Label = label;
                m_Sections.back().Mode = mode;
                return m_Sections.back();
            };

        PaletteProfileEditorSection& source = addSection("PALETTE SOURCE", PaletteEditorSectionMode::ALWAYS);
        PaletteProfileEditorSection& generatedHull = addSection("GENERATED / HULL RANGES", PaletteEditorSectionMode::GENERATED);
        PaletteProfileEditorSection& generatedRoles = addSection("GENERATED / SEMANTIC ROLE RANGES", PaletteEditorSectionMode::GENERATED);
        PaletteProfileEditorSection& generatedBehavior = addSection("GENERATED / RELATIONSHIPS", PaletteEditorSectionMode::GENERATED);
        PaletteProfileEditorSection& fixedGeneral = addSection("FIXED / GENERAL", PaletteEditorSectionMode::FIXED);
        PaletteProfileEditorSection& fixedHull = addSection("FIXED / HULL", PaletteEditorSectionMode::FIXED);
        PaletteProfileEditorSection& fixedCockpit = addSection("FIXED / COCKPIT", PaletteEditorSectionMode::FIXED);
        PaletteProfileEditorSection& fixedEngine = addSection("FIXED / ENGINE / MECHANICAL", PaletteEditorSectionMode::FIXED);
        PaletteProfileEditorSection& fixedExhaustAccent = addSection("FIXED / EXHAUST / ACCENT", PaletteEditorSectionMode::FIXED);
        PaletteProfileEditorSection& fixedLights = addSection("FIXED / LIGHTS", PaletteEditorSectionMode::FIXED);

        PaletteChoiceFieldBinding mode;
        mode.Path = "Mode";
        mode.Control.configure("SOURCE MODE", { "GENERATED", "FIXED" }, 0u);
        mode.Read = [](const Configuration& c) { return c.Mode == SpectralShipGen::ShipPaletteSourceMode::FIXED ? 1u : 0u; };
        mode.Write = [](Configuration& c, uint32_t value) { c.Mode = value == 1u ? SpectralShipGen::ShipPaletteSourceMode::FIXED : SpectralShipGen::ShipPaletteSourceMode::EXPLICIT_GENERATED; };
        source.Choices.push_back(std::move(mode));

        const auto addUIntRange = [](PaletteProfileEditorSection& section, const char* path, const char* label, int32_t minimum, int32_t maximum,
            std::function<const SpectralShipGen::PaletteUIntRange& (const Configuration&)> readRef,
            std::function<SpectralShipGen::PaletteUIntRange& (Configuration&)> writeRef)
            {
                PaletteRangeFieldBinding field;
                field.Path = path;
                field.Control.configure(label, minimum, maximum, 1, 0, 0);
                field.Read = [readRef](const Configuration& c) { const auto& r = readRef(c); return PaletteRangeValue{ static_cast<int32_t>(r.Min), static_cast<int32_t>(r.Max) }; };
                field.Write = [writeRef](Configuration& c, PaletteRangeValue value) { auto& r = writeRef(c); r.Min = static_cast<uint32_t>(value.Min); r.Max = static_cast<uint32_t>(value.Max); };
                section.Ranges.push_back(std::move(field));
            };
        const auto addIntRange = [](PaletteProfileEditorSection& section, const char* path, const char* label, int32_t minimum, int32_t maximum,
            std::function<const SpectralShipGen::PaletteIntRange& (const Configuration&)> readRef,
            std::function<SpectralShipGen::PaletteIntRange& (Configuration&)> writeRef)
            {
                PaletteRangeFieldBinding field;
                field.Path = path;
                field.Control.configure(label, minimum, maximum, 1, 0, 0);
                field.Read = [readRef](const Configuration& c) { const auto& r = readRef(c); return PaletteRangeValue{ r.Min, r.Max }; };
                field.Write = [writeRef](Configuration& c, PaletteRangeValue value) { auto& r = writeRef(c); r.Min = value.Min; r.Max = value.Max; };
                section.Ranges.push_back(std::move(field));
            };
        const auto addInteger = [](PaletteProfileEditorSection& section, const char* path, const char* label, ConfigurationNumericSemantic semantic, int32_t minimum, int32_t maximum,
            std::function<int32_t(const Configuration&)> read, std::function<void(Configuration&, int32_t)> write)
            {
                PaletteIntegerFieldBinding field;
                field.Path = path;
                field.Control.configure(label, semantic, minimum, maximum, 1, 0);
                field.Read = std::move(read);
                field.Write = std::move(write);
                section.Integers.push_back(std::move(field));
            };
        const auto addChoice = [](PaletteProfileEditorSection& section, const char* path, const char* label, std::vector<std::string> options,
            std::function<uint32_t(const Configuration&)> read, std::function<void(Configuration&, uint32_t)> write)
            {
                PaletteChoiceFieldBinding field;
                field.Path = path;
                field.Control.configure(label, std::move(options), 0u);
                field.Read = std::move(read);
                field.Write = std::move(write);
                section.Choices.push_back(std::move(field));
            };
        const auto addColor = [](PaletteProfileEditorSection& section, const char* path, const char* label,
            std::function<SpectralShipGen::Color(const Configuration&)> read, std::function<void(Configuration&, SpectralShipGen::Color)> write)
            {
                PaletteColorFieldBinding field;
                field.Path = path;
                field.Control.configure(label, 0u, 0u, 0u, 255u);
                field.Read = std::move(read);
                field.Write = std::move(write);
                section.Colors.push_back(std::move(field));
            };

#define ADD_URANGE(section, member, label, minv, maxv) addUIntRange(section, "Generated.Ranges." #member, label, minv, maxv, [](const Configuration& c)->const SpectralShipGen::PaletteUIntRange& { return c.Generated.Ranges.member; }, [](Configuration& c)->SpectralShipGen::PaletteUIntRange& { return c.Generated.Ranges.member; })
#define ADD_IRANGE(section, member, label, minv, maxv) addIntRange(section, "Generated.Ranges." #member, label, minv, maxv, [](const Configuration& c)->const SpectralShipGen::PaletteIntRange& { return c.Generated.Ranges.member; }, [](Configuration& c)->SpectralShipGen::PaletteIntRange& { return c.Generated.Ranges.member; })
        ADD_URANGE(generatedHull, HullHue, "HULL HUE", 0, 359);
        ADD_URANGE(generatedHull, HullSaturation, "HULL SATURATION", 0, 100);
        ADD_URANGE(generatedHull, HullValue, "HULL VALUE", 0, 100);
        ADD_IRANGE(generatedRoles, Accent.HueOffset, "ACCENT HUE OFFSET", -360, 360);
        ADD_URANGE(generatedRoles, Accent.Saturation, "ACCENT SATURATION", 0, 100);
        ADD_URANGE(generatedRoles, Accent.Value, "ACCENT VALUE", 0, 100);
        ADD_IRANGE(generatedRoles, Cockpit.HueOffset, "COCKPIT HUE OFFSET", -360, 360);
        ADD_URANGE(generatedRoles, Cockpit.Saturation, "COCKPIT SATURATION", 0, 100);
        ADD_URANGE(generatedRoles, Cockpit.Value, "COCKPIT VALUE", 0, 100);
        ADD_IRANGE(generatedRoles, Light.HueOffset, "LIGHT HUE OFFSET", -360, 360);
        ADD_URANGE(generatedRoles, Light.Saturation, "LIGHT SATURATION", 0, 100);
        ADD_URANGE(generatedRoles, Light.Value, "LIGHT VALUE", 0, 100);
        ADD_IRANGE(generatedRoles, Exhaust.HueOffset, "EXHAUST HUE OFFSET", -360, 360);
        ADD_URANGE(generatedRoles, Exhaust.Saturation, "EXHAUST SATURATION", 0, 100);
        ADD_URANGE(generatedRoles, Exhaust.Value, "EXHAUST VALUE", 0, 100);
        ADD_URANGE(generatedRoles, MechanicalSaturation, "MECHANICAL SATURATION", 0, 100);
        ADD_URANGE(generatedRoles, MechanicalValue, "MECHANICAL VALUE", 0, 100);
#undef ADD_IRANGE
#undef ADD_URANGE

        addChoice(generatedBehavior, "Generated.Behavior.HullValueMode", "HULL VALUE MODE", { "PROFILE RANGE", "ALTERNATING BRIGHT/DARK" },
            [](const Configuration& c) { return static_cast<uint32_t>(c.Generated.Behavior.HullValueMode); },
            [](Configuration& c, uint32_t value) { c.Generated.Behavior.HullValueMode = static_cast<SpectralShipGen::ShipFactionHullValueMode>(value); });
        addUIntRange(generatedBehavior, "Generated.Behavior.BrightHullValue", "BRIGHT HULL VALUE", 0, 100,
            [](const Configuration& c)->const SpectralShipGen::PaletteUIntRange& { return c.Generated.Behavior.BrightHullValue; },
            [](Configuration& c)->SpectralShipGen::PaletteUIntRange& { return c.Generated.Behavior.BrightHullValue; });
        addUIntRange(generatedBehavior, "Generated.Behavior.DarkHullValue", "DARK HULL VALUE", 0, 100,
            [](const Configuration& c)->const SpectralShipGen::PaletteUIntRange& { return c.Generated.Behavior.DarkHullValue; },
            [](Configuration& c)->SpectralShipGen::PaletteUIntRange& { return c.Generated.Behavior.DarkHullValue; });
        addChoice(generatedBehavior, "Generated.Behavior.SecondaryToneDirection", "SECONDARY TONE", { "RANDOM", "DARKER", "LIGHTER", "CONTRAST FROM MIDPOINT" },
            [](const Configuration& c) { return static_cast<uint32_t>(c.Generated.Behavior.SecondaryToneDirection); },
            [](Configuration& c, uint32_t value) { c.Generated.Behavior.SecondaryToneDirection = static_cast<SpectralShipGen::ShipFactionSecondaryToneDirection>(value); });
        addInteger(generatedBehavior, "Generated.Behavior.MinimumAccentHueDistance", "MIN ACCENT HUE DISTANCE", ConfigurationNumericSemantic::COUNT, 0, 180,
            [](const Configuration& c) { return static_cast<int32_t>(c.Generated.Behavior.MinimumAccentHueDistance); },
            [](Configuration& c, int32_t value) { c.Generated.Behavior.MinimumAccentHueDistance = static_cast<uint32_t>(value); });
        addInteger(generatedBehavior, "Generated.Behavior.AccentHueSeparationShiftA", "ACCENT SEPARATION SHIFT A", ConfigurationNumericSemantic::SIGNED_OFFSET, -1000000, 1000000,
            [](const Configuration& c) { return c.Generated.Behavior.AccentHueSeparationShiftA; },
            [](Configuration& c, int32_t value) { c.Generated.Behavior.AccentHueSeparationShiftA = value; });
        addInteger(generatedBehavior, "Generated.Behavior.AccentHueSeparationShiftB", "ACCENT SEPARATION SHIFT B", ConfigurationNumericSemantic::SIGNED_OFFSET, -1000000, 1000000,
            [](const Configuration& c) { return c.Generated.Behavior.AccentHueSeparationShiftB; },
            [](Configuration& c, int32_t value) { c.Generated.Behavior.AccentHueSeparationShiftB = value; });

#define ADD_COLOR(section, member, label) addColor(section, "Fixed." #member, label, [](const Configuration& c) { return c.Fixed.member; }, [](Configuration& c, SpectralShipGen::Color value) { c.Fixed.member = value; })
        ADD_COLOR(fixedGeneral, Transparent, "TRANSPARENT");
        ADD_COLOR(fixedGeneral, Outline, "OUTLINE");
        ADD_COLOR(fixedHull, HullDeepShadow, "HULL DEEP SHADOW");
        ADD_COLOR(fixedHull, HullShadow, "HULL SHADOW");
        ADD_COLOR(fixedHull, HullBase, "HULL BASE");
        ADD_COLOR(fixedHull, HullHighlight, "HULL HIGHLIGHT");
        ADD_COLOR(fixedHull, HullSecondary, "HULL SECONDARY");
        ADD_COLOR(fixedHull, HullEdgeHighlight, "HULL EDGE HIGHLIGHT");
        ADD_COLOR(fixedCockpit, CockpitDark, "COCKPIT DARK");
        ADD_COLOR(fixedCockpit, CockpitBase, "COCKPIT BASE");
        ADD_COLOR(fixedCockpit, CockpitHighlight, "COCKPIT HIGHLIGHT");
        ADD_COLOR(fixedCockpit, CockpitGlint, "COCKPIT GLINT");
        ADD_COLOR(fixedEngine, EngineDark, "ENGINE DARK");
        ADD_COLOR(fixedEngine, EngineBase, "ENGINE BASE");
        ADD_COLOR(fixedEngine, EngineHighlight, "ENGINE HIGHLIGHT");
        ADD_COLOR(fixedEngine, EngineHotCore, "ENGINE HOT CORE");
        ADD_COLOR(fixedEngine, MechanicalDark, "MECHANICAL DARK");
        ADD_COLOR(fixedEngine, MechanicalBase, "MECHANICAL BASE");
        ADD_COLOR(fixedExhaustAccent, ExhaustBase, "EXHAUST BASE");
        ADD_COLOR(fixedExhaustAccent, ExhaustHighlight, "EXHAUST HIGHLIGHT");
        ADD_COLOR(fixedExhaustAccent, ExhaustHotCore, "EXHAUST HOT CORE");
        ADD_COLOR(fixedExhaustAccent, HullAccentDark, "ACCENT DARK");
        ADD_COLOR(fixedExhaustAccent, HullAccent, "ACCENT BASE");
        ADD_COLOR(fixedExhaustAccent, HullAccentHighlight, "ACCENT HIGHLIGHT");
        ADD_COLOR(fixedLights, LightBase, "LIGHT BASE");
        ADD_COLOR(fixedLights, LightHighlight, "LIGHT HIGHLIGHT");
#undef ADD_COLOR
    }

    void ShipPaletteConfigurationEditorBindings::load(const Configuration& configuration)
    {
        for (PaletteProfileEditorSection& section : m_Sections)
        {
            for (PaletteIntegerFieldBinding& field : section.Integers) { field.Control.setValue(field.Read(configuration)); }
            for (PaletteRangeFieldBinding& field : section.Ranges) { const PaletteRangeValue value = field.Read(configuration); field.Control.setValues(value.Min, value.Max); }
            for (PaletteChoiceFieldBinding& field : section.Choices) { field.Control.setValue(field.Read(configuration)); }
            for (PaletteColorFieldBinding& field : section.Colors)
            {
                const SpectralShipGen::Color color = field.Read(configuration);
                field.Control.setValues(color.R, color.G, color.B, color.A);
            }
        }
    }

    void ShipPaletteConfigurationEditorBindings::write(Configuration& configuration) const
    {
        for (const PaletteProfileEditorSection& section : m_Sections)
        {
            for (const PaletteIntegerFieldBinding& field : section.Integers) { field.Write(configuration, field.Control.Value); }
            for (const PaletteRangeFieldBinding& field : section.Ranges) { field.Write(configuration, { field.Control.MinimumValue, field.Control.MaximumValue }); }
            for (const PaletteChoiceFieldBinding& field : section.Choices) { field.Write(configuration, field.Control.Value); }
            for (const PaletteColorFieldBinding& field : section.Colors)
            {
                field.Write(configuration, SpectralShipGen::Color(static_cast<uint8_t>(field.Control.Red), static_cast<uint8_t>(field.Control.Green), static_cast<uint8_t>(field.Control.Blue), static_cast<uint8_t>(field.Control.Alpha)));
            }
        }
    }

    SpectralShipGen::ShipPaletteSourceMode ShipPaletteConfigurationEditorBindings::getEditedMode() const
    {
        const PaletteChoiceFieldBinding* mode = findChoice("Mode");
        return mode != nullptr && mode->Control.Value == 1u ? SpectralShipGen::ShipPaletteSourceMode::FIXED : SpectralShipGen::ShipPaletteSourceMode::EXPLICIT_GENERATED;
    }

    bool ShipPaletteConfigurationEditorBindings::isSectionVisible(const PaletteProfileEditorSection& section) const
    {
        if (section.Mode == PaletteEditorSectionMode::ALWAYS) { return true; }
        const SpectralShipGen::ShipPaletteSourceMode mode = getEditedMode();
        return section.Mode == PaletteEditorSectionMode::GENERATED ? mode == SpectralShipGen::ShipPaletteSourceMode::EXPLICIT_GENERATED : mode == SpectralShipGen::ShipPaletteSourceMode::FIXED;
    }

    bool ShipPaletteConfigurationEditorBindings::equivalent(const Configuration& first, const Configuration& second) const
    {
        const auto normalizeMode = [](SpectralShipGen::ShipPaletteSourceMode mode)
            {
                return mode == SpectralShipGen::ShipPaletteSourceMode::FIXED ? SpectralShipGen::ShipPaletteSourceMode::FIXED : SpectralShipGen::ShipPaletteSourceMode::EXPLICIT_GENERATED;
            };
        if (normalizeMode(first.Mode) != normalizeMode(second.Mode)) { return false; }
        const SpectralShipGen::ShipPaletteSourceMode mode = normalizeMode(first.Mode);
        for (const PaletteProfileEditorSection& section : m_Sections)
        {
            if (section.Mode == PaletteEditorSectionMode::GENERATED && mode != SpectralShipGen::ShipPaletteSourceMode::EXPLICIT_GENERATED) { continue; }
            if (section.Mode == PaletteEditorSectionMode::FIXED && mode != SpectralShipGen::ShipPaletteSourceMode::FIXED) { continue; }
            for (const PaletteIntegerFieldBinding& field : section.Integers) { if (field.Read(first) != field.Read(second)) { return false; } }
            for (const PaletteRangeFieldBinding& field : section.Ranges) { if (field.Read(first) != field.Read(second)) { return false; } }
            for (const PaletteChoiceFieldBinding& field : section.Choices) { if (field.Path != "Mode" && field.Read(first) != field.Read(second)) { return false; } }
            for (const PaletteColorFieldBinding& field : section.Colors) { if (field.Read(first) != field.Read(second)) { return false; } }
        }
        return true;
    }

    std::vector<PaletteProfileEditorSection>& ShipPaletteConfigurationEditorBindings::getSections() { return m_Sections; }
    const std::vector<PaletteProfileEditorSection>& ShipPaletteConfigurationEditorBindings::getSections() const { return m_Sections; }
    PaletteIntegerFieldBinding* ShipPaletteConfigurationEditorBindings::findInteger(std::string_view path) { return findByPath(m_Sections, path, &PaletteProfileEditorSection::Integers); }
    PaletteRangeFieldBinding* ShipPaletteConfigurationEditorBindings::findRange(std::string_view path) { return findByPath(m_Sections, path, &PaletteProfileEditorSection::Ranges); }
    PaletteChoiceFieldBinding* ShipPaletteConfigurationEditorBindings::findChoice(std::string_view path) { return findByPath(m_Sections, path, &PaletteProfileEditorSection::Choices); }
    PaletteColorFieldBinding* ShipPaletteConfigurationEditorBindings::findColor(std::string_view path) { return findByPath(m_Sections, path, &PaletteProfileEditorSection::Colors); }
    const PaletteIntegerFieldBinding* ShipPaletteConfigurationEditorBindings::findInteger(std::string_view path) const { return findByPath(m_Sections, path, &PaletteProfileEditorSection::Integers); }
    const PaletteRangeFieldBinding* ShipPaletteConfigurationEditorBindings::findRange(std::string_view path) const { return findByPath(m_Sections, path, &PaletteProfileEditorSection::Ranges); }
    const PaletteChoiceFieldBinding* ShipPaletteConfigurationEditorBindings::findChoice(std::string_view path) const { return findByPath(m_Sections, path, &PaletteProfileEditorSection::Choices); }
    const PaletteColorFieldBinding* ShipPaletteConfigurationEditorBindings::findColor(std::string_view path) const { return findByPath(m_Sections, path, &PaletteProfileEditorSection::Colors); }

    std::size_t ShipPaletteConfigurationEditorBindings::getBoundValueCount() const
    {
        std::size_t total = 0u;
        for (const PaletteProfileEditorSection& section : m_Sections)
        {
            total += section.Integers.size() + section.Choices.size() + section.Ranges.size() * 2u + section.Colors.size() * 4u;
        }
        return total;
    }
}
