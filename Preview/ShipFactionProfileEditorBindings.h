#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <SpectralShipGen/ShipFactionProfile.h>

#include "ConfigurationEditorControls.h"

namespace SpectralShipGenStudioPreview
{
    struct FactionRangeValue
    {
        int32_t Min = 0;
        int32_t Max = 0;

        bool operator==(const FactionRangeValue& other) const { return Min == other.Min && Max == other.Max; }
        bool operator!=(const FactionRangeValue& other) const { return !(*this == other); }
    };

    struct FactionIntegerFieldBinding
    {
        std::string Path;
        ConfigurationIntegerControl Control;
        std::function<int32_t(const SpectralShipGen::ShipFactionProfile&)> Read;
        std::function<void(SpectralShipGen::ShipFactionProfile&, int32_t)> Write;
    };

    struct FactionRangeFieldBinding
    {
        std::string Path;
        ConfigurationRangeControl Control;
        std::function<FactionRangeValue(const SpectralShipGen::ShipFactionProfile&)> Read;
        std::function<void(SpectralShipGen::ShipFactionProfile&, FactionRangeValue)> Write;
    };

    struct FactionToggleFieldBinding
    {
        std::string Path;
        ConfigurationToggleControl Control;
        std::function<bool(const SpectralShipGen::ShipFactionProfile&)> Read;
        std::function<void(SpectralShipGen::ShipFactionProfile&, bool)> Write;
    };

    struct FactionChoiceFieldBinding
    {
        std::string Path;
        ConfigurationChoiceControl Control;
        std::function<uint32_t(const SpectralShipGen::ShipFactionProfile&)> Read;
        std::function<void(SpectralShipGen::ShipFactionProfile&, uint32_t)> Write;
    };

    struct FactionWeightGroupBinding
    {
        std::string Path;
        ConfigurationWeightGroupControl Control;
        std::function<std::array<uint32_t, ConfigurationWeightGroupControl::MaximumRows>(const SpectralShipGen::ShipFactionProfile&)> Read;
        std::function<void(SpectralShipGen::ShipFactionProfile&, const std::array<uint32_t, ConfigurationWeightGroupControl::MaximumRows>&)> Write;
    };

    struct FactionProfileEditorSection
    {
        std::string Label;
        ConfigurationEditorRect HeaderBounds;
        bool Expanded = false;
        std::vector<FactionIntegerFieldBinding> Integers;
        std::vector<FactionRangeFieldBinding> Ranges;
        std::vector<FactionToggleFieldBinding> Toggles;
        std::vector<FactionChoiceFieldBinding> Choices;
        std::vector<FactionWeightGroupBinding> WeightGroups;
    };

    // Application-side field registry for the unified public ShipFactionProfile.
    // Values are edited directly on a working copy of the public Core profile.
    class ShipFactionProfileEditorBindings
    {
    public:
        ShipFactionProfileEditorBindings();

        void load(const SpectralShipGen::ShipFactionProfile& profile);
        void write(SpectralShipGen::ShipFactionProfile& profile) const;
        bool equivalent(const SpectralShipGen::ShipFactionProfile& first, const SpectralShipGen::ShipFactionProfile& second) const;

        std::vector<FactionProfileEditorSection>& getSections();
        const std::vector<FactionProfileEditorSection>& getSections() const;

        FactionIntegerFieldBinding* findInteger(std::string_view path);
        FactionRangeFieldBinding* findRange(std::string_view path);
        FactionToggleFieldBinding* findToggle(std::string_view path);
        FactionChoiceFieldBinding* findChoice(std::string_view path);
        FactionWeightGroupBinding* findWeightGroup(std::string_view path);
        const FactionIntegerFieldBinding* findInteger(std::string_view path) const;
        const FactionRangeFieldBinding* findRange(std::string_view path) const;
        const FactionToggleFieldBinding* findToggle(std::string_view path) const;
        const FactionChoiceFieldBinding* findChoice(std::string_view path) const;
        const FactionWeightGroupBinding* findWeightGroup(std::string_view path) const;

        std::size_t getBoundValueCount() const;

    private:
        std::vector<FactionProfileEditorSection> m_Sections;
    };
}
