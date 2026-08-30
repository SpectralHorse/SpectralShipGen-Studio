#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "Color.h"
#include "GeneratedShip.h"
#include "ShipFactionType.h"
#include "ShipGenerationProfile.h"
#include "ShipGenerationSeeds.h"

#include "PreviewGenerationRecipe.h"
#include "PreviewWorkspace.h"
#include "PreviewResolution.h"

namespace PixelShipGeneratorPreview
{
    inline constexpr uint32_t PreviewWindowWidth = 1640u;
    inline constexpr uint32_t PreviewWindowHeight = 1000u;
    inline constexpr uint32_t PreviewWorkspaceNavigationHeight = 38u;
    inline constexpr uint32_t PreviewContentWidth = 880u;
    inline constexpr uint32_t NativePreviewSpriteAreaSize = 256u;
    inline constexpr uint32_t NativePreviewPanelWidth = 280u;
    inline constexpr uint32_t NativePreviewPanelHeight = 300u;
    inline constexpr uint32_t NativePreviewPanelX = PreviewContentWidth - NativePreviewPanelWidth - 12u;
    inline constexpr uint32_t NativePreviewPanelY = (PreviewWindowHeight - NativePreviewPanelHeight) / 2u;
    inline constexpr uint32_t PreviewEnlargedContentWidth = NativePreviewPanelX - 12u;
    inline constexpr uint32_t PreviewAreaWidth = PreviewEnlargedContentWidth - 40u;
    inline constexpr uint32_t PreviewAreaHeight = 840u;
    inline constexpr uint32_t PreviewStatePanelWidth = 360u;
    inline constexpr uint32_t PreviewCommandPanelWidth = PreviewWindowWidth - PreviewContentWidth - PreviewStatePanelWidth;
    inline constexpr uint32_t PreviewStatePanelX = PreviewContentWidth;
    inline constexpr uint32_t PreviewCommandPanelX = PreviewContentWidth + PreviewStatePanelWidth;
    inline constexpr float PreviewThumbnailCellPadding = 8.0f;

    inline constexpr std::array<PixelShipGenerator::ShipStyle, static_cast<std::size_t>(PixelShipGenerator::ShipStyle::SHIP_STYLE_END)> SupportedPreviewStyles = {
        PixelShipGenerator::ShipStyle::SLEEK,
        PixelShipGenerator::ShipStyle::FIGHTER,
        PixelShipGenerator::ShipStyle::HEAVY,
        PixelShipGenerator::ShipStyle::INDUSTRIAL,
        PixelShipGenerator::ShipStyle::SPEARHEAD,
        PixelShipGenerator::ShipStyle::DELTA
    };
    inline constexpr std::array<PixelShipGenerator::ShipFactionType, static_cast<std::size_t>(PixelShipGenerator::ShipFactionType::SHIP_FACTION_TYPE_END)> SupportedPreviewFactions = {
        PixelShipGenerator::ShipFactionType::FRONTIER,
        PixelShipGenerator::ShipFactionType::MILITARY,
        PixelShipGenerator::ShipFactionType::ASCENDANT,
        PixelShipGenerator::ShipFactionType::XENO,
        PixelShipGenerator::ShipFactionType::CORPORATE,
        PixelShipGenerator::ShipFactionType::RELIC
    };

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
        inline constexpr PixelShipGenerator::Color HullLayerLower(95u, 170u, 225u, 255u);
        inline constexpr PixelShipGenerator::Color HullLayerUpper(235u, 205u, 95u, 255u);
        inline constexpr PixelShipGenerator::Color CoreRegion(55u, 65u, 85u, 255u);
        inline constexpr PixelShipGenerator::Color CoreSecondary(95u, 145u, 195u, 255u);
        inline constexpr PixelShipGenerator::Color CoreRaised(235u, 205u, 95u, 255u);
        inline constexpr PixelShipGenerator::Color CoreRecessed(75u, 85u, 105u, 255u);
        inline constexpr PixelShipGenerator::Color CoreLuminous(95u, 235u, 205u, 255u);
        inline constexpr PixelShipGenerator::Color Overlap(255u, 70u, 120u, 255u);
        inline constexpr PixelShipGenerator::Color SpatialLow(75u, 130u, 215u, 255u);
        inline constexpr PixelShipGenerator::Color SpatialModerate(95u, 190u, 125u, 255u);
        inline constexpr PixelShipGenerator::Color SpatialHigh(235u, 185u, 70u, 255u);
        inline constexpr PixelShipGenerator::Color SpatialOverloaded(240u, 80u, 85u, 255u);
        inline constexpr PixelShipGenerator::Color MacroAsymmetryBase(45u, 50u, 65u, 255u);
        inline constexpr PixelShipGenerator::Color MacroAsymmetryFeature(245u, 105u, 210u, 255u);
    }

    enum class DiagnosticViewMode : uint32_t
    {
        FINAL = 0u,
        HULL,
        COCKPIT,
        ENGINES,
        DETAILS,
        ATTACHMENTS,
        HULL_LAYERS,
        CORE_TREATMENT,
        SEMANTIC_LOAD,
        MACRO_ASYMMETRY,
        COMBINED,
        DIAGNOSTIC_VIEW_MODE_END
    };

    struct PreviewThumbnailItem
    {
        PreviewGenerationRecipe Recipe;
        sf::Texture Texture;
        bool Valid = false;
    };

    struct PreviewThumbnailGridState
    {
        uint32_t Columns = 5u;
        uint32_t Rows = 5u;
        uint32_t SelectedIndex = 0u;
        int32_t HoveredIndex = -1;
        std::vector<PreviewThumbnailItem> Items;
    };

    struct GalleryState
    {
        uint64_t BatchSeed = 0u;
        uint32_t CandidateCount = 25u;
        PreviewThumbnailGridState Grid;
        PreviewGenerationRecipe TemplateRecipe;
    };

    struct FavoritesState
    {
        PreviewThumbnailGridState Grid;
    };

    struct GenerationLocks
    {
        bool Structure = false;
        bool Palette = false;
        bool Details = false;
        bool Attachments = false;
    };

    struct PinnedShipReference
    {
        PreviewGenerationRecipe Recipe;
        PixelShipGenerator::GeneratedShip Ship;
        bool Valid = false;
    };

    struct PreviewComparisonState
    {
        PinnedShipReference Pinned;
        bool ViewEnabled = false;
    };

    struct PreviewDiagnosticState
    {
        DiagnosticViewMode ViewMode = DiagnosticViewMode::FINAL;
        bool HelpVisible = false;
        bool GenerationInspectorVisible = false;
        bool PaletteInspectorVisible = false;
        bool GenerationStageView = false;
        uint32_t GenerationStageIndex = 0u;
    };
}
