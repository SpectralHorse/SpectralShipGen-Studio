#include "PreviewInspection.h"

#include <algorithm>
#include <array>

namespace SpectralShipGenStudioPreview
{
    namespace
    {
        constexpr std::array<DiagnosticViewMode, 8u> StructureViews = {
            DiagnosticViewMode::HULL,
            DiagnosticViewMode::COCKPIT,
            DiagnosticViewMode::ENGINES,
            DiagnosticViewMode::ATTACHMENTS,
            DiagnosticViewMode::HULL_LAYERS,
            DiagnosticViewMode::CORE_TREATMENT,
            DiagnosticViewMode::WEAPONS,
            DiagnosticViewMode::COMBINED
        };

        constexpr std::array<DiagnosticViewMode, 5u> CompositionViews = {
            DiagnosticViewMode::DETAILS,
            DiagnosticViewMode::MATERIALS,
            DiagnosticViewMode::LIVERY,
            DiagnosticViewMode::DETAIL_MOTIFS,
            DiagnosticViewMode::MACRO_ASYMMETRY
        };

        constexpr std::array<DiagnosticViewMode, 2u> ConstraintViews = {
            DiagnosticViewMode::NEGATIVE_SPACE,
            DiagnosticViewMode::SEMANTIC_LOAD
        };

        template <std::size_t Size>
        DiagnosticViewMode wrapView(const std::array<DiagnosticViewMode, Size>& views, DiagnosticViewMode current, int32_t delta)
        {
            std::size_t currentIndex = 0u;
            for (std::size_t index = 0u; index < views.size(); ++index)
            {
                if (views[index] == current)
                {
                    currentIndex = index;
                    break;
                }
            }

            int32_t wrapped = static_cast<int32_t>(currentIndex) + delta;
            const int32_t count = static_cast<int32_t>(views.size());
            while (wrapped < 0) { wrapped += count; }
            while (wrapped >= count) { wrapped -= count; }
            return views[static_cast<std::size_t>(wrapped)];
        }

        bool maskMatches(const SpectralShipGen::PixelMask& mask, uint32_t width, uint32_t height)
        {
            return mask.getWidth() == width && mask.getHeight() == height;
        }

        SpectralShipGen::Color blendDiagnosticColor(const SpectralShipGen::Color& base, const SpectralShipGen::Color& overlay)
        {
            if (base.A == 0u) { return overlay; }
            constexpr uint32_t OverlayWeight = 60u;
            constexpr uint32_t BaseWeight = 100u - OverlayWeight;
            return SpectralShipGen::Color(
                static_cast<uint8_t>((static_cast<uint32_t>(base.R) * BaseWeight + static_cast<uint32_t>(overlay.R) * OverlayWeight) / 100u),
                static_cast<uint8_t>((static_cast<uint32_t>(base.G) * BaseWeight + static_cast<uint32_t>(overlay.G) * OverlayWeight) / 100u),
                static_cast<uint8_t>((static_cast<uint32_t>(base.B) * BaseWeight + static_cast<uint32_t>(overlay.B) * OverlayWeight) / 100u),
                255u);
        }

        void drawDiagnosticPixel(SpectralShipGen::Image& image, uint32_t x, uint32_t y, const SpectralShipGen::Color& color, PreviewInspectionPresentation presentation)
        {
            if (!image.isInBounds(x, y)) { return; }
            image.setPixel(x, y, presentation == PreviewInspectionPresentation::OVERLAY ? blendDiagnosticColor(image.getPixel(x, y), color) : color);
        }

        void drawBounds(SpectralShipGen::Image& image, uint32_t minX, uint32_t maxX, uint32_t minY, uint32_t maxY, const SpectralShipGen::Color& color, PreviewInspectionPresentation presentation)
        {
            if (minX > maxX || minY > maxY) { return; }
            for (uint32_t x = minX; x <= maxX; ++x)
            {
                drawDiagnosticPixel(image, x, minY, color, presentation);
                drawDiagnosticPixel(image, x, maxY, color, presentation);
                if (x == maxX) { break; }
            }
            for (uint32_t y = minY; y <= maxY; ++y)
            {
                drawDiagnosticPixel(image, minX, y, color, presentation);
                drawDiagnosticPixel(image, maxX, y, color, presentation);
                if (y == maxY) { break; }
            }
        }
    }

    const char* getPreviewInspectionGroupName(PreviewInspectionGroup group)
    {
        switch (group)
        {
        case PreviewInspectionGroup::STRUCTURE: return "Structure";
        case PreviewInspectionGroup::COMPOSITION: return "Composition";
        case PreviewInspectionGroup::CONSTRAINTS: return "Constraints";
        default: return "Unknown";
        }
    }

    const char* getPreviewInspectionPresentationName(PreviewInspectionPresentation presentation)
    {
        switch (presentation)
        {
        case PreviewInspectionPresentation::OVERLAY: return "Overlay";
        case PreviewInspectionPresentation::ISOLATE: return "Isolate";
        default: return "Unknown";
        }
    }

    const char* getDiagnosticViewModeName(DiagnosticViewMode mode)
    {
        switch (mode)
        {
        case DiagnosticViewMode::FINAL: return "Final";
        case DiagnosticViewMode::HULL: return "Hull";
        case DiagnosticViewMode::COCKPIT: return "Cockpit";
        case DiagnosticViewMode::ENGINES: return "Engines";
        case DiagnosticViewMode::ATTACHMENTS: return "Attachments";
        case DiagnosticViewMode::HULL_LAYERS: return "Hull Layers";
        case DiagnosticViewMode::CORE_TREATMENT: return "Core Treatment";
        case DiagnosticViewMode::WEAPONS: return "Weapons";
        case DiagnosticViewMode::DETAILS: return "Surface Details";
        case DiagnosticViewMode::MATERIALS: return "Materials";
        case DiagnosticViewMode::LIVERY: return "Livery";
        case DiagnosticViewMode::DETAIL_MOTIFS: return "Detail Motifs";
        case DiagnosticViewMode::MACRO_ASYMMETRY: return "Macro Asymmetry";
        case DiagnosticViewMode::NEGATIVE_SPACE: return "Negative Space";
        case DiagnosticViewMode::SEMANTIC_LOAD: return "Spatial Budget";
        case DiagnosticViewMode::COMBINED: return "Combined Structure";
        default: return "Unknown";
        }
    }

    PreviewInspectionGroup getDiagnosticViewGroup(DiagnosticViewMode mode)
    {
        switch (mode)
        {
        case DiagnosticViewMode::DETAILS:
        case DiagnosticViewMode::MATERIALS:
        case DiagnosticViewMode::LIVERY:
        case DiagnosticViewMode::DETAIL_MOTIFS:
        case DiagnosticViewMode::MACRO_ASYMMETRY:
            return PreviewInspectionGroup::COMPOSITION;
        case DiagnosticViewMode::NEGATIVE_SPACE:
        case DiagnosticViewMode::SEMANTIC_LOAD:
            return PreviewInspectionGroup::CONSTRAINTS;
        default:
            return PreviewInspectionGroup::STRUCTURE;
        }
    }

    PreviewInspectionGroup getWrappedPreviewInspectionGroup(PreviewInspectionGroup group, int32_t delta)
    {
        const int32_t count = static_cast<int32_t>(PreviewInspectionGroupCount);
        int32_t index = static_cast<int32_t>(group) + delta;
        while (index < 0) { index += count; }
        while (index >= count) { index -= count; }
        return static_cast<PreviewInspectionGroup>(index);
    }

    DiagnosticViewMode getDefaultDiagnosticViewForGroup(PreviewInspectionGroup group)
    {
        switch (group)
        {
        case PreviewInspectionGroup::STRUCTURE: return StructureViews.front();
        case PreviewInspectionGroup::COMPOSITION: return CompositionViews.front();
        case PreviewInspectionGroup::CONSTRAINTS: return ConstraintViews.front();
        default: return DiagnosticViewMode::HULL;
        }
    }

    DiagnosticViewMode getWrappedDiagnosticView(PreviewInspectionGroup group, DiagnosticViewMode current, int32_t delta)
    {
        switch (group)
        {
        case PreviewInspectionGroup::STRUCTURE: return wrapView(StructureViews, current, delta);
        case PreviewInspectionGroup::COMPOSITION: return wrapView(CompositionViews, current, delta);
        case PreviewInspectionGroup::CONSTRAINTS: return wrapView(ConstraintViews, current, delta);
        default: return DiagnosticViewMode::HULL;
        }
    }

    bool hasPreviewInspectionShip(const SpectralShipGen::GeneratedShip& ship)
    {
        return !ship.FinalImage.empty();
    }

    SpectralShipGen::Image createPreviewInspectionImage(
        const SpectralShipGen::GeneratedShip& ship,
        const SpectralShipGen::ShipGenerationDebugInfo& debugInfo,
        DiagnosticViewMode mode,
        PreviewInspectionPresentation presentation,
        bool generationStageView,
        uint32_t generationStageIndex)
    {
        const uint32_t width = ship.FinalImage.getWidth();
        const uint32_t height = ship.FinalImage.getHeight();
        SpectralShipGen::Image image;
        if (presentation == PreviewInspectionPresentation::OVERLAY) { image = ship.FinalImage; }
        else { image.reset(width, height, SpectralShipGen::Color(0u, 0u, 0u, 0u)); }

        if (width == 0u || height == 0u) { return image; }

        if (generationStageView && !debugInfo.HullStages.empty())
        {
            const uint32_t index = std::min(generationStageIndex, static_cast<uint32_t>(debugInfo.HullStages.size() - 1u));
            const SpectralShipGen::PixelMask& stageMask = debugInfo.HullStages[index].HullMask;
            if (!maskMatches(stageMask, width, height)) { return image; }
            for (uint32_t y = 0u; y < height; ++y)
            {
                for (uint32_t x = 0u; x < width; ++x)
                {
                    if (stageMask.get(x, y)) { drawDiagnosticPixel(image, x, y, PreviewDiagnosticColors::Hull, presentation); }
                }
            }
            return image;
        }

        if (mode == DiagnosticViewMode::FINAL) { return ship.FinalImage; }

        const auto drawMask = [&](const SpectralShipGen::PixelMask& mask, const SpectralShipGen::Color& color)
        {
            if (!maskMatches(mask, width, height)) { return; }
            for (uint32_t y = 0u; y < height; ++y)
            {
                for (uint32_t x = 0u; x < width; ++x)
                {
                    if (mask.get(x, y)) { drawDiagnosticPixel(image, x, y, color, presentation); }
                }
            }
        };

        switch (mode)
        {
        case DiagnosticViewMode::HULL:
            drawMask(ship.HullMask, PreviewDiagnosticColors::Hull);
            break;
        case DiagnosticViewMode::COCKPIT:
            drawMask(ship.CockpitMask, PreviewDiagnosticColors::Cockpit);
            break;
        case DiagnosticViewMode::ENGINES:
            drawMask(ship.EngineMask, PreviewDiagnosticColors::Engine);
            drawMask(ship.EngineExhaustMask, PreviewDiagnosticColors::Exhaust);
            break;
        case DiagnosticViewMode::ATTACHMENTS:
            drawMask(ship.AttachmentMask, PreviewDiagnosticColors::Attachment);
            for (const SpectralShipGen::ShipAttachmentPlacement& attachment : ship.AttachmentPlacements)
            {
                drawBounds(image, attachment.MinimumX, attachment.MaximumX, attachment.MinimumY, attachment.MaximumY, PreviewDiagnosticColors::AttachmentBounds, presentation);
                drawDiagnosticPixel(image, attachment.AnchorX, attachment.AnchorY, PreviewDiagnosticColors::AttachmentRoot, presentation);
            }
            break;
        case DiagnosticViewMode::HULL_LAYERS:
            drawMask(debugInfo.HullLayerMask, PreviewDiagnosticColors::HullLayerLower);
            drawMask(debugInfo.HullLayerUpperMask, PreviewDiagnosticColors::HullLayerUpper);
            break;
        case DiagnosticViewMode::CORE_TREATMENT:
            drawMask(debugInfo.CoreRegionMask, PreviewDiagnosticColors::CoreRegion);
            drawMask(debugInfo.CoreSecondaryMaterialMask, PreviewDiagnosticColors::CoreSecondary);
            drawMask(debugInfo.CoreRecessedMask, PreviewDiagnosticColors::CoreRecessed);
            drawMask(debugInfo.CoreRaisedMask, PreviewDiagnosticColors::CoreRaised);
            drawMask(debugInfo.CoreLuminousMask, PreviewDiagnosticColors::CoreLuminous);
            break;
        case DiagnosticViewMode::WEAPONS:
            drawMask(debugInfo.WeaponOccupiedMask, PreviewDiagnosticColors::Weapon);
            for (const SpectralShipGen::WeaponUnitDebugInfo& weapon : debugInfo.WeaponUnits)
            {
                drawBounds(image, weapon.BodyMinX, weapon.BodyMaxX, weapon.BodyMinY, weapon.BodyMaxY, PreviewDiagnosticColors::WeaponBounds, presentation);
                drawBounds(image, weapon.BarrelMinX, weapon.BarrelMaxX, weapon.BarrelMinY, weapon.BarrelMaxY, PreviewDiagnosticColors::WeaponBounds, presentation);
                drawDiagnosticPixel(image, weapon.AnchorX, weapon.AnchorY, PreviewDiagnosticColors::WeaponRoot, presentation);
                drawDiagnosticPixel(image, weapon.MuzzleX, weapon.MuzzleY, PreviewDiagnosticColors::WeaponMuzzle, presentation);
            }
            break;
        case DiagnosticViewMode::DETAILS:
            drawMask(ship.AccentMask, PreviewDiagnosticColors::Accent);
            drawMask(ship.MechanicalDetailMask, PreviewDiagnosticColors::Mechanical);
            drawMask(ship.LightMask, PreviewDiagnosticColors::Light);
            break;
        case DiagnosticViewMode::MATERIALS:
            drawMask(debugInfo.MaterialSecondaryHullMask, PreviewDiagnosticColors::MaterialSecondary);
            drawMask(debugInfo.MaterialMechanicalMask, PreviewDiagnosticColors::MaterialMechanical);
            break;
        case DiagnosticViewMode::LIVERY:
            drawMask(debugInfo.LiveryPrimaryMask, PreviewDiagnosticColors::LiveryPrimary);
            drawMask(debugInfo.LiverySecondaryMask, PreviewDiagnosticColors::LiverySecondary);
            break;
        case DiagnosticViewMode::DETAIL_MOTIFS:
            drawMask(debugInfo.PrimaryDetailMotifMask, PreviewDiagnosticColors::MotifPrimary);
            drawMask(debugInfo.SecondaryDetailMotifMask, PreviewDiagnosticColors::MotifSecondary);
            break;
        case DiagnosticViewMode::MACRO_ASYMMETRY:
            drawMask(ship.HullMask, PreviewDiagnosticColors::MacroAsymmetryBase);
            drawMask(debugInfo.MacroAsymmetryMask, PreviewDiagnosticColors::MacroAsymmetryFeature);
            break;
        case DiagnosticViewMode::NEGATIVE_SPACE:
            drawMask(debugInfo.ReservedNegativeSpaceMask, PreviewDiagnosticColors::NegativeSpace);
            break;
        case DiagnosticViewMode::SEMANTIC_LOAD:
            if (debugInfo.SpatialRegionMapWidth == width && debugInfo.SpatialRegionMapHeight == height && debugInfo.SpatialRegionMap.size() == static_cast<std::size_t>(width) * height)
            {
                for (uint32_t y = 0u; y < height; ++y)
                {
                    for (uint32_t x = 0u; x < width; ++x)
                    {
                        const uint8_t regionValue = debugInfo.SpatialRegionMap[static_cast<std::size_t>(y) * width + x];
                        if (regionValue >= static_cast<uint8_t>(SpectralShipGen::GenerationSpatialRegion::GENERATION_SPATIAL_REGION_END)) { continue; }
                        const std::size_t regionIndex = static_cast<std::size_t>(regionValue);
                        const uint32_t capacity = debugInfo.SpatialRegionCapacities[regionIndex];
                        const uint32_t utilization = capacity == 0u ? 0u : (debugInfo.SpatialRegionLoads[regionIndex] * 100u) / capacity;
                        const SpectralShipGen::Color color = utilization < 40u ? PreviewDiagnosticColors::SpatialLow : utilization < 75u ? PreviewDiagnosticColors::SpatialModerate : utilization < 100u ? PreviewDiagnosticColors::SpatialHigh : PreviewDiagnosticColors::SpatialOverloaded;
                        drawDiagnosticPixel(image, x, y, color, presentation);
                    }
                }
            }
            break;
        case DiagnosticViewMode::COMBINED:
            drawMask(ship.HullMask, PreviewDiagnosticColors::Hull);
            drawMask(ship.CockpitMask, PreviewDiagnosticColors::Cockpit);
            drawMask(ship.EngineMask, PreviewDiagnosticColors::Engine);
            drawMask(ship.EngineExhaustMask, PreviewDiagnosticColors::Exhaust);
            drawMask(ship.AttachmentMask, PreviewDiagnosticColors::Attachment);
            drawMask(debugInfo.WeaponOccupiedMask, PreviewDiagnosticColors::Weapon);
            break;
        default:
            break;
        }

        return image;
    }
}
