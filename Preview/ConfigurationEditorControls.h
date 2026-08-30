#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace PixelShipGeneratorPreview
{
    struct ConfigurationEditorRect
    {
        float Left = 0.0f;
        float Top = 0.0f;
        float Width = 0.0f;
        float Height = 0.0f;

        bool contains(float x, float y) const
        {
            return x >= Left && y >= Top && x <= Left + Width && y <= Top + Height;
        }
    };

    enum class ConfigurationNumericSemantic : uint32_t
    {
        COUNT = 0u,
        PROBABILITY,
        RELATIVE_WEIGHT,
        MULTIPLIER_PERCENT,
        SIGNED_OFFSET
    };

    struct ConfigurationIntegerControl
    {
        std::string Label;
        ConfigurationNumericSemantic Semantic = ConfigurationNumericSemantic::COUNT;
        int32_t Minimum = 0;
        int32_t Maximum = 100;
        int32_t Step = 1;
        int32_t Value = 0;
        ConfigurationEditorRect RowBounds;
        ConfigurationEditorRect DecrementBounds;
        ConfigurationEditorRect IncrementBounds;
        ConfigurationEditorRect TrackBounds;
        bool Dragging = false;

        void configure(std::string label, ConfigurationNumericSemantic semantic, int32_t minimum, int32_t maximum, int32_t step, int32_t value);
        void setRowBounds(const ConfigurationEditorRect& bounds);
        void setValue(int32_t value);
        void decrement();
        void increment();
        int32_t valueForTrackPosition(float x) const;
        bool beginPointer(float x, float y);
        bool updatePointer(float x);
        bool endPointer(float x, float y);
        std::string getDisplayValue() const;
    };

    struct ConfigurationRangeControl
    {
        std::string Label;
        int32_t MinimumLimit = 0;
        int32_t MaximumLimit = 100;
        int32_t Step = 1;
        int32_t MinimumValue = 0;
        int32_t MaximumValue = 100;
        ConfigurationEditorRect RowBounds;
        ConfigurationEditorRect MinimumDecrementBounds;
        ConfigurationEditorRect MinimumIncrementBounds;
        ConfigurationEditorRect MaximumDecrementBounds;
        ConfigurationEditorRect MaximumIncrementBounds;

        void configure(std::string label, int32_t minimumLimit, int32_t maximumLimit, int32_t step, int32_t minimumValue, int32_t maximumValue);
        void setRowBounds(const ConfigurationEditorRect& bounds);
        void setValues(int32_t minimumValue, int32_t maximumValue);
        bool activate(float x, float y);
    };

    struct ConfigurationWeightRow
    {
        ConfigurationIntegerControl Control;
        uint32_t ProbabilityPercent = 0u;
    };

    class ConfigurationWeightGroupControl
    {
    public:
        static constexpr std::size_t MaximumRows = 10u;

        void configure(std::string label, const std::array<std::string, MaximumRows>& labels, const std::array<uint32_t, MaximumRows>& weights, std::size_t rowCount, uint32_t maximumWeight = 500u);
        void setBounds(const ConfigurationEditorRect& bounds);
        bool beginPointer(float x, float y);
        bool updatePointer(float x);
        bool endPointer(float x, float y);
        void refreshProbabilities();

        const std::string& getLabel() const;
        const std::array<ConfigurationWeightRow, MaximumRows>& getRows() const;
        std::array<ConfigurationWeightRow, MaximumRows>& getRows();
        std::size_t getRowCount() const;
        ConfigurationEditorRect getBounds() const;

    private:
        std::string m_Label;
        std::array<ConfigurationWeightRow, MaximumRows> m_Rows = {};
        std::size_t m_RowCount = 0u;
        uint32_t m_MaximumWeight = 500u;
        ConfigurationEditorRect m_Bounds;
    };


    struct ConfigurationToggleControl
    {
        std::string Label;
        bool Value = false;
        ConfigurationEditorRect RowBounds;
        ConfigurationEditorRect ToggleBounds;

        void configure(std::string label, bool value);
        void setRowBounds(const ConfigurationEditorRect& bounds);
        bool activate(float x, float y);
        std::string getDisplayValue() const;
    };

    struct ConfigurationChoiceControl
    {
        std::string Label;
        std::vector<std::string> Options;
        uint32_t Value = 0u;
        ConfigurationEditorRect RowBounds;
        ConfigurationEditorRect PreviousBounds;
        ConfigurationEditorRect NextBounds;

        void configure(std::string label, std::vector<std::string> options, uint32_t value);
        void setRowBounds(const ConfigurationEditorRect& bounds);
        void setValue(uint32_t value);
        bool activate(float x, float y);
        std::string getDisplayValue() const;
    };

    struct ConfigurationColorControl
    {
        std::string Label;
        uint32_t Red = 0u;
        uint32_t Green = 0u;
        uint32_t Blue = 0u;
        uint32_t Alpha = 255u;
        ConfigurationEditorRect RowBounds;
        std::array<ConfigurationEditorRect, 4u> TrackBounds = {};
        ConfigurationEditorRect SwatchBounds;
        int32_t DraggingChannel = -1;

        void configure(std::string label, uint32_t red, uint32_t green, uint32_t blue, uint32_t alpha = 255u);
        void setRowBounds(const ConfigurationEditorRect& bounds);
        void setValues(uint32_t red, uint32_t green, uint32_t blue, uint32_t alpha);
        uint32_t getChannel(std::size_t channel) const;
        void setChannel(std::size_t channel, uint32_t value);
        uint32_t valueForTrackPosition(std::size_t channel, float x) const;
        bool beginPointer(float x, float y);
        bool updatePointer(float x);
        bool endPointer(float x, float y);
        std::string getDisplayValue() const;
    };

    struct ConfigurationTextField
    {
        std::string Label;
        std::string Value;
        std::size_t MaximumCharacters = 40u;
        ConfigurationEditorRect Bounds;
        bool Focused = false;

        bool activate(float x, float y);
        bool onTextEntered(uint32_t unicode);
    };
}
