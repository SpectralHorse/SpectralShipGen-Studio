#include "PreviewRenderer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "PreviewCommand.h"
#include "SFMLPixelText.h"
#include "PreviewThumbnailGrid.h"

namespace
{
    constexpr float StatePanelX = static_cast<float>(PixelShipGeneratorPreview::PreviewStatePanelX);
    constexpr float CommandPanelX = static_cast<float>(PixelShipGeneratorPreview::PreviewCommandPanelX);
    constexpr float OverlayMargin = 24.0f;
    constexpr uint32_t TextScale = 2u;
    constexpr uint32_t SmallTextScale = 1u;
    constexpr float PanelPadding = 12.0f;
    constexpr float TextLineHeight = 14.0f;
    constexpr float LargeTextLineHeight = 20.0f;

    struct Bounds
    {
        uint32_t MinX = 0u;
        uint32_t MaxX = 0u;
        uint32_t MinY = 0u;
        uint32_t MaxY = 0u;
        bool Valid = false;
    };

    void drawDebugText(sf::RenderTarget& target, const std::string& text, float x, float y, const sf::Color& color, uint32_t scale = TextScale)
    {
        PixelShipGeneratorApplication::drawPixelText(target, text, x, y, color, scale);
    }

    float getDebugTextWidth(const std::string& text, uint32_t scale)
    {
        return PixelShipGeneratorApplication::getPixelTextWidth(text, scale);
    }

    std::string wrapDebugText(const std::string& text, std::size_t maximumCharactersPerLine)
    {
        return PixelShipGeneratorApplication::wrapPixelText(text, maximumCharactersPerLine);
    }

    std::string getCommandPanelButtonLabel(PixelShipGeneratorPreview::PreviewCommandType type)
    {
        using PixelShipGeneratorPreview::PreviewCommandType;
        switch (type)
        {
        case PreviewCommandType::PREVIOUS_STYLE:
        case PreviewCommandType::PREVIOUS_FACTION:
        case PreviewCommandType::PREVIOUS_RESOLUTION: return "<";
        case PreviewCommandType::NEXT_STYLE:
        case PreviewCommandType::NEXT_FACTION:
        case PreviewCommandType::NEXT_RESOLUTION: return ">";
        case PreviewCommandType::GALLERY_LEFT: return "LEFT";
        case PreviewCommandType::GALLERY_RIGHT: return "RIGHT";
        case PreviewCommandType::GALLERY_UP: return "UP";
        case PreviewCommandType::GALLERY_DOWN: return "DOWN";
        default: return PixelShipGeneratorPreview::getPreviewCommandData(type).Label;
        }
    }

    bool isStatefulCommand(PixelShipGeneratorPreview::PreviewCommandType type)
    {
        using PixelShipGeneratorPreview::PreviewCommandType;
        switch (type)
        {
        case PreviewCommandType::TOGGLE_ATTACHMENTS_ENABLED:
        case PreviewCommandType::TOGGLE_ASPECT_RATIO_LOCK:
        case PreviewCommandType::TOGGLE_STRUCTURE_LOCK:
        case PreviewCommandType::TOGGLE_PALETTE_LOCK:
        case PreviewCommandType::TOGGLE_DETAILS_LOCK:
        case PreviewCommandType::TOGGLE_ATTACHMENTS_LOCK:
        case PreviewCommandType::TOGGLE_HELP:
        case PreviewCommandType::TOGGLE_GENERATION_INSPECTOR:
        case PreviewCommandType::TOGGLE_PALETTE_INSPECTOR:
        case PreviewCommandType::TOGGLE_GENERATION_STAGE_VIEW:
        case PreviewCommandType::TOGGLE_ANIMATION:
        case PreviewCommandType::TOGGLE_FRAME_INSPECTION:
        case PreviewCommandType::OPEN_FAVORITES:
        case PreviewCommandType::TOGGLE_COMPARISON:
        case PreviewCommandType::CALIBRATION_TOGGLE_SHOW_VALUES:
        case PreviewCommandType::CALIBRATION_TOGGLE_CONTEXT_FILTER: return true;
        default: return false;
        }
    }

    void drawPanel(sf::RenderTarget& target, float x, float y, float width, float height, const sf::Color& fillColor, const sf::Color& outlineColor)
    {
        sf::RectangleShape panel(sf::Vector2f(width, height));
        panel.setPosition(x, y);
        panel.setFillColor(fillColor);
        panel.setOutlineThickness(1.0f);
        panel.setOutlineColor(outlineColor);
        target.draw(panel);
    }

    sf::Color toSFMLColor(const PixelShipGenerator::Color& color)
    {
        return sf::Color(color.R, color.G, color.B, color.A);
    }

    std::string getStyleName(PixelShipGenerator::ShipStyle style)
    {
        switch (style)
        {
        case PixelShipGenerator::ShipStyle::SLEEK: return "SLEEK";
        case PixelShipGenerator::ShipStyle::FIGHTER: return "FIGHTER";
        case PixelShipGenerator::ShipStyle::HEAVY: return "HEAVY";
        case PixelShipGenerator::ShipStyle::INDUSTRIAL: return "INDUSTRIAL";
        case PixelShipGenerator::ShipStyle::SPEARHEAD: return "SPEARHEAD";
        case PixelShipGenerator::ShipStyle::DELTA: return "DELTA";
        case PixelShipGenerator::ShipStyle::SHIP_STYLE_END: return "CUSTOM";
        default: return "UNKNOWN";
        }
    }

    std::string getFactionName(PixelShipGenerator::ShipFactionType faction)
    {
        switch (faction)
        {
        case PixelShipGenerator::ShipFactionType::FRONTIER: return "FRONTIER";
        case PixelShipGenerator::ShipFactionType::MILITARY: return "MILITARY";
        case PixelShipGenerator::ShipFactionType::ASCENDANT: return "ASCENDANT";
        case PixelShipGenerator::ShipFactionType::XENO: return "XENO";
        case PixelShipGenerator::ShipFactionType::CORPORATE: return "CORPORATE";
        case PixelShipGenerator::ShipFactionType::RELIC: return "RELIC";
        case PixelShipGenerator::ShipFactionType::SHIP_FACTION_TYPE_END: return "CUSTOM";
        default: return "UNKNOWN";
        }
    }

    std::string getPreviewModeName(PixelShipGeneratorPreview::PreviewMode mode)
    {
        switch (mode)
        {
        case PixelShipGeneratorPreview::PreviewMode::STATIC: return "STATIC";
        case PixelShipGeneratorPreview::PreviewMode::ANIMATION: return "ANIMATION";
        case PixelShipGeneratorPreview::PreviewMode::FRAME_INSPECTION: return "FRAME INSPECTION";
        case PixelShipGeneratorPreview::PreviewMode::GALLERY: return "GALLERY";
        case PixelShipGeneratorPreview::PreviewMode::FAVORITES: return "FAVORITES";
        case PixelShipGeneratorPreview::PreviewMode::REROLL_STUDIO: return "REROLL STUDIO";
        case PixelShipGeneratorPreview::PreviewMode::CALIBRATION: return "CALIBRATION";
        case PixelShipGeneratorPreview::PreviewMode::CONFIGURATION_EDITOR: return "CONFIGURATION EDITOR";
        default: return "UNKNOWN";
        }
    }

    std::string getDiagnosticViewName(PixelShipGeneratorPreview::DiagnosticViewMode mode)
    {
        switch (mode)
        {
        case PixelShipGeneratorPreview::DiagnosticViewMode::FINAL: return "FINAL";
        case PixelShipGeneratorPreview::DiagnosticViewMode::HULL: return "HULL";
        case PixelShipGeneratorPreview::DiagnosticViewMode::COCKPIT: return "COCKPIT";
        case PixelShipGeneratorPreview::DiagnosticViewMode::ENGINES: return "ENGINES";
        case PixelShipGeneratorPreview::DiagnosticViewMode::DETAILS: return "DETAILS";
        case PixelShipGeneratorPreview::DiagnosticViewMode::ATTACHMENTS: return "ATTACHMENTS";
        case PixelShipGeneratorPreview::DiagnosticViewMode::HULL_LAYERS: return "HULL LAYERS";
        case PixelShipGeneratorPreview::DiagnosticViewMode::CORE_TREATMENT: return "CORE TREATMENT";
        case PixelShipGeneratorPreview::DiagnosticViewMode::SEMANTIC_LOAD: return "SEMANTIC LOAD";
        case PixelShipGeneratorPreview::DiagnosticViewMode::MACRO_ASYMMETRY: return "MACRO ASYMMETRY";
        case PixelShipGeneratorPreview::DiagnosticViewMode::COMBINED: return "COMBINED";
        default: return "UNKNOWN";
        }
    }

    std::string getHullModifierName(PixelShipGenerator::HullModifierType type)
    {
        switch (type)
        {
        case PixelShipGenerator::HullModifierType::BROADER_SHOULDERS: return "BROADER SHOULDERS";
        case PixelShipGenerator::HullModifierType::SIDE_LOBES: return "SIDE LOBES";
        case PixelShipGenerator::HullModifierType::STEPPED_WING_EXTENSION: return "STEPPED WING";
        case PixelShipGenerator::HullModifierType::NARROW_WAIST: return "NARROW WAIST";
        case PixelShipGenerator::HullModifierType::WING_CUTOUT: return "WING CUTOUT";
        case PixelShipGenerator::HullModifierType::SPLIT_NOSE: return "SPLIT NOSE";
        default: return "UNKNOWN";
        }
    }


    std::string getWingShapeName(PixelShipGenerator::WingShapeType type)
    {
        switch (type)
        {
        case PixelShipGenerator::WingShapeType::NONE: return "NONE";
        case PixelShipGenerator::WingShapeType::SMALL: return "SMALL";
        case PixelShipGenerator::WingShapeType::SWEPT: return "SWEPT";
        case PixelShipGenerator::WingShapeType::BROAD: return "BROAD";
        default: return "UNKNOWN";
        }
    }

    std::string getMajorFeatureName(PixelShipGenerator::ShipMajorFeatureType type)
    {
        switch (type)
        {
        case PixelShipGenerator::ShipMajorFeatureType::CENTRAL_SPINE: return "CENTRAL SPINE";
        case PixelShipGenerator::ShipMajorFeatureType::ARMOR_PLATE: return "ARMOR PLATE";
        case PixelShipGenerator::ShipMajorFeatureType::RECESSED_BAY: return "RECESSED BAY";
        case PixelShipGenerator::ShipMajorFeatureType::VENT_BANK: return "VENT BANK";
        case PixelShipGenerator::ShipMajorFeatureType::WING_PLATE: return "WING PLATE";
        case PixelShipGenerator::ShipMajorFeatureType::TECH_CORE: return "TECH CORE";
        default: return "UNKNOWN";
        }
    }

    std::string getWeaponTypeName(PixelShipGenerator::ShipWeaponType type)
    {
        switch (type)
        {
        case PixelShipGenerator::ShipWeaponType::SINGLE_CANNON: return "SINGLE CANNON";
        case PixelShipGenerator::ShipWeaponType::TWIN_CANNON: return "TWIN CANNON";
        case PixelShipGenerator::ShipWeaponType::COMPACT_TURRET: return "COMPACT TURRET";
        case PixelShipGenerator::ShipWeaponType::RAIL_WEAPON: return "RAIL WEAPON";
        case PixelShipGenerator::ShipWeaponType::WEAPON_POD: return "WEAPON POD";
        default: return "UNKNOWN";
        }
    }

    std::string getWeaponRegionName(PixelShipGenerator::ShipWeaponHardpointRegion region)
    {
        switch (region)
        {
        case PixelShipGenerator::ShipWeaponHardpointRegion::CENTRAL_NOSE: return "CENTRAL NOSE";
        case PixelShipGenerator::ShipWeaponHardpointRegion::FORWARD_FUSELAGE_SIDE: return "FWD FUSELAGE";
        case PixelShipGenerator::ShipWeaponHardpointRegion::WING_ROOT: return "WING ROOT";
        case PixelShipGenerator::ShipWeaponHardpointRegion::OUTER_WING: return "OUTER WING";
        case PixelShipGenerator::ShipWeaponHardpointRegion::FORWARD_SHOULDER: return "FWD SHOULDER";
        case PixelShipGenerator::ShipWeaponHardpointRegion::CENTRAL_BODY: return "CENTRAL BODY";
        default: return "UNKNOWN";
        }
    }

    std::string getEngineLayoutName(PixelShipGenerator::EngineLayoutType type)
    {
        switch (type)
        {
        case PixelShipGenerator::EngineLayoutType::CENTRAL: return "CENTRAL";
        case PixelShipGenerator::EngineLayoutType::TWIN: return "TWIN";
        case PixelShipGenerator::EngineLayoutType::QUAD: return "QUAD";
        case PixelShipGenerator::EngineLayoutType::CENTRAL_AUXILIARY: return "CENTRAL + AUX";
        case PixelShipGenerator::EngineLayoutType::WIDE_BANK: return "WIDE BANK";
        case PixelShipGenerator::EngineLayoutType::ENGINE_LAYOUT_TYPE_END: return "NONE";
        default: return "UNKNOWN";
        }
    }

    std::string getEngineSizeName(PixelShipGenerator::EngineSizeClass type)
    {
        switch (type)
        {
        case PixelShipGenerator::EngineSizeClass::SMALL: return "SMALL";
        case PixelShipGenerator::EngineSizeClass::MEDIUM: return "MEDIUM";
        case PixelShipGenerator::EngineSizeClass::LARGE: return "LARGE";
        default: return "UNKNOWN";
        }
    }

    std::string getDebugStageName(PixelShipGenerator::ShipGenerationDebugStageType type)
    {
        switch (type)
        {
        case PixelShipGenerator::ShipGenerationDebugStageType::BASE_HULL: return "BASE HULL";
        case PixelShipGenerator::ShipGenerationDebugStageType::CLEANED_BASE_HULL: return "CLEANED BASE";
        case PixelShipGenerator::ShipGenerationDebugStageType::AFTER_ADDITIVE_MODIFIERS: return "AFTER ADDITIVE";
        case PixelShipGenerator::ShipGenerationDebugStageType::AFTER_SUBTRACTIVE_MODIFIERS: return "AFTER SUBTRACTIVE";
        case PixelShipGenerator::ShipGenerationDebugStageType::FINAL_HULL: return "FINAL HULL";
        default: return "UNKNOWN";
        }
    }

    std::string getAttachmentTypeName(PixelShipGenerator::ShipAttachmentType type)
    {
        switch (type)
        {
        case PixelShipGenerator::ShipAttachmentType::WEAPON_MOUNT: return "WEAPON";
        case PixelShipGenerator::ShipAttachmentType::SENSOR_ARRAY: return "SENSOR";
        case PixelShipGenerator::ShipAttachmentType::AUXILIARY_POD: return "AUX POD";
        case PixelShipGenerator::ShipAttachmentType::RADIATOR: return "RADIATOR";
        case PixelShipGenerator::ShipAttachmentType::ARMOR_FIN: return "ARMOR FIN";
        case PixelShipGenerator::ShipAttachmentType::TECHNOLOGY_NODE: return "TECH NODE";
        default: return "UNKNOWN";
        }
    }

    Bounds calculateMaskBounds(const PixelShipGenerator::PixelMask& mask)
    {
        Bounds bounds;
        bounds.MinX = mask.getWidth();
        bounds.MinY = mask.getHeight();

        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (!mask.get(x, y))
                {
                    continue;
                }

                bounds.MinX = std::min(bounds.MinX, x);
                bounds.MaxX = std::max(bounds.MaxX, x);
                bounds.MinY = std::min(bounds.MinY, y);
                bounds.MaxY = std::max(bounds.MaxY, y);
                bounds.Valid = true;
            }
        }

        return bounds;
    }

    Bounds calculateImageBounds(const PixelShipGenerator::Image& image, uint32_t width, uint32_t height)
    {
        Bounds bounds;
        bounds.MinX = width;
        bounds.MinY = height;

        for (uint32_t y = 0u; y < height; ++y)
        {
            for (uint32_t x = 0u; x < width; ++x)
            {
                if (image.getPixel(x, y).A == 0u)
                {
                    continue;
                }

                bounds.MinX = std::min(bounds.MinX, x);
                bounds.MaxX = std::max(bounds.MaxX, x);
                bounds.MinY = std::min(bounds.MinY, y);
                bounds.MaxY = std::max(bounds.MaxY, y);
                bounds.Valid = true;
            }
        }

        return bounds;
    }

    uint32_t countMaskPixels(const PixelShipGenerator::PixelMask& mask)
    {
        uint32_t count = 0u;

        for (uint32_t y = 0u; y < mask.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < mask.getWidth(); ++x)
            {
                if (mask.get(x, y))
                {
                    ++count;
                }
            }
        }

        return count;
    }

    std::string getBoundsString(const Bounds& bounds)
    {
        if (!bounds.Valid)
        {
            return "NONE";
        }

        return std::to_string(bounds.MinX) + "," + std::to_string(bounds.MinY) + " - " + std::to_string(bounds.MaxX) + "," + std::to_string(bounds.MaxY);
    }

    std::string getLockState(bool locked)
    {
        return locked ? "LOCK" : "OPEN";
    }

    std::string getOnOff(bool enabled)
    {
        return enabled ? "ON" : "OFF";
    }

    std::string getAnimationTypeDisplayName(PixelShipGenerator::ShipAnimationType type)
    {
        switch (type)
        {
        case PixelShipGenerator::ShipAnimationType::IDLE: return "IDLE";
        case PixelShipGenerator::ShipAnimationType::MOVE_LEFT: return "MOVE LEFT";
        case PixelShipGenerator::ShipAnimationType::MOVE_RIGHT: return "MOVE RIGHT";
        case PixelShipGenerator::ShipAnimationType::MOVE_UP: return "MOVE UP";
        case PixelShipGenerator::ShipAnimationType::MOVE_DOWN: return "MOVE DOWN";
        case PixelShipGenerator::ShipAnimationType::FIRE: return "FIRE";
        default: return "UNSUPPORTED";
        }
    }

    std::string getMovementPhaseDisplayName(PixelShipGenerator::ShipMovementAnimationPhase phase)
    {
        switch (phase)
        {
        case PixelShipGenerator::ShipMovementAnimationPhase::ENTER: return "ENTER";
        case PixelShipGenerator::ShipMovementAnimationPhase::SUSTAIN: return "SUSTAIN";
        case PixelShipGenerator::ShipMovementAnimationPhase::EXIT: return "EXIT";
        default: return "UNKNOWN";
        }
    }

    std::string getFiringPhaseDisplayName(PixelShipGenerator::ShipFiringAnimationPhase phase)
    {
        switch (phase)
        {
        case PixelShipGenerator::ShipFiringAnimationPhase::REST: return "REST";
        case PixelShipGenerator::ShipFiringAnimationPhase::PRE_FIRE: return "PRE-FIRE";
        case PixelShipGenerator::ShipFiringAnimationPhase::RECOIL: return "RECOIL";
        case PixelShipGenerator::ShipFiringAnimationPhase::RECOVERY: return "RECOVERY";
        default: return "UNKNOWN";
        }
    }

    std::string getCurrentFiringPhaseDisplayName(const PixelShipGenerator::ShipFiringAnimation& animation, uint32_t frameIndex)
    {
        if (animation.NormalizedSampleTimes.empty()) { return "REST"; }
        const uint32_t index = std::min(frameIndex, static_cast<uint32_t>(animation.NormalizedSampleTimes.size() - 1u));
        return getFiringPhaseDisplayName(PixelShipGenerator::getFiringAnimationPhase(animation.NormalizedSampleTimes[index]));
    }

    std::string getSameDifferent(bool same)
    {
        return same ? "SAME" : "DIFFERENT";
    }

    uint32_t calculateComparisonScale(const PixelShipGeneratorPreview::PreviewGenerationRecipe& pinned, const PixelShipGeneratorPreview::PreviewGenerationRecipe& current, uint32_t availableWidth, uint32_t availableHeight)
    {
        const uint32_t maximumWidth = std::max(pinned.Dimensions.Width, current.Dimensions.Width);
        const uint32_t maximumHeight = std::max(pinned.Dimensions.Height, current.Dimensions.Height);

        if (maximumWidth == 0u || maximumHeight == 0u)
        {
            return 1u;
        }

        return std::max(1u, std::min(availableWidth / maximumWidth, availableHeight / maximumHeight));
    }

    std::string getChangedRecipeComponents(const PixelShipGeneratorPreview::PreviewGenerationRecipe& pinned, const PixelShipGeneratorPreview::PreviewGenerationRecipe& current)
    {
        std::vector<std::string> changed;
        if (pinned.Seeds.Structure != current.Seeds.Structure) { changed.push_back("STRUCTURE"); }
        if (pinned.Seeds.Palette != current.Seeds.Palette) { changed.push_back("PALETTE"); }
        if (pinned.Seeds.Details != current.Seeds.Details || pinned.DetailDensity != current.DetailDensity || pinned.AsymmetricDetailChance != current.AsymmetricDetailChance) { changed.push_back("DETAILS"); }
        if (pinned.Seeds.Attachments != current.Seeds.Attachments || pinned.AttachmentsEnabled != current.AttachmentsEnabled) { changed.push_back("ATTACHMENTS"); }
        if (pinned.Style != current.Style) { changed.push_back("STYLE"); }
        if (pinned.Faction != current.Faction) { changed.push_back("FACTION"); }
        if (pinned.Dimensions.Width != current.Dimensions.Width || pinned.Dimensions.Height != current.Dimensions.Height) { changed.push_back("DIMENSIONS"); }

        if (changed.empty())
        {
            return "NONE";
        }

        std::string result;
        for (std::size_t index = 0u; index < changed.size(); ++index)
        {
            if (index != 0u) { result += " "; }
            result += changed[index];
        }
        return result;
    }

    std::string getAspectRatioString(uint32_t width, uint32_t height)
    {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(3) << (height == 0u ? 0.0 : static_cast<double>(width) / height);
        return stream.str();
    }

    std::string getMillisecondsString(double milliseconds)
    {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(1) << milliseconds << " ms";
        return stream.str();
    }

    std::string colorToHex(const PixelShipGenerator::Color& color)
    {
        std::ostringstream stream;
        stream << '#' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint32_t>(color.R) << std::setw(2) << static_cast<uint32_t>(color.G) << std::setw(2) << static_cast<uint32_t>(color.B) << std::setw(2) << static_cast<uint32_t>(color.A);
        return stream.str();
    }

    void drawLabelValue(sf::RenderTarget& target, const std::string& label, const std::string& value, float x, float& y, const sf::Color& labelColor = sf::Color(145, 150, 165), const sf::Color& valueColor = sf::Color(230, 232, 238))
    {
        drawDebugText(target, label, x, y, labelColor, TextScale);
        drawDebugText(target, value, x + 104.0f, y, valueColor, TextScale);
        y += LargeTextLineHeight;
    }

    void drawSectionHeader(sf::RenderTarget& target, const std::string& text, float x, float& y)
    {
        drawDebugText(target, text, x, y, sf::Color(235, 210, 105), TextScale);
        y += LargeTextLineHeight;
    }

    void drawDiagnosticLegendEntry(sf::RenderTarget& target, const std::string& label, const PixelShipGenerator::Color& color, float x, float& y)
    {
        sf::RectangleShape swatch(sf::Vector2f(12.0f, 10.0f));
        swatch.setPosition(x, y);
        swatch.setFillColor(toSFMLColor(color));
        swatch.setOutlineThickness(1.0f);
        swatch.setOutlineColor(sf::Color(150, 155, 165));
        target.draw(swatch);
        drawDebugText(target, label, x + 20.0f, y, sf::Color(220, 222, 228), SmallTextScale);
        y += 15.0f;
    }
}

namespace PixelShipGeneratorPreview
{
    void PreviewRenderer::render(sf::RenderWindow& window, const PreviewRenderData& data) const
    {
        window.clear(sf::Color(24, 24, 28));

        if (data.Mode == PreviewMode::CALIBRATION && data.CalibrationPair != nullptr)
        {
            renderCalibration(window, data);
        }
        else if (data.Mode == PreviewMode::REROLL_STUDIO && data.RerollStudio != nullptr)
        {
            renderRerollStudio(window, data);
        }
        else if (data.Mode == PreviewMode::GALLERY && data.Gallery != nullptr)
        {
            renderGallery(window, *data.Gallery);
        }
        else if (data.Mode == PreviewMode::FAVORITES && data.Favorites != nullptr)
        {
            renderFavorites(window, *data.Favorites);
        }
        else if (data.Comparison != nullptr && data.Comparison->ViewEnabled && data.Comparison->Pinned.Valid && data.CurrentStaticTexture != nullptr && data.PinnedTexture != nullptr && data.Recipe != nullptr)
        {
            renderComparison(window, data);
        }
        else if (data.PreviewSprite != nullptr)
        {
            renderSingle(window, *data.PreviewSprite);
        }

        const bool singlePreviewMode = data.Mode == PreviewMode::STATIC || data.Mode == PreviewMode::ANIMATION || data.Mode == PreviewMode::FRAME_INSPECTION || data.Mode == PreviewMode::CONFIGURATION_EDITOR;
        const bool comparisonVisible = data.Comparison != nullptr && data.Comparison->ViewEnabled && data.Comparison->Pinned.Valid;
        if (singlePreviewMode && !comparisonVisible && data.NativePreviewTexture != nullptr && data.Recipe != nullptr)
        {
            renderNativePreview(window, data);
        }

        if (data.Mode == PreviewMode::CONFIGURATION_EDITOR && data.ConfigurationEditor != nullptr)
        {
            renderConfigurationEditor(window, *data.ConfigurationEditor);
        }
        else
        {
            renderPersistentStatePanel(window, data);
            if (data.CommandPanel != nullptr) { renderCommandPanel(window, *data.CommandPanel); }
        }

        if (data.Diagnostics != nullptr)
        {
            if (data.Diagnostics->HelpVisible) { renderHelpOverlay(window); }
            if (data.Diagnostics->GenerationInspectorVisible) { renderGenerationInspector(window, data); }
            if (data.Diagnostics->PaletteInspectorVisible) { renderPaletteInspector(window, data); }
        }

        window.display();
    }

    void PreviewRenderer::renderCommandPanel(sf::RenderWindow& window, const PreviewCommandPanel& commandPanel) const
    {
        drawPanel(window, CommandPanelX, 0.0f, static_cast<float>(PreviewCommandPanelWidth), static_cast<float>(PreviewWindowHeight), sf::Color(18, 19, 24, 248), sf::Color(72, 76, 88));
        drawDebugText(window, "COMMANDS", CommandPanelX + 10.0f, 4.0f, sf::Color(240, 215, 105), TextScale);

        for (const PreviewCommandPanelGroupHeader& groupHeader : commandPanel.getGroupHeaders())
        {
            drawDebugText(window, groupHeader.Label, groupHeader.Position.x, groupHeader.Position.y, sf::Color(110, 190, 230), TextScale);
        }

        const int32_t hoveredIndex = commandPanel.getHoveredButtonIndex();
        const int32_t pressedIndex = commandPanel.getPressedButtonIndex();
        const std::vector<PreviewCommandPanelButton>& buttons = commandPanel.getButtons();

        for (std::size_t index = 0u; index < buttons.size(); ++index)
        {
            const PreviewCommandPanelButton& button = buttons[index];
            sf::Color fillColor(34, 36, 44);
            sf::Color outlineColor(78, 82, 96);
            sf::Color textColor(220, 224, 232);

            if (!button.Enabled)
            {
                fillColor = sf::Color(24, 25, 30);
                outlineColor = sf::Color(48, 50, 58);
                textColor = sf::Color(90, 94, 105);
            }
            else if (static_cast<int32_t>(index) == pressedIndex)
            {
                fillColor = sf::Color(75, 82, 96);
                outlineColor = sf::Color(180, 190, 215);
            }
            else if (static_cast<int32_t>(index) == hoveredIndex)
            {
                fillColor = sf::Color(54, 59, 72);
                outlineColor = sf::Color(135, 175, 220);
            }
            else if (button.Active)
            {
                fillColor = sf::Color(45, 70, 58);
                outlineColor = sf::Color(105, 190, 135);
            }

            drawPanel(window, button.Bounds.left, button.Bounds.top, button.Bounds.width, button.Bounds.height, fillColor, outlineColor);
            const std::string label = button.Label.empty() ? getCommandPanelButtonLabel(button.Command.Type) : button.Label;
            const uint32_t labelScale = button.Command.Type == PreviewCommandType::SELECT_RESOLUTION_BOOKMARK ? SmallTextScale : TextScale;
            const float labelX = button.Bounds.left + (labelScale == SmallTextScale ? 4.0f : 7.0f);
            const float labelY = button.Bounds.top + (labelScale == SmallTextScale ? 5.0f : 6.0f);
            drawDebugText(window, label, labelX, labelY, textColor, labelScale);

            const PreviewCommandData& commandData = getPreviewCommandData(button.Command.Type);
            const std::string shortcut = commandData.Shortcut;
            const float shortcutWidth = getDebugTextWidth(shortcut, SmallTextScale);

            if (isStatefulCommand(button.Command.Type) && button.Bounds.width >= 120.0f)
            {
                const std::string state = button.Active ? "ON" : "OFF";
                const float stateWidth = getDebugTextWidth(state, TextScale);
                const float stateX = button.Bounds.left + button.Bounds.width - stateWidth - 7.0f;
                drawDebugText(window, state, stateX, labelY, button.Enabled ? sf::Color(145, 220, 165) : textColor, TextScale);

                if (!shortcut.empty() && getDebugTextWidth(label, labelScale) + shortcutWidth + stateWidth + 28.0f < button.Bounds.width)
                {
                    drawDebugText(window, shortcut, stateX - shortcutWidth - 7.0f, button.Bounds.top + 8.0f, button.Enabled ? sf::Color(130, 170, 205) : textColor, SmallTextScale);
                }
            }
            else if (!shortcut.empty() && button.Bounds.width >= 180.0f && getDebugTextWidth(label, labelScale) + shortcutWidth + 22.0f < button.Bounds.width)
            {
                drawDebugText(window, shortcut, button.Bounds.left + button.Bounds.width - shortcutWidth - 7.0f, button.Bounds.top + 8.0f, button.Enabled ? sf::Color(130, 170, 205) : textColor, SmallTextScale);
            }
        }

        for (const PreviewCommandPanelSelector& selector : commandPanel.getSelectors())
        {
            drawDebugText(window, selector.Label, selector.ValueBounds.left - 120.0f, selector.ValueBounds.top + 6.0f, sf::Color(160, 165, 180), TextScale);
            drawPanel(window, selector.ValueBounds.left, selector.ValueBounds.top, selector.ValueBounds.width, selector.ValueBounds.height, sf::Color(24, 26, 32), sf::Color(58, 62, 72));
            const float valueWidth = getDebugTextWidth(selector.Value, TextScale);
            drawDebugText(window, selector.Value, selector.ValueBounds.left + std::max(4.0f, (selector.ValueBounds.width - valueWidth) * 0.5f), selector.ValueBounds.top + 6.0f, sf::Color(232, 234, 240), TextScale);
        }

        const auto drawDimensionSlider = [&](const PreviewCommandPanelSlider& slider)
            {
                drawPanel(window, slider.ValueBounds.left, slider.ValueBounds.top, slider.ValueBounds.width, slider.ValueBounds.height, slider.Enabled ? sf::Color(24, 26, 32) : sf::Color(20, 21, 25), sf::Color(58, 62, 72));
                drawDebugText(window, slider.Label, slider.ValueBounds.left + 5.0f, slider.ValueBounds.top + 4.0f, sf::Color(160, 165, 180), SmallTextScale);
                const std::string valueText = std::to_string(slider.Value);
                const float valueWidth = getDebugTextWidth(valueText, SmallTextScale);
                drawDebugText(window, valueText, slider.TrackBounds.left + std::max(0.0f, (slider.TrackBounds.width - valueWidth) * 0.5f), slider.ValueBounds.top + 3.0f, slider.Enabled ? sf::Color(232, 234, 240) : sf::Color(90, 94, 105), SmallTextScale);
                if (!slider.DetailText.empty())
                {
                    const float detailWidth = getDebugTextWidth(slider.DetailText, SmallTextScale);
                    drawDebugText(window, slider.DetailText, slider.ValueBounds.left + slider.ValueBounds.width - detailWidth - 5.0f, slider.ValueBounds.top + 4.0f, sf::Color(125, 175, 205), SmallTextScale);
                }
                sf::RectangleShape sliderTrack(sf::Vector2f(slider.TrackBounds.width, slider.TrackBounds.height));
                sliderTrack.setPosition(slider.TrackBounds.left, slider.TrackBounds.top);
                sliderTrack.setFillColor(slider.Enabled ? sf::Color(62, 66, 78) : sf::Color(42, 44, 50));
                window.draw(sliderTrack);
                const float sliderRange = static_cast<float>(std::max(1u, slider.Maximum - slider.Minimum));
                const float sliderNormalized = static_cast<float>(slider.Value - slider.Minimum) / sliderRange;
                const float knobX = slider.TrackBounds.left + sliderNormalized * slider.TrackBounds.width;
                sf::RectangleShape sliderKnob(sf::Vector2f(4.0f, 10.0f));
                sliderKnob.setPosition(knobX - 2.0f, slider.TrackBounds.top - 3.0f);
                sliderKnob.setFillColor(slider.Dragging ? sf::Color(235, 215, 110) : (slider.Enabled ? sf::Color(125, 190, 230) : sf::Color(75, 78, 88)));
                window.draw(sliderKnob);
            };

        if (commandPanel.getMode() == PreviewCommandPanelMode::CALIBRATION)
        {
            for (const PreviewCommandPanelSlider& slider : commandPanel.getCalibrationSliders()) { drawDimensionSlider(slider); }
        }
        else if (commandPanel.getMode() == PreviewCommandPanelMode::NORMAL)
        {
            drawDimensionSlider(commandPanel.getWidthSlider());
            drawDimensionSlider(commandPanel.getHeightSlider());
        }

        const float descriptionY = static_cast<float>(PreviewWindowHeight) - 66.0f;
        drawPanel(window, CommandPanelX + 10.0f, descriptionY, static_cast<float>(PreviewCommandPanelWidth) - 20.0f, 54.0f, sf::Color(12, 13, 17, 245), sf::Color(55, 58, 68));
        const PreviewCommandData* hoveredCommandData = commandPanel.getHoveredCommandData();
        drawDebugText(window, hoveredCommandData != nullptr ? hoveredCommandData->Label : "HOVER A CONTROL", CommandPanelX + 18.0f, descriptionY + 8.0f, sf::Color(235, 215, 110), TextScale);

        if (hoveredCommandData != nullptr)
        {
            drawDebugText(window, wrapDebugText(hoveredCommandData->Description, 84u), CommandPanelX + 18.0f, descriptionY + 26.0f, sf::Color(185, 190, 202), SmallTextScale);
        }
    }

    void PreviewRenderer::renderConfigurationEditor(sf::RenderWindow& window, const PreviewConfigurationEditor& editor) const
    {
        const ConfigurationEditorRect panel = editor.getPanelBounds();
        const ConfigurationEditorRect viewport = editor.getContentViewport();
        drawPanel(window, panel.Left, panel.Top, panel.Width, panel.Height, sf::Color(18, 19, 24, 252), sf::Color(82, 88, 104));
        const bool factionEditor = editor.getProfileKind() == ConfigurationEditorProfileKind::FACTION;
        const bool paletteEditor = editor.getProfileKind() == ConfigurationEditorProfileKind::PALETTE;
        const char* editorTitle = paletteEditor ? "PALETTE CONFIGURATION EDITOR" : factionEditor ? "FACTION PROFILE EDITOR" : "STRUCTURAL PROFILE EDITOR";
        const char* editorSubtitle = paletteEditor ? "PUBLIC ShipPaletteConfiguration / GENERATED + FIXED" : factionEditor ? "PUBLIC ShipFactionProfile / RUNTIME CUSTOM FACTIONS" : "PUBLIC ShipGenerationProfile / RUNTIME CUSTOM PRESETS";
        drawDebugText(window, editorTitle, panel.Left + 18.0f, panel.Top + 12.0f, sf::Color(240, 215, 105), TextScale);
        drawDebugText(window, editorSubtitle, panel.Left + 18.0f, panel.Top + 34.0f, sf::Color(125, 180, 215), SmallTextScale);
        drawDebugText(window, "PgDn structural | Shift+PgDn faction | Ctrl+PgDn palette", panel.Left + panel.Width - 390.0f, panel.Top + 12.0f, sf::Color(130, 135, 150), SmallTextScale);

        drawPanel(window, viewport.Left - 6.0f, viewport.Top - 4.0f, viewport.Width + 12.0f, viewport.Height + 8.0f, sf::Color(13, 14, 18, 248), sf::Color(48, 52, 62));

        const auto visible = [&](const ConfigurationEditorRect& bounds)
            {
                return bounds.Top + bounds.Height >= viewport.Top && bounds.Top <= viewport.Top + viewport.Height;
            };
        const auto drawSmallButton = [&](const ConfigurationEditorRect& bounds, const char* label, bool enabled = true)
            {
                if (!visible(bounds)) { return; }
                drawPanel(window, bounds.Left, bounds.Top, bounds.Width, bounds.Height, enabled ? sf::Color(38, 42, 51) : sf::Color(24, 25, 29), enabled ? sf::Color(82, 92, 110) : sf::Color(46, 48, 55));
                const float textWidth = getDebugTextWidth(label, SmallTextScale);
                drawDebugText(window, label, bounds.Left + std::max(2.0f, (bounds.Width - textWidth) * 0.5f), bounds.Top + 6.0f, enabled ? sf::Color(220, 224, 232) : sf::Color(90, 94, 105), SmallTextScale);
            };
        const auto drawInteger = [&](const ConfigurationIntegerControl& control, bool showProbability = false, uint32_t probability = 0u)
            {
                if (!visible(control.RowBounds)) { return; }
                drawDebugText(window, control.Label, control.RowBounds.Left + 6.0f, control.RowBounds.Top + 9.0f, sf::Color(185, 190, 204), SmallTextScale);
                sf::RectangleShape track(sf::Vector2f(control.TrackBounds.Width, control.TrackBounds.Height));
                track.setPosition(control.TrackBounds.Left, control.TrackBounds.Top);
                track.setFillColor(sf::Color(58, 64, 76));
                window.draw(track);
                const float span = static_cast<float>(std::max(1, control.Maximum - control.Minimum));
                const float normalized = static_cast<float>(control.Value - control.Minimum) / span;
                sf::RectangleShape knob(sf::Vector2f(4.0f, 14.0f));
                knob.setPosition(control.TrackBounds.Left + normalized * control.TrackBounds.Width - 2.0f, control.TrackBounds.Top - 4.0f);
                knob.setFillColor(control.Dragging ? sf::Color(240, 215, 105) : sf::Color(120, 190, 230));
                window.draw(knob);
                const std::string value = control.getDisplayValue();
                drawDebugText(window, value, control.TrackBounds.Left + control.TrackBounds.Width + 8.0f, control.RowBounds.Top + 8.0f, sf::Color(232, 234, 240), SmallTextScale);
                if (showProbability)
                {
                    const std::string probabilityText = std::to_string(probability) + "% share";
                    drawDebugText(window, probabilityText, control.TrackBounds.Left - 74.0f, control.RowBounds.Top + 8.0f, sf::Color(125, 175, 205), SmallTextScale);
                }
                drawSmallButton(control.DecrementBounds, "-");
                drawSmallButton(control.IncrementBounds, "+");
            };
        const auto drawRange = [&](const ConfigurationRangeControl& range)
            {
                if (!visible(range.RowBounds)) { return; }
                drawDebugText(window, range.Label, range.RowBounds.Left + 6.0f, range.RowBounds.Top + 9.0f, sf::Color(185, 190, 204), SmallTextScale);
                drawDebugText(window, "MIN " + std::to_string(range.MinimumValue), range.MinimumDecrementBounds.Left - 72.0f, range.RowBounds.Top + 9.0f, sf::Color(220, 224, 232), SmallTextScale);
                drawSmallButton(range.MinimumDecrementBounds, "-");
                drawSmallButton(range.MinimumIncrementBounds, "+");
                drawDebugText(window, "MAX " + std::to_string(range.MaximumValue), range.MaximumDecrementBounds.Left - 72.0f, range.RowBounds.Top + 9.0f, sf::Color(220, 224, 232), SmallTextScale);
                drawSmallButton(range.MaximumDecrementBounds, "-");
                drawSmallButton(range.MaximumIncrementBounds, "+");
            };
        const auto drawToggle = [&](const ConfigurationToggleControl& control)
            {
                if (!visible(control.RowBounds)) { return; }
                drawDebugText(window, control.Label, control.RowBounds.Left + 6.0f, control.RowBounds.Top + 9.0f, sf::Color(185, 190, 204), SmallTextScale);
                drawPanel(window, control.ToggleBounds.Left, control.ToggleBounds.Top, control.ToggleBounds.Width, control.ToggleBounds.Height,
                    control.Value ? sf::Color(45, 78, 62) : sf::Color(38, 42, 51), control.Value ? sf::Color(100, 190, 130) : sf::Color(82, 92, 110));
                const std::string text = control.getDisplayValue();
                const float textWidth = getDebugTextWidth(text, SmallTextScale);
                drawDebugText(window, text, control.ToggleBounds.Left + std::max(3.0f, (control.ToggleBounds.Width - textWidth) * 0.5f), control.ToggleBounds.Top + 8.0f,
                    control.Value ? sf::Color(180, 235, 195) : sf::Color(210, 214, 224), SmallTextScale);
            };
        const auto drawChoice = [&](const ConfigurationChoiceControl& control)
            {
                if (!visible(control.RowBounds)) { return; }
                drawDebugText(window, control.Label, control.RowBounds.Left + 6.0f, control.RowBounds.Top + 9.0f, sf::Color(185, 190, 204), SmallTextScale);
                drawDebugText(window, control.getDisplayValue(), control.PreviousBounds.Left - 150.0f, control.RowBounds.Top + 9.0f, sf::Color(232, 234, 240), SmallTextScale);
                drawSmallButton(control.PreviousBounds, "<");
                drawSmallButton(control.NextBounds, ">");
            };
        const auto drawColor = [&](const ConfigurationColorControl& control)
            {
                if (!visible(control.RowBounds)) { return; }
                drawDebugText(window, control.Label, control.RowBounds.Left + 6.0f, control.RowBounds.Top + 8.0f, sf::Color(185, 190, 204), SmallTextScale);
                static constexpr std::array<const char*, 4u> channelLabels = { "R", "G", "B", "A" };
                for (std::size_t channel = 0u; channel < control.TrackBounds.size(); ++channel)
                {
                    const ConfigurationEditorRect& trackBounds = control.TrackBounds[channel];
                    drawDebugText(window, channelLabels[channel], trackBounds.Left - 14.0f, trackBounds.Top - 3.0f, sf::Color(150, 160, 176), SmallTextScale);
                    sf::RectangleShape track(sf::Vector2f(trackBounds.Width, trackBounds.Height));
                    track.setPosition(trackBounds.Left, trackBounds.Top);
                    track.setFillColor(sf::Color(58, 64, 76));
                    window.draw(track);
                    const float normalized = static_cast<float>(control.getChannel(channel)) / 255.0f;
                    sf::RectangleShape knob(sf::Vector2f(4.0f, 11.0f));
                    knob.setPosition(trackBounds.Left + normalized * trackBounds.Width - 2.0f, trackBounds.Top - 3.0f);
                    knob.setFillColor(control.DraggingChannel == static_cast<int32_t>(channel) ? sf::Color(240, 215, 105) : sf::Color(120, 190, 230));
                    window.draw(knob);
                    drawDebugText(window, std::to_string(control.getChannel(channel)), trackBounds.Left + trackBounds.Width + 7.0f, trackBounds.Top - 3.0f, sf::Color(232, 234, 240), SmallTextScale);
                }
                drawPanel(window, control.SwatchBounds.Left, control.SwatchBounds.Top, control.SwatchBounds.Width, control.SwatchBounds.Height,
                    sf::Color(static_cast<sf::Uint8>(control.Red), static_cast<sf::Uint8>(control.Green), static_cast<sf::Uint8>(control.Blue), static_cast<sf::Uint8>(control.Alpha)), sf::Color(130, 135, 150));
            };
        const auto drawSectionHeader = [&](const std::string& label, const ConfigurationEditorRect& bounds, bool expanded)
            {
                if (!visible(bounds)) { return; }
                drawPanel(window, bounds.Left, bounds.Top, bounds.Width, bounds.Height, sf::Color(29, 32, 40), sf::Color(55, 64, 78));
                drawDebugText(window, expanded ? "[-]" : "[+]", bounds.Left + 6.0f, bounds.Top + 7.0f, sf::Color(125, 190, 230), SmallTextScale);
                drawDebugText(window, label, bounds.Left + 34.0f, bounds.Top + 7.0f, sf::Color(205, 212, 225), SmallTextScale);
            };

        const ConfigurationTextField& nameField = editor.getNameField();
        if (visible(nameField.Bounds))
        {
            drawDebugText(window, nameField.Label, nameField.Bounds.Left + 6.0f, nameField.Bounds.Top + 7.0f, sf::Color(170, 175, 190), SmallTextScale);
            const float boxLeft = nameField.Bounds.Left + 150.0f;
            const float boxWidth = nameField.Bounds.Width - 156.0f;
            drawPanel(window, boxLeft, nameField.Bounds.Top + 2.0f, boxWidth, nameField.Bounds.Height - 4.0f, sf::Color(23, 25, 31), nameField.Focused ? sf::Color(135, 185, 225) : sf::Color(61, 66, 78));
            drawDebugText(window, nameField.Value + (nameField.Focused ? "_" : ""), boxLeft + 7.0f, nameField.Bounds.Top + 8.0f, sf::Color(232, 234, 240), SmallTextScale);
        }

        const auto drawProfileSections = [&](const auto& sections)
            {
                for (const auto& section : sections)
                {
                    drawSectionHeader(section.Label, section.HeaderBounds, section.Expanded);
                    if (!section.Expanded) { continue; }
                    for (const auto& field : section.Integers) { drawInteger(field.Control); }
                    for (const auto& field : section.Ranges) { drawRange(field.Control); }
                    for (const auto& field : section.Toggles) { drawToggle(field.Control); }
                    for (const auto& field : section.Choices) { drawChoice(field.Control); }
                    for (const auto& field : section.WeightGroups)
                    {
                        const auto& rows = field.Control.getRows();
                        for (std::size_t index = 0u; index < field.Control.getRowCount(); ++index) { drawInteger(rows[index].Control, true, rows[index].ProbabilityPercent); }
                    }
                }
            };
        if (paletteEditor)
        {
            for (const auto& section : editor.getPaletteProfileSections())
            {
                if (!editor.isPaletteSectionVisible(section)) { continue; }
                drawSectionHeader(section.Label, section.HeaderBounds, section.Expanded);
                if (!section.Expanded) { continue; }
                for (const auto& field : section.Integers) { drawInteger(field.Control); }
                for (const auto& field : section.Ranges) { drawRange(field.Control); }
                for (const auto& field : section.Choices) { drawChoice(field.Control); }
                for (const auto& field : section.Colors) { drawColor(field.Control); }
            }
        }
        else if (factionEditor) { drawProfileSections(editor.getFactionProfileSections()); }
        else { drawProfileSections(editor.getProfileSections()); }

        const ConfigurationEditorSectionState& validationSection = editor.getValidationSection();
        drawSectionHeader(validationSection.Label, validationSection.HeaderBounds, validationSection.Expanded);
        if (validationSection.Expanded)
        {
            float y = validationSection.HeaderBounds.Top + validationSection.HeaderBounds.Height + 6.0f;
            const auto drawIssue = [&](const PixelShipGenerator::ValidationIssue& issue, const sf::Color& color)
                {
                    if (y > viewport.Top + viewport.Height) { return; }
                    if (y + 24.0f >= viewport.Top)
                    {
                        drawDebugText(window, issue.Field.empty() ? "CONFIG" : issue.Field, viewport.Left + 8.0f, y, color, SmallTextScale);
                        drawDebugText(window, wrapDebugText(issue.Message, 76u), viewport.Left + 170.0f, y, sf::Color(190, 194, 204), SmallTextScale);
                    }
                    y += 28.0f;
                };
            const auto& validation = editor.getValidationResult();
            if (validation.Errors.empty() && validation.Warnings.empty())
            {
                if (y >= viewport.Top && y <= viewport.Top + viewport.Height) { drawDebugText(window, "VALID / NO CORE VALIDATION ISSUES", viewport.Left + 8.0f, y, sf::Color(125, 215, 150), SmallTextScale); }
            }
            else
            {
                for (const auto& issue : validation.Errors) { drawIssue(issue, sf::Color(235, 105, 105)); }
                for (const auto& issue : validation.Warnings) { drawIssue(issue, sf::Color(235, 195, 100)); }
            }
        }

        if (editor.getMaximumScrollOffset() > 0.0f)
        {
            const float trackX = viewport.Left + viewport.Width - 4.0f;
            sf::RectangleShape scrollTrack(sf::Vector2f(3.0f, viewport.Height));
            scrollTrack.setPosition(trackX, viewport.Top);
            scrollTrack.setFillColor(sf::Color(42, 45, 54));
            window.draw(scrollTrack);
            const float thumbHeight = std::max(30.0f, viewport.Height * viewport.Height / (viewport.Height + editor.getMaximumScrollOffset()));
            const float available = viewport.Height - thumbHeight;
            const float normalized = editor.getScrollOffset() / editor.getMaximumScrollOffset();
            sf::RectangleShape thumb(sf::Vector2f(3.0f, thumbHeight));
            thumb.setPosition(trackX, viewport.Top + normalized * available);
            thumb.setFillColor(sf::Color(120, 185, 225));
            window.draw(thumb);
        }

        for (const ConfigurationEditorActionButton& button : editor.getActionButtons())
        {
            drawPanel(window, button.Bounds.Left, button.Bounds.Top, button.Bounds.Width, button.Bounds.Height, button.Enabled ? sf::Color(39, 46, 57) : sf::Color(24, 25, 30), button.Enabled ? sf::Color(88, 104, 124) : sf::Color(45, 48, 56));
            const float textWidth = getDebugTextWidth(button.Label, SmallTextScale);
            drawDebugText(window, button.Label, button.Bounds.Left + std::max(4.0f, (button.Bounds.Width - textWidth) * 0.5f), button.Bounds.Top + 10.0f, button.Enabled ? sf::Color(225, 229, 237) : sf::Color(90, 94, 105), SmallTextScale);
        }

        const std::string dirty = editor.hasUnsavedChanges() ? "UNAPPLIED CHANGES" : "NO CHANGES";
        drawDebugText(window, dirty + "  |  " + std::to_string(editor.getBoundValueCount()) + " PROFILE VALUES", panel.Left + 18.0f, panel.Top + panel.Height - 22.0f, editor.hasUnsavedChanges() ? sf::Color(235, 195, 100) : sf::Color(120, 175, 140), SmallTextScale);
    }

    void PreviewRenderer::renderCalibration(sf::RenderWindow& window, const PreviewRenderData& data) const
    {
        const CalibrationCandidatePair& pair = *data.CalibrationPair;
        drawPanel(window, 12.0f, 12.0f, static_cast<float>(PreviewContentWidth) - 24.0f, 42.0f, sf::Color(19, 21, 27), sf::Color(74, 80, 94));
        drawDebugText(window, "CALIBRATION LAB", 24.0f, 22.0f, sf::Color(240, 215, 105), TextScale);
        drawDebugText(window, getCalibrationGroupName(data.CalibrationGroup), 210.0f, 22.0f, sf::Color(130, 195, 230), TextScale);

        if (!pair.Valid || data.CalibrationTextureA == nullptr || data.CalibrationTextureB == nullptr)
        {
            drawDebugText(window, "NO VALID CONTROLLED PAIR", 250.0f, 300.0f, sf::Color(220, 120, 120), TextScale);
            return;
        }

        constexpr float columnWidth = static_cast<float>(PreviewContentWidth) * 0.5f;
        constexpr float top = 82.0f;
        constexpr float spriteRegionHeight = 510.0f;
        constexpr float padding = 22.0f;
        const sf::Texture* leftTexture = pair.DisplayAOnLeft ? data.CalibrationTextureA : data.CalibrationTextureB;
        const sf::Texture* rightTexture = pair.DisplayAOnLeft ? data.CalibrationTextureB : data.CalibrationTextureA;
        const uint32_t leftOption = pair.DisplayAOnLeft ? pair.OptionA : pair.OptionB;
        const uint32_t rightOption = pair.DisplayAOnLeft ? pair.OptionB : pair.OptionA;

        const auto drawCandidate = [&](const sf::Texture& texture, float columnX, const char* label, uint32_t option)
            {
                const sf::Vector2u size = texture.getSize();
                const uint32_t availableWidth = static_cast<uint32_t>(columnWidth - padding * 2.0f);
                const uint32_t availableHeight = static_cast<uint32_t>(spriteRegionHeight - 54.0f);
                const uint32_t scaleX = size.x == 0u ? 1u : availableWidth / size.x;
                const uint32_t scaleY = size.y == 0u ? 1u : availableHeight / size.y;
                const uint32_t scale = std::max(1u, std::min(scaleX, scaleY));
                sf::Sprite sprite(texture);
                sprite.setScale(static_cast<float>(scale), static_cast<float>(scale));
                const float width = static_cast<float>(size.x * scale);
                const float height = static_cast<float>(size.y * scale);
                sprite.setPosition(columnX + (columnWidth - width) * 0.5f, top + 40.0f + (spriteRegionHeight - 54.0f - height) * 0.5f);
                window.draw(sprite);
                const float labelWidth = getDebugTextWidth(label, TextScale);
                drawDebugText(window, label, columnX + (columnWidth - labelWidth) * 0.5f, top, sf::Color(235, 237, 242), TextScale);
                if (data.CalibrationShowValues)
                {
                    const std::string optionText = getCalibrationOptionName(pair.Group, option);
                    const float optionWidth = getDebugTextWidth(optionText, SmallTextScale);
                    drawDebugText(window, optionText, columnX + (columnWidth - optionWidth) * 0.5f, top + 22.0f, sf::Color(150, 190, 220), SmallTextScale);
                }
            };

        drawCandidate(*leftTexture, 0.0f, "SHIP A", leftOption);
        drawCandidate(*rightTexture, columnWidth, "SHIP B", rightOption);

        float infoY = 620.0f;
        drawDebugText(window, "CONTROLLED CONTEXT", 22.0f, infoY, sf::Color(235, 210, 105), TextScale); infoY += 20.0f;
        drawDebugText(window, std::to_string(pair.Recipe.Dimensions.Width) + "X" + std::to_string(pair.Recipe.Dimensions.Height) + "  " + getStyleName(pair.Recipe.Style) + "  " + getFactionName(pair.Recipe.Faction), 22.0f, infoY, sf::Color(215, 220, 230), TextScale); infoY += 22.0f;
        drawDebugText(window, "PAIR " + std::to_string(pair.PairIndex) + "  A/B POSITION RANDOMIZED", 22.0f, infoY, sf::Color(145, 170, 195), SmallTextScale); infoY += 16.0f;
        drawDebugText(window, wrapDebugText(pair.IsolationNote, 104u), 22.0f, infoY, sf::Color(165, 170, 182), SmallTextScale);
    }

    uint32_t PreviewRenderer::renderSideBySideSprites(sf::RenderWindow& window, const PreviewGenerationRecipe& leftRecipe, const sf::Texture& leftTexture, const std::string& leftLabel, const sf::Color& leftColor, const PreviewGenerationRecipe& rightRecipe, const sf::Texture& rightTexture, const std::string& rightLabel, const sf::Color& rightColor, float spriteRegionTop, float spriteRegionHeight) const
    {
        constexpr float columnWidth = static_cast<float>(PreviewContentWidth) * 0.5f;
        constexpr float columnPadding = 20.0f;
        const uint32_t availableWidth = static_cast<uint32_t>(columnWidth - columnPadding * 2.0f);
        const uint32_t availableHeight = static_cast<uint32_t>(std::max(1.0f, spriteRegionHeight));
        const uint32_t scale = calculateComparisonScale(leftRecipe, rightRecipe, availableWidth, availableHeight);

        drawDebugText(window, leftLabel, 20.0f, 22.0f, leftColor, TextScale);
        drawDebugText(window, rightLabel, columnWidth + 20.0f, 22.0f, rightColor, TextScale);

        sf::Sprite leftSprite(leftTexture);
        sf::Sprite rightSprite(rightTexture);
        const float leftWidth = static_cast<float>(leftRecipe.Dimensions.Width * scale);
        const float leftHeight = static_cast<float>(leftRecipe.Dimensions.Height * scale);
        const float rightWidth = static_cast<float>(rightRecipe.Dimensions.Width * scale);
        const float rightHeight = static_cast<float>(rightRecipe.Dimensions.Height * scale);
        leftSprite.setScale(static_cast<float>(scale), static_cast<float>(scale));
        rightSprite.setScale(static_cast<float>(scale), static_cast<float>(scale));
        leftSprite.setPosition((columnWidth - leftWidth) * 0.5f, spriteRegionTop + (spriteRegionHeight - leftHeight) * 0.5f);
        rightSprite.setPosition(columnWidth + (columnWidth - rightWidth) * 0.5f, spriteRegionTop + (spriteRegionHeight - rightHeight) * 0.5f);
        window.draw(leftSprite);
        window.draw(rightSprite);
        return scale;
    }

    void PreviewRenderer::renderComparison(sf::RenderWindow& window, const PreviewRenderData& data) const
    {
        const PreviewGenerationRecipe& pinnedRecipe = data.Comparison->Pinned.Recipe;
        const PreviewGenerationRecipe& currentRecipe = *data.Recipe;
        constexpr float columnWidth = static_cast<float>(PreviewContentWidth) * 0.5f;
        constexpr float spriteRegionTop = 62.0f;
        constexpr float spriteRegionHeight = 520.0f;

        drawPanel(window, 8.0f, 8.0f, columnWidth - 16.0f, static_cast<float>(PreviewWindowHeight) - 16.0f, sf::Color(18, 19, 24, 100), sf::Color(60, 64, 74));
        drawPanel(window, columnWidth + 8.0f, 8.0f, columnWidth - 16.0f, static_cast<float>(PreviewWindowHeight) - 16.0f, sf::Color(18, 19, 24, 100), sf::Color(60, 64, 74));
        const uint32_t scale = renderSideBySideSprites(window, pinnedRecipe, *data.PinnedTexture, "PINNED", sf::Color(235, 210, 105), currentRecipe, *data.CurrentStaticTexture, "CURRENT", sf::Color(125, 205, 235), spriteRegionTop, spriteRegionHeight);

        float pinnedY = 610.0f;
        float currentY = 610.0f;
        drawLabelValue(window, "SEED", std::to_string(pinnedRecipe.Seeds.Master), 20.0f, pinnedY);
        drawLabelValue(window, "RES", std::to_string(pinnedRecipe.Dimensions.Width) + "X" + std::to_string(pinnedRecipe.Dimensions.Height), 20.0f, pinnedY);
        drawLabelValue(window, "STYLE", getStyleName(pinnedRecipe.Style), 20.0f, pinnedY);
        drawLabelValue(window, "FACTION", getFactionName(pinnedRecipe.Faction), 20.0f, pinnedY);
        drawLabelValue(window, "ATT GEN", getOnOff(pinnedRecipe.AttachmentsEnabled), 20.0f, pinnedY);
        drawLabelValue(window, "SEED", std::to_string(currentRecipe.Seeds.Master), columnWidth + 20.0f, currentY);
        drawLabelValue(window, "RES", std::to_string(currentRecipe.Dimensions.Width) + "X" + std::to_string(currentRecipe.Dimensions.Height), columnWidth + 20.0f, currentY);
        drawLabelValue(window, "STYLE", getStyleName(currentRecipe.Style), columnWidth + 20.0f, currentY);
        drawLabelValue(window, "FACTION", getFactionName(currentRecipe.Faction), columnWidth + 20.0f, currentY);
        drawLabelValue(window, "ATT GEN", getOnOff(currentRecipe.AttachmentsEnabled), columnWidth + 20.0f, currentY);

        float differenceY = 730.0f;
        drawSectionHeader(window, "RECIPE DIFFERENCES", 20.0f, differenceY);
        drawDebugText(window, "CHANGED: " + getChangedRecipeComponents(pinnedRecipe, currentRecipe), 20.0f, differenceY, sf::Color(225, 228, 235), TextScale);
        differenceY += LargeTextLineHeight + 3.0f;
        drawDebugText(window, "STRUCT SEED: " + getSameDifferent(pinnedRecipe.Seeds.Structure == currentRecipe.Seeds.Structure) + "    PALETTE SEED: " + getSameDifferent(pinnedRecipe.Seeds.Palette == currentRecipe.Seeds.Palette), 20.0f, differenceY, sf::Color(185, 190, 205), SmallTextScale);
        differenceY += TextLineHeight;
        drawDebugText(window, "DETAIL SEED: " + getSameDifferent(pinnedRecipe.Seeds.Details == currentRecipe.Seeds.Details) + "    ATTACH SEED: " + getSameDifferent(pinnedRecipe.Seeds.Attachments == currentRecipe.Seeds.Attachments), 20.0f, differenceY, sf::Color(185, 190, 205), SmallTextScale);
        differenceY += TextLineHeight;
        drawDebugText(window, "STYLE: " + getSameDifferent(pinnedRecipe.Style == currentRecipe.Style) + "    FACTION: " + getSameDifferent(pinnedRecipe.Faction == currentRecipe.Faction) + "    RES: " + getSameDifferent(pinnedRecipe.Dimensions.Width == currentRecipe.Dimensions.Width && pinnedRecipe.Dimensions.Height == currentRecipe.Dimensions.Height), 20.0f, differenceY, sf::Color(185, 190, 205), SmallTextScale);
        differenceY += TextLineHeight;
        drawDebugText(window, "COMMON DISPLAY SCALE: " + std::to_string(scale) + "X - STATIC BASE FRAMES", 20.0f, differenceY, sf::Color(145, 150, 165), SmallTextScale);
    }

    void PreviewRenderer::renderRerollStudio(sf::RenderWindow& window, const PreviewRenderData& data) const
    {
        if (data.RerollStudio == nullptr || data.CurrentStaticTexture == nullptr)
        {
            return;
        }

        const AttributeRerollStudioState& studio = *data.RerollStudio;
        const PreviewGenerationRecipe& baseRecipe = studio.BaseRecipe;
        const PreviewGenerationRecipe& candidateRecipe = studio.CandidateValid ? studio.CandidateRecipe : studio.BaseRecipe;
        const sf::Texture& candidateTexture = studio.CandidateValid && data.RerollStudioCandidateTexture != nullptr ? *data.RerollStudioCandidateTexture : *data.CurrentStaticTexture;

        constexpr float columnWidth = static_cast<float>(PreviewContentWidth) * 0.5f;
        constexpr float spriteRegionTop = 62.0f;
        constexpr float spriteRegionHeight = 420.0f;
        drawPanel(window, 8.0f, 8.0f, columnWidth - 16.0f, 640.0f, sf::Color(18, 19, 24, 110), sf::Color(60, 64, 74));
        drawPanel(window, columnWidth + 8.0f, 8.0f, columnWidth - 16.0f, 640.0f, sf::Color(18, 19, 24, 110), sf::Color(60, 64, 74));

        const uint32_t scale = renderSideBySideSprites(window, baseRecipe, *data.CurrentStaticTexture, "ORIGINAL", sf::Color(235, 210, 105), candidateRecipe, candidateTexture, studio.CandidateValid ? "CANDIDATE" : "CANDIDATE - NOT GENERATED", sf::Color(125, 205, 235), spriteRegionTop, spriteRegionHeight);
        if (!studio.CandidateValid)
        {
            sf::RectangleShape placeholder(sf::Vector2f(columnWidth - 40.0f, spriteRegionHeight));
            placeholder.setPosition(columnWidth + 20.0f, spriteRegionTop);
            placeholder.setFillColor(sf::Color(24, 25, 30));
            window.draw(placeholder);
            drawDebugText(window, "SELECT ATTRIBUTES + REROLL", columnWidth + 116.0f, spriteRegionTop + spriteRegionHeight * 0.5f, sf::Color(145, 150, 165), SmallTextScale);
        }

        float leftY = 505.0f;
        float rightY = 505.0f;
        drawLabelValue(window, "RES", std::to_string(baseRecipe.Dimensions.Width) + "X" + std::to_string(baseRecipe.Dimensions.Height), 20.0f, leftY);
        drawLabelValue(window, "MASTER", std::to_string(baseRecipe.Seeds.Master), 20.0f, leftY);
        drawLabelValue(window, "SCALE", std::to_string(scale) + "X", 20.0f, leftY);
        drawLabelValue(window, "RES", std::to_string(candidateRecipe.Dimensions.Width) + "X" + std::to_string(candidateRecipe.Dimensions.Height), columnWidth + 20.0f, rightY);
        drawLabelValue(window, "REROLL", studio.CandidateValid ? std::to_string(studio.CandidateSequence) : "-", columnWidth + 20.0f, rightY);
        drawLabelValue(window, "SEED", studio.CandidateValid ? std::to_string(studio.CandidateRerollSeed) : "-", columnWidth + 20.0f, rightY);

        std::string selectedText;
        PixelShipGenerator::GenerationDomain firstSelected = PixelShipGenerator::GenerationDomain::GENERATION_DOMAIN_END;
        for (std::size_t index = 0u; index < studio.SelectedDomains.size(); ++index)
        {
            if (!studio.SelectedDomains[index]) { continue; }
            const PixelShipGenerator::GenerationDomain domain = static_cast<PixelShipGenerator::GenerationDomain>(index);
            if (firstSelected == PixelShipGenerator::GenerationDomain::GENERATION_DOMAIN_END) { firstSelected = domain; }
            if (!selectedText.empty()) { selectedText += ", "; }
            selectedText += PixelShipGenerator::getGenerationDomainName(domain);
        }
        if (selectedText.empty()) { selectedText = "NONE"; }

        drawDebugText(window, "SELECTED = REROLLED; UNSELECTED DOMAIN SEEDS ARE PRESERVED.", 20.0f, 575.0f, sf::Color(165, 205, 175), SmallTextScale);
        drawDebugText(window, wrapDebugText("SELECTED: " + selectedText, 102u), 20.0f, 591.0f, sf::Color(220, 223, 232), SmallTextScale);

        if (firstSelected != PixelShipGenerator::GenerationDomain::GENERATION_DOMAIN_END)
        {
            drawDebugText(window, wrapDebugText(std::string("DEPENDENCY: ") + PixelShipGenerator::getGenerationDomainDependencyDescription(firstSelected), 102u), 20.0f, 611.0f, sf::Color(170, 175, 190), SmallTextScale);
        }

        if (studio.CandidateValid)
        {
            const PixelShipGenerator::GenerationDomainSeeds baseSeeds = PixelShipGenerator::resolveGenerationDomainSeeds(baseRecipe.Seeds, baseRecipe.DomainSeedOverrides, baseRecipe.RandomStreamMode);
            const PixelShipGenerator::GenerationDomainSeeds candidateSeeds = PixelShipGenerator::resolveGenerationDomainSeeds(candidateRecipe.Seeds, candidateRecipe.DomainSeedOverrides, candidateRecipe.RandomStreamMode);
            std::string seedText = "DOMAIN SEEDS:";
            uint32_t shown = 0u;
            for (std::size_t index = 0u; index < studio.SelectedDomains.size() && shown < 3u; ++index)
            {
                if (!studio.SelectedDomains[index]) { continue; }
                const PixelShipGenerator::GenerationDomain domain = static_cast<PixelShipGenerator::GenerationDomain>(index);
                seedText += " ";
                seedText += PixelShipGenerator::getGenerationDomainName(domain);
                seedText += " ";
                seedText += std::to_string(baseSeeds.Values[index]);
                seedText += "->";
                seedText += std::to_string(candidateSeeds.Values[index]);
                ++shown;
                if (shown < 3u) { seedText += ";"; }
            }
            if (getSelectedAttributeRerollDomains(studio).size() > shown) { seedText += "; ..."; }
            drawDebugText(window, wrapDebugText(seedText, 102u), 20.0f, 637.0f, sf::Color(145, 170, 195), SmallTextScale);
        }

        const auto drawNative = [&](float panelX, const sf::Texture& texture, const PreviewGenerationRecipe& recipe, const std::string& label, bool valid)
            {
                constexpr float panelY = 690.0f;
                constexpr float panelWidth = 280.0f;
                constexpr float panelHeight = 300.0f;
                constexpr float spriteAreaSize = 256.0f;
                drawPanel(window, panelX, panelY, panelWidth, panelHeight, sf::Color(30, 31, 35, 248), sf::Color(92, 96, 106));
                drawDebugText(window, label + ": " + std::to_string(recipe.Dimensions.Width) + "X" + std::to_string(recipe.Dimensions.Height), panelX + 12.0f, panelY + 10.0f, sf::Color(224, 226, 232), TextScale);
                sf::RectangleShape background(sf::Vector2f(spriteAreaSize, spriteAreaSize));
                background.setPosition(panelX + 12.0f, panelY + 32.0f);
                background.setFillColor(sf::Color(48, 49, 54));
                window.draw(background);
                if (!valid)
                {
                    drawDebugText(window, "REROLL TO CREATE", panelX + 74.0f, panelY + 152.0f, sf::Color(145, 150, 165), SmallTextScale);
                    return;
                }
                const uint32_t horizontalOffset = recipe.Dimensions.Width < 256u ? (256u - recipe.Dimensions.Width) / 2u : 0u;
                const uint32_t verticalOffset = recipe.Dimensions.Height < 256u ? (256u - recipe.Dimensions.Height) / 2u : 0u;
                sf::Sprite nativeSprite(texture);
                nativeSprite.setScale(1.0f, 1.0f);
                nativeSprite.setPosition(panelX + 12.0f + static_cast<float>(horizontalOffset), panelY + 32.0f + static_cast<float>(verticalOffset));
                window.draw(nativeSprite);
            };

        drawNative((columnWidth - 280.0f) * 0.5f, *data.CurrentStaticTexture, baseRecipe, "NATIVE ORIGINAL", true);
        drawNative(columnWidth + (columnWidth - 280.0f) * 0.5f, candidateTexture, candidateRecipe, "NATIVE CANDIDATE", studio.CandidateValid);
    }

    void PreviewRenderer::renderFavorites(sf::RenderWindow& window, const FavoritesState& favoritesState) const
    {
        renderThumbnailGrid(window, favoritesState.Grid);
    }

    void PreviewRenderer::renderGallery(sf::RenderWindow& window, const GalleryState& galleryState) const
    {
        renderThumbnailGrid(window, galleryState.Grid);
    }

    void PreviewRenderer::renderThumbnailGrid(sf::RenderWindow& window, const PreviewThumbnailGridState& gridState) const
    {
        const uint32_t pageStart = getPreviewThumbnailPageStart(gridState);
        const uint32_t pageEnd = std::min(static_cast<uint32_t>(gridState.Items.size()), pageStart + getPreviewThumbnailPageCapacity(gridState));

        for (uint32_t index = pageStart; index < pageEnd; ++index)
        {
            const PreviewThumbnailItem& item = gridState.Items[index];
            const sf::FloatRect cellBounds = getPreviewThumbnailCellBounds(gridState, index);
            sf::RectangleShape cellOutline;
            cellOutline.setPosition(cellBounds.left, cellBounds.top);
            cellOutline.setSize(sf::Vector2f(cellBounds.width, cellBounds.height));
            cellOutline.setFillColor(sf::Color::Transparent);
            cellOutline.setOutlineThickness(index == gridState.SelectedIndex ? 2.0f : 1.0f);

            if (!item.Valid) { cellOutline.setOutlineColor(sf::Color(120, 60, 60)); }
            else if (index == gridState.SelectedIndex) { cellOutline.setOutlineColor(sf::Color(220, 220, 100)); }
            else if (static_cast<int32_t>(index) == gridState.HoveredIndex) { cellOutline.setOutlineColor(sf::Color(120, 175, 225)); }
            else { cellOutline.setOutlineColor(sf::Color(70, 70, 80)); }

            window.draw(cellOutline);

            if (item.Valid)
            {
                const float thumbnailScale = calculatePreviewThumbnailScale(item.Recipe.Dimensions.Width, item.Recipe.Dimensions.Height, cellBounds);
                sf::Sprite thumbnailSprite(item.Texture);
                thumbnailSprite.setScale(static_cast<float>(thumbnailScale), static_cast<float>(thumbnailScale));
                thumbnailSprite.setPosition(calculatePreviewThumbnailPosition(item.Recipe.Dimensions.Width, item.Recipe.Dimensions.Height, thumbnailScale, cellBounds));
                window.draw(thumbnailSprite);
            }
        }
    }

    void PreviewRenderer::renderSingle(sf::RenderWindow& window, const sf::Sprite& previewSprite) const
    {
        window.draw(previewSprite);
    }

    void PreviewRenderer::renderNativePreview(sf::RenderWindow& window, const PreviewRenderData& data) const
    {
        if (data.NativePreviewTexture == nullptr || data.Recipe == nullptr)
        {
            return;
        }

        const float panelX = static_cast<float>(NativePreviewPanelX);
        const float panelY = static_cast<float>(NativePreviewPanelY);
        drawPanel(window, panelX, panelY, static_cast<float>(NativePreviewPanelWidth), static_cast<float>(NativePreviewPanelHeight), sf::Color(30, 31, 35, 248), sf::Color(92, 96, 106));

        const PixelShipGenerator::ShipDimensions dimensions = data.Recipe->Dimensions;
        const std::string dimensionLabel = "NATIVE: " + std::to_string(dimensions.Width) + "x" + std::to_string(dimensions.Height);
        drawDebugText(window, dimensionLabel, panelX + 12.0f, panelY + 10.0f, sf::Color(224, 226, 232), TextScale);

        constexpr float spriteAreaPadding = 12.0f;
        constexpr float labelAreaHeight = 28.0f;
        const float spriteAreaX = panelX + spriteAreaPadding;
        const float spriteAreaY = panelY + labelAreaHeight + 4.0f;
        const float spriteAreaSize = static_cast<float>(NativePreviewSpriteAreaSize);

        sf::RectangleShape spriteBackground(sf::Vector2f(spriteAreaSize, spriteAreaSize));
        spriteBackground.setPosition(spriteAreaX, spriteAreaY);
        spriteBackground.setFillColor(sf::Color(44, 45, 49));
        spriteBackground.setOutlineThickness(1.0f);
        spriteBackground.setOutlineColor(sf::Color(72, 75, 83));
        window.draw(spriteBackground);

        const uint32_t horizontalOffset = dimensions.Width < NativePreviewSpriteAreaSize ? (NativePreviewSpriteAreaSize - dimensions.Width) / 2u : 0u;
        const uint32_t verticalOffset = dimensions.Height < NativePreviewSpriteAreaSize ? (NativePreviewSpriteAreaSize - dimensions.Height) / 2u : 0u;
        const float spriteX = spriteAreaX + static_cast<float>(horizontalOffset);
        const float spriteY = spriteAreaY + static_cast<float>(verticalOffset);
        sf::Sprite nativeSprite(*data.NativePreviewTexture);
        nativeSprite.setScale(1.0f, 1.0f);
        nativeSprite.setPosition(spriteX, spriteY);
        window.draw(nativeSprite);
    }

    void PreviewRenderer::renderPersistentStatePanel(sf::RenderWindow& window, const PreviewRenderData& data) const
    {
        drawPanel(window, StatePanelX, 0.0f, static_cast<float>(PreviewStatePanelWidth), static_cast<float>(PreviewWindowHeight), sf::Color(15, 16, 20, 245), sf::Color(65, 68, 78));
        float x = StatePanelX + PanelPadding;
        float y = 14.0f;
        drawSectionHeader(window, "CURRENT STATE", x, y);

        drawLabelValue(window, "MODE", getPreviewModeName(data.Mode), x, y);

        if (data.Mode == PreviewMode::CALIBRATION && data.CalibrationSession != nullptr)
        {
            const CalibrationGroupStatistics statistics = calculateCalibrationGroupStatistics(*data.CalibrationSession, data.CalibrationGroup, data.CalibrationFilter);
            const std::vector<uint32_t> suggested = calculateSuggestedGroupWeights(*data.CalibrationSession, data.Recipe != nullptr ? data.Recipe->Style : PixelShipGenerator::ShipStyle::FIGHTER, data.CalibrationGroup, data.CalibrationFilter);
            drawLabelValue(window, "GROUP", getCalibrationGroupName(data.CalibrationGroup), x, y);
            drawLabelValue(window, "EVIDENCE", getCalibrationEvidenceName(statistics.Evidence), x, y);
            drawLabelValue(window, "USEFUL", std::to_string(statistics.UsefulComparisonCount), x, y);
            drawLabelValue(window, "RECORDS", std::to_string(data.CalibrationSession->Records.size()), x, y);
            drawLabelValue(window, "FILTER", data.CalibrationFilter.Style.has_value() ? "CURRENT CONTEXT" : "ALL", x, y);
            drawLabelValue(window, "VALUES", data.CalibrationShowValues ? "VISIBLE" : "BLIND", x, y);

            y += 4.0f;
            drawSectionHeader(window, "PREFERENCE", x, y);
            const uint32_t optionCount = PixelShipGenerator::getGenerationWeightOptionCount(data.CalibrationGroup);
            const PixelShipGenerator::ShipStyle style = data.Recipe != nullptr ? data.Recipe->Style : PixelShipGenerator::ShipStyle::FIGHTER;
            for (uint32_t index = 0u; index < optionCount && index < statistics.Options.size(); ++index)
            {
                const CalibrationOptionStatistics& option = statistics.Options[index];
                const uint32_t current = PixelShipGenerator::getGenerationTuningWeight(data.CalibrationSession->TunedProfile, style, data.CalibrationGroup, index);
                const uint32_t proposed = index < suggested.size() ? suggested[index] : current;
                drawDebugText(window, getCalibrationOptionName(data.CalibrationGroup, index), x, y, sf::Color(215, 220, 230), SmallTextScale);
                y += 12.0f;
                drawDebugText(window, "W" + std::to_string(option.Wins) + " L" + std::to_string(option.Losses) + " T" + std::to_string(option.Ties) + "  CUR " + std::to_string(current) + " SUG " + std::to_string(proposed), x + 6.0f, y, sf::Color(135, 175, 205), SmallTextScale);
                y += 14.0f;
            }

            if (data.CalibrationPair != nullptr && data.CalibrationPair->Valid)
            {
                y += 4.0f;
                drawSectionHeader(window, "OBJECTIVE A/B", x, y);
                const auto drawObjective = [&](const char* label, const PixelShipGenerator::ShipGenerationDebugInfo& info)
                    {
                        drawDebugText(window, std::string(label) + " ENG " + std::to_string(info.EngineCount) + " FEAT " + std::to_string(info.MajorFeatureCount) + " GUN " + std::to_string(info.WeaponCount) + " ATT " + std::to_string(info.AttachmentPlacedGroupCount), x, y, sf::Color(180, 185, 195), SmallTextScale);
                        y += 14.0f;
                    };
                const PixelShipGenerator::ShipGenerationDebugInfo& left = data.CalibrationPair->DisplayAOnLeft ? data.CalibrationPair->DebugA : data.CalibrationPair->DebugB;
                const PixelShipGenerator::ShipGenerationDebugInfo& right = data.CalibrationPair->DisplayAOnLeft ? data.CalibrationPair->DebugB : data.CalibrationPair->DebugA;
                drawObjective("A", left);
                drawObjective("B", right);
            }

            if (data.ObjectiveBatch != nullptr && data.ObjectiveBatch->Valid)
            {
                const auto& objective = *data.ObjectiveBatch;
                y += 4.0f;
                drawSectionHeader(window, "TASK33 BATCH", x, y);
                drawDebugText(window, "PROD -> TUNED", x, y, sf::Color(180, 185, 195), SmallTextScale); y += 14.0f;
                drawDebugText(window, "ENG " + std::to_string(objective.Production.EngineCount.average()) + " -> " + std::to_string(objective.Tuned.EngineCount.average()), x, y, sf::Color(150, 180, 205), SmallTextScale); y += 14.0f;
                drawDebugText(window, "ATT " + std::to_string(objective.Production.AttachmentGroupCount.average()) + " -> " + std::to_string(objective.Tuned.AttachmentGroupCount.average()), x, y, sf::Color(150, 180, 205), SmallTextScale); y += 14.0f;
                drawDebugText(window, "HULL MOD " + std::to_string(objective.Production.HullModifierCount.average()) + " -> " + std::to_string(objective.Tuned.HullModifierCount.average()), x, y, sf::Color(150, 180, 205), SmallTextScale); y += 14.0f;
            }

            if (data.StatusMessage != nullptr && !data.StatusMessage->empty())
            {
                y += 4.0f;
                drawSectionHeader(window, "STATUS", x, y);
                drawDebugText(window, wrapDebugText(*data.StatusMessage, 42u), x, y, sf::Color(210, 215, 225), SmallTextScale);
            }
            return;
        }

        if (data.Comparison != nullptr)
        {
            drawLabelValue(window, "COMPARE", data.Comparison->ViewEnabled && data.Comparison->Pinned.Valid ? "ON" : "OFF", x, y);
            if (data.Comparison->Pinned.Valid) { drawLabelValue(window, "PIN SEED", std::to_string(data.Comparison->Pinned.Recipe.Seeds.Master), x, y); }
        }

        drawLabelValue(window, "FAVORITE", data.CurrentIsFavorite ? "YES" : "NO", x, y);
        if (data.Favorites != nullptr) { drawLabelValue(window, "FAVORITES", std::to_string(data.Favorites->Grid.Items.size()), x, y); }

        if (data.Diagnostics != nullptr)
        {
            drawLabelValue(window, "VIEW", data.Diagnostics->GenerationStageView ? "GEN STAGE" : getDiagnosticViewName(data.Diagnostics->ViewMode), x, y);

            if (!data.Diagnostics->GenerationStageView && data.Diagnostics->ViewMode != DiagnosticViewMode::FINAL)
            {
                y += 2.0f;
                drawSectionHeader(window, "DEBUG COLORS", x, y);

                switch (data.Diagnostics->ViewMode)
                {
                case DiagnosticViewMode::HULL:
                    drawDiagnosticLegendEntry(window, "HULL", PreviewDiagnosticColors::Hull, x, y);
                    break;
                case DiagnosticViewMode::COCKPIT:
                    drawDiagnosticLegendEntry(window, "COCKPIT", PreviewDiagnosticColors::Cockpit, x, y);
                    break;
                case DiagnosticViewMode::ENGINES:
                    drawDiagnosticLegendEntry(window, "ENGINE", PreviewDiagnosticColors::Engine, x, y);
                    drawDiagnosticLegendEntry(window, "EXHAUST", PreviewDiagnosticColors::Exhaust, x, y);
                    break;
                case DiagnosticViewMode::DETAILS:
                    drawDiagnosticLegendEntry(window, "ACCENT", PreviewDiagnosticColors::Accent, x, y);
                    drawDiagnosticLegendEntry(window, "MECHANICAL", PreviewDiagnosticColors::Mechanical, x, y);
                    drawDiagnosticLegendEntry(window, "LIGHT", PreviewDiagnosticColors::Light, x, y);
                    break;
                case DiagnosticViewMode::ATTACHMENTS:
                    drawDiagnosticLegendEntry(window, "ATTACHMENT", PreviewDiagnosticColors::Attachment, x, y);
                    break;
                case DiagnosticViewMode::HULL_LAYERS:
                    drawDiagnosticLegendEntry(window, "LOWER LAYER", PreviewDiagnosticColors::HullLayerLower, x, y);
                    drawDiagnosticLegendEntry(window, "UPPER LAYER", PreviewDiagnosticColors::HullLayerUpper, x, y);
                    break;
                case DiagnosticViewMode::CORE_TREATMENT:
                    drawDiagnosticLegendEntry(window, "CORE REGION", PreviewDiagnosticColors::CoreRegion, x, y);
                    drawDiagnosticLegendEntry(window, "MATERIAL ZONE", PreviewDiagnosticColors::CoreSecondary, x, y);
                    drawDiagnosticLegendEntry(window, "RAISED", PreviewDiagnosticColors::CoreRaised, x, y);
                    drawDiagnosticLegendEntry(window, "RECESSED", PreviewDiagnosticColors::CoreRecessed, x, y);
                    drawDiagnosticLegendEntry(window, "LUMINOUS", PreviewDiagnosticColors::CoreLuminous, x, y);
                    break;
                case DiagnosticViewMode::SEMANTIC_LOAD:
                    drawDiagnosticLegendEntry(window, "LOW LOAD", PreviewDiagnosticColors::SpatialLow, x, y);
                    drawDiagnosticLegendEntry(window, "MODERATE", PreviewDiagnosticColors::SpatialModerate, x, y);
                    drawDiagnosticLegendEntry(window, "HIGH", PreviewDiagnosticColors::SpatialHigh, x, y);
                    drawDiagnosticLegendEntry(window, "OVERLOADED", PreviewDiagnosticColors::SpatialOverloaded, x, y);
                    break;
                case DiagnosticViewMode::MACRO_ASYMMETRY:
                    drawDiagnosticLegendEntry(window, "BASE HULL", PreviewDiagnosticColors::MacroAsymmetryBase, x, y);
                    drawDiagnosticLegendEntry(window, "ASYMMETRIC FEATURE", PreviewDiagnosticColors::MacroAsymmetryFeature, x, y);
                    break;
                case DiagnosticViewMode::COMBINED:
                    drawDiagnosticLegendEntry(window, "HULL", PreviewDiagnosticColors::Hull, x, y);
                    drawDiagnosticLegendEntry(window, "COCKPIT", PreviewDiagnosticColors::Cockpit, x, y);
                    drawDiagnosticLegendEntry(window, "ENGINE", PreviewDiagnosticColors::Engine, x, y);
                    drawDiagnosticLegendEntry(window, "EXHAUST", PreviewDiagnosticColors::Exhaust, x, y);
                    drawDiagnosticLegendEntry(window, "ACCENT", PreviewDiagnosticColors::Accent, x, y);
                    drawDiagnosticLegendEntry(window, "MECHANICAL", PreviewDiagnosticColors::Mechanical, x, y);
                    drawDiagnosticLegendEntry(window, "LIGHT", PreviewDiagnosticColors::Light, x, y);
                    drawDiagnosticLegendEntry(window, "ATTACHMENT", PreviewDiagnosticColors::Attachment, x, y);
                    drawDiagnosticLegendEntry(window, "OVERLAP", PreviewDiagnosticColors::Overlap, x, y);
                    break;
                default:
                    break;
                }

                y += 3.0f;
            }
        }

        if (data.Mode == PreviewMode::GALLERY && data.Gallery != nullptr)
        {
            drawLabelValue(window, "BATCH", std::to_string(data.Gallery->BatchSeed), x, y);
            drawLabelValue(window, "SELECT", data.Gallery->Grid.Items.empty() ? "0/0" : std::to_string(data.Gallery->Grid.SelectedIndex + 1u) + "/" + std::to_string(data.Gallery->Grid.Items.size()), x, y);
        }

        if (data.Mode == PreviewMode::FAVORITES && data.Favorites != nullptr)
        {
            const PreviewThumbnailGridState& grid = data.Favorites->Grid;
            const uint32_t pageCount = getPreviewThumbnailPageCount(grid);
            drawLabelValue(window, "SELECT", grid.Items.empty() ? "0/0" : std::to_string(grid.SelectedIndex + 1u) + "/" + std::to_string(grid.Items.size()), x, y);
            drawLabelValue(window, "PAGE", pageCount == 0u ? "0/0" : std::to_string(getPreviewThumbnailCurrentPage(grid) + 1u) + "/" + std::to_string(pageCount), x, y);

            if (!grid.Items.empty() && grid.SelectedIndex < grid.Items.size())
            {
                const PreviewGenerationRecipe& favoriteRecipe = grid.Items[grid.SelectedIndex].Recipe;
                drawLabelValue(window, "FAV SEED", std::to_string(favoriteRecipe.Seeds.Master), x, y);
                drawLabelValue(window, "FAV RES", std::to_string(favoriteRecipe.Dimensions.Width) + "X" + std::to_string(favoriteRecipe.Dimensions.Height), x, y);
                drawLabelValue(window, "FAV STYLE", getStyleName(favoriteRecipe.Style), x, y);
                drawLabelValue(window, "FAV FACT", getFactionName(favoriteRecipe.Faction), x, y);
            }
        }

        if (data.Recipe != nullptr)
        {
            drawLabelValue(window, "RES", std::to_string(data.Recipe->Dimensions.Width) + "X" + std::to_string(data.Recipe->Dimensions.Height), x, y);
            drawLabelValue(window, "STYLE", getStyleName(data.Recipe->Style), x, y);
            drawLabelValue(window, "FACTION", getFactionName(data.Recipe->Faction), x, y);
            drawLabelValue(window, "MASTER", std::to_string(data.Recipe->Seeds.Master), x, y);
            drawLabelValue(window, "STRUCT", std::to_string(data.Recipe->Seeds.Structure), x, y);
            drawLabelValue(window, "PALETTE", std::to_string(data.Recipe->Seeds.Palette), x, y);
            drawLabelValue(window, "DETAILS", std::to_string(data.Recipe->Seeds.Details), x, y);
            drawLabelValue(window, "ATTACH", std::to_string(data.Recipe->Seeds.Attachments), x, y);
            drawLabelValue(window, "ATT GEN", getOnOff(data.Recipe->AttachmentsEnabled), x, y);
        }

        if (data.Locks != nullptr)
        {
            y += 4.0f;
            drawSectionHeader(window, "CHANNEL LOCKS", x, y);
            drawLabelValue(window, "STRUCT", getLockState(data.Locks->Structure), x, y);
            drawLabelValue(window, "PALETTE", getLockState(data.Locks->Palette), x, y);
            drawLabelValue(window, "DETAILS", getLockState(data.Locks->Details), x, y);
            drawLabelValue(window, "ATTACH", getLockState(data.Locks->Attachments), x, y);
        }

        drawLabelValue(window, "HISTORY", data.HistoryCount == 0u ? "0/0" : std::to_string(data.HistoryIndex + 1u) + "/" + std::to_string(data.HistoryCount), x, y);

        if (data.StatusMessage != nullptr && !data.StatusMessage->empty())
        {
            y += 4.0f;
            drawSectionHeader(window, "STATUS", x, y);
            const std::string status = data.StatusMessage->size() > 42u ? data.StatusMessage->substr(0u, 39u) + "..." : *data.StatusMessage;
            drawDebugText(window, status, x, y, sf::Color(210, 215, 225), SmallTextScale);
            y += 16.0f;
        }

        if (data.SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE && data.IdleAnimation != nullptr && data.IdleAnimationSettings != nullptr && !data.IdleAnimation->Frames.empty())
        {
            y += 4.0f;
            drawSectionHeader(window, "IDLE ANIMATION", x, y);
            drawLabelValue(window, "TYPE", getAnimationTypeDisplayName(data.SelectedAnimationType), x, y);
            drawLabelValue(window, "SEED", std::to_string(data.IdleAnimation->Seed), x, y);
            drawLabelValue(window, "DURATION", std::to_string(data.IdleAnimation->DurationMilliseconds) + " ms", x, y);
            drawLabelValue(window, "FRAMES", std::to_string(data.IdleAnimation->Sampling.ActualFrameCount), x, y);
            drawLabelValue(window, "SAMPLING", data.IdleAnimation->Sampling.Mode == PixelShipGenerator::AnimationSamplingMode::ADAPTIVE ? "ADAPTIVE" : "EXACT", x, y);
            drawLabelValue(window, "FRAME LIMITS", std::to_string(data.IdleAnimation->Sampling.MinimumFrameCount) + "-" + std::to_string(data.IdleAnimation->Sampling.MaximumFrameCount), x, y);
            drawLabelValue(window, "FRAME TIME", getMillisecondsString(data.IdleAnimation->FrameDurationMilliseconds), x, y);
            drawLabelValue(window, "FRAME", std::to_string(data.AnimationFrameIndex + 1u) + "/" + std::to_string(data.IdleAnimation->Frames.size()), x, y);
            drawLabelValue(window, "COMPONENTS", std::to_string(data.IdleAnimation->Sampling.ActiveAnimatedComponentCount), x, y);
            drawLabelValue(window, "MAX TRAVEL", std::to_string(std::max(data.IdleAnimation->Sampling.MaximumMechanicalTravelPixels, data.IdleAnimation->Sampling.MaximumExhaustTravelPixels)) + " px", x, y);
            drawLabelValue(window, "PHASE GROUPS", std::to_string(data.IdleAnimation->Sampling.IndependentPhaseGroupCount), x, y);
            drawLabelValue(window, "PLAYBACK", data.Mode == PreviewMode::ANIMATION ? "PLAY" : data.Mode == PreviewMode::FRAME_INSPECTION ? "PAUSED" : "STATIC", x, y);
            drawLabelValue(window, "ENGINE", getOnOff(data.IdleAnimationSettings->EngineFlicker), x, y);
            drawLabelValue(window, "LIGHTS", getOnOff(data.IdleAnimationSettings->LightBlinking), x, y);
            drawLabelValue(window, "MICRO", getOnOff(data.IdleAnimationSettings->MechanicalMicroMovement), x, y);
            drawLabelValue(window, "HOVER", getOnOff(data.IdleAnimationSettings->HoverOffset), x, y);
            drawLabelValue(window, "DETAIL FX", getOnOff(data.IdleAnimationSettings->SmallDetailVariation), x, y);
        }
        else if (data.SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE && data.FiringAnimation != nullptr && data.FiringAnimationSettings != nullptr && !data.FiringAnimation->Frames.empty())
        {
            y += 4.0f;
            drawSectionHeader(window, "FIRING ANIMATION", x, y);
            drawLabelValue(window, "TYPE", "FIRE", x, y);
            drawLabelValue(window, "SEED", std::to_string(data.FiringAnimation->Seed), x, y);
            drawLabelValue(window, "TARGET", std::to_string(data.FiringAnimation->Target.WeaponComponentIndex), x, y);
            drawLabelValue(window, "PHASE", getCurrentFiringPhaseDisplayName(*data.FiringAnimation, data.AnimationFrameIndex), x, y);
            drawLabelValue(window, "GROUP", std::to_string(data.FiringAnimation->Diagnostics.TargetSymmetryGroup), x, y);
            drawLabelValue(window, "DURATION", std::to_string(data.FiringAnimation->DurationMilliseconds) + " ms", x, y);
            drawLabelValue(window, "FRAMES", std::to_string(data.FiringAnimation->Sampling.ActualFrameCount), x, y);
            drawLabelValue(window, "SAMPLING", data.FiringAnimation->Sampling.Mode == PixelShipGenerator::AnimationSamplingMode::ADAPTIVE ? "ADAPTIVE" : "EXACT", x, y);
            drawLabelValue(window, "FRAME TIME", getMillisecondsString(data.FiringAnimation->FrameDurationMilliseconds), x, y);
            drawLabelValue(window, "FRAME", std::to_string(data.AnimationFrameIndex + 1u) + "/" + std::to_string(data.FiringAnimation->Frames.size()), x, y);
            drawLabelValue(window, "WEAPONS", std::to_string(data.FiringAnimation->Diagnostics.ActiveWeaponCount), x, y);
            drawLabelValue(window, "RECOIL", std::to_string(data.FiringAnimation->Diagnostics.MaximumRecoilTravelPixels) + " px", x, y);
            drawLabelValue(window, "PRE-FIRE", std::to_string(data.FiringAnimation->Diagnostics.MaximumPreFireExtensionPixels) + " px", x, y);
            if (data.TransientStatePreviewActive)
            {
                drawLabelValue(window, "UNDERLYING", getAnimationTypeDisplayName(data.RuntimeMovementType), x, y);
                drawLabelValue(window, "OVERRIDES", std::to_string(data.FiringAnimation->Diagnostics.ActiveWeaponCount) + " weapon", x, y);
            }
            drawLabelValue(window, "PLAYBACK", data.Mode == PreviewMode::ANIMATION ? "PLAY" : data.Mode == PreviewMode::FRAME_INSPECTION ? "PAUSED" : "STATIC", x, y);
        }
        else if (data.MovementAnimation != nullptr && data.MovementAnimationSettings != nullptr && data.MovementAnimation->Type == data.SelectedAnimationType)
        {
            const PixelShipGenerator::ShipMovementAnimationClip& clip = PixelShipGenerator::getMovementAnimationClip(*data.MovementAnimation, data.MovementPhase);
            if (!clip.Frames.empty())
            {
                y += 4.0f;
                drawSectionHeader(window, "MOVEMENT ANIMATION", x, y);
                drawLabelValue(window, "TYPE", getAnimationTypeDisplayName(data.SelectedAnimationType), x, y);
                drawLabelValue(window, "PHASE", getMovementPhaseDisplayName(data.MovementPhase), x, y);
                if (data.MovementTransitionPending) { drawLabelValue(window, "NEXT", getAnimationTypeDisplayName(data.PendingMovementType), x, y); }
                drawLabelValue(window, "SEED", std::to_string(data.MovementAnimation->Seed), x, y);
                drawLabelValue(window, "DURATION", std::to_string(clip.DurationMilliseconds) + " ms", x, y);
                drawLabelValue(window, "FRAMES", std::to_string(clip.Sampling.ActualFrameCount), x, y);
                drawLabelValue(window, "SAMPLING", clip.Sampling.Mode == PixelShipGenerator::AnimationSamplingMode::ADAPTIVE ? "ADAPTIVE" : "EXACT", x, y);
                drawLabelValue(window, "FRAME LIMITS", std::to_string(clip.Sampling.MinimumFrameCount) + "-" + std::to_string(clip.Sampling.MaximumFrameCount), x, y);
                drawLabelValue(window, "FRAME TIME", getMillisecondsString(clip.FrameDurationMilliseconds), x, y);
                drawLabelValue(window, "FRAME", std::to_string(data.AnimationFrameIndex + 1u) + "/" + std::to_string(clip.Frames.size()), x, y);
                drawLabelValue(window, "COMPONENTS", std::to_string(clip.Sampling.ActiveAnimatedComponentCount), x, y);
                drawLabelValue(window, "MAX TRAVEL", std::to_string(data.MovementAnimation->Diagnostics.MaximumMechanicalTravelPixels) + " px", x, y);
                drawLabelValue(window, "EXHAUST TRAVEL", std::to_string(data.MovementAnimation->Diagnostics.MaximumExhaustTravelPixels) + " px", x, y);
                drawLabelValue(window, "PHASE GROUPS", std::to_string(data.MovementAnimation->Diagnostics.IndependentPhaseGroupCount), x, y);
                drawLabelValue(window, "PLAYBACK", data.Mode == PreviewMode::ANIMATION ? "PLAY" : data.Mode == PreviewMode::FRAME_INSPECTION ? "PAUSED" : "STATIC", x, y);
                drawLabelValue(window, "ENGINES", std::to_string(data.MovementAnimation->Diagnostics.ActiveEngineCount), x, y);
                drawLabelValue(window, "WEAPONS", std::to_string(data.MovementAnimation->Diagnostics.ActiveWeaponCount), x, y);
                drawLabelValue(window, "ATTACH", std::to_string(data.MovementAnimation->Diagnostics.ActiveAttachmentCount), x, y);
                drawLabelValue(window, "BRAKES", std::to_string(data.MovementAnimation->Diagnostics.ActiveBrakingComponentCount), x, y);
            }
        }

        if (data.Diagnostics != nullptr && data.Diagnostics->GenerationStageView && data.GenerationDebugInfo != nullptr && !data.GenerationDebugInfo->HullStages.empty())
        {
            const uint32_t stageIndex = std::min(data.Diagnostics->GenerationStageIndex, static_cast<uint32_t>(data.GenerationDebugInfo->HullStages.size() - 1u));
            y += 4.0f;
            drawSectionHeader(window, "GENERATION STAGE", x, y);
            drawLabelValue(window, "STAGE", std::to_string(stageIndex + 1u) + "/" + std::to_string(data.GenerationDebugInfo->HullStages.size()), x, y);
            drawDebugText(window, getDebugStageName(data.GenerationDebugInfo->HullStages[stageIndex].Type), x, y, sf::Color(230, 232, 238), TextScale);
        }
    }

    void PreviewRenderer::renderHelpOverlay(sf::RenderWindow& window) const
    {
        const float width = static_cast<float>(PreviewCommandPanelX) - OverlayMargin * 2.0f;
        const float height = static_cast<float>(PreviewWindowHeight) - OverlayMargin * 2.0f;
        drawPanel(window, OverlayMargin, OverlayMargin, width, height, sf::Color(8, 9, 12, 248), sf::Color(120, 125, 145));
        drawDebugText(window, "HELP / CONTROLS - F5 OR ESCAPE TO CLOSE", OverlayMargin + 16.0f, OverlayMargin + 14.0f, sf::Color(240, 215, 105), TextScale);

        std::vector<const PreviewCommandData*> helpEntries;

        for (const PreviewCommandData& commandData : getPreviewCommandDataTable())
        {
            if (commandData.Shortcut[0] != '\0') { helpEntries.push_back(&commandData); }
        }

        const std::size_t splitIndex = (helpEntries.size() + 1u) / 2u;
        const float leftX = OverlayMargin + 18.0f;
        const float rightX = OverlayMargin + width * 0.51f;
        float leftY = OverlayMargin + 48.0f;
        float rightY = leftY;

        for (std::size_t index = 0u; index < helpEntries.size(); ++index)
        {
            const bool rightColumn = index >= splitIndex;
            const float x = rightColumn ? rightX : leftX;
            float& y = rightColumn ? rightY : leftY;
            const PreviewCommandData& commandData = *helpEntries[index];
            const std::string description = wrapDebugText(commandData.Description, 45u);
            const uint32_t descriptionLineCount = 1u + static_cast<uint32_t>(std::count(description.begin(), description.end(), '\n'));
            drawDebugText(window, commandData.Shortcut, x, y, sf::Color(125, 205, 235), TextScale);
            drawDebugText(window, description, x + 190.0f, y, sf::Color(225, 228, 235), TextScale);
            y += 28.0f + static_cast<float>((descriptionLineCount - 1u) * 14u);
        }
    }

    void PreviewRenderer::renderGenerationInspector(sf::RenderWindow& window, const PreviewRenderData& data) const
    {
        if (data.Ship == nullptr || data.GenerationDebugInfo == nullptr)
        {
            return;
        }

        drawPanel(window, OverlayMargin, OverlayMargin, static_cast<float>(PreviewContentWidth) - OverlayMargin * 2.0f, static_cast<float>(PreviewWindowHeight) - OverlayMargin * 2.0f, sf::Color(8, 9, 12, 248), sf::Color(120, 125, 145));
        const PixelShipGenerator::GeneratedShip& ship = *data.Ship;
        const PixelShipGenerator::ShipGenerationDebugInfo& debug = *data.GenerationDebugInfo;
        float x = OverlayMargin + 16.0f;
        float y = OverlayMargin + 14.0f;
        drawSectionHeader(window, "GENERATION INSPECTOR - F6 OR ESCAPE TO CLOSE", x, y);

        const Bounds finalBounds = calculateImageBounds(ship.FinalImage, ship.HullMask.getWidth(), ship.HullMask.getHeight());
        const Bounds hullBounds = calculateMaskBounds(ship.HullMask);
        const Bounds cockpitBounds = calculateMaskBounds(ship.CockpitMask);
        drawLabelValue(window, "WIDTH", std::to_string(ship.FinalImage.getWidth()), x, y);
        drawLabelValue(window, "HEIGHT", std::to_string(ship.FinalImage.getHeight()), x, y);
        drawLabelValue(window, "ASPECT", getAspectRatioString(ship.FinalImage.getWidth(), ship.FinalImage.getHeight()), x, y);
        drawLabelValue(window, "ATTEMPTS", std::to_string(debug.HullGenerationAttemptCount), x, y);
        drawLabelValue(window, "PRIMARY ANCHOR", PixelShipGenerator::getShipVisualAnchorTypeName(debug.PrimaryVisualAnchor), x, y);
        drawLabelValue(window, "SECONDARY ANCHOR", PixelShipGenerator::getShipVisualAnchorTypeName(debug.SecondaryVisualAnchor), x, y);
        drawLabelValue(window, "ANCHOR REGION", PixelShipGenerator::getGenerationSpatialRegionName(debug.VisualAnchorTargetRegion), x, y);
        drawLabelValue(window, "ANCHOR RESERVE", std::to_string(debug.VisualHierarchyReservedComplexity), x, y);
        drawLabelValue(window, "ANCHOR FALLBACK", debug.VisualHierarchyFallbackOccurred ? "YES" : "NO", x, y);
        drawLabelValue(window, "HULL USE", std::to_string(debug.SilhouetteMetrics.NormalizedWidthPercent) + "% W / " + std::to_string(debug.SilhouetteMetrics.NormalizedHeightPercent) + "% H", x, y);
        drawLabelValue(window, "BOUNDS FILL", std::to_string(debug.SilhouetteMetrics.BoundingFillPercent) + "%", x, y);
        drawLabelValue(window, "ARTICULATION", std::to_string(debug.SilhouetteMetrics.ArticulationCount), x, y);
        drawLabelValue(window, "SHOULDER / WAIST", std::to_string(debug.SilhouetteMetrics.ShoulderProminencePercent) + "% / " + std::to_string(debug.SilhouetteMetrics.InteriorContractionPercent) + "%", x, y);
        drawLabelValue(window, "NOSE / REAR TAPER", std::to_string(debug.SilhouetteMetrics.NoseTaperPercent) + "% / " + std::to_string(debug.SilhouetteMetrics.RearTaperPercent) + "%", x, y);
        drawLabelValue(window, "STABLE WIDTH RUN", std::to_string(debug.SilhouetteMetrics.LongestStableWidthRunPercent) + "%", x, y);
        drawLabelValue(window, "SILHOUETTE GUIDE", std::to_string(debug.SilhouetteGuidanceAppliedCount), x, y);
        drawLabelValue(window, "MATERIAL ZONES", std::to_string(debug.MaterialZoneCount), x, y);
        drawLabelValue(window, "MATERIAL PX", std::to_string(debug.MaterialSecondaryHullPixelCount) + " SECONDARY / " + std::to_string(debug.MaterialMechanicalPixelCount) + " MECH", x, y);
        if (debug.MaterialZoneCount > 0u)
        {
            std::string materialZoneTypes;
            for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipMaterialZoneType::SHIP_MATERIAL_ZONE_TYPE_END); ++index)
            {
                if (debug.MaterialZoneTypeCounts[index] == 0u) { continue; }
                if (!materialZoneTypes.empty()) { materialZoneTypes += " + "; }
                materialZoneTypes += PixelShipGenerator::getShipMaterialZoneTypeName(static_cast<PixelShipGenerator::ShipMaterialZoneType>(index));
            }
            drawLabelValue(window, "MATERIAL TYPES", materialZoneTypes, x, y);
        }
        drawLabelValue(window, "LIVERY", std::to_string(debug.LiveryMarkingCount) + " / " + std::to_string(debug.LiveryPrimaryPixelCount + debug.LiverySecondaryPixelCount) + " PX", x, y);
        drawLabelValue(window, "PRIMARY DETAIL MOTIF", PixelShipGenerator::getShipDetailMotifTypeName(debug.PrimaryDetailMotif), x, y);
        drawLabelValue(window, "SECONDARY DETAIL MOTIF", PixelShipGenerator::getShipDetailMotifTypeName(debug.SecondaryDetailMotif), x, y);
        drawLabelValue(window, "MOTIF OCCURRENCES", std::to_string(debug.PrimaryDetailMotifOccurrenceCount) + " PRIMARY / " + std::to_string(debug.SecondaryDetailMotifOccurrenceCount) + " SECONDARY", x, y);
        drawLabelValue(window, "MOTIF REGION", PixelShipGenerator::getGenerationSpatialRegionName(debug.PrimaryDetailMotifRegion), x, y);
        drawLabelValue(window, "MOTIF REJECTS", std::to_string(debug.DetailMotifRejectedPlacementCount), x, y);
        if (debug.LiveryMarkingCount > 0u)
        {
            std::string liveryTypes;
            for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipLiveryType::SHIP_LIVERY_TYPE_END); ++index)
            {
                if (debug.LiveryTypeCounts[index] == 0u) { continue; }
                if (!liveryTypes.empty()) { liveryTypes += " + "; }
                liveryTypes += PixelShipGenerator::getShipLiveryTypeName(static_cast<PixelShipGenerator::ShipLiveryType>(index));
            }
            drawLabelValue(window, "LIVERY TYPES", liveryTypes, x, y);
        }
        drawLabelValue(window, "STRUCTURAL VOIDS", std::to_string(debug.StructuralNegativeSpaceCount) + " / " + std::to_string(debug.StructuralNegativeSpacePixelCount) + " PX", x, y);
        if (debug.StructuralNegativeSpaceCount > 0u)
        {
            std::string negativeSpaceTypes;
            for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipStructuralNegativeSpaceType::SHIP_STRUCTURAL_NEGATIVE_SPACE_TYPE_END); ++index)
            {
                if (debug.StructuralNegativeSpaceTypeCounts[index] == 0u) { continue; }
                if (!negativeSpaceTypes.empty()) { negativeSpaceTypes += " + "; }
                negativeSpaceTypes += PixelShipGenerator::getShipStructuralNegativeSpaceTypeName(static_cast<PixelShipGenerator::ShipStructuralNegativeSpaceType>(index));
            }
            drawLabelValue(window, "VOID TYPES", negativeSpaceTypes, x, y);
        }
        drawLabelValue(window, "LAST HULL RETRY", PixelShipGenerator::getSilhouetteValidationFailureReasonName(debug.LastSilhouetteValidationFailure), x, y);
        drawLabelValue(window, "FINAL BOUNDS", getBoundsString(finalBounds), x, y);
        drawLabelValue(window, "HULL BOUNDS", getBoundsString(hullBounds), x, y);
        drawLabelValue(window, "HULL PIXELS", std::to_string(countMaskPixels(ship.HullMask)), x, y);
        drawLabelValue(window, "WING SHAPE", getWingShapeName(debug.WingShape), x, y);
        if (debug.WingShape != PixelShipGenerator::WingShapeType::NONE)
        {
            drawLabelValue(window, "WING SPAN", std::to_string(debug.WingMaximumSpan) + " PX", x, y);
            drawLabelValue(window, "WING EXTENSION", std::to_string(debug.WingMaximumExtension) + " PX", x, y);
            drawLabelValue(window, "WING ROOT", std::to_string(debug.WingRootThickness) + " PX", x, y);
            drawLabelValue(window, "WING PIXELS", std::to_string(debug.WingPixelCount), x, y);
        }
        drawLabelValue(window, "HULL LAYERS", std::to_string(debug.HullLayerCount), x, y);
        drawLabelValue(window, "LAYER DEPTH", std::to_string(debug.HullLayerLowerCount) + " LOWER / " + std::to_string(debug.HullLayerUpperCount) + " UPPER", x, y);
        drawLabelValue(window, "LAYER PX", std::to_string(debug.HullLayerPixelCount), x, y);
        drawLabelValue(window, "LAYER REJECTS", std::to_string(debug.HullLayerPlacementRejectionCount), x, y);
        drawLabelValue(window, "CORE TREATMENTS", std::to_string(debug.CoreTreatmentCount), x, y);
        drawLabelValue(window, "CORE REGION PX", std::to_string(debug.CoreRegionPixelCount), x, y);
        drawLabelValue(window, "CORE RAISED/RECESS", std::to_string(debug.CoreRaisedPixelCount) + " / " + std::to_string(debug.CoreRecessedPixelCount), x, y);
        drawLabelValue(window, "CORE MATERIAL/LIGHT", std::to_string(debug.CoreSecondaryMaterialPixelCount) + " / " + std::to_string(debug.CoreLuminousPixelCount), x, y);
        drawLabelValue(window, "CORE COST", std::to_string(debug.CoreTreatmentComplexityCost), x, y);
        drawLabelValue(window, "CORE REJECTS", std::to_string(debug.CoreTreatmentPlacementRejectionCount), x, y);
        drawLabelValue(window, "MAJOR FEATURES", std::to_string(debug.MajorFeatureCount), x, y);
        drawLabelValue(window, "MAJOR PX", std::to_string(debug.MajorFeaturePixelCount), x, y);
        drawLabelValue(window, "MAJOR REJECTS", std::to_string(debug.MajorFeaturePlacementRejectionCount) + "/" + std::to_string(debug.MajorFeaturePlacementAttemptCount), x, y);
        drawLabelValue(window, "COCKPIT", getBoundsString(cockpitBounds), x, y);
        drawLabelValue(window, "COCKPIT PX", std::to_string(countMaskPixels(ship.CockpitMask)), x, y);
        drawLabelValue(window, "COCKPIT SIZE", PixelShipGenerator::getCockpitSizeClassName(debug.CockpitSize), x, y);
        drawLabelValue(window, "COCKPIT SHAPE", PixelShipGenerator::getCockpitShapeTypeName(debug.CockpitShape), x, y);
        drawLabelValue(window, "GLASS / FRAME", std::to_string(debug.CockpitGlassPixelCount) + " / " + std::to_string(debug.CockpitFramePixelCount), x, y);
        drawLabelValue(window, "BASE / UPPER", std::to_string(debug.CockpitBasePixelCount) + " / " + std::to_string(debug.CockpitUpperSectionPixelCount), x, y);
        drawLabelValue(window, "COCKPIT COST", std::to_string(debug.CockpitComplexityCost), x, y);
        drawLabelValue(window, "ENGINES", std::to_string(debug.EngineCount), x, y);
        drawLabelValue(window, "ENGINE LAYOUT", getEngineLayoutName(debug.EngineLayout), x, y);
        if (!debug.EngineUnits.empty())
        {
            uint32_t maximumHousingWidth = 0u;
            uint32_t maximumExhaustLength = 0u;
            uint32_t nacelleCount = 0u;
            std::array<uint32_t, static_cast<std::size_t>(PixelShipGenerator::EngineSizeClass::ENGINE_SIZE_CLASS_END)> sizeCounts = {};

            for (const PixelShipGenerator::EngineUnitDebugInfo& engineUnit : debug.EngineUnits)
            {
                maximumHousingWidth = std::max(maximumHousingWidth, engineUnit.HousingWidth);
                maximumExhaustLength = std::max(maximumExhaustLength, engineUnit.ExhaustLength);
                if (engineUnit.Nacelle) { ++nacelleCount; }
                if (engineUnit.SizeClass != PixelShipGenerator::EngineSizeClass::ENGINE_SIZE_CLASS_END) { ++sizeCounts[static_cast<std::size_t>(engineUnit.SizeClass)]; }
            }

            PixelShipGenerator::EngineSizeClass dominantSize = PixelShipGenerator::EngineSizeClass::SMALL;
            for (uint32_t index = 1u; index < sizeCounts.size(); ++index) { if (sizeCounts[index] > sizeCounts[static_cast<std::size_t>(dominantSize)]) { dominantSize = static_cast<PixelShipGenerator::EngineSizeClass>(index); } }
            drawLabelValue(window, "ENGINE SIZE", getEngineSizeName(dominantSize), x, y);
            drawLabelValue(window, "MAX HOUSING", std::to_string(maximumHousingWidth) + " PX", x, y);
            drawLabelValue(window, "MAX EXHAUST", std::to_string(maximumExhaustLength) + " PX", x, y);
            drawLabelValue(window, "NACELLES", std::to_string(nacelleCount), x, y);
        }
        drawLabelValue(window, "LARGE WEAPONS", std::to_string(debug.WeaponCount), x, y);
        drawLabelValue(window, "WEAPON HARDPOINTS", std::to_string(debug.WeaponHardpointCount), x, y);
        drawLabelValue(window, "WEAPON REJECTS", std::to_string(debug.WeaponPlacementRejectionCount) + "/" + std::to_string(debug.WeaponPlacementAttemptCount), x, y);
        if (!debug.WeaponUnits.empty())
        {
            const PixelShipGenerator::WeaponUnitDebugInfo& firstWeapon = debug.WeaponUnits.front();
            drawLabelValue(window, "FIRST WEAPON", getWeaponTypeName(firstWeapon.Type), x, y);
            drawLabelValue(window, "HARDPOINT", getWeaponRegionName(firstWeapon.Region), x, y);
        }
        drawLabelValue(window, "ATTACHMENTS", std::to_string(ship.AttachmentPlacements.size()), x, y);

        std::set<uint32_t> symmetryGroups;
        std::array<uint32_t, static_cast<std::size_t>(PixelShipGenerator::ShipAttachmentType::SHIP_ATTACHMENT_TYPE_END)> attachmentCounts = {};

        for (const PixelShipGenerator::ShipAttachmentPlacement& placement : ship.AttachmentPlacements)
        {
            if (placement.SymmetryGroup != 0u) { symmetryGroups.insert(placement.SymmetryGroup); }
            const std::size_t typeIndex = static_cast<std::size_t>(placement.Type);
            if (typeIndex < attachmentCounts.size()) { ++attachmentCounts[typeIndex]; }
        }

        drawLabelValue(window, "SYM GROUPS", std::to_string(symmetryGroups.size()), x, y);
        y += 5.0f;
        drawSectionHeader(window, "MACRO ASYMMETRY", x, y);
        if (!debug.MacroAsymmetryPlanned)
        {
            drawLabelValue(window, "PLAN", "OFF", x, y);
        }
        else
        {
            const std::string state = debug.MacroAsymmetryFulfilled ? "FULFILLED" : (debug.MacroAsymmetryRejected ? "REJECTED" : "PLANNED");
            drawLabelValue(window, "PLAN", state, x, y);
            drawLabelValue(window, "SIDE", PixelShipGenerator::getMacroAsymmetrySideName(debug.MacroAsymmetryDominantSide), x, y);
            drawLabelValue(window, "FEATURE", PixelShipGenerator::getMacroAsymmetryCategoryName(debug.MacroAsymmetryFeatureCategory), x, y);
            drawLabelValue(window, "TARGET", PixelShipGenerator::getGenerationSpatialRegionName(debug.MacroAsymmetryTargetRegion), x, y);
            drawLabelValue(window, "BALANCE", std::to_string(debug.MacroAsymmetryBalanceScore) + "%", x, y);
            drawLabelValue(window, "VISUAL WEIGHT", std::to_string(debug.MacroAsymmetryActualVisualWeight) + "/" + std::to_string(debug.MacroAsymmetryDesiredVisualWeight), x, y);
        }
        y += 5.0f;
        drawSectionHeader(window, "SEMANTIC SPATIAL LOAD", x, y);
        drawLabelValue(window, "LOAD REJECTIONS", std::to_string(debug.SpatialOverloadRejectionCount), x, y);
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END); ++index)
        {
            if (debug.SpatialRegionCapacities[index] == 0u) { continue; }
            const auto region = static_cast<PixelShipGenerator::GenerationSpatialRegion>(index);
            const uint32_t utilization = debug.SpatialRegionLoads[index] * 100u / debug.SpatialRegionCapacities[index];
            drawLabelValue(window, PixelShipGenerator::getGenerationSpatialRegionName(region), std::to_string(utilization) + "%  D" + std::to_string(debug.SpatialRegionDominantCounts[index]), x, y);
        }
        y += 5.0f;
        drawSectionHeader(window, "SILHOUETTE MODIFIERS", x, y);
        drawLabelValue(window, "COUNT", std::to_string(debug.AppliedHullModifiers.size()), x, y);

        if (debug.AppliedHullModifiers.empty())
        {
            drawDebugText(window, "NONE", x, y, sf::Color(220, 222, 228), TextScale);
            y += LargeTextLineHeight;
        }
        else
        {
            for (PixelShipGenerator::HullModifierType type : debug.AppliedHullModifiers)
            {
                drawDebugText(window, "- " + getHullModifierName(type), x, y, sf::Color(220, 222, 228), TextScale);
                y += LargeTextLineHeight;
            }
        }

        y += 5.0f;
        drawSectionHeader(window, "HULL LAYER TYPES", x, y);
        bool hasHullLayerType = false;
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipHullLayerType::SHIP_HULL_LAYER_TYPE_END); ++index)
        {
            if (debug.HullLayerTypeCounts[index] == 0u) { continue; }
            hasHullLayerType = true;
            drawLabelValue(window, PixelShipGenerator::getShipHullLayerTypeName(static_cast<PixelShipGenerator::ShipHullLayerType>(index)), std::to_string(debug.HullLayerTypeCounts[index]), x, y);
        }
        if (!hasHullLayerType)
        {
            drawDebugText(window, "NONE", x, y, sf::Color(220, 222, 228), TextScale);
            y += LargeTextLineHeight;
        }

        y += 5.0f;
        drawSectionHeader(window, "MAJOR FEATURE TYPES", x, y);
        bool hasMajorFeatureType = false;
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipMajorFeatureType::SHIP_MAJOR_FEATURE_TYPE_END); ++index)
        {
            if (debug.MajorFeatureTypeCounts[index] == 0u)
            {
                continue;
            }

            hasMajorFeatureType = true;
            drawLabelValue(window, getMajorFeatureName(static_cast<PixelShipGenerator::ShipMajorFeatureType>(index)), std::to_string(debug.MajorFeatureTypeCounts[index]), x, y);
        }
        if (!hasMajorFeatureType)
        {
            drawDebugText(window, "NONE", x, y, sf::Color(220, 222, 228), TextScale);
            y += LargeTextLineHeight;
        }

        y += 5.0f;
        drawSectionHeader(window, "LARGE WEAPON TYPES", x, y);
        bool hasWeaponType = false;
        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipWeaponType::SHIP_WEAPON_TYPE_END); ++index)
        {
            if (debug.WeaponTypeCounts[index] == 0u)
            {
                continue;
            }

            hasWeaponType = true;
            drawLabelValue(window, getWeaponTypeName(static_cast<PixelShipGenerator::ShipWeaponType>(index)), std::to_string(debug.WeaponTypeCounts[index]), x, y);
        }
        if (!hasWeaponType)
        {
            drawDebugText(window, "NONE", x, y, sf::Color(220, 222, 228), TextScale);
            y += LargeTextLineHeight;
        }

        float rightX = OverlayMargin + 430.0f;
        float rightY = OverlayMargin + 54.0f;
        drawSectionHeader(window, "ATTACHMENT TYPES", rightX, rightY);

        for (uint32_t index = 0u; index < static_cast<uint32_t>(PixelShipGenerator::ShipAttachmentType::SHIP_ATTACHMENT_TYPE_END); ++index)
        {
            const PixelShipGenerator::ShipAttachmentType type = static_cast<PixelShipGenerator::ShipAttachmentType>(index);
            drawLabelValue(window, getAttachmentTypeName(type), std::to_string(attachmentCounts[index]), rightX, rightY);
        }

        rightY += 5.0f;
        drawSectionHeader(window, "DETAIL MASKS", rightX, rightY);
        drawLabelValue(window, "ACCENT", std::to_string(countMaskPixels(ship.AccentMask)), rightX, rightY);
        drawLabelValue(window, "MECHANICAL", std::to_string(countMaskPixels(ship.MechanicalDetailMask)), rightX, rightY);
        drawLabelValue(window, "LIGHT", std::to_string(countMaskPixels(ship.LightMask)), rightX, rightY);

        if (debug.HasSurfaceDetailProfile)
        {
            rightY += 5.0f;
            drawSectionHeader(window, "RESOLVED DETAIL PROFILE", rightX, rightY);
            drawLabelValue(window, "DENSITY %", std::to_string(debug.SurfaceDetailProfile.DetailDensityPercent), rightX, rightY);
            drawLabelValue(window, "MECH %", std::to_string(debug.SurfaceDetailProfile.MechanicalPatternCountPercent), rightX, rightY);
            drawLabelValue(window, "LIGHT %", std::to_string(debug.SurfaceDetailProfile.LightPatternCountPercent), rightX, rightY);
            drawLabelValue(window, "ASYM %", std::to_string(debug.SurfaceDetailProfile.AsymmetricDetailChance), rightX, rightY);
            drawLabelValue(window, "PANEL WT", std::to_string(debug.SurfaceDetailProfile.AccentPanelWeight), rightX, rightY);
            drawLabelValue(window, "STRIPE WT", std::to_string(debug.SurfaceDetailProfile.AccentStripeWeight), rightX, rightY);
            drawLabelValue(window, "ARMOR WT", std::to_string(debug.SurfaceDetailProfile.AccentArmorWeight), rightX, rightY);
        }

        if (data.SelectedAnimationType == PixelShipGenerator::ShipAnimationType::IDLE && data.IdleAnimation != nullptr && data.IdleAnimationSettings != nullptr && !data.IdleAnimation->Frames.empty())
        {
            rightY += 5.0f;
            drawSectionHeader(window, "ANIMATION", rightX, rightY);
            drawLabelValue(window, "TYPE", getAnimationTypeDisplayName(data.SelectedAnimationType), rightX, rightY);
            drawLabelValue(window, "SEED", std::to_string(data.IdleAnimation->Seed), rightX, rightY);
            drawLabelValue(window, "DURATION", std::to_string(data.IdleAnimation->DurationMilliseconds) + " ms", rightX, rightY);
            drawLabelValue(window, "FRAMES", std::to_string(data.IdleAnimation->Sampling.ActualFrameCount), rightX, rightY);
            drawLabelValue(window, "SAMPLING", data.IdleAnimation->Sampling.Mode == PixelShipGenerator::AnimationSamplingMode::ADAPTIVE ? "ADAPTIVE" : "EXACT", rightX, rightY);
            drawLabelValue(window, "FRAME TIME", getMillisecondsString(data.IdleAnimation->FrameDurationMilliseconds), rightX, rightY);
            drawLabelValue(window, "FRAME", std::to_string(data.AnimationFrameIndex + 1u) + "/" + std::to_string(data.IdleAnimation->Frames.size()), rightX, rightY);
            drawLabelValue(window, "COMPONENTS", std::to_string(data.IdleAnimation->Sampling.ActiveAnimatedComponentCount), rightX, rightY);
            drawLabelValue(window, "MAX TRAVEL", std::to_string(std::max(data.IdleAnimation->Sampling.MaximumMechanicalTravelPixels, data.IdleAnimation->Sampling.MaximumExhaustTravelPixels)) + " px", rightX, rightY);
            drawLabelValue(window, "PHASE GROUPS", std::to_string(data.IdleAnimation->Sampling.IndependentPhaseGroupCount), rightX, rightY);
            drawLabelValue(window, "ENGINE FX", getOnOff(data.IdleAnimationSettings->EngineFlicker), rightX, rightY);
            drawLabelValue(window, "LIGHT FX", getOnOff(data.IdleAnimationSettings->LightBlinking), rightX, rightY);
            drawLabelValue(window, "MICRO MOVE", getOnOff(data.IdleAnimationSettings->MechanicalMicroMovement), rightX, rightY);
            drawLabelValue(window, "HOVER", getOnOff(data.IdleAnimationSettings->HoverOffset), rightX, rightY);
            drawLabelValue(window, "DETAIL FX", getOnOff(data.IdleAnimationSettings->SmallDetailVariation), rightX, rightY);
        }
        else if (data.SelectedAnimationType == PixelShipGenerator::ShipAnimationType::FIRE && data.FiringAnimation != nullptr && data.FiringAnimationSettings != nullptr && !data.FiringAnimation->Frames.empty())
        {
            rightY += 5.0f;
            drawSectionHeader(window, "ANIMATION", rightX, rightY);
            drawLabelValue(window, "TYPE", "FIRE", rightX, rightY);
            drawLabelValue(window, "SEED", std::to_string(data.FiringAnimation->Seed), rightX, rightY);
            drawLabelValue(window, "TARGET", std::to_string(data.FiringAnimation->Target.WeaponComponentIndex), rightX, rightY);
            drawLabelValue(window, "PHASE", getCurrentFiringPhaseDisplayName(*data.FiringAnimation, data.AnimationFrameIndex), rightX, rightY);
            drawLabelValue(window, "GROUP", std::to_string(data.FiringAnimation->Diagnostics.TargetSymmetryGroup), rightX, rightY);
            drawLabelValue(window, "DURATION", std::to_string(data.FiringAnimation->DurationMilliseconds) + " ms", rightX, rightY);
            drawLabelValue(window, "FRAMES", std::to_string(data.FiringAnimation->Sampling.ActualFrameCount), rightX, rightY);
            drawLabelValue(window, "SAMPLING", data.FiringAnimation->Sampling.Mode == PixelShipGenerator::AnimationSamplingMode::ADAPTIVE ? "ADAPTIVE" : "EXACT", rightX, rightY);
            drawLabelValue(window, "FRAME TIME", getMillisecondsString(data.FiringAnimation->FrameDurationMilliseconds), rightX, rightY);
            drawLabelValue(window, "FRAME", std::to_string(data.AnimationFrameIndex + 1u) + "/" + std::to_string(data.FiringAnimation->Frames.size()), rightX, rightY);
            drawLabelValue(window, "WEAPONS", std::to_string(data.FiringAnimation->Diagnostics.ActiveWeaponCount), rightX, rightY);
            drawLabelValue(window, "RECOIL", std::to_string(data.FiringAnimation->Diagnostics.MaximumRecoilTravelPixels) + " px", rightX, rightY);
            drawLabelValue(window, "PRE-FIRE", std::to_string(data.FiringAnimation->Diagnostics.MaximumPreFireExtensionPixels) + " px", rightX, rightY);
            if (data.TransientStatePreviewActive)
            {
                drawLabelValue(window, "UNDERLYING", getAnimationTypeDisplayName(data.RuntimeMovementType), rightX, rightY);
                drawLabelValue(window, "OVERRIDES", std::to_string(data.FiringAnimation->Diagnostics.ActiveWeaponCount) + " weapon", rightX, rightY);
            }
        }
        else if (data.MovementAnimation != nullptr && data.MovementAnimationSettings != nullptr && data.MovementAnimation->Type == data.SelectedAnimationType)
        {
            const PixelShipGenerator::ShipMovementAnimationClip& clip = PixelShipGenerator::getMovementAnimationClip(*data.MovementAnimation, data.MovementPhase);
            if (!clip.Frames.empty())
            {
                rightY += 5.0f;
                drawSectionHeader(window, "ANIMATION", rightX, rightY);
                drawLabelValue(window, "TYPE", getAnimationTypeDisplayName(data.SelectedAnimationType), rightX, rightY);
                drawLabelValue(window, "PHASE", getMovementPhaseDisplayName(data.MovementPhase), rightX, rightY);
                if (data.MovementTransitionPending) { drawLabelValue(window, "NEXT", getAnimationTypeDisplayName(data.PendingMovementType), rightX, rightY); }
                drawLabelValue(window, "SEED", std::to_string(data.MovementAnimation->Seed), rightX, rightY);
                drawLabelValue(window, "DURATION", std::to_string(clip.DurationMilliseconds) + " ms", rightX, rightY);
                drawLabelValue(window, "FRAMES", std::to_string(clip.Sampling.ActualFrameCount), rightX, rightY);
                drawLabelValue(window, "SAMPLING", clip.Sampling.Mode == PixelShipGenerator::AnimationSamplingMode::ADAPTIVE ? "ADAPTIVE" : "EXACT", rightX, rightY);
                drawLabelValue(window, "FRAME TIME", getMillisecondsString(clip.FrameDurationMilliseconds), rightX, rightY);
                drawLabelValue(window, "FRAME", std::to_string(data.AnimationFrameIndex + 1u) + "/" + std::to_string(clip.Frames.size()), rightX, rightY);
                drawLabelValue(window, "COMPONENTS", std::to_string(clip.Sampling.ActiveAnimatedComponentCount), rightX, rightY);
                drawLabelValue(window, "MAX TRAVEL", std::to_string(data.MovementAnimation->Diagnostics.MaximumMechanicalTravelPixels) + " px", rightX, rightY);
                drawLabelValue(window, "EXHAUST TRAVEL", std::to_string(data.MovementAnimation->Diagnostics.MaximumExhaustTravelPixels) + " px", rightX, rightY);
                drawLabelValue(window, "PHASE GROUPS", std::to_string(data.MovementAnimation->Diagnostics.IndependentPhaseGroupCount), rightX, rightY);
                drawLabelValue(window, "ENGINES", std::to_string(data.MovementAnimation->Diagnostics.ActiveEngineCount), rightX, rightY);
                drawLabelValue(window, "WEAPONS", std::to_string(data.MovementAnimation->Diagnostics.ActiveWeaponCount), rightX, rightY);
                drawLabelValue(window, "ATTACH", std::to_string(data.MovementAnimation->Diagnostics.ActiveAttachmentCount), rightX, rightY);
                drawLabelValue(window, "BRAKES", std::to_string(data.MovementAnimation->Diagnostics.ActiveBrakingComponentCount), rightX, rightY);
            }
        }
    }

    void PreviewRenderer::renderPaletteInspector(sf::RenderWindow& window, const PreviewRenderData& data) const
    {
        if (data.Ship == nullptr)
        {
            return;
        }

        drawPanel(window, OverlayMargin, OverlayMargin, static_cast<float>(PreviewContentWidth) - OverlayMargin * 2.0f, static_cast<float>(PreviewWindowHeight) - OverlayMargin * 2.0f, sf::Color(8, 9, 12, 248), sf::Color(120, 125, 145));
        const PixelShipGenerator::ShipPalette& palette = data.Ship->Palette;
        const std::array<std::pair<const char*, PixelShipGenerator::Color>, 25u> entries = { {
            { "TRANSPARENT", palette.Transparent }, { "OUTLINE", palette.Outline }, { "HULL DEEP SHADOW", palette.HullDeepShadow }, { "HULL SHADOW", palette.HullShadow }, { "HULL BASE", palette.HullBase }, { "HULL SECONDARY", palette.HullSecondary }, { "HULL HIGHLIGHT", palette.HullHighlight }, { "HULL EDGE", palette.HullEdgeHighlight }, { "ACCENT DARK", palette.HullAccentDark }, { "ACCENT BASE", palette.HullAccent }, { "ACCENT HIGHLIGHT", palette.HullAccentHighlight }, { "COCKPIT DARK", palette.CockpitDark }, { "COCKPIT BASE", palette.CockpitBase }, { "COCKPIT HIGHLIGHT", palette.CockpitHighlight }, { "COCKPIT GLINT", palette.CockpitGlint }, { "ENGINE DARK", palette.EngineDark }, { "ENGINE BASE", palette.EngineBase }, { "ENGINE HIGHLIGHT", palette.EngineHighlight }, { "ENGINE HOT CORE", palette.EngineHotCore }, { "EXHAUST BASE", palette.ExhaustBase }, { "EXHAUST HIGHLIGHT", palette.ExhaustHighlight }, { "EXHAUST HOT CORE", palette.ExhaustHotCore }, { "MECHANICAL DARK", palette.MechanicalDark }, { "MECHANICAL BASE", palette.MechanicalBase }, { "LIGHT BASE", palette.LightBase }
        } };

        drawDebugText(window, "PALETTE INSPECTOR - F7 OR ESCAPE TO CLOSE", OverlayMargin + 16.0f, OverlayMargin + 14.0f, sf::Color(240, 215, 105), TextScale);
        float leftY = OverlayMargin + 54.0f;
        float rightY = leftY;

        for (std::size_t index = 0u; index < entries.size(); ++index)
        {
            const bool rightColumn = index >= (entries.size() + 1u) / 2u;
            const float x = rightColumn ? OverlayMargin + 430.0f : OverlayMargin + 18.0f;
            float& y = rightColumn ? rightY : leftY;
            sf::RectangleShape swatch(sf::Vector2f(30.0f, 18.0f));
            swatch.setPosition(x, y - 2.0f);
            swatch.setFillColor(toSFMLColor(entries[index].second));
            swatch.setOutlineThickness(1.0f);
            swatch.setOutlineColor(sf::Color(180, 185, 195));
            window.draw(swatch);
            drawDebugText(window, entries[index].first, x + 42.0f, y, sf::Color(225, 228, 235), TextScale);
            drawDebugText(window, colorToHex(entries[index].second), x + 222.0f, y, sf::Color(145, 205, 235), TextScale);
            y += 28.0f;
        }

        sf::RectangleShape extraSwatch(sf::Vector2f(30.0f, 18.0f));
        extraSwatch.setPosition(OverlayMargin + 430.0f, rightY - 2.0f);
        extraSwatch.setFillColor(toSFMLColor(palette.LightHighlight));
        extraSwatch.setOutlineThickness(1.0f);
        extraSwatch.setOutlineColor(sf::Color(180, 185, 195));
        window.draw(extraSwatch);
        drawDebugText(window, "LIGHT HIGHLIGHT", OverlayMargin + 472.0f, rightY, sf::Color(225, 228, 235), TextScale);
        drawDebugText(window, colorToHex(palette.LightHighlight), OverlayMargin + 652.0f, rightY, sf::Color(145, 205, 235), TextScale);
    }
}
