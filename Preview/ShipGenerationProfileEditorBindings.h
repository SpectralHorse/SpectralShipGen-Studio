#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <SpectralShipGen/ShipGenerationProfile.h>

#include "ConfigurationEditorControls.h"

namespace SpectralShipGenStudioPreview
{
    struct StructuralIntegerFieldBinding
    {
        std::string Path;
        ConfigurationIntegerControl Control;
        std::function<int32_t(const SpectralShipGen::ShipGenerationProfile&)> Read;
        std::function<void(SpectralShipGen::ShipGenerationProfile&, int32_t)> Write;
    };

    struct StructuralRangeFieldBinding
    {
        std::string Path;
        ConfigurationRangeControl Control;
        std::function<SpectralShipGen::UIntRange(const SpectralShipGen::ShipGenerationProfile&)> Read;
        std::function<void(SpectralShipGen::ShipGenerationProfile&, SpectralShipGen::UIntRange)> Write;
    };

    struct StructuralToggleFieldBinding
    {
        std::string Path;
        ConfigurationToggleControl Control;
        std::function<bool(const SpectralShipGen::ShipGenerationProfile&)> Read;
        std::function<void(SpectralShipGen::ShipGenerationProfile&, bool)> Write;
    };

    struct StructuralChoiceFieldBinding
    {
        std::string Path;
        ConfigurationChoiceControl Control;
        std::function<uint32_t(const SpectralShipGen::ShipGenerationProfile&)> Read;
        std::function<void(SpectralShipGen::ShipGenerationProfile&, uint32_t)> Write;
    };

    struct StructuralWeightGroupBinding
    {
        std::string Path;
        ConfigurationWeightGroupControl Control;
        std::function<std::array<uint32_t, ConfigurationWeightGroupControl::MaximumRows>(const SpectralShipGen::ShipGenerationProfile&)> Read;
        std::function<void(SpectralShipGen::ShipGenerationProfile&, const std::array<uint32_t, ConfigurationWeightGroupControl::MaximumRows>&)> Write;
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

        void load(const SpectralShipGen::ShipGenerationProfile& profile);
        void write(SpectralShipGen::ShipGenerationProfile& profile) const;
        bool equivalent(const SpectralShipGen::ShipGenerationProfile& first, const SpectralShipGen::ShipGenerationProfile& second) const;

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
