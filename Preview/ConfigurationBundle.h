#pragma once

#include <string>

#include <PixelShipGenerator/ShipFactionProfile.h>
#include <PixelShipGenerator/ShipGenerationProfile.h>
#include <PixelShipGenerator/ShipGenerationRecipe.h>
#include <PixelShipGenerator/ShipPaletteConfiguration.h>
#include <PixelShipGenerator/Validation.h>

namespace PixelShipGeneratorPreview
{
    // Application-side reusable configuration bundle. It intentionally contains
    // only semantic generation configuration and display metadata; generation
    // seeds, dimensions and other ship-specific recipe state remain outside it.
    struct ConfigurationBundle
    {
        std::string StructuralDisplayName;
        std::string FactionDisplayName;
        std::string PaletteDisplayName;
        PixelShipGenerator::ShipGenerationProfile StructuralProfile;
        PixelShipGenerator::ShipFactionProfile FactionProfile;
        PixelShipGenerator::ShipPaletteConfiguration PaletteConfiguration;
    };

    ConfigurationBundle makeConfigurationBundle(const PixelShipGenerator::ShipGenerationRecipe& recipe,
        std::string structuralDisplayName,
        std::string factionDisplayName,
        std::string paletteDisplayName);
    void applyConfigurationBundle(const ConfigurationBundle& bundle, PixelShipGenerator::ShipGenerationRecipe& recipe);
    PixelShipGenerator::ValidationResult validateConfigurationBundle(const ConfigurationBundle& bundle);
}
