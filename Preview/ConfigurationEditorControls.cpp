#include "ConfigurationEditorControls.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace PixelShipGeneratorPreview
{
    namespace
    {
        int32_t clampSteppedValue(int32_t value, int32_t minimum, int32_t maximum, int32_t step)
        {
            const int32_t safeStep = std::max(1, step);
            const int64_t clamped = std::clamp<int64_t>(value, minimum, maximum);
            const int64_t relative = clamped - minimum;
            const int64_t stepped = minimum + ((relative + safeStep / 2) / safeStep) * safeStep;
            return static_cast<int32_t>(std::clamp<int64_t>(stepped, minimum, maximum));
        }
    }

    void ConfigurationIntegerControl::configure(std::string label, ConfigurationNumericSemantic semantic, int32_t minimum, int32_t maximum, int32_t step, int32_t value)
    {
        Label = std::move(label);
        Semantic = semantic;
        Minimum = minimum;
        Maximum = std::max(minimum, maximum);
        Step = std::max(1, step);
        setValue(value);
    }

    void ConfigurationIntegerControl::setRowBounds(const ConfigurationEditorRect& bounds)
    {
        RowBounds = bounds;
        constexpr float ButtonWidth = 26.0f;
        constexpr float ButtonGap = 4.0f;
        constexpr float ValueWidth = 74.0f;
        constexpr float MinimumTrackWidth = 80.0f;
        constexpr float LabelAreaWidth = 250.0f;
        const float right = bounds.Left + bounds.Width;
        IncrementBounds = { right - ButtonWidth, bounds.Top, ButtonWidth, bounds.Height };
        DecrementBounds = { IncrementBounds.Left - ButtonGap - ButtonWidth, bounds.Top, ButtonWidth, bounds.Height };
        const float valueLeft = DecrementBounds.Left - ButtonGap - ValueWidth;
        const float trackLeft = bounds.Left + LabelAreaWidth;
        TrackBounds = { trackLeft, bounds.Top + bounds.Height * 0.5f - 3.0f, std::max(MinimumTrackWidth, valueLeft - ButtonGap - trackLeft), 6.0f };
    }

    void ConfigurationIntegerControl::setValue(int32_t value)
    {
        // Loading an existing Core configuration must preserve any valid value exactly.
        // Step controls interaction increments and track snapping; it is not a storage grid.
        Value = std::clamp(value, Minimum, Maximum);
    }

    void ConfigurationIntegerControl::decrement() { setValue(Value - Step); }
    void ConfigurationIntegerControl::increment() { setValue(Value + Step); }

    int32_t ConfigurationIntegerControl::valueForTrackPosition(float x) const
    {
        const float width = std::max(1.0f, TrackBounds.Width);
        const float normalized = std::clamp((x - TrackBounds.Left) / width, 0.0f, 1.0f);
        const double raw = static_cast<double>(Minimum) + static_cast<double>(Maximum - Minimum) * normalized;
        return clampSteppedValue(static_cast<int32_t>(std::llround(raw)), Minimum, Maximum, Step);
    }

    bool ConfigurationIntegerControl::beginPointer(float x, float y)
    {
        if (TrackBounds.contains(x, y))
        {
            Dragging = true;
            setValue(valueForTrackPosition(x));
            return true;
        }
        return DecrementBounds.contains(x, y) || IncrementBounds.contains(x, y);
    }

    bool ConfigurationIntegerControl::updatePointer(float x)
    {
        if (!Dragging) { return false; }
        setValue(valueForTrackPosition(x));
        return true;
    }

    bool ConfigurationIntegerControl::endPointer(float x, float y)
    {
        bool changed = false;
        if (Dragging)
        {
            setValue(valueForTrackPosition(x));
            changed = true;
        }
        else if (DecrementBounds.contains(x, y))
        {
            decrement();
            changed = true;
        }
        else if (IncrementBounds.contains(x, y))
        {
            increment();
            changed = true;
        }
        Dragging = false;
        return changed;
    }

    std::string ConfigurationIntegerControl::getDisplayValue() const
    {
        switch (Semantic)
        {
        case ConfigurationNumericSemantic::PROBABILITY: return std::to_string(Value) + "%";
        case ConfigurationNumericSemantic::MULTIPLIER_PERCENT: return std::to_string(Value) + "% x";
        case ConfigurationNumericSemantic::SIGNED_OFFSET: return Value > 0 ? "+" + std::to_string(Value) : std::to_string(Value);
        default: return std::to_string(Value);
        }
    }

    void ConfigurationRangeControl::configure(std::string label, int32_t minimumLimit, int32_t maximumLimit, int32_t step, int32_t minimumValue, int32_t maximumValue)
    {
        Label = std::move(label);
        MinimumLimit = minimumLimit;
        MaximumLimit = std::max(minimumLimit, maximumLimit);
        Step = std::max(1, step);
        setValues(minimumValue, maximumValue);
    }

    void ConfigurationRangeControl::setRowBounds(const ConfigurationEditorRect& bounds)
    {
        RowBounds = bounds;
        constexpr float ButtonWidth = 24.0f;
        constexpr float Gap = 4.0f;
        const float right = bounds.Left + bounds.Width;
        MaximumIncrementBounds = { right - ButtonWidth, bounds.Top, ButtonWidth, bounds.Height };
        MaximumDecrementBounds = { MaximumIncrementBounds.Left - Gap - ButtonWidth, bounds.Top, ButtonWidth, bounds.Height };
        constexpr float RangeGroupSeparation = 104.0f;
        MinimumIncrementBounds = { MaximumDecrementBounds.Left - RangeGroupSeparation - Gap - ButtonWidth, bounds.Top, ButtonWidth, bounds.Height };
        MinimumDecrementBounds = { MinimumIncrementBounds.Left - Gap - ButtonWidth, bounds.Top, ButtonWidth, bounds.Height };
    }

    void ConfigurationRangeControl::setValues(int32_t minimumValue, int32_t maximumValue)
    {
        MinimumValue = clampSteppedValue(minimumValue, MinimumLimit, MaximumLimit, Step);
        MaximumValue = clampSteppedValue(maximumValue, MinimumLimit, MaximumLimit, Step);
        if (MinimumValue > MaximumValue) { MaximumValue = MinimumValue; }
    }

    bool ConfigurationRangeControl::activate(float x, float y)
    {
        if (MinimumDecrementBounds.contains(x, y))
        {
            MinimumValue = std::max(MinimumLimit, MinimumValue - Step);
            return true;
        }
        if (MinimumIncrementBounds.contains(x, y))
        {
            MinimumValue = std::min(MaximumValue, MinimumValue + Step);
            return true;
        }
        if (MaximumDecrementBounds.contains(x, y))
        {
            MaximumValue = std::max(MinimumValue, MaximumValue - Step);
            return true;
        }
        if (MaximumIncrementBounds.contains(x, y))
        {
            MaximumValue = std::min(MaximumLimit, MaximumValue + Step);
            return true;
        }
        return false;
    }

    void ConfigurationWeightGroupControl::configure(std::string label, const std::array<std::string, MaximumRows>& labels, const std::array<uint32_t, MaximumRows>& weights, std::size_t rowCount, uint32_t maximumWeight)
    {
        m_Label = std::move(label);
        m_RowCount = std::min(rowCount, MaximumRows);
        m_MaximumWeight = std::max(1u, maximumWeight);
        for (std::size_t index = 0u; index < m_RowCount; ++index)
        {
            m_Rows[index].Control.configure(labels[index], ConfigurationNumericSemantic::RELATIVE_WEIGHT, 0, static_cast<int32_t>(m_MaximumWeight), 1, static_cast<int32_t>(weights[index]));
        }
        refreshProbabilities();
    }

    void ConfigurationWeightGroupControl::setBounds(const ConfigurationEditorRect& bounds)
    {
        m_Bounds = bounds;
        constexpr float HeaderHeight = 28.0f;
        constexpr float RowHeight = 38.0f;
        for (std::size_t index = 0u; index < m_RowCount; ++index)
        {
            m_Rows[index].Control.setRowBounds({ bounds.Left, bounds.Top + HeaderHeight + static_cast<float>(index) * RowHeight, bounds.Width, RowHeight - 2.0f });
        }
    }

    bool ConfigurationWeightGroupControl::beginPointer(float x, float y)
    {
        for (std::size_t index = 0u; index < m_RowCount; ++index)
        {
            if (m_Rows[index].Control.beginPointer(x, y)) { return true; }
        }
        return false;
    }

    bool ConfigurationWeightGroupControl::updatePointer(float x)
    {
        bool changed = false;
        for (std::size_t index = 0u; index < m_RowCount; ++index) { changed = m_Rows[index].Control.updatePointer(x) || changed; }
        if (changed) { refreshProbabilities(); }
        return changed;
    }

    bool ConfigurationWeightGroupControl::endPointer(float x, float y)
    {
        bool changed = false;
        for (std::size_t index = 0u; index < m_RowCount; ++index) { changed = m_Rows[index].Control.endPointer(x, y) || changed; }
        if (changed) { refreshProbabilities(); }
        return changed;
    }

    void ConfigurationWeightGroupControl::refreshProbabilities()
    {
        uint64_t total = 0u;
        for (std::size_t index = 0u; index < m_RowCount; ++index) { total += static_cast<uint32_t>(m_Rows[index].Control.Value); }
        for (std::size_t index = 0u; index < m_RowCount; ++index)
        {
            const uint64_t weight = static_cast<uint32_t>(m_Rows[index].Control.Value);
            m_Rows[index].ProbabilityPercent = total == 0u ? 0u : static_cast<uint32_t>((weight * 100u + total / 2u) / total);
        }
    }

    const std::string& ConfigurationWeightGroupControl::getLabel() const { return m_Label; }
    const std::array<ConfigurationWeightRow, ConfigurationWeightGroupControl::MaximumRows>& ConfigurationWeightGroupControl::getRows() const { return m_Rows; }
    std::array<ConfigurationWeightRow, ConfigurationWeightGroupControl::MaximumRows>& ConfigurationWeightGroupControl::getRows() { return m_Rows; }
    std::size_t ConfigurationWeightGroupControl::getRowCount() const { return m_RowCount; }
    ConfigurationEditorRect ConfigurationWeightGroupControl::getBounds() const { return m_Bounds; }

    void ConfigurationToggleControl::configure(std::string label, bool value)
    {
        Label = std::move(label);
        Value = value;
    }

    void ConfigurationToggleControl::setRowBounds(const ConfigurationEditorRect& bounds)
    {
        RowBounds = bounds;
        constexpr float ToggleWidth = 78.0f;
        ToggleBounds = { bounds.Left + bounds.Width - ToggleWidth, bounds.Top, ToggleWidth, bounds.Height };
    }

    bool ConfigurationToggleControl::activate(float x, float y)
    {
        if (!ToggleBounds.contains(x, y)) { return false; }
        Value = !Value;
        return true;
    }

    std::string ConfigurationToggleControl::getDisplayValue() const
    {
        return Value ? "ON" : "OFF";
    }

    void ConfigurationChoiceControl::configure(std::string label, std::vector<std::string> options, uint32_t value)
    {
        Label = std::move(label);
        Options = std::move(options);
        setValue(value);
    }

    void ConfigurationChoiceControl::setRowBounds(const ConfigurationEditorRect& bounds)
    {
        RowBounds = bounds;
        constexpr float ButtonWidth = 30.0f;
        constexpr float Gap = 4.0f;
        const float right = bounds.Left + bounds.Width;
        NextBounds = { right - ButtonWidth, bounds.Top, ButtonWidth, bounds.Height };
        PreviousBounds = { NextBounds.Left - Gap - ButtonWidth, bounds.Top, ButtonWidth, bounds.Height };
    }

    void ConfigurationChoiceControl::setValue(uint32_t value)
    {
        Value = Options.empty() ? 0u : value % static_cast<uint32_t>(Options.size());
    }

    bool ConfigurationChoiceControl::activate(float x, float y)
    {
        if (Options.empty()) { return false; }
        if (PreviousBounds.contains(x, y))
        {
            Value = Value == 0u ? static_cast<uint32_t>(Options.size() - 1u) : Value - 1u;
            return true;
        }
        if (NextBounds.contains(x, y))
        {
            Value = (Value + 1u) % static_cast<uint32_t>(Options.size());
            return true;
        }
        return false;
    }

    std::string ConfigurationChoiceControl::getDisplayValue() const
    {
        return Options.empty() || Value >= Options.size() ? std::string("-") : Options[Value];
    }

    void ConfigurationColorControl::configure(std::string label, uint32_t red, uint32_t green, uint32_t blue, uint32_t alpha)
    {
        Label = std::move(label);
        setValues(red, green, blue, alpha);
    }

    void ConfigurationColorControl::setRowBounds(const ConfigurationEditorRect& bounds)
    {
        RowBounds = bounds;
        constexpr float LabelWidth = 220.0f;
        constexpr float SwatchSize = 52.0f;
        constexpr float SwatchGap = 12.0f;
        constexpr float ValueWidth = 42.0f;
        constexpr float ChannelHeight = 14.0f;
        constexpr float TrackHeight = 5.0f;
        const float right = bounds.Left + bounds.Width;
        SwatchBounds = { right - SwatchSize, bounds.Top + (bounds.Height - SwatchSize) * 0.5f, SwatchSize, SwatchSize };
        const float trackLeft = bounds.Left + LabelWidth;
        const float trackRight = SwatchBounds.Left - SwatchGap - ValueWidth;
        const float trackWidth = std::max(80.0f, trackRight - trackLeft);
        for (std::size_t channel = 0u; channel < TrackBounds.size(); ++channel)
        {
            const float channelTop = bounds.Top + 6.0f + static_cast<float>(channel) * ChannelHeight;
            TrackBounds[channel] = { trackLeft, channelTop + 4.0f, trackWidth, TrackHeight };
        }
    }

    void ConfigurationColorControl::setValues(uint32_t red, uint32_t green, uint32_t blue, uint32_t alpha)
    {
        Red = std::min(255u, red);
        Green = std::min(255u, green);
        Blue = std::min(255u, blue);
        Alpha = std::min(255u, alpha);
    }

    uint32_t ConfigurationColorControl::getChannel(std::size_t channel) const
    {
        switch (channel)
        {
        case 0u: return Red;
        case 1u: return Green;
        case 2u: return Blue;
        case 3u: return Alpha;
        default: return 0u;
        }
    }

    void ConfigurationColorControl::setChannel(std::size_t channel, uint32_t value)
    {
        value = std::min(255u, value);
        switch (channel)
        {
        case 0u: Red = value; break;
        case 1u: Green = value; break;
        case 2u: Blue = value; break;
        case 3u: Alpha = value; break;
        default: break;
        }
    }

    uint32_t ConfigurationColorControl::valueForTrackPosition(std::size_t channel, float x) const
    {
        if (channel >= TrackBounds.size()) { return 0u; }
        const ConfigurationEditorRect& track = TrackBounds[channel];
        const float normalized = std::clamp((x - track.Left) / std::max(1.0f, track.Width), 0.0f, 1.0f);
        return static_cast<uint32_t>(std::lround(normalized * 255.0f));
    }

    bool ConfigurationColorControl::beginPointer(float x, float y)
    {
        for (std::size_t channel = 0u; channel < TrackBounds.size(); ++channel)
        {
            ConfigurationEditorRect hit = TrackBounds[channel];
            hit.Top -= 4.0f;
            hit.Height += 8.0f;
            if (!hit.contains(x, y)) { continue; }
            DraggingChannel = static_cast<int32_t>(channel);
            setChannel(channel, valueForTrackPosition(channel, x));
            return true;
        }
        return false;
    }

    bool ConfigurationColorControl::updatePointer(float x)
    {
        if (DraggingChannel < 0 || static_cast<std::size_t>(DraggingChannel) >= TrackBounds.size()) { return false; }
        const std::size_t channel = static_cast<std::size_t>(DraggingChannel);
        setChannel(channel, valueForTrackPosition(channel, x));
        return true;
    }

    bool ConfigurationColorControl::endPointer(float x, float)
    {
        if (DraggingChannel < 0 || static_cast<std::size_t>(DraggingChannel) >= TrackBounds.size()) { return false; }
        const std::size_t channel = static_cast<std::size_t>(DraggingChannel);
        setChannel(channel, valueForTrackPosition(channel, x));
        DraggingChannel = -1;
        return true;
    }

    std::string ConfigurationColorControl::getDisplayValue() const
    {
        return "RGBA " + std::to_string(Red) + "," + std::to_string(Green) + "," + std::to_string(Blue) + "," + std::to_string(Alpha);
    }

    bool ConfigurationTextField::activate(float x, float y)
    {
        Focused = Bounds.contains(x, y);
        return Focused;
    }

    bool ConfigurationTextField::onTextEntered(uint32_t unicode)
    {
        if (!Focused) { return false; }
        if (unicode == 8u)
        {
            if (!Value.empty()) { Value.pop_back(); }
            return true;
        }
        if (unicode < 32u || unicode > 126u || Value.size() >= MaximumCharacters) { return false; }
        Value.push_back(static_cast<char>(unicode));
        return true;
    }
}
