#include "PreviewRenderer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "PreviewCommand.h"
#include "SFMLPixelText.h"
#include "PreviewThumbnailGrid.h"

namespace
{
    constexpr float StatePanelX = static_cast<float>(SpectralShipGenStudioPreview::PreviewStatePanelX);
    constexpr float CommandPanelX = static_cast<float>(SpectralShipGenStudioPreview::PreviewCommandPanelX);
    constexpr float OverlayMargin = 24.0f;
    constexpr uint32_t TextScale = 2u;
    constexpr uint32_t SmallTextScale = 1u;
    constexpr uint32_t EditorTextScale = 2u;
    constexpr uint32_t EditorSecondaryTextScale = 1u;
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
        SpectralShipGenStudioApplication::drawPixelText(target, text, x, y, color, scale);
    }

    float getDebugTextWidth(const std::string& text, uint32_t scale)
    {
        return SpectralShipGenStudioApplication::getPixelTextWidth(text, scale);
    }

    std::string wrapDebugText(const std::string& text, std::size_t maximumCharactersPerLine)
    {
        return SpectralShipGenStudioApplication::wrapPixelText(text, maximumCharactersPerLine);
    }

    std::string getFixedString(double value, uint32_t precision)
    {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(static_cast<int>(precision)) << value;
        return stream.str();
    }

    std::string fitDebugTextToWidth(std::string text, float maximumWidth, uint32_t scale)
    {
        if (maximumWidth <= 0.0f || text.empty()) { return {}; }
        if (getDebugTextWidth(text, scale) <= maximumWidth) { return text; }

        const std::string ellipsis = "...";
        const float ellipsisWidth = getDebugTextWidth(ellipsis, scale);
        if (ellipsisWidth > maximumWidth) { return {}; }

        while (!text.empty() && getDebugTextWidth(text, scale) + ellipsisWidth > maximumWidth) { text.pop_back(); }
        return text + ellipsis;
    }

    std::string getCommandPanelButtonLabel(SpectralShipGenStudioPreview::PreviewCommandType type)
    {
        using SpectralShipGenStudioPreview::PreviewCommandType;
        switch (type)
        {
        case PreviewCommandType::GALLERY_LEFT: return "LEFT";
        case PreviewCommandType::GALLERY_RIGHT: return "RIGHT";
        case PreviewCommandType::GALLERY_UP: return "UP";
        case PreviewCommandType::GALLERY_DOWN: return "DOWN";
        default: return SpectralShipGenStudioPreview::getPreviewCommandCompactLabel(type);
        }
    }

    bool isStatefulCommand(SpectralShipGenStudioPreview::PreviewCommandType type)
    {
        using SpectralShipGenStudioPreview::PreviewCommandType;
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

    sf::Color toSFMLColor(const SpectralShipGen::Color& color)
    {
        return sf::Color(color.R, color.G, color.B, color.A);
    }

    std::string getStyleName(SpectralShipGen::ShipStyle style)
    {
        switch (style)
        {
        case SpectralShipGen::ShipStyle::SLEEK: return "SLEEK";
        case SpectralShipGen::ShipStyle::FIGHTER: return "FIGHTER";
        case SpectralShipGen::ShipStyle::HEAVY: return "HEAVY";
        case SpectralShipGen::ShipStyle::INDUSTRIAL: return "INDUSTRIAL";
        case SpectralShipGen::ShipStyle::SPEARHEAD: return "SPEARHEAD";
        case SpectralShipGen::ShipStyle::DELTA: return "DELTA";
        default: return "UNKNOWN";
        }
    }

    std::string getFactionName(SpectralShipGen::ShipFactionType faction)
    {
        switch (faction)
        {
        case SpectralShipGen::ShipFactionType::FRONTIER: return "FRONTIER";
        case SpectralShipGen::ShipFactionType::MILITARY: return "MILITARY";
        case SpectralShipGen::ShipFactionType::ASCENDANT: return "ASCENDANT";
        case SpectralShipGen::ShipFactionType::XENO: return "XENO";
        case SpectralShipGen::ShipFactionType::CORPORATE: return "CORPORATE";
        case SpectralShipGen::ShipFactionType::RELIC: return "RELIC";
        default: return "UNKNOWN";
        }
    }

    std::string getStructuralRecipeDisplayName(const SpectralShipGenStudioPreview::PreviewGenerationRecipe& recipe)
    {
        return recipe.StructuralPreset.has_value() ? getStyleName(*recipe.StructuralPreset) : "CUSTOM";
    }

    std::string getFactionRecipeDisplayName(const SpectralShipGenStudioPreview::PreviewGenerationRecipe& recipe)
    {
        return recipe.FactionPreset.has_value() ? getFactionName(*recipe.FactionPreset) : "CUSTOM";
    }

    std::string getPresetSourceName(bool builtIn)
    {
        return builtIn ? "BUILT-IN" : "CUSTOM";
    }

    std::string getPreviewModeName(SpectralShipGenStudioPreview::PreviewMode mode)
    {
        switch (mode)
        {
        case SpectralShipGenStudioPreview::PreviewMode::STATIC: return "STATIC";
        case SpectralShipGenStudioPreview::PreviewMode::ANIMATION: return "ANIMATION";
        case SpectralShipGenStudioPreview::PreviewMode::FRAME_INSPECTION: return "FRAME INSPECTION";
        case SpectralShipGenStudioPreview::PreviewMode::GALLERY: return "GALLERY";
        case SpectralShipGenStudioPreview::PreviewMode::FAVORITES: return "FAVORITES";
        case SpectralShipGenStudioPreview::PreviewMode::REROLL_STUDIO: return "REROLL STUDIO";
        case SpectralShipGenStudioPreview::PreviewMode::CALIBRATION: return "CALIBRATION";
        case SpectralShipGenStudioPreview::PreviewMode::CONFIGURATION_EDITOR: return "CONFIGURATION EDITOR";
        default: return "UNKNOWN";
        }
    }

    std::string getWingShapeName(SpectralShipGen::WingShapeType type)
    {
        switch (type)
        {
        case SpectralShipGen::WingShapeType::NONE: return "NONE";
        case SpectralShipGen::WingShapeType::SMALL: return "SMALL";
        case SpectralShipGen::WingShapeType::SWEPT: return "SWEPT";
        case SpectralShipGen::WingShapeType::BROAD: return "BROAD";
        default: return "UNKNOWN";
        }
    }

    std::string getWeaponTypeName(SpectralShipGen::ShipWeaponType type)
    {
        switch (type)
        {
        case SpectralShipGen::ShipWeaponType::SINGLE_CANNON: return "SINGLE CANNON";
        case SpectralShipGen::ShipWeaponType::TWIN_CANNON: return "TWIN CANNON";
        case SpectralShipGen::ShipWeaponType::COMPACT_TURRET: return "COMPACT TURRET";
        case SpectralShipGen::ShipWeaponType::RAIL_WEAPON: return "RAIL WEAPON";
        case SpectralShipGen::ShipWeaponType::WEAPON_POD: return "WEAPON POD";
        default: return "UNKNOWN";
        }
    }

    std::string getWeaponRegionName(SpectralShipGen::ShipWeaponHardpointRegion region)
    {
        switch (region)
        {
        case SpectralShipGen::ShipWeaponHardpointRegion::CENTRAL_NOSE: return "CENTRAL NOSE";
        case SpectralShipGen::ShipWeaponHardpointRegion::FORWARD_FUSELAGE_SIDE: return "FWD FUSELAGE";
        case SpectralShipGen::ShipWeaponHardpointRegion::WING_ROOT: return "WING ROOT";
        case SpectralShipGen::ShipWeaponHardpointRegion::OUTER_WING: return "OUTER WING";
        case SpectralShipGen::ShipWeaponHardpointRegion::FORWARD_SHOULDER: return "FWD SHOULDER";
        case SpectralShipGen::ShipWeaponHardpointRegion::CENTRAL_BODY: return "CENTRAL BODY";
        default: return "UNKNOWN";
        }
    }

    std::string getEngineLayoutName(SpectralShipGen::EngineLayoutType type)
    {
        switch (type)
        {
        case SpectralShipGen::EngineLayoutType::CENTRAL: return "CENTRAL";
        case SpectralShipGen::EngineLayoutType::TWIN: return "TWIN";
        case SpectralShipGen::EngineLayoutType::QUAD: return "QUAD";
        case SpectralShipGen::EngineLayoutType::CENTRAL_AUXILIARY: return "CENTRAL + AUX";
        case SpectralShipGen::EngineLayoutType::WIDE_BANK: return "WIDE BANK";
        case SpectralShipGen::EngineLayoutType::ENGINE_LAYOUT_TYPE_END: return "NONE";
        default: return "UNKNOWN";
        }
    }

    std::string getDebugStageName(SpectralShipGen::ShipGenerationDebugStageType type)
    {
        switch (type)
        {
        case SpectralShipGen::ShipGenerationDebugStageType::BASE_HULL: return "BASE HULL";
        case SpectralShipGen::ShipGenerationDebugStageType::CLEANED_BASE_HULL: return "CLEANED BASE";
        case SpectralShipGen::ShipGenerationDebugStageType::AFTER_ADDITIVE_MODIFIERS: return "AFTER ADDITIVE";
        case SpectralShipGen::ShipGenerationDebugStageType::AFTER_SUBTRACTIVE_MODIFIERS: return "AFTER SUBTRACTIVE";
        case SpectralShipGen::ShipGenerationDebugStageType::FINAL_HULL: return "FINAL HULL";
        default: return "UNKNOWN";
        }
    }

    Bounds calculateMaskBounds(const SpectralShipGen::PixelMask& mask)
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

    Bounds calculateImageBounds(const SpectralShipGen::Image& image, uint32_t width, uint32_t height)
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

    std::string getAnimationTypeDisplayName(SpectralShipGen::ShipAnimationType type)
    {
        switch (type)
        {
        case SpectralShipGen::ShipAnimationType::IDLE: return "IDLE";
        case SpectralShipGen::ShipAnimationType::MOVE_LEFT: return "MOVE LEFT";
        case SpectralShipGen::ShipAnimationType::MOVE_RIGHT: return "MOVE RIGHT";
        case SpectralShipGen::ShipAnimationType::MOVE_UP: return "MOVE UP";
        case SpectralShipGen::ShipAnimationType::MOVE_DOWN: return "MOVE DOWN";
        case SpectralShipGen::ShipAnimationType::FIRE: return "FIRE";
        default: return "UNSUPPORTED";
        }
    }

    std::string getMovementPhaseDisplayName(SpectralShipGen::ShipMovementAnimationPhase phase)
    {
        switch (phase)
        {
        case SpectralShipGen::ShipMovementAnimationPhase::ENTER: return "ENTER";
        case SpectralShipGen::ShipMovementAnimationPhase::SUSTAIN: return "SUSTAIN";
        case SpectralShipGen::ShipMovementAnimationPhase::EXIT: return "EXIT";
        default: return "UNKNOWN";
        }
    }

    std::string getFiringPhaseDisplayName(SpectralShipGen::ShipFiringAnimationPhase phase)
    {
        switch (phase)
        {
        case SpectralShipGen::ShipFiringAnimationPhase::REST: return "REST";
        case SpectralShipGen::ShipFiringAnimationPhase::PRE_FIRE: return "PRE-FIRE";
        case SpectralShipGen::ShipFiringAnimationPhase::RECOIL: return "RECOIL";
        case SpectralShipGen::ShipFiringAnimationPhase::RECOVERY: return "RECOVERY";
        default: return "UNKNOWN";
        }
    }

    std::string getCurrentFiringPhaseDisplayName(const SpectralShipGen::ShipFiringAnimation& animation, uint32_t frameIndex)
    {
        if (animation.NormalizedSampleTimes.empty()) { return "REST"; }
        const uint32_t index = std::min(frameIndex, static_cast<uint32_t>(animation.NormalizedSampleTimes.size() - 1u));
        return getFiringPhaseDisplayName(SpectralShipGen::getFiringAnimationPhase(animation.NormalizedSampleTimes[index]));
    }

    std::string getSameDifferent(bool same)
    {
        return same ? "SAME" : "DIFFERENT";
    }

    uint32_t calculateComparisonScale(const SpectralShipGenStudioPreview::PreviewGenerationRecipe& pinned, const SpectralShipGenStudioPreview::PreviewGenerationRecipe& current, uint32_t availableWidth, uint32_t availableHeight)
    {
        const uint32_t maximumWidth = std::max(pinned.Dimensions.Width, current.Dimensions.Width);
        const uint32_t maximumHeight = std::max(pinned.Dimensions.Height, current.Dimensions.Height);

        if (maximumWidth == 0u || maximumHeight == 0u)
        {
            return 1u;
        }

        return std::max(1u, std::min(availableWidth / maximumWidth, availableHeight / maximumHeight));
    }

    std::string getChangedRecipeComponents(const SpectralShipGenStudioPreview::PreviewGenerationRecipe& pinned, const SpectralShipGenStudioPreview::PreviewGenerationRecipe& current)
    {
        std::vector<std::string> changed;
        if (pinned.Seeds.Structure != current.Seeds.Structure) { changed.push_back("STRUCTURE"); }
        if (pinned.Seeds.Palette != current.Seeds.Palette) { changed.push_back("PALETTE"); }
        if (pinned.Seeds.Details != current.Seeds.Details || pinned.DetailDensity != current.DetailDensity || pinned.AsymmetricDetailChance != current.AsymmetricDetailChance) { changed.push_back("DETAILS"); }
        if (pinned.Seeds.Attachments != current.Seeds.Attachments || pinned.AttachmentsEnabled != current.AttachmentsEnabled) { changed.push_back("ATTACHMENTS"); }
        if (pinned.StructuralPreset != current.StructuralPreset) { changed.push_back("STRUCTURE"); }
        if (pinned.FactionPreset != current.FactionPreset) { changed.push_back("FACTION"); }
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

    std::string getMillisecondsString(double milliseconds)
    {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(1) << milliseconds << " ms";
        return stream.str();
    }

    std::string colorToHex(const SpectralShipGen::Color& color)
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

    void drawDiagnosticLegendEntry(sf::RenderTarget& target, const std::string& label, const SpectralShipGen::Color& color, float x, float& y)
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

namespace SpectralShipGenStudioPreview
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
        else if (data.Workspace == PreviewWorkspace::INSPECT && data.Ship == nullptr)
        {
            renderInspectionEmptyState(window);
        }
        else if (data.Workspace == PreviewWorkspace::ANIMATION && data.Ship == nullptr)
        {
            renderAnimationEmptyState(window);
        }
        else if (data.Workspace == PreviewWorkspace::INSPECT && data.Comparison != nullptr && data.Comparison->ViewEnabled && data.Comparison->Pinned.Valid && data.CurrentStaticTexture != nullptr && data.PinnedTexture != nullptr && data.Recipe != nullptr)
        {
            renderComparison(window, data);
        }
        else if (data.PreviewSprite != nullptr)
        {
            renderSingle(window, *data.PreviewSprite);
        }

        const bool singlePreviewMode = data.Mode == PreviewMode::STATIC || data.Mode == PreviewMode::ANIMATION || data.Mode == PreviewMode::FRAME_INSPECTION || data.Mode == PreviewMode::CONFIGURATION_EDITOR;
        const bool comparisonVisible = data.Workspace == PreviewWorkspace::INSPECT && data.Comparison != nullptr && data.Comparison->ViewEnabled && data.Comparison->Pinned.Valid;
        if (singlePreviewMode && !comparisonVisible && data.NativePreviewTexture != nullptr && data.Recipe != nullptr && (data.Workspace != PreviewWorkspace::INSPECT || data.Ship != nullptr) && (data.Workspace != PreviewWorkspace::ANIMATION || data.Ship != nullptr))
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

        if (data.WorkspaceNavigation != nullptr) { renderWorkspaceNavigation(window, *data.WorkspaceNavigation); }

        if (data.Diagnostics != nullptr)
        {
            if (data.Diagnostics->HelpVisible) { renderHelpOverlay(window, data.Workspace); }
            if (data.Workspace == PreviewWorkspace::INSPECT && data.Diagnostics->GenerationInspectorVisible) { renderGenerationInspector(window, data); }
            if (data.Workspace == PreviewWorkspace::INSPECT && data.Diagnostics->PaletteInspectorVisible) { renderPaletteInspector(window, data); }
        }

        window.display();
    }

    void PreviewRenderer::renderCommandPanel(sf::RenderWindow& window, const PreviewCommandPanel& commandPanel) const
    {
        drawPanel(window, CommandPanelX, 0.0f, static_cast<float>(PreviewCommandPanelWidth), static_cast<float>(PreviewWindowHeight), sf::Color(18, 19, 24, 248), sf::Color(72, 76, 88));
        drawDebugText(window, "COMMANDS", CommandPanelX + 10.0f, static_cast<float>(PreviewWorkspaceNavigationHeight) + 4.0f, sf::Color(240, 215, 105), TextScale);

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
            const std::string shortcut = commandPanel.getMode() == PreviewCommandPanelMode::GENERATE && button.Command.Type == PreviewCommandType::TOGGLE_ANIMATION ? std::string() : std::string(commandData.Shortcut);
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
            const std::string value = fitDebugTextToWidth(selector.Value, selector.ValueBounds.width - 8.0f, TextScale);
            const float valueWidth = getDebugTextWidth(value, TextScale);
            drawDebugText(window, value, selector.ValueBounds.left + std::max(4.0f, (selector.ValueBounds.width - valueWidth) * 0.5f), selector.ValueBounds.top + 6.0f, sf::Color(232, 234, 240), TextScale);
        }

        const auto drawDimensionSlider = [&](const PreviewCommandPanelSlider& slider)
            {
                drawPanel(window, slider.ValueBounds.left, slider.ValueBounds.top, slider.ValueBounds.width, slider.ValueBounds.height, slider.Enabled ? sf::Color(24, 26, 32) : sf::Color(20, 21, 25), sf::Color(58, 62, 72));
                drawDebugText(window, slider.Label, slider.ValueBounds.left + 5.0f, slider.ValueBounds.top + 4.0f, sf::Color(160, 165, 180), SmallTextScale);
                const std::string valueText = slider.ApplyCommand == PreviewCommandType::SET_ANIMATION_NORMALIZED_TIME
                    ? getFixedString(static_cast<double>(slider.Value) / 1000.0, 3u)
                    : std::to_string(slider.Value);
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
        else if (commandPanel.getMode() == PreviewCommandPanelMode::GENERATE)
        {
            drawDimensionSlider(commandPanel.getWidthSlider());
            drawDimensionSlider(commandPanel.getHeightSlider());
        }
        else if (commandPanel.getMode() == PreviewCommandPanelMode::ANIMATION)
        {
            drawDimensionSlider(commandPanel.getAnimationTimelineSlider());
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
        const bool bundleEditor = editor.getProfileKind() == ConfigurationEditorProfileKind::FULL_CONFIGURATION;
        const char* editorTitle = bundleEditor ? "FULL CONFIGURATION BUNDLE" : paletteEditor ? "PALETTE CONFIGURATION EDITOR" : factionEditor ? "FACTION PROFILE EDITOR" : "STRUCTURAL PROFILE EDITOR";
        const char* editorSubtitle = bundleEditor ? "APPLICATION BUNDLE / STRUCTURAL + FACTION + PALETTE" : paletteEditor ? "PUBLIC ShipPaletteConfiguration / GENERATED + FIXED" : factionEditor ? "PUBLIC ShipFactionProfile / USER PRESETS" : "PUBLIC ShipGenerationProfile / USER PRESETS";
        drawDebugText(window, editorTitle, panel.Left + 18.0f, panel.Top + 12.0f, sf::Color(240, 215, 105), TextScale);
        drawDebugText(window, editorSubtitle, panel.Left + 18.0f, panel.Top + 34.0f, sf::Color(125, 180, 215), EditorSecondaryTextScale);
        drawDebugText(window, "Ctrl+D duplicate | Ctrl+O import | Ctrl+E export", panel.Left + panel.Width - 360.0f, panel.Top + 12.0f, sf::Color(130, 135, 150), EditorSecondaryTextScale);

        drawPanel(window, viewport.Left - 6.0f, viewport.Top - 4.0f, viewport.Width + 12.0f, viewport.Height + 8.0f, sf::Color(13, 14, 18, 248), sf::Color(48, 52, 62));

        const auto visible = [&](const ConfigurationEditorRect& bounds)
            {
                return bounds.Top + bounds.Height >= viewport.Top && bounds.Top <= viewport.Top + viewport.Height;
            };
        const auto drawSmallButton = [&](const ConfigurationEditorRect& bounds, const char* label, bool enabled = true)
            {
                if (!visible(bounds)) { return; }
                drawPanel(window, bounds.Left, bounds.Top, bounds.Width, bounds.Height, enabled ? sf::Color(38, 42, 51) : sf::Color(24, 25, 29), enabled ? sf::Color(82, 92, 110) : sf::Color(46, 48, 55));
                const std::string fittedLabel = fitDebugTextToWidth(label, std::max(4.0f, bounds.Width - 6.0f), EditorTextScale);
                const float textWidth = getDebugTextWidth(fittedLabel, EditorTextScale);
                drawDebugText(window, fittedLabel, bounds.Left + std::max(2.0f, (bounds.Width - textWidth) * 0.5f), bounds.Top + 6.0f, enabled ? sf::Color(220, 224, 232) : sf::Color(90, 94, 105), EditorTextScale);
            };
        const auto drawInteger = [&](const ConfigurationIntegerControl& control, bool showProbability = false, uint32_t probability = 0u)
            {
                if (!visible(control.RowBounds)) { return; }
                const float labelWidth = showProbability ? 128.0f : 238.0f;
                drawDebugText(window, fitDebugTextToWidth(control.Label, labelWidth, EditorTextScale), control.RowBounds.Left + 6.0f, control.RowBounds.Top + 9.0f, sf::Color(185, 190, 204), EditorTextScale);
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
                drawDebugText(window, value, control.TrackBounds.Left + control.TrackBounds.Width + 8.0f, control.RowBounds.Top + 8.0f, sf::Color(232, 234, 240), EditorTextScale);
                if (showProbability)
                {
                    const std::string probabilityText = fitDebugTextToWidth(std::to_string(probability) + "% share", 108.0f, EditorTextScale);
                    drawDebugText(window, probabilityText, control.TrackBounds.Left - 110.0f, control.RowBounds.Top + 8.0f, sf::Color(125, 175, 205), EditorTextScale);
                }
                drawSmallButton(control.DecrementBounds, "-");
                drawSmallButton(control.IncrementBounds, "+");
            };
        const auto drawRange = [&](const ConfigurationRangeControl& range)
            {
                if (!visible(range.RowBounds)) { return; }
                drawDebugText(window, fitDebugTextToWidth(range.Label, 178.0f, EditorTextScale), range.RowBounds.Left + 6.0f, range.RowBounds.Top + 9.0f, sf::Color(185, 190, 204), EditorTextScale);
                const float span = static_cast<float>(std::max(1, range.MaximumLimit - range.MinimumLimit));
                const auto drawEndpoint = [&](const ConfigurationEditorRect& trackBounds, int32_t value, const char* prefix, bool dragging)
                    {
                        drawDebugText(window, prefix + std::to_string(value), trackBounds.Left - 56.0f, range.RowBounds.Top + 9.0f, sf::Color(220, 224, 232), EditorTextScale);
                        sf::RectangleShape track(sf::Vector2f(trackBounds.Width, trackBounds.Height));
                        track.setPosition(trackBounds.Left, trackBounds.Top);
                        track.setFillColor(sf::Color(58, 64, 76));
                        window.draw(track);
                        const float normalized = static_cast<float>(value - range.MinimumLimit) / span;
                        sf::RectangleShape knob(sf::Vector2f(4.0f, 14.0f));
                        knob.setPosition(trackBounds.Left + normalized * trackBounds.Width - 2.0f, trackBounds.Top - 4.0f);
                        knob.setFillColor(dragging ? sf::Color(240, 215, 105) : sf::Color(120, 190, 230));
                        window.draw(knob);
                    };
                drawEndpoint(range.MinimumTrackBounds, range.MinimumValue, "MIN ", range.DraggingEndpoint == 0);
                drawSmallButton(range.MinimumDecrementBounds, "-");
                drawSmallButton(range.MinimumIncrementBounds, "+");
                drawEndpoint(range.MaximumTrackBounds, range.MaximumValue, "MAX ", range.DraggingEndpoint == 1);
                drawSmallButton(range.MaximumDecrementBounds, "-");
                drawSmallButton(range.MaximumIncrementBounds, "+");
            };
        const auto drawToggle = [&](const ConfigurationToggleControl& control)
            {
                if (!visible(control.RowBounds)) { return; }
                drawDebugText(window, fitDebugTextToWidth(control.Label, 198.0f, EditorTextScale), control.RowBounds.Left + 6.0f, control.RowBounds.Top + 9.0f, sf::Color(185, 190, 204), EditorTextScale);
                drawPanel(window, control.ToggleBounds.Left, control.ToggleBounds.Top, control.ToggleBounds.Width, control.ToggleBounds.Height,
                    control.Value ? sf::Color(45, 78, 62) : sf::Color(38, 42, 51), control.Value ? sf::Color(100, 190, 130) : sf::Color(82, 92, 110));
                const std::string text = control.getDisplayValue();
                const float textWidth = getDebugTextWidth(text, EditorTextScale);
                drawDebugText(window, text, control.ToggleBounds.Left + std::max(3.0f, (control.ToggleBounds.Width - textWidth) * 0.5f), control.ToggleBounds.Top + 8.0f,
                    control.Value ? sf::Color(180, 235, 195) : sf::Color(210, 214, 224), EditorTextScale);
            };
        const auto drawChoice = [&](const ConfigurationChoiceControl& control)
            {
                if (!visible(control.RowBounds)) { return; }
                drawDebugText(window, fitDebugTextToWidth(control.Label, 198.0f, EditorTextScale), control.RowBounds.Left + 6.0f, control.RowBounds.Top + 9.0f, sf::Color(185, 190, 204), EditorTextScale);
                const std::string choiceValue = fitDebugTextToWidth(control.getDisplayValue(), 140.0f, EditorTextScale);
                drawDebugText(window, choiceValue, control.PreviousBounds.Left - 150.0f, control.RowBounds.Top + 9.0f, sf::Color(232, 234, 240), EditorTextScale);
                drawSmallButton(control.PreviousBounds, "<");
                drawSmallButton(control.NextBounds, ">");
            };
        const auto drawColor = [&](const ConfigurationColorControl& control)
            {
                if (!visible(control.RowBounds)) { return; }
                drawDebugText(window, fitDebugTextToWidth(control.Label, 208.0f, EditorTextScale), control.RowBounds.Left + 6.0f, control.RowBounds.Top + 8.0f, sf::Color(185, 190, 204), EditorTextScale);
                static constexpr std::array<const char*, 4u> channelLabels = { "R", "G", "B", "A" };
                for (std::size_t channel = 0u; channel < control.TrackBounds.size(); ++channel)
                {
                    const ConfigurationEditorRect& trackBounds = control.TrackBounds[channel];
                    drawDebugText(window, channelLabels[channel], trackBounds.Left - 14.0f, trackBounds.Top - 3.0f, sf::Color(150, 160, 176), EditorTextScale);
                    sf::RectangleShape track(sf::Vector2f(trackBounds.Width, trackBounds.Height));
                    track.setPosition(trackBounds.Left, trackBounds.Top);
                    track.setFillColor(sf::Color(58, 64, 76));
                    window.draw(track);
                    const float normalized = static_cast<float>(control.getChannel(channel)) / 255.0f;
                    sf::RectangleShape knob(sf::Vector2f(4.0f, 11.0f));
                    knob.setPosition(trackBounds.Left + normalized * trackBounds.Width - 2.0f, trackBounds.Top - 3.0f);
                    knob.setFillColor(control.DraggingChannel == static_cast<int32_t>(channel) ? sf::Color(240, 215, 105) : sf::Color(120, 190, 230));
                    window.draw(knob);
                    drawDebugText(window, std::to_string(control.getChannel(channel)), trackBounds.Left + trackBounds.Width + 7.0f, trackBounds.Top - 3.0f, sf::Color(232, 234, 240), EditorTextScale);
                }
                drawPanel(window, control.SwatchBounds.Left, control.SwatchBounds.Top, control.SwatchBounds.Width, control.SwatchBounds.Height,
                    sf::Color(static_cast<sf::Uint8>(control.Red), static_cast<sf::Uint8>(control.Green), static_cast<sf::Uint8>(control.Blue), static_cast<sf::Uint8>(control.Alpha)), sf::Color(130, 135, 150));
            };
        const auto drawSectionHeader = [&](const std::string& label, const ConfigurationEditorRect& bounds, bool expanded)
            {
                if (!visible(bounds)) { return; }
                drawPanel(window, bounds.Left, bounds.Top, bounds.Width, bounds.Height, sf::Color(29, 32, 40), sf::Color(55, 64, 78));
                drawDebugText(window, expanded ? "[-]" : "[+]", bounds.Left + 6.0f, bounds.Top + 7.0f, sf::Color(125, 190, 230), EditorTextScale);
                drawDebugText(window, label, bounds.Left + 34.0f, bounds.Top + 7.0f, sf::Color(205, 212, 225), EditorTextScale);
            };

        const ConfigurationTextField& nameField = editor.getNameField();
        if (visible(nameField.Bounds))
        {
            drawDebugText(window, nameField.Label, nameField.Bounds.Left + 6.0f, nameField.Bounds.Top + 7.0f, sf::Color(170, 175, 190), EditorTextScale);
            const float boxLeft = nameField.Bounds.Left + 150.0f;
            const float boxWidth = nameField.Bounds.Width - 156.0f;
            drawPanel(window, boxLeft, nameField.Bounds.Top + 2.0f, boxWidth, nameField.Bounds.Height - 4.0f, sf::Color(23, 25, 31), nameField.Focused ? sf::Color(135, 185, 225) : sf::Color(61, 66, 78));
            drawDebugText(window, nameField.Value + (nameField.Focused ? "_" : ""), boxLeft + 7.0f, nameField.Bounds.Top + 8.0f, sf::Color(232, 234, 240), EditorTextScale);
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
        if (bundleEditor)
        {
            for (const ConfigurationBundleComponentControl& component : editor.getBundleComponentControls())
            {
                if (!visible(component.RowBounds)) { continue; }
                drawDebugText(window, component.Label, component.RowBounds.Left + 6.0f, component.RowBounds.Top + 9.0f, sf::Color(185, 190, 204), EditorTextScale);
                const float valueLeft = component.RowBounds.Left + 150.0f;
                const float valueWidth = std::max(0.0f, component.ReplaceBounds.Left - valueLeft - 8.0f);
                const std::string value = fitDebugTextToWidth(component.Value.empty() ? "CUSTOM" : component.Value, valueWidth, EditorTextScale);
                drawDebugText(window, value, valueLeft, component.RowBounds.Top + 9.0f, sf::Color(232, 234, 240), EditorTextScale);
                drawSmallButton(component.ReplaceBounds, "USE CURRENT");
            }
        }
        else if (paletteEditor)
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
            const auto drawIssue = [&](const SpectralShipGen::ValidationIssue& issue, const sf::Color& color)
                {
                    if (y > viewport.Top + viewport.Height) { return; }
                    if (y + 24.0f >= viewport.Top)
                    {
                        drawDebugText(window, fitDebugTextToWidth(issue.Field.empty() ? "CONFIG" : issue.Field, 160.0f, EditorTextScale), viewport.Left + 8.0f, y, color, EditorTextScale);
                        drawDebugText(window, fitDebugTextToWidth(issue.Message, viewport.Width - 188.0f, EditorTextScale), viewport.Left + 180.0f, y, sf::Color(190, 194, 204), EditorTextScale);
                    }
                    y += 28.0f;
                };
            const auto& validation = editor.getValidationResult();
            if (validation.Errors.empty() && validation.Warnings.empty())
            {
                if (y >= viewport.Top && y <= viewport.Top + viewport.Height) { drawDebugText(window, "VALID / NO CORE VALIDATION ISSUES", viewport.Left + 8.0f, y, sf::Color(125, 215, 150), EditorTextScale); }
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
            const std::string actionLabel = fitDebugTextToWidth(button.Label, button.Bounds.Width - 8.0f, EditorTextScale);
            const float textWidth = getDebugTextWidth(actionLabel, EditorTextScale);
            drawDebugText(window, actionLabel, button.Bounds.Left + std::max(4.0f, (button.Bounds.Width - textWidth) * 0.5f), button.Bounds.Top + 8.0f, button.Enabled ? sf::Color(225, 229, 237) : sf::Color(90, 94, 105), EditorTextScale);
        }

        const std::string dirty = editor.hasUnsavedChanges() ? "UNAPPLIED CHANGES" : "NO CHANGES";
        const char* valueLabel = bundleEditor ? "COMPONENTS" : "PROFILE VALUES";
        drawDebugText(window, dirty + "  |  " + std::to_string(editor.getBoundValueCount()) + " " + valueLabel, panel.Left + 18.0f, panel.Top + panel.Height - 22.0f, editor.hasUnsavedChanges() ? sf::Color(235, 195, 100) : sf::Color(120, 175, 140), EditorSecondaryTextScale);
    }

    void PreviewRenderer::renderCalibration(sf::RenderWindow& window, const PreviewRenderData& data) const
    {
        const CalibrationCandidatePair& pair = *data.CalibrationPair;
        drawPanel(window, 12.0f, static_cast<float>(PreviewWorkspaceNavigationHeight) + 8.0f, static_cast<float>(PreviewContentWidth) - 24.0f, 42.0f, sf::Color(19, 21, 27), sf::Color(74, 80, 94));
        drawDebugText(window, "CALIBRATION LAB", 24.0f, static_cast<float>(PreviewWorkspaceNavigationHeight) + 18.0f, sf::Color(240, 215, 105), TextScale);
        drawDebugText(window, getCalibrationGroupName(data.CalibrationGroup), 210.0f, static_cast<float>(PreviewWorkspaceNavigationHeight) + 18.0f, sf::Color(130, 195, 230), TextScale);

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
        drawDebugText(window, std::to_string(pair.Recipe.Dimensions.Width) + "X" + std::to_string(pair.Recipe.Dimensions.Height) + "  " + getStructuralRecipeDisplayName(pair.Recipe) + "  " + getFactionRecipeDisplayName(pair.Recipe), 22.0f, infoY, sf::Color(215, 220, 230), TextScale); infoY += 22.0f;
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

        drawDebugText(window, leftLabel, 20.0f, static_cast<float>(PreviewWorkspaceNavigationHeight) + 12.0f, leftColor, TextScale);
        drawDebugText(window, rightLabel, columnWidth + 20.0f, static_cast<float>(PreviewWorkspaceNavigationHeight) + 12.0f, rightColor, TextScale);

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
        drawLabelValue(window, "STRUCT", getStructuralRecipeDisplayName(pinnedRecipe), 20.0f, pinnedY);
        drawLabelValue(window, "FACTION", getFactionRecipeDisplayName(pinnedRecipe), 20.0f, pinnedY);
        drawLabelValue(window, "ATT GEN", getOnOff(pinnedRecipe.AttachmentsEnabled), 20.0f, pinnedY);
        drawLabelValue(window, "SEED", std::to_string(currentRecipe.Seeds.Master), columnWidth + 20.0f, currentY);
        drawLabelValue(window, "RES", std::to_string(currentRecipe.Dimensions.Width) + "X" + std::to_string(currentRecipe.Dimensions.Height), columnWidth + 20.0f, currentY);
        drawLabelValue(window, "STRUCT", getStructuralRecipeDisplayName(currentRecipe), columnWidth + 20.0f, currentY);
        drawLabelValue(window, "FACTION", getFactionRecipeDisplayName(currentRecipe), columnWidth + 20.0f, currentY);
        drawLabelValue(window, "ATT GEN", getOnOff(currentRecipe.AttachmentsEnabled), columnWidth + 20.0f, currentY);

        float differenceY = 730.0f;
        drawSectionHeader(window, "RECIPE DIFFERENCES", 20.0f, differenceY);
        drawDebugText(window, "CHANGED: " + getChangedRecipeComponents(pinnedRecipe, currentRecipe), 20.0f, differenceY, sf::Color(225, 228, 235), TextScale);
        differenceY += LargeTextLineHeight + 3.0f;
        drawDebugText(window, "STRUCT SEED: " + getSameDifferent(pinnedRecipe.Seeds.Structure == currentRecipe.Seeds.Structure) + "    PALETTE SEED: " + getSameDifferent(pinnedRecipe.Seeds.Palette == currentRecipe.Seeds.Palette), 20.0f, differenceY, sf::Color(185, 190, 205), SmallTextScale);
        differenceY += TextLineHeight;
        drawDebugText(window, "DETAIL SEED: " + getSameDifferent(pinnedRecipe.Seeds.Details == currentRecipe.Seeds.Details) + "    ATTACH SEED: " + getSameDifferent(pinnedRecipe.Seeds.Attachments == currentRecipe.Seeds.Attachments), 20.0f, differenceY, sf::Color(185, 190, 205), SmallTextScale);
        differenceY += TextLineHeight;
        drawDebugText(window, "STRUCT: " + getSameDifferent(pinnedRecipe.StructuralPreset == currentRecipe.StructuralPreset) + "    FACTION: " + getSameDifferent(pinnedRecipe.FactionPreset == currentRecipe.FactionPreset) + "    RES: " + getSameDifferent(pinnedRecipe.Dimensions.Width == currentRecipe.Dimensions.Width && pinnedRecipe.Dimensions.Height == currentRecipe.Dimensions.Height), 20.0f, differenceY, sf::Color(185, 190, 205), SmallTextScale);
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
        SpectralShipGen::GenerationDomain firstSelected = SpectralShipGen::GenerationDomain::GENERATION_DOMAIN_END;
        for (std::size_t index = 0u; index < studio.SelectedDomains.size(); ++index)
        {
            if (!studio.SelectedDomains[index]) { continue; }
            const SpectralShipGen::GenerationDomain domain = static_cast<SpectralShipGen::GenerationDomain>(index);
            if (firstSelected == SpectralShipGen::GenerationDomain::GENERATION_DOMAIN_END) { firstSelected = domain; }
            if (!selectedText.empty()) { selectedText += ", "; }
            selectedText += SpectralShipGen::getGenerationDomainName(domain);
        }
        if (selectedText.empty()) { selectedText = "NONE"; }

        drawDebugText(window, "SELECTED = REROLLED; UNSELECTED DOMAIN SEEDS ARE PRESERVED.", 20.0f, 575.0f, sf::Color(165, 205, 175), SmallTextScale);
        drawDebugText(window, wrapDebugText("SELECTED: " + selectedText, 102u), 20.0f, 591.0f, sf::Color(220, 223, 232), SmallTextScale);

        if (firstSelected != SpectralShipGen::GenerationDomain::GENERATION_DOMAIN_END)
        {
            drawDebugText(window, wrapDebugText(std::string("DEPENDENCY: ") + SpectralShipGen::getGenerationDomainDependencyDescription(firstSelected), 102u), 20.0f, 611.0f, sf::Color(170, 175, 190), SmallTextScale);
        }

        if (studio.CandidateValid)
        {
            const SpectralShipGen::GenerationDomainSeeds baseSeeds = SpectralShipGen::resolveGenerationDomainSeeds(baseRecipe.Seeds, baseRecipe.DomainSeedOverrides);
            const SpectralShipGen::GenerationDomainSeeds candidateSeeds = SpectralShipGen::resolveGenerationDomainSeeds(candidateRecipe.Seeds, candidateRecipe.DomainSeedOverrides);
            std::string seedText = "DOMAIN SEEDS:";
            uint32_t shown = 0u;
            for (std::size_t index = 0u; index < studio.SelectedDomains.size() && shown < 3u; ++index)
            {
                if (!studio.SelectedDomains[index]) { continue; }
                const SpectralShipGen::GenerationDomain domain = static_cast<SpectralShipGen::GenerationDomain>(index);
                seedText += " ";
                seedText += SpectralShipGen::getGenerationDomainName(domain);
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
        if (favoritesState.Grid.Items.empty())
        {
            drawDebugText(window, "No Favorites yet.", 28.0f, static_cast<float>(PreviewWorkspaceNavigationHeight) + 28.0f, sf::Color(205, 208, 216), TextScale);
            drawDebugText(window, "Bookmark ships from Generate, Gallery, Inspect or Animation to add them here.", 28.0f, static_cast<float>(PreviewWorkspaceNavigationHeight) + 52.0f, sf::Color(155, 160, 172), SmallTextScale);
            return;
        }
        renderThumbnailGrid(window, favoritesState.Grid, false);
    }

    void PreviewRenderer::renderGallery(sf::RenderWindow& window, const GalleryState& galleryState) const
    {
        renderThumbnailGrid(window, galleryState.Grid, true);
    }

    void PreviewRenderer::renderThumbnailGrid(sf::RenderWindow& window, const PreviewThumbnailGridState& gridState, bool showFavoriteMarkers) const
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

                if (showFavoriteMarkers && item.Favorite)
                {
                    constexpr float markerWidth = 26.0f;
                    constexpr float markerHeight = 14.0f;
                    const float markerX = cellBounds.left + cellBounds.width - markerWidth - 4.0f;
                    const float markerY = cellBounds.top + 4.0f;
                    drawPanel(window, markerX, markerY, markerWidth, markerHeight, sf::Color(28, 29, 34, 235), sf::Color(220, 190, 80));
                    drawDebugText(window, "FAV", markerX + 4.0f, markerY + 3.0f, sf::Color(235, 210, 95), SmallTextScale);
                }
            }
            else
            {
                drawDebugText(window, "UNAVAILABLE", cellBounds.left + 12.0f, cellBounds.top + cellBounds.height * 0.5f - 5.0f, sf::Color(190, 105, 105), SmallTextScale);
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

        const SpectralShipGen::ShipDimensions dimensions = data.Recipe->Dimensions;
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
        float y = static_cast<float>(PreviewWorkspaceNavigationHeight) + 14.0f;
        drawSectionHeader(window, "CURRENT STATE", x, y);

        if (data.Workspace == PreviewWorkspace::INSPECT)
        {
            renderInspectionStatePanel(window, data, x, y);
            return;
        }

        drawLabelValue(window, "MODE", getPreviewModeName(data.Mode), x, y);

        if (data.Workspace == PreviewWorkspace::ANIMATION)
        {
            y += 4.0f;
            drawSectionHeader(window, "ANIMATION LAB", x, y);
            drawLabelValue(window, "BASE", getAnimationTypeDisplayName(data.RuntimeMovementType), x, y);
            drawLabelValue(window, "TRANSIENT", data.TransientStatePreviewActive ? "FIRE" : "NONE", x, y);
            drawLabelValue(window, "TIME", getFixedString(data.AnimationNormalizedTime, 3u), x, y);
            drawLabelValue(window, "PHASE", data.AnimationSemanticPhase.empty() ? "-" : data.AnimationSemanticPhase, x, y);
            drawLabelValue(window, "PLAYBACK", data.Mode == PreviewMode::ANIMATION ? "PLAY" : "PAUSED", x, y);
            drawLabelValue(window, "SPEED", getFixedString(data.AnimationPlaybackSpeed, data.AnimationPlaybackSpeed < 1.0 ? 2u : 1u) + "x", x, y);
            drawLabelValue(window, "LOOP", data.AnimationLooping ? "LOOP" : "ONE-SHOT", x, y);
            drawLabelValue(window, "ANIMATED", std::to_string(data.AnimationAnimatedComponentCount), x, y);
        }

        if (data.Mode == PreviewMode::CALIBRATION && data.CalibrationSession != nullptr)
        {
            const CalibrationGroupStatistics statistics = calculateCalibrationGroupStatistics(*data.CalibrationSession, data.CalibrationGroup, data.CalibrationFilter);
            const std::optional<SpectralShipGen::ShipStyle> structuralPreset = data.Recipe != nullptr ? data.Recipe->StructuralPreset : std::nullopt;
            const std::vector<uint32_t> suggested = structuralPreset.has_value() ? calculateSuggestedGroupWeights(*data.CalibrationSession, *structuralPreset, data.CalibrationGroup, data.CalibrationFilter) : std::vector<uint32_t>{};
            drawLabelValue(window, "GROUP", getCalibrationGroupName(data.CalibrationGroup), x, y);
            drawLabelValue(window, "EVIDENCE", getCalibrationEvidenceName(statistics.Evidence), x, y);
            drawLabelValue(window, "USEFUL", std::to_string(statistics.UsefulComparisonCount), x, y);
            drawLabelValue(window, "RECORDS", std::to_string(data.CalibrationSession->Records.size()), x, y);
            drawLabelValue(window, "FILTER", data.CalibrationFilter.Style.has_value() ? "CURRENT CONTEXT" : "ALL", x, y);
            drawLabelValue(window, "VALUES", data.CalibrationShowValues ? "VISIBLE" : "BLIND", x, y);

            y += 4.0f;
            drawSectionHeader(window, "PREFERENCE", x, y);
            const uint32_t optionCount = structuralPreset.has_value() ? SpectralShipGen::getGenerationWeightOptionCount(data.CalibrationGroup) : 0u;
            for (uint32_t index = 0u; index < optionCount && index < statistics.Options.size(); ++index)
            {
                const CalibrationOptionStatistics& option = statistics.Options[index];
                const uint32_t current = SpectralShipGen::getGenerationTuningWeight(data.CalibrationSession->TunedProfile, *structuralPreset, data.CalibrationGroup, index);
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
                const auto drawObjective = [&](const char* label, const SpectralShipGen::ShipGenerationDebugInfo& info)
                    {
                        drawDebugText(window, std::string(label) + " ENG " + std::to_string(info.EngineCount) + " FEAT " + std::to_string(info.MajorFeatureCount) + " GUN " + std::to_string(info.WeaponCount) + " ATT " + std::to_string(info.AttachmentPlacedGroupCount), x, y, sf::Color(180, 185, 195), SmallTextScale);
                        y += 14.0f;
                    };
                const SpectralShipGen::ShipGenerationDebugInfo& left = data.CalibrationPair->DisplayAOnLeft ? data.CalibrationPair->DebugA : data.CalibrationPair->DebugB;
                const SpectralShipGen::ShipGenerationDebugInfo& right = data.CalibrationPair->DisplayAOnLeft ? data.CalibrationPair->DebugB : data.CalibrationPair->DebugA;
                drawObjective("A", left);
                drawObjective("B", right);
            }

            if (data.ObjectiveBatch != nullptr && data.ObjectiveBatch->Valid)
            {
                const auto& objective = *data.ObjectiveBatch;
                y += 4.0f;
                drawSectionHeader(window, "OBJECTIVE BATCH", x, y);
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


        if (data.Mode == PreviewMode::GALLERY && data.Gallery != nullptr)
        {
            drawLabelValue(window, "BATCH", std::to_string(data.Gallery->BatchSeed), x, y);
            drawLabelValue(window, "SELECT", data.Gallery->Grid.Items.empty() ? "0/0" : std::to_string(data.Gallery->Grid.SelectedIndex + 1u) + "/" + std::to_string(data.Gallery->Grid.Items.size()), x, y);
        }

        if (data.Mode == PreviewMode::FAVORITES && data.Favorites != nullptr)
        {
            const PreviewThumbnailGridState& grid = data.Favorites->Grid;
            const uint32_t pageCount = getPreviewThumbnailPageCount(grid);
            y += 4.0f;
            drawSectionHeader(window, "SELECTED FAVORITE", x, y);
            drawLabelValue(window, "SELECT", grid.Items.empty() ? "0/0" : std::to_string(grid.SelectedIndex + 1u) + "/" + std::to_string(grid.Items.size()), x, y);
            drawLabelValue(window, "PAGE", pageCount == 0u ? "0/0" : std::to_string(getPreviewThumbnailCurrentPage(grid) + 1u) + "/" + std::to_string(pageCount), x, y);

            if (!grid.Items.empty() && grid.SelectedIndex < grid.Items.size())
            {
                const PreviewThumbnailItem& item = grid.Items[grid.SelectedIndex];
                const PreviewGenerationRecipe& favoriteRecipe = item.Recipe;
                const std::string structuralSource = getPresetSourceName(favoriteRecipe.StructuralPreset.has_value());
                const std::string factionSource = getPresetSourceName(favoriteRecipe.FactionPreset.has_value());
                std::string paletteSource = "UNKNOWN";
                switch (favoriteRecipe.PaletteConfiguration.Mode)
                {
                case SpectralShipGen::ShipPaletteSourceMode::FACTION_PROFILE_GENERATED: paletteSource = "FACTION GENERATED"; break;
                case SpectralShipGen::ShipPaletteSourceMode::EXPLICIT_GENERATED: paletteSource = "EXPLICIT GENERATED"; break;
                case SpectralShipGen::ShipPaletteSourceMode::FIXED: paletteSource = "FIXED"; break;
                default: break;
                }
                drawLabelValue(window, "STATUS", item.Valid ? "AVAILABLE" : "UNAVAILABLE", x, y);
                drawLabelValue(window, "SEED", std::to_string(favoriteRecipe.Seeds.Master), x, y);
                drawLabelValue(window, "RES", std::to_string(favoriteRecipe.Dimensions.Width) + "X" + std::to_string(favoriteRecipe.Dimensions.Height), x, y);
                drawLabelValue(window, "STRUCT", getStructuralRecipeDisplayName(favoriteRecipe), x, y);
                drawLabelValue(window, "STRUCT SRC", structuralSource, x, y);
                drawLabelValue(window, "FACTION", getFactionRecipeDisplayName(favoriteRecipe), x, y);
                drawLabelValue(window, "FACTION SRC", factionSource, x, y);
                drawLabelValue(window, "PALETTE", paletteSource, x, y);
            }

            y += 4.0f;
            drawSectionHeader(window, "CURRENT SHIP", x, y);
        }

        if (data.Recipe != nullptr)
        {
            drawLabelValue(window, "RES", std::to_string(data.Recipe->Dimensions.Width) + "X" + std::to_string(data.Recipe->Dimensions.Height), x, y);
            drawLabelValue(window, "STRUCTURE", fitDebugTextToWidth(data.StructuralDisplayName, 190.0f, SmallTextScale), x, y);
            drawLabelValue(window, "FACTION", fitDebugTextToWidth(data.FactionDisplayName, 190.0f, SmallTextScale), x, y);
            drawLabelValue(window, "MASTER", std::to_string(data.Recipe->Seeds.Master), x, y);
            drawLabelValue(window, "STRUCT SEED", std::to_string(data.Recipe->Seeds.Structure), x, y);
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

        if (data.SelectedAnimationType == SpectralShipGen::ShipAnimationType::IDLE && data.IdleAnimation != nullptr && data.IdleAnimationSettings != nullptr && !data.IdleAnimation->Frames.empty())
        {
            y += 4.0f;
            drawSectionHeader(window, "IDLE ANIMATION", x, y);
            drawLabelValue(window, "TYPE", getAnimationTypeDisplayName(data.SelectedAnimationType), x, y);
            drawLabelValue(window, "SEED", std::to_string(data.IdleAnimation->Seed), x, y);
            drawLabelValue(window, "DURATION", std::to_string(data.IdleAnimation->DurationMilliseconds) + " ms", x, y);
            drawLabelValue(window, "FRAMES", std::to_string(data.IdleAnimation->Sampling.ActualFrameCount), x, y);
            drawLabelValue(window, "SAMPLING", data.IdleAnimation->Sampling.Mode == SpectralShipGen::AnimationSamplingMode::ADAPTIVE ? "ADAPTIVE" : "EXACT", x, y);
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
        else if (data.SelectedAnimationType == SpectralShipGen::ShipAnimationType::FIRE && data.FiringAnimation != nullptr && data.FiringAnimationSettings != nullptr && !data.FiringAnimation->Frames.empty())
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
            drawLabelValue(window, "SAMPLING", data.FiringAnimation->Sampling.Mode == SpectralShipGen::AnimationSamplingMode::ADAPTIVE ? "ADAPTIVE" : "EXACT", x, y);
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
            const SpectralShipGen::ShipMovementAnimationClip& clip = SpectralShipGen::getMovementAnimationClip(*data.MovementAnimation, data.MovementPhase);
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
                drawLabelValue(window, "SAMPLING", clip.Sampling.Mode == SpectralShipGen::AnimationSamplingMode::ADAPTIVE ? "ADAPTIVE" : "EXACT", x, y);
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

    void PreviewRenderer::renderInspectionEmptyState(sf::RenderWindow& window) const
    {
        drawDebugText(window, "INSPECT", 36.0f, static_cast<float>(PreviewWorkspaceNavigationHeight) + 42.0f, sf::Color(240, 215, 105), TextScale);
        drawDebugText(window, "No current ship to inspect.", 36.0f, static_cast<float>(PreviewWorkspaceNavigationHeight) + 74.0f, sf::Color(210, 215, 225), TextScale);
        drawDebugText(window, "Generate or load a ship, then return to Inspect.", 36.0f, static_cast<float>(PreviewWorkspaceNavigationHeight) + 98.0f, sf::Color(150, 180, 205), SmallTextScale);
    }

    void PreviewRenderer::renderAnimationEmptyState(sf::RenderWindow& window) const
    {
        drawDebugText(window, "ANIMATION LAB", 36.0f, static_cast<float>(PreviewWorkspaceNavigationHeight) + 42.0f, sf::Color(240, 215, 105), TextScale);
        drawDebugText(window, "No current ship to animate.", 36.0f, static_cast<float>(PreviewWorkspaceNavigationHeight) + 74.0f, sf::Color(210, 215, 225), TextScale);
        drawDebugText(window, "Generate or load a ship, then return to Animation.", 36.0f, static_cast<float>(PreviewWorkspaceNavigationHeight) + 98.0f, sf::Color(150, 180, 205), SmallTextScale);
    }

    void PreviewRenderer::renderInspectionStatePanel(sf::RenderWindow& window, const PreviewRenderData& data, float x, float& y) const
    {
        if (data.Ship == nullptr || data.GenerationDebugInfo == nullptr || data.Recipe == nullptr)
        {
            drawLabelValue(window, "SHIP", "NONE", x, y);
            drawDebugText(window, "Generate or load a ship to inspect.", x, y + 4.0f, sf::Color(180, 185, 195), SmallTextScale);
            return;
        }

        const SpectralShipGen::GeneratedShip& ship = *data.Ship;
        const SpectralShipGen::ShipGenerationDebugInfo& debug = *data.GenerationDebugInfo;
        const PreviewGenerationRecipe& recipe = *data.Recipe;
        const auto paletteSourceName = [](SpectralShipGen::ShipPaletteSourceMode mode)
        {
            switch (mode)
            {
            case SpectralShipGen::ShipPaletteSourceMode::FACTION_PROFILE_GENERATED: return "FACTION GENERATED";
            case SpectralShipGen::ShipPaletteSourceMode::EXPLICIT_GENERATED: return "EXPLICIT GENERATED";
            case SpectralShipGen::ShipPaletteSourceMode::FIXED: return "FIXED";
            default: return "UNKNOWN";
            }
        };

        drawSectionHeader(window, "CONFIGURATION", x, y);
        drawLabelValue(window, "RES", std::to_string(recipe.Dimensions.Width) + "X" + std::to_string(recipe.Dimensions.Height), x, y);
        drawLabelValue(window, "MASTER", std::to_string(recipe.Seeds.Master), x, y);
        drawLabelValue(window, "STRUCT", fitDebugTextToWidth(data.StructuralDisplayName, 190.0f, SmallTextScale), x, y);
        drawLabelValue(window, "STRUCT SRC", getPresetSourceName(recipe.StructuralPreset.has_value()), x, y);
        drawLabelValue(window, "FACTION", fitDebugTextToWidth(data.FactionDisplayName, 190.0f, SmallTextScale), x, y);
        drawLabelValue(window, "FACTION SRC", getPresetSourceName(recipe.FactionPreset.has_value()), x, y);
        drawLabelValue(window, "PALETTE", fitDebugTextToWidth(data.PaletteDisplayName, 190.0f, SmallTextScale), x, y);
        drawLabelValue(window, "PALETTE SRC", paletteSourceName(recipe.PaletteConfiguration.Mode), x, y);
        if (!data.ConfigurationBundleDisplayName.empty() && data.ConfigurationBundleDisplayName != "INDIVIDUAL COMPONENTS")
        {
            drawLabelValue(window, "FULL CFG", fitDebugTextToWidth(data.ConfigurationBundleDisplayName, 190.0f, SmallTextScale), x, y);
        }
        y += 4.0f;
        drawSectionHeader(window, "INSPECTION", x, y);
        if (data.Diagnostics != nullptr)
        {
            drawLabelValue(window, "GROUP", getPreviewInspectionGroupName(data.Diagnostics->InspectionGroup), x, y);
            drawLabelValue(window, "VIEW", data.Diagnostics->GenerationStageView ? "Generation Stage" : getDiagnosticViewModeName(data.Diagnostics->ViewMode), x, y);
            drawLabelValue(window, "DISPLAY", getPreviewInspectionPresentationName(data.Diagnostics->InspectionPresentation), x, y);

            y += 2.0f;
            drawSectionHeader(window, "DEBUG COLORS", x, y);
            if (data.Diagnostics->GenerationStageView)
            {
                drawDiagnosticLegendEntry(window, "HULL STAGE", PreviewDiagnosticColors::Hull, x, y);
            }
            else
            {
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
                case DiagnosticViewMode::ATTACHMENTS:
                    drawDiagnosticLegendEntry(window, "OCCUPIED", PreviewDiagnosticColors::Attachment, x, y);
                    drawDiagnosticLegendEntry(window, "BOUNDS", PreviewDiagnosticColors::AttachmentBounds, x, y);
                    drawDiagnosticLegendEntry(window, "ROOT", PreviewDiagnosticColors::AttachmentRoot, x, y);
                    break;
                case DiagnosticViewMode::HULL_LAYERS:
                    drawDiagnosticLegendEntry(window, "LOWER LAYER", PreviewDiagnosticColors::HullLayerLower, x, y);
                    drawDiagnosticLegendEntry(window, "UPPER LAYER", PreviewDiagnosticColors::HullLayerUpper, x, y);
                    break;
                case DiagnosticViewMode::CORE_TREATMENT:
                    drawDiagnosticLegendEntry(window, "CORE REGION", PreviewDiagnosticColors::CoreRegion, x, y);
                    drawDiagnosticLegendEntry(window, "SECONDARY", PreviewDiagnosticColors::CoreSecondary, x, y);
                    drawDiagnosticLegendEntry(window, "RAISED", PreviewDiagnosticColors::CoreRaised, x, y);
                    drawDiagnosticLegendEntry(window, "RECESSED", PreviewDiagnosticColors::CoreRecessed, x, y);
                    drawDiagnosticLegendEntry(window, "LUMINOUS", PreviewDiagnosticColors::CoreLuminous, x, y);
                    break;
                case DiagnosticViewMode::WEAPONS:
                    drawDiagnosticLegendEntry(window, "OCCUPIED", PreviewDiagnosticColors::Weapon, x, y);
                    drawDiagnosticLegendEntry(window, "BOUNDS", PreviewDiagnosticColors::WeaponBounds, x, y);
                    drawDiagnosticLegendEntry(window, "ROOT", PreviewDiagnosticColors::WeaponRoot, x, y);
                    drawDiagnosticLegendEntry(window, "MUZZLE", PreviewDiagnosticColors::WeaponMuzzle, x, y);
                    break;
                case DiagnosticViewMode::DETAILS:
                    drawDiagnosticLegendEntry(window, "ACCENT", PreviewDiagnosticColors::Accent, x, y);
                    drawDiagnosticLegendEntry(window, "MECHANICAL", PreviewDiagnosticColors::Mechanical, x, y);
                    drawDiagnosticLegendEntry(window, "LIGHT", PreviewDiagnosticColors::Light, x, y);
                    break;
                case DiagnosticViewMode::MATERIALS:
                    drawDiagnosticLegendEntry(window, "SECONDARY HULL", PreviewDiagnosticColors::MaterialSecondary, x, y);
                    drawDiagnosticLegendEntry(window, "MECHANICAL", PreviewDiagnosticColors::MaterialMechanical, x, y);
                    break;
                case DiagnosticViewMode::LIVERY:
                    drawDiagnosticLegendEntry(window, "PRIMARY", PreviewDiagnosticColors::LiveryPrimary, x, y);
                    drawDiagnosticLegendEntry(window, "SECONDARY", PreviewDiagnosticColors::LiverySecondary, x, y);
                    break;
                case DiagnosticViewMode::DETAIL_MOTIFS:
                    drawDiagnosticLegendEntry(window, "PRIMARY", PreviewDiagnosticColors::MotifPrimary, x, y);
                    drawDiagnosticLegendEntry(window, "SECONDARY", PreviewDiagnosticColors::MotifSecondary, x, y);
                    break;
                case DiagnosticViewMode::MACRO_ASYMMETRY:
                    drawDiagnosticLegendEntry(window, "BASE HULL", PreviewDiagnosticColors::MacroAsymmetryBase, x, y);
                    drawDiagnosticLegendEntry(window, "ASYMMETRIC FEATURE", PreviewDiagnosticColors::MacroAsymmetryFeature, x, y);
                    break;
                case DiagnosticViewMode::NEGATIVE_SPACE:
                    drawDiagnosticLegendEntry(window, "RESERVED VOID", PreviewDiagnosticColors::NegativeSpace, x, y);
                    break;
                case DiagnosticViewMode::SEMANTIC_LOAD:
                    drawDiagnosticLegendEntry(window, "LOW LOAD", PreviewDiagnosticColors::SpatialLow, x, y);
                    drawDiagnosticLegendEntry(window, "MODERATE", PreviewDiagnosticColors::SpatialModerate, x, y);
                    drawDiagnosticLegendEntry(window, "HIGH", PreviewDiagnosticColors::SpatialHigh, x, y);
                    drawDiagnosticLegendEntry(window, "OVERLOADED", PreviewDiagnosticColors::SpatialOverloaded, x, y);
                    break;
                case DiagnosticViewMode::COMBINED:
                    drawDiagnosticLegendEntry(window, "HULL", PreviewDiagnosticColors::Hull, x, y);
                    drawDiagnosticLegendEntry(window, "COCKPIT", PreviewDiagnosticColors::Cockpit, x, y);
                    drawDiagnosticLegendEntry(window, "ENGINE", PreviewDiagnosticColors::Engine, x, y);
                    drawDiagnosticLegendEntry(window, "ATTACHMENT", PreviewDiagnosticColors::Attachment, x, y);
                    drawDiagnosticLegendEntry(window, "WEAPON", PreviewDiagnosticColors::Weapon, x, y);
                    break;
                default:
                    break;
                }
            }
        }

        y += 4.0f;
        drawSectionHeader(window, "GENERATION DECISIONS", x, y);
        drawLabelValue(window, "HULL ATTEMPTS", std::to_string(debug.HullGenerationAttemptCount), x, y);
        drawLabelValue(window, "PRIMARY ANCHOR", SpectralShipGen::getShipVisualAnchorTypeName(debug.PrimaryVisualAnchor), x, y);
        drawLabelValue(window, "ANCHOR REGION", SpectralShipGen::getGenerationSpatialRegionName(debug.VisualAnchorTargetRegion), x, y);
        drawLabelValue(window, "COMPLEXITY", std::to_string(debug.ComplexityConsumedBudget) + "/" + std::to_string(debug.ComplexityInitialBudget), x, y);
        drawLabelValue(window, "SPATIAL REJECT", std::to_string(debug.SpatialOverloadRejectionCount), x, y);
        drawLabelValue(window, "HULL LAYERS", std::to_string(debug.HullLayerCount), x, y);
        drawLabelValue(window, "MAJOR FEATURES", std::to_string(debug.MajorFeatureCount), x, y);
        drawLabelValue(window, "WEAPONS", std::to_string(debug.WeaponRealizedGroupCount) + "/" + std::to_string(debug.WeaponRequestedGroupCount) + " GROUPS", x, y);
        drawLabelValue(window, "NEG SPACE", std::to_string(debug.StructuralNegativeSpaceSuccessCount) + "/" + std::to_string(debug.StructuralNegativeSpaceAttemptCount), x, y);
        drawLabelValue(window, "MATERIAL ZONES", std::to_string(debug.MaterialZoneCount), x, y);
        drawLabelValue(window, "LIVERY", std::to_string(debug.LiveryCoveragePermille / 10u) + "." + std::to_string(debug.LiveryCoveragePermille % 10u) + "%", x, y);

        y += 4.0f;
        drawSectionHeader(window, "DOMAIN SEEDS", x, y);
        for (std::size_t index = 0u; index < SpectralShipGen::GenerationDomainCount; ++index)
        {
            const auto domain = static_cast<SpectralShipGen::GenerationDomain>(index);
            const bool overridden = recipe.DomainSeedOverrides.Values[index].has_value();
            const std::string label = std::string(overridden ? "*" : " ") + SpectralShipGen::getGenerationDomainName(domain);
            const std::string value = std::to_string(ship.DomainSeeds.Values[index]);
            drawDebugText(window, fitDebugTextToWidth(label, 128.0f, SmallTextScale), x, y, overridden ? sf::Color(240, 205, 105) : sf::Color(180, 185, 195), SmallTextScale);
            drawDebugText(window, value, x + 130.0f, y, sf::Color(145, 205, 235), SmallTextScale);
            y += 14.0f;
        }
        drawDebugText(window, "* explicit domain override", x, y + 2.0f, sf::Color(150, 155, 165), SmallTextScale);
    }

    void PreviewRenderer::renderWorkspaceNavigation(sf::RenderWindow& window, const PreviewWorkspaceNavigation& navigation) const
    {
        drawPanel(window, 0.0f, 0.0f, static_cast<float>(PreviewWindowWidth), static_cast<float>(PreviewWorkspaceNavigationHeight), sf::Color(13, 14, 18, 252), sf::Color(72, 76, 88));
        const auto& buttons = navigation.getButtons();
        const int32_t hovered = navigation.getHoveredButtonIndex();
        const int32_t pressed = navigation.getPressedButtonIndex();
        for (std::size_t index = 0u; index < buttons.size(); ++index)
        {
            const PreviewWorkspaceNavigationButton& button = buttons[index];
            sf::Color fill(30, 32, 39);
            sf::Color outline(72, 76, 88);
            sf::Color text(205, 210, 220);
            if (button.Active)
            {
                fill = sf::Color(45, 70, 58);
                outline = sf::Color(105, 190, 135);
                text = sf::Color(235, 240, 238);
            }
            else if (static_cast<int32_t>(index) == pressed)
            {
                fill = sf::Color(75, 82, 96);
                outline = sf::Color(180, 190, 215);
            }
            else if (static_cast<int32_t>(index) == hovered)
            {
                fill = sf::Color(54, 59, 72);
                outline = sf::Color(135, 175, 220);
            }
            drawPanel(window, button.Bounds.left, button.Bounds.top, button.Bounds.width, button.Bounds.height, fill, outline);
            const float textWidth = getDebugTextWidth(button.Label, TextScale);
            drawDebugText(window, button.Label, button.Bounds.left + std::max(5.0f, (button.Bounds.width - textWidth) * 0.5f), button.Bounds.top + 6.0f, text, TextScale);
        }
    }

    void PreviewRenderer::renderHelpOverlay(sf::RenderWindow& window, PreviewWorkspace workspace) const
    {
        const float width = static_cast<float>(PreviewCommandPanelX) - OverlayMargin * 2.0f;
        const float height = static_cast<float>(PreviewWindowHeight) - OverlayMargin * 2.0f;
        drawPanel(window, OverlayMargin, OverlayMargin, width, height, sf::Color(8, 9, 12, 248), sf::Color(120, 125, 145));
        drawDebugText(window, "HELP - " + std::string(getPreviewWorkspaceName(workspace)) + " - F1 OR ESC TO CLOSE", OverlayMargin + 16.0f, OverlayMargin + 14.0f, sf::Color(240, 215, 105), TextScale);

        float y = OverlayMargin + 50.0f;
        const float shortcutX = OverlayMargin + 20.0f;
        const float descriptionX = shortcutX + 190.0f;
        const auto drawSection = [&](const char* title, const PreviewHelpSection& section, float& sectionY)
        {
            drawSectionHeader(window, title, shortcutX, sectionY);
            for (std::size_t index = 0u; index < section.Count; ++index)
            {
                drawDebugText(window, section.Entries[index].Shortcut, shortcutX, sectionY, sf::Color(125, 205, 235), TextScale);
                drawDebugText(window, section.Entries[index].Description, descriptionX, sectionY, sf::Color(225, 228, 235), TextScale);
                sectionY += 27.0f;
            }
            sectionY += 12.0f;
        };

        drawSection("GLOBAL", getPreviewGlobalHelpSection(), y);
        drawSection((std::string(getPreviewWorkspaceName(workspace)) + " WORKSPACE").c_str(), getPreviewWorkspaceHelpSection(workspace), y);
    }

    void PreviewRenderer::renderGenerationInspector(sf::RenderWindow& window, const PreviewRenderData& data) const
    {
        if (data.Ship == nullptr || data.GenerationDebugInfo == nullptr)
        {
            return;
        }

        drawPanel(window, OverlayMargin, OverlayMargin, static_cast<float>(PreviewContentWidth) - OverlayMargin * 2.0f, static_cast<float>(PreviewWindowHeight) - OverlayMargin * 2.0f, sf::Color(8, 9, 12, 248), sf::Color(120, 125, 145));
        const SpectralShipGen::GeneratedShip& ship = *data.Ship;
        const SpectralShipGen::ShipGenerationDebugInfo& debug = *data.GenerationDebugInfo;
        const float leftX = OverlayMargin + 16.0f;
        const float rightX = OverlayMargin + 430.0f;
        float leftY = OverlayMargin + 14.0f;
        float rightY = OverlayMargin + 54.0f;
        drawSectionHeader(window, "DECISION DETAILS - ESC TO CLOSE", leftX, leftY);

        const Bounds finalBounds = calculateImageBounds(ship.FinalImage, ship.HullMask.getWidth(), ship.HullMask.getHeight());
        const Bounds hullBounds = calculateMaskBounds(ship.HullMask);
        const Bounds cockpitBounds = calculateMaskBounds(ship.CockpitMask);

        drawSectionHeader(window, "STRUCTURE", leftX, leftY);
        drawLabelValue(window, "FINAL BOUNDS", getBoundsString(finalBounds), leftX, leftY);
        drawLabelValue(window, "HULL BOUNDS", getBoundsString(hullBounds), leftX, leftY);
        drawLabelValue(window, "COCKPIT", getBoundsString(cockpitBounds), leftX, leftY);
        drawLabelValue(window, "WING", getWingShapeName(debug.WingShape), leftX, leftY);
        if (debug.WingShape != SpectralShipGen::WingShapeType::NONE)
        {
            drawLabelValue(window, "WING SPAN / ROOT", std::to_string(debug.WingMaximumSpan) + " / " + std::to_string(debug.WingRootThickness) + " PX", leftX, leftY);
        }
        drawLabelValue(window, "HULL LAYERS", std::to_string(debug.HullLayerCount) + " (" + std::to_string(debug.HullLayerLowerCount) + " LOWER / " + std::to_string(debug.HullLayerUpperCount) + " UPPER)", leftX, leftY);
        drawLabelValue(window, "CORE", std::to_string(debug.CoreTreatmentCount) + " TREATMENTS / " + std::to_string(debug.CoreRegionPixelCount) + " PX", leftX, leftY);
        drawLabelValue(window, "ENGINES", std::to_string(debug.EngineCount) + " / " + getEngineLayoutName(debug.EngineLayout), leftX, leftY);
        drawLabelValue(window, "ATTACHMENTS", std::to_string(ship.AttachmentPlacements.size()), leftX, leftY);
        drawLabelValue(window, "MAJOR FEATURES", std::to_string(debug.MajorFeatureCount), leftX, leftY);

        leftY += 6.0f;
        drawSectionHeader(window, "VISUAL HIERARCHY", leftX, leftY);
        drawLabelValue(window, "PRIMARY", SpectralShipGen::getShipVisualAnchorTypeName(debug.PrimaryVisualAnchor), leftX, leftY);
        drawLabelValue(window, "SECONDARY", SpectralShipGen::getShipVisualAnchorTypeName(debug.SecondaryVisualAnchor), leftX, leftY);
        drawLabelValue(window, "REGION", SpectralShipGen::getGenerationSpatialRegionName(debug.VisualAnchorTargetRegion), leftX, leftY);
        drawLabelValue(window, "RESERVED COST", std::to_string(debug.VisualHierarchyReservedComplexity), leftX, leftY);
        drawLabelValue(window, "FALLBACK", debug.VisualHierarchyFallbackOccurred ? "YES" : "NO", leftX, leftY);

        leftY += 6.0f;
        drawSectionHeader(window, "GENERATION / BUDGET", leftX, leftY);
        drawLabelValue(window, "HULL ATTEMPTS", std::to_string(debug.HullGenerationAttemptCount), leftX, leftY);
        drawLabelValue(window, "LAST HULL RETRY", SpectralShipGen::getSilhouetteValidationFailureReasonName(debug.LastSilhouetteValidationFailure), leftX, leftY);
        drawLabelValue(window, "COMPLEXITY", std::to_string(debug.ComplexityConsumedBudget) + "/" + std::to_string(debug.ComplexityInitialBudget), leftX, leftY);
        drawLabelValue(window, "UNUSED", std::to_string(debug.ComplexityUnusedBudget), leftX, leftY);
        drawLabelValue(window, "SPATIAL REJECTS", std::to_string(debug.SpatialOverloadRejectionCount), leftX, leftY);

        drawSectionHeader(window, "COMPOSITION", rightX, rightY);
        drawLabelValue(window, "MATERIAL ZONES", std::to_string(debug.MaterialZoneCount), rightX, rightY);
        drawLabelValue(window, "MATERIAL PX", std::to_string(debug.MaterialSecondaryHullPixelCount) + " SECONDARY / " + std::to_string(debug.MaterialMechanicalPixelCount) + " MECH", rightX, rightY);
        drawLabelValue(window, "LIVERY", std::to_string(debug.LiveryMarkingCount) + " MARKS / " + std::to_string(debug.LiveryCoveragePermille / 10u) + "." + std::to_string(debug.LiveryCoveragePermille % 10u) + "%", rightX, rightY);
        drawLabelValue(window, "PRIMARY MOTIF", SpectralShipGen::getShipDetailMotifTypeName(debug.PrimaryDetailMotif), rightX, rightY);
        drawLabelValue(window, "SECONDARY MOTIF", SpectralShipGen::getShipDetailMotifTypeName(debug.SecondaryDetailMotif), rightX, rightY);
        drawLabelValue(window, "MOTIF REGION", SpectralShipGen::getGenerationSpatialRegionName(debug.PrimaryDetailMotifRegion), rightX, rightY);
        const std::string macroState = !debug.MacroAsymmetryPlanned ? "OFF" : debug.MacroAsymmetryFulfilled ? "FULFILLED" : debug.MacroAsymmetryRejected ? "REJECTED" : "PLANNED";
        drawLabelValue(window, "MACRO ASYM", macroState, rightX, rightY);

        rightY += 6.0f;
        drawSectionHeader(window, "WEAPONS", rightX, rightY);
        drawLabelValue(window, "GROUPS", std::to_string(debug.WeaponRealizedGroupCount) + "/" + std::to_string(debug.WeaponRequestedGroupCount) + " REALIZED", rightX, rightY);
        drawLabelValue(window, "UNITS", std::to_string(debug.WeaponCount), rightX, rightY);
        drawLabelValue(window, "HARDPOINTS", std::to_string(debug.WeaponHardpointCount), rightX, rightY);
        drawLabelValue(window, "PLACEMENT", std::to_string(debug.WeaponPlacementRejectionCount) + " REJECT / " + std::to_string(debug.WeaponPlacementAttemptCount) + " TRY", rightX, rightY);
        const std::size_t weaponDetailCount = std::min<std::size_t>(debug.WeaponUnits.size(), 4u);
        for (std::size_t index = 0u; index < weaponDetailCount; ++index)
        {
            const SpectralShipGen::WeaponUnitDebugInfo& weapon = debug.WeaponUnits[index];
            const std::string weaponFlags = std::string(weapon.MovableBarrel ? " MOV" : "") + (weapon.Emissive ? " EM" : "");
            drawDebugText(window, "W" + std::to_string(index + 1u) + " " + getWeaponTypeName(weapon.Type) + " / " + getWeaponRegionName(weapon.Region) + " G" + std::to_string(weapon.SymmetryGroup) + weaponFlags, rightX, rightY, sf::Color(220, 222, 228), SmallTextScale);
            rightY += 14.0f;
            drawDebugText(window, "  ROOT " + std::to_string(weapon.AnchorX) + "," + std::to_string(weapon.AnchorY) + "  MUZZLE " + std::to_string(weapon.MuzzleX) + "," + std::to_string(weapon.MuzzleY), rightX, rightY, sf::Color(145, 205, 235), SmallTextScale);
            rightY += 14.0f;
        }
        if (debug.WeaponUnits.size() > weaponDetailCount)
        {
            drawDebugText(window, "+ " + std::to_string(debug.WeaponUnits.size() - weaponDetailCount) + " more weapon units", rightX, rightY, sf::Color(150, 155, 165), SmallTextScale);
            rightY += 14.0f;
        }

        rightY += 6.0f;
        drawSectionHeader(window, "CONSTRAINTS", rightX, rightY);
        drawLabelValue(window, "NEG SPACE", std::to_string(debug.StructuralNegativeSpaceSuccessCount) + "/" + std::to_string(debug.StructuralNegativeSpaceAttemptCount) + " SUCCESS", rightX, rightY);
        drawLabelValue(window, "VOID PX", std::to_string(debug.StructuralNegativeSpacePixelCount), rightX, rightY);
        drawLabelValue(window, "MAJOR REJECTS", std::to_string(debug.MajorFeaturePlacementRejectionCount) + "/" + std::to_string(debug.MajorFeaturePlacementAttemptCount), rightX, rightY);
        drawLabelValue(window, "MOTIF REJECTS", std::to_string(debug.DetailMotifRejectedPlacementCount), rightX, rightY);
    }

    void PreviewRenderer::renderPaletteInspector(sf::RenderWindow& window, const PreviewRenderData& data) const
    {
        if (data.Ship == nullptr)
        {
            return;
        }

        drawPanel(window, OverlayMargin, OverlayMargin, static_cast<float>(PreviewContentWidth) - OverlayMargin * 2.0f, static_cast<float>(PreviewWindowHeight) - OverlayMargin * 2.0f, sf::Color(8, 9, 12, 248), sf::Color(120, 125, 145));
        const SpectralShipGen::ShipPalette& palette = data.Ship->Palette;
        const std::array<std::pair<const char*, SpectralShipGen::Color>, 25u> entries = { {
            { "TRANSPARENT", palette.Transparent }, { "OUTLINE", palette.Outline }, { "HULL DEEP SHADOW", palette.HullDeepShadow }, { "HULL SHADOW", palette.HullShadow }, { "HULL BASE", palette.HullBase }, { "HULL SECONDARY", palette.HullSecondary }, { "HULL HIGHLIGHT", palette.HullHighlight }, { "HULL EDGE", palette.HullEdgeHighlight }, { "ACCENT DARK", palette.HullAccentDark }, { "ACCENT BASE", palette.HullAccent }, { "ACCENT HIGHLIGHT", palette.HullAccentHighlight }, { "COCKPIT DARK", palette.CockpitDark }, { "COCKPIT BASE", palette.CockpitBase }, { "COCKPIT HIGHLIGHT", palette.CockpitHighlight }, { "COCKPIT GLINT", palette.CockpitGlint }, { "ENGINE DARK", palette.EngineDark }, { "ENGINE BASE", palette.EngineBase }, { "ENGINE HIGHLIGHT", palette.EngineHighlight }, { "ENGINE HOT CORE", palette.EngineHotCore }, { "EXHAUST BASE", palette.ExhaustBase }, { "EXHAUST HIGHLIGHT", palette.ExhaustHighlight }, { "EXHAUST HOT CORE", palette.ExhaustHotCore }, { "MECHANICAL DARK", palette.MechanicalDark }, { "MECHANICAL BASE", palette.MechanicalBase }, { "LIGHT BASE", palette.LightBase }
        } };

        drawDebugText(window, "PALETTE INSPECTOR - ESC TO CLOSE", OverlayMargin + 16.0f, OverlayMargin + 14.0f, sf::Color(240, 215, 105), TextScale);
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
