#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <SpectralShipGen/Color.h>
#include <SpectralShipGen/GeneratedShip.h>
#include <SpectralShipGen/ShipFactionType.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipGenerationSeeds.h>

#include "PreviewGenerationRecipe.h"
#include "PreviewInspection.h"
#include "PreviewWorkspace.h"
#include "PreviewResolution.h"

namespace SpectralShipGenStudioPreview
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

    inline constexpr std::array<SpectralShipGen::ShipStyle, static_cast<std::size_t>(SpectralShipGen::ShipStyle::SHIP_STYLE_END)> SupportedPreviewStyles = {
        SpectralShipGen::ShipStyle::SLEEK,
        SpectralShipGen::ShipStyle::FIGHTER,
        SpectralShipGen::ShipStyle::HEAVY,
        SpectralShipGen::ShipStyle::INDUSTRIAL,
        SpectralShipGen::ShipStyle::SPEARHEAD,
        SpectralShipGen::ShipStyle::DELTA
    };
    inline constexpr std::array<SpectralShipGen::ShipFactionType, static_cast<std::size_t>(SpectralShipGen::ShipFactionType::SHIP_FACTION_TYPE_END)> SupportedPreviewFactions = {
        SpectralShipGen::ShipFactionType::FRONTIER,
        SpectralShipGen::ShipFactionType::MILITARY,
        SpectralShipGen::ShipFactionType::ASCENDANT,
        SpectralShipGen::ShipFactionType::XENO,
        SpectralShipGen::ShipFactionType::CORPORATE,
        SpectralShipGen::ShipFactionType::RELIC
    };

    struct PreviewThumbnailItem
    {
        PreviewGenerationRecipe Recipe;
        sf::Texture Texture;
        bool Valid = false;
        bool Favorite = false;
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
        SpectralShipGen::GeneratedShip Ship;
        bool Valid = false;
    };

    struct PreviewComparisonState
    {
        PinnedShipReference Pinned;
        bool ViewEnabled = false;
    };

    struct PreviewDiagnosticState
    {
        PreviewInspectionGroup InspectionGroup = PreviewInspectionGroup::STRUCTURE;
        DiagnosticViewMode ViewMode = DiagnosticViewMode::HULL;
        PreviewInspectionPresentation InspectionPresentation = PreviewInspectionPresentation::OVERLAY;
        bool HelpVisible = false;
        bool GenerationInspectorVisible = false;
        bool PaletteInspectorVisible = false;
        bool GenerationStageView = false;
        uint32_t GenerationStageIndex = 0u;
    };
}
