#pragma once

#include <cstdint>

#include "ShipDimensions.h"
#include "ShipFactionType.h"
#include "ShipGenerationProfile.h"
#include "ShipGenerationSeeds.h"

namespace PixelShipGeneratorPreview
{
    struct PreviewGenerationRecipe
    {
        PixelShipGenerator::ShipGenerationSeeds Seeds;
        PixelShipGenerator::GenerationDomainSeedOverrides DomainSeedOverrides;
        PixelShipGenerator::GenerationRandomStreamMode RandomStreamMode = PixelShipGenerator::GenerationRandomStreamMode::DOMAIN_SUBSTREAMS;
        PixelShipGenerator::ShipDimensions Dimensions;
        PixelShipGenerator::ShipStyle Style = PixelShipGenerator::ShipStyle::FIGHTER;
        PixelShipGenerator::ShipFactionType Faction = PixelShipGenerator::ShipFactionType::FRONTIER;
        uint32_t DetailDensity = 50u;
        uint32_t AsymmetricDetailChance = 10u;
        bool AttachmentsEnabled = true;
    };

    inline bool operator==(const PreviewGenerationRecipe& first, const PreviewGenerationRecipe& second)
    {
        return first.Seeds == second.Seeds && first.DomainSeedOverrides == second.DomainSeedOverrides && first.RandomStreamMode == second.RandomStreamMode && first.Dimensions == second.Dimensions && first.Style == second.Style && first.Faction == second.Faction && first.DetailDensity == second.DetailDensity && first.AsymmetricDetailChance == second.AsymmetricDetailChance && first.AttachmentsEnabled == second.AttachmentsEnabled;
    }

    inline bool operator!=(const PreviewGenerationRecipe& first, const PreviewGenerationRecipe& second)
    {
        return !(first == second);
    }
}
