#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <PixelShipGenerator/ShipGenerationProfile.h>

#include "ConfigurationEditorControls.h"

namespace PixelShipGeneratorPreview
{
    struct StructuralIntegerFieldBinding
    {
        std::string Path;
        ConfigurationIntegerControl Control;
        std::function<int32_t(const PixelShipGenerator::ShipGenerationProfile&)> Read;
        std::function<void(PixelShipGenerator::ShipGenerationProfile&, int32_t)> Write;
    };

    struct StructuralRangeFieldBinding
    {
        std::string Path;
        ConfigurationRangeControl Control;
        std::function<PixelShipGenerator::UIntRange(const PixelShipGenerator::ShipGenerationProfile&)> Read;
        std::function<void(PixelShipGenerator::ShipGenerationProfile&, PixelShipGenerator::UIntRange)> Write;
    };

    struct StructuralToggleFieldBinding
    {
        std::string Path;
        ConfigurationToggleControl Control;
        std::function<bool(const PixelShipGenerator::ShipGenerationProfile&)> Read;
        std::function<void(PixelShipGenerator::ShipGenerationProfile&, bool)> Write;
    };

    struct StructuralChoiceFieldBinding
    {
        std::string Path;
        ConfigurationChoiceControl Control;
        std::function<uint32_t(const PixelShipGenerator::ShipGenerationProfile&)> Read;
        std::function<void(PixelShipGenerator::ShipGenerationProfile&, uint32_t)> Write;
    };

    struct StructuralWeightGroupBinding
    {
        std::string Path;
        ConfigurationWeightGroupControl Control;
        std::function<std::array<uint32_t, ConfigurationWeightGroupControl::MaximumRows>(const PixelShipGenerator::ShipGenerationProfile&)> Read;
        std::function<void(PixelShipGenerator::ShipGenerationProfile&, const std::array<uint32_t, ConfigurationWeightGroupControl::MaximumRows>&)> Write;
    };

    struct StructuralProfileEditorSection
    {
        std::string Label;
        ConfigurationEditorRect HeaderBounds;
        bool Expanded = false;
        std::vector<StructuralIntegerFieldBinding> Integers;
        std::vector<StructuralRangeFieldBinding> Ranges;
        std::vector<StructuralToggleFieldBinding> Toggles;
        std::vector<StructuralChoiceFieldBinding> Choices;
        std::vector<StructuralWeightGroupBinding> WeightGroups;
    };

    // Application-side field registry for the public ShipGenerationProfile.
    // It deliberately mirrors the semantic public model instead of introducing
    // a Preview-only shadow configuration type.
    class ShipGenerationProfileEditorBindings
    {
    public:
        ShipGenerationProfileEditorBindings();

        void load(const PixelShipGenerator::ShipGenerationProfile& profile);
        void write(PixelShipGenerator::ShipGenerationProfile& profile) const;
        bool equivalent(const PixelShipGenerator::ShipGenerationProfile& first, const PixelShipGenerator::ShipGenerationProfile& second) const;

        std::vector<StructuralProfileEditorSection>& getSections();
        const std::vector<StructuralProfileEditorSection>& getSections() const;

        StructuralIntegerFieldBinding* findInteger(std::string_view path);
        StructuralRangeFieldBinding* findRange(std::string_view path);
        StructuralToggleFieldBinding* findToggle(std::string_view path);
        StructuralChoiceFieldBinding* findChoice(std::string_view path);
        StructuralWeightGroupBinding* findWeightGroup(std::string_view path);
        const StructuralIntegerFieldBinding* findInteger(std::string_view path) const;
        const StructuralRangeFieldBinding* findRange(std::string_view path) const;
        const StructuralToggleFieldBinding* findToggle(std::string_view path) const;
        const StructuralChoiceFieldBinding* findChoice(std::string_view path) const;
        const StructuralWeightGroupBinding* findWeightGroup(std::string_view path) const;

        std::size_t getBoundValueCount() const;

    private:
        std::vector<StructuralProfileEditorSection> m_Sections;
    };
}
