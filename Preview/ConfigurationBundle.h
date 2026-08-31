#pragma once

#include <string>

#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipGenerationRecipe.h>
#include <SpectralShipGen/ShipPaletteConfiguration.h>
#include <SpectralShipGen/Validation.h>

namespace SpectralShipGenStudioPreview
{
    // Application-side reusable configuration bundle. It intentionally contains
    // only semantic generation configuration and display metadata; generation
    // seeds, dimensions and other ship-specific recipe state remain outside it.
    struct ConfigurationBundle
    {
        std::string StructuralDisplayName;
        std::string FactionDisplayName;
        std::string PaletteDisplayName;
        SpectralShipGen::ShipGenerationProfile StructuralProfile;
        SpectralShipGen::ShipFactionProfile FactionProfile;
        SpectralShipGen::ShipPaletteConfiguration PaletteConfiguration;
    };

    ConfigurationBundle makeConfigurationBundle(const SpectralShipGen::ShipGenerationRecipe& recipe,
        std::string structuralDisplayName,
        std::string factionDisplayName,
        std::string paletteDisplayName);
    void applyConfigurationBundle(const ConfigurationBundle& bundle, SpectralShipGen::ShipGenerationRecipe& recipe);
    SpectralShipGen::ValidationResult validateConfigurationBundle(const ConfigurationBundle& bundle);
}
