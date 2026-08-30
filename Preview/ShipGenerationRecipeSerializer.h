#pragma once

#include <PixelShipGenerator/ShipGenerationRecipeSerializer.h>

namespace PixelShipGeneratorPreview
{
    using PixelShipGenerator::ShipGenerationRecipeFormatVersion;
    using PixelShipGenerator::ShipGenerationRecipeDocument;
    using PixelShipGenerator::ShipGenerationRecipeLoadResult;
    using PixelShipGenerator::shipStyleToRecipeString;
    using PixelShipGenerator::shipFactionToRecipeString;
    using PixelShipGenerator::shipStyleFromRecipeString;
    using PixelShipGenerator::shipFactionFromRecipeString;
    using PixelShipGenerator::serializeShipGenerationRecipe;
    using PixelShipGenerator::deserializeShipGenerationRecipe;
    using PixelShipGenerator::saveShipGenerationRecipe;
    using PixelShipGenerator::loadShipGenerationRecipe;
}
