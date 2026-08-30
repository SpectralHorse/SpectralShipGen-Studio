#pragma once

#include <cstddef>
#include <cstdint>

#include "Color.h"
#include "GeneratedShip.h"
#include "ShipGenerationDebugInfo.h"

namespace PixelShipGeneratorPreview
{
    namespace PreviewDiagnosticColors
    {
        inline constexpr PixelShipGenerator::Color Hull(80u, 140u, 230u, 255u);
        inline constexpr PixelShipGenerator::Color Cockpit(80u, 230u, 235u, 255u);
        inline constexpr PixelShipGenerator::Color Engine(245u, 160u, 70u, 255u);
        inline constexpr PixelShipGenerator::Color Exhaust(245u, 80u, 65u, 255u);
        inline constexpr PixelShipGenerator::Color Accent(220u, 90u, 225u, 255u);
        inline constexpr PixelShipGenerator::Color Mechanical(100u, 220u, 120u, 255u);
        inline constexpr PixelShipGenerator::Color Light(250u, 235u, 90u, 255u);
        inline constexpr PixelShipGenerator::Color Attachment(165u, 105u, 245u, 255u);
        inline constexpr PixelShipGenerator::Color AttachmentBounds(215u, 165u, 250u, 255u);
        inline constexpr PixelShipGenerator::Color AttachmentRoot(90u, 235u, 245u, 255u);
        inline constexpr PixelShipGenerator::Color HullLayerLower(95u, 170u, 225u, 255u);
        inline constexpr PixelShipGenerator::Color HullLayerUpper(235u, 205u, 95u, 255u);
        inline constexpr PixelShipGenerator::Color CoreRegion(55u, 65u, 85u, 255u);
        inline constexpr PixelShipGenerator::Color CoreSecondary(95u, 145u, 195u, 255u);
        inline constexpr PixelShipGenerator::Color CoreRaised(235u, 205u, 95u, 255u);
        inline constexpr PixelShipGenerator::Color CoreRecessed(75u, 85u, 105u, 255u);
        inline constexpr PixelShipGenerator::Color CoreLuminous(95u, 235u, 205u, 255u);
        inline constexpr PixelShipGenerator::Color Weapon(230u, 105u, 75u, 255u);
        inline constexpr PixelShipGenerator::Color WeaponBounds(245u, 190u, 80u, 255u);
        inline constexpr PixelShipGenerator::Color WeaponRoot(90u, 235u, 245u, 255u);
        inline constexpr PixelShipGenerator::Color WeaponMuzzle(255u, 245u, 110u, 255u);
        inline constexpr PixelShipGenerator::Color MaterialSecondary(95u, 145u, 195u, 255u);
        inline constexpr PixelShipGenerator::Color MaterialMechanical(100u, 220u, 120u, 255u);
        inline constexpr PixelShipGenerator::Color LiveryPrimary(230u, 95u, 210u, 255u);
        inline constexpr PixelShipGenerator::Color LiverySecondary(110u, 205u, 245u, 255u);
        inline constexpr PixelShipGenerator::Color MotifPrimary(245u, 180u, 75u, 255u);
        inline constexpr PixelShipGenerator::Color MotifSecondary(150u, 110u, 245u, 255u);
        inline constexpr PixelShipGenerator::Color NegativeSpace(245u, 90u, 100u, 255u);
        inline constexpr PixelShipGenerator::Color Overlap(255u, 70u, 120u, 255u);
        inline constexpr PixelShipGenerator::Color SpatialLow(75u, 130u, 215u, 255u);
        inline constexpr PixelShipGenerator::Color SpatialModerate(95u, 190u, 125u, 255u);
        inline constexpr PixelShipGenerator::Color SpatialHigh(235u, 185u, 70u, 255u);
        inline constexpr PixelShipGenerator::Color SpatialOverloaded(240u, 80u, 85u, 255u);
        inline constexpr PixelShipGenerator::Color MacroAsymmetryBase(45u, 50u, 65u, 255u);
        inline constexpr PixelShipGenerator::Color MacroAsymmetryFeature(245u, 105u, 210u, 255u);
    }

    enum class PreviewInspectionGroup : uint32_t
    {
        STRUCTURE = 0u,
        COMPOSITION,
        CONSTRAINTS,
        PREVIEW_INSPECTION_GROUP_END
    };

    inline constexpr std::size_t PreviewInspectionGroupCount = static_cast<std::size_t>(PreviewInspectionGroup::PREVIEW_INSPECTION_GROUP_END);

    enum class PreviewInspectionPresentation : uint32_t
    {
        OVERLAY = 0u,
        ISOLATE,
        PREVIEW_INSPECTION_PRESENTATION_END
    };

    enum class DiagnosticViewMode : uint32_t
    {
        FINAL = 0u,
        HULL,
        COCKPIT,
        ENGINES,
        ATTACHMENTS,
        HULL_LAYERS,
        CORE_TREATMENT,
        WEAPONS,
        DETAILS,
        MATERIALS,
        LIVERY,
        DETAIL_MOTIFS,
        MACRO_ASYMMETRY,
        NEGATIVE_SPACE,
        SEMANTIC_LOAD,
        COMBINED,
        DIAGNOSTIC_VIEW_MODE_END
    };

    const char* getPreviewInspectionGroupName(PreviewInspectionGroup group);
    const char* getPreviewInspectionPresentationName(PreviewInspectionPresentation presentation);
    const char* getDiagnosticViewModeName(DiagnosticViewMode mode);
    PreviewInspectionGroup getDiagnosticViewGroup(DiagnosticViewMode mode);
    PreviewInspectionGroup getWrappedPreviewInspectionGroup(PreviewInspectionGroup group, int32_t delta);
    DiagnosticViewMode getDefaultDiagnosticViewForGroup(PreviewInspectionGroup group);
    DiagnosticViewMode getWrappedDiagnosticView(PreviewInspectionGroup group, DiagnosticViewMode current, int32_t delta);
    bool hasPreviewInspectionShip(const PixelShipGenerator::GeneratedShip& ship);
    PixelShipGenerator::Image createPreviewInspectionImage(
        const PixelShipGenerator::GeneratedShip& ship,
        const PixelShipGenerator::ShipGenerationDebugInfo& debugInfo,
        DiagnosticViewMode mode,
        PreviewInspectionPresentation presentation,
        bool generationStageView = false,
        uint32_t generationStageIndex = 0u);
}
