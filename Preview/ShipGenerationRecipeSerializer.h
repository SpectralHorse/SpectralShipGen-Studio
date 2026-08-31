#pragma once

#include <SpectralShipGen/ShipGenerationRecipeSerializer.h>

namespace SpectralShipGenStudioPreview
{
    using SpectralShipGen::ShipGenerationRecipeFormatVersion;
    using SpectralShipGen::ShipGenerationRecipeDocument;
    using SpectralShipGen::ShipGenerationRecipeLoadResult;
    using SpectralShipGen::shipStyleToRecipeString;
    using SpectralShipGen::shipFactionToRecipeString;
    using SpectralShipGen::shipStyleFromRecipeString;
    using SpectralShipGen::shipFactionFromRecipeString;
    using SpectralShipGen::serializeShipGenerationRecipe;
    using SpectralShipGen::deserializeShipGenerationRecipe;
    using SpectralShipGen::saveShipGenerationRecipe;
    using SpectralShipGen::loadShipGenerationRecipe;
}
