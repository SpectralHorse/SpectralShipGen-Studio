#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <SpectralShipGen/ShipPaletteConfiguration.h>

#include "ConfigurationEditorControls.h"

namespace SpectralShipGenStudioPreview
{
    struct PaletteRangeValue
    {
        int32_t Min = 0;
        int32_t Max = 0;
        bool operator==(const PaletteRangeValue& other) const { return Min == other.Min && Max == other.Max; }
        bool operator!=(const PaletteRangeValue& other) const { return !(*this == other); }
    };

    enum class PaletteEditorSectionMode : uint32_t
    {
        ALWAYS = 0u,
        GENERATED,
        FIXED
    };

    struct PaletteIntegerFieldBinding
    {
        std::string Path;
        ConfigurationIntegerControl Control;
        std::function<int32_t(const SpectralShipGen::ShipPaletteConfiguration&)> Read;
        std::function<void(SpectralShipGen::ShipPaletteConfiguration&, int32_t)> Write;
    };

    struct PaletteRangeFieldBinding
    {
        std::string Path;
        ConfigurationRangeControl Control;
        std::function<PaletteRangeValue(const SpectralShipGen::ShipPaletteConfiguration&)> Read;
        std::function<void(SpectralShipGen::ShipPaletteConfiguration&, PaletteRangeValue)> Write;
    };

    struct PaletteChoiceFieldBinding
    {
        std::string Path;
        ConfigurationChoiceControl Control;
        std::function<uint32_t(const SpectralShipGen::ShipPaletteConfiguration&)> Read;
        std::function<void(SpectralShipGen::ShipPaletteConfiguration&, uint32_t)> Write;
    };

    struct PaletteColorFieldBinding
    {
        std::string Path;
        ConfigurationColorControl Control;
        std::function<SpectralShipGen::Color(const SpectralShipGen::ShipPaletteConfiguration&)> Read;
        std::function<void(SpectralShipGen::ShipPaletteConfiguration&, SpectralShipGen::Color)> Write;
    };

    struct PaletteProfileEditorSection
    {
        std::string Label;
        ConfigurationEditorRect HeaderBounds;
        bool Expanded = false;
        PaletteEditorSectionMode Mode = PaletteEditorSectionMode::ALWAYS;
        std::vector<PaletteIntegerFieldBinding> Integers;
        std::vector<PaletteRangeFieldBinding> Ranges;
        std::vector<PaletteChoiceFieldBinding> Choices;
        std::vector<PaletteColorFieldBinding> Colors;
    };

    class ShipPaletteConfigurationEditorBindings
    {
    public:
        ShipPaletteConfigurationEditorBindings();

        void load(const SpectralShipGen::ShipPaletteConfiguration& configuration);
        void write(SpectralShipGen::ShipPaletteConfiguration& configuration) const;
        bool equivalent(const SpectralShipGen::ShipPaletteConfiguration& first, const SpectralShipGen::ShipPaletteConfiguration& second) const;
        bool isSectionVisible(const PaletteProfileEditorSection& section) const;

        std::vector<PaletteProfileEditorSection>& getSections();
        const std::vector<PaletteProfileEditorSection>& getSections() const;

        PaletteIntegerFieldBinding* findInteger(std::string_view path);
        PaletteRangeFieldBinding* findRange(std::string_view path);
        PaletteChoiceFieldBinding* findChoice(std::string_view path);
        PaletteColorFieldBinding* findColor(std::string_view path);
        const PaletteIntegerFieldBinding* findInteger(std::string_view path) const;
        const PaletteRangeFieldBinding* findRange(std::string_view path) const;
        const PaletteChoiceFieldBinding* findChoice(std::string_view path) const;
        const PaletteColorFieldBinding* findColor(std::string_view path) const;

        std::size_t getBoundValueCount() const;

    private:
        SpectralShipGen::ShipPaletteSourceMode getEditedMode() const;

    private:
        std::vector<PaletteProfileEditorSection> m_Sections;
    };
}
