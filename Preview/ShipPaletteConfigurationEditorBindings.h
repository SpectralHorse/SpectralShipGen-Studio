#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <PixelShipGenerator/ShipPaletteConfiguration.h>

#include "ConfigurationEditorControls.h"

namespace PixelShipGeneratorPreview
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
        std::function<int32_t(const PixelShipGenerator::ShipPaletteConfiguration&)> Read;
        std::function<void(PixelShipGenerator::ShipPaletteConfiguration&, int32_t)> Write;
    };

    struct PaletteRangeFieldBinding
    {
        std::string Path;
        ConfigurationRangeControl Control;
        std::function<PaletteRangeValue(const PixelShipGenerator::ShipPaletteConfiguration&)> Read;
        std::function<void(PixelShipGenerator::ShipPaletteConfiguration&, PaletteRangeValue)> Write;
    };

    struct PaletteChoiceFieldBinding
    {
        std::string Path;
        ConfigurationChoiceControl Control;
        std::function<uint32_t(const PixelShipGenerator::ShipPaletteConfiguration&)> Read;
        std::function<void(PixelShipGenerator::ShipPaletteConfiguration&, uint32_t)> Write;
    };

    struct PaletteColorFieldBinding
    {
        std::string Path;
        ConfigurationColorControl Control;
        std::function<PixelShipGenerator::Color(const PixelShipGenerator::ShipPaletteConfiguration&)> Read;
        std::function<void(PixelShipGenerator::ShipPaletteConfiguration&, PixelShipGenerator::Color)> Write;
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

        void load(const PixelShipGenerator::ShipPaletteConfiguration& configuration);
        void write(PixelShipGenerator::ShipPaletteConfiguration& configuration) const;
        bool equivalent(const PixelShipGenerator::ShipPaletteConfiguration& first, const PixelShipGenerator::ShipPaletteConfiguration& second) const;
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
        PixelShipGenerator::ShipPaletteSourceMode getEditedMode() const;

    private:
        std::vector<PaletteProfileEditorSection> m_Sections;
    };
}
