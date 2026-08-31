#include "ConfigurationBundle.h"

#include <utility>

#include <SpectralShipGen/ShipFactionProfile.h>
#include <SpectralShipGen/ShipGenerationProfile.h>
#include <SpectralShipGen/ShipResolvedGenerationConfiguration.h>

namespace SpectralShipGenStudioPreview
{
    ConfigurationBundle makeConfigurationBundle(const SpectralShipGen::ShipGenerationRecipe& recipe,
        std::string structuralDisplayName,
        std::string factionDisplayName,
        std::string paletteDisplayName)
    {
        ConfigurationBundle bundle;
        bundle.StructuralDisplayName = std::move(structuralDisplayName);
        bundle.FactionDisplayName = std::move(factionDisplayName);
        bundle.PaletteDisplayName = std::move(paletteDisplayName);
        bundle.StructuralProfile = recipe.StructuralSource == SpectralShipGen::ShipGenerationRecipeProfileSource::BUILT_IN_PRESET
            ? SpectralShipGen::getShipGenerationProfile(recipe.Style)
            : recipe.StructuralProfile;
        bundle.FactionProfile = recipe.FactionSource == SpectralShipGen::ShipGenerationRecipeProfileSource::BUILT_IN_PRESET
            ? SpectralShipGen::getShipFactionProfile(recipe.Faction)
            : recipe.FactionProfile;
        bundle.PaletteConfiguration = recipe.PaletteConfiguration;
        return bundle;
    }

    void applyConfigurationBundle(const ConfigurationBundle& bundle, SpectralShipGen::ShipGenerationRecipe& recipe)
    {
        recipe.StructuralSource = SpectralShipGen::ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        recipe.Style = SpectralShipGen::ShipStyle::SHIP_STYLE_END;
        recipe.StructuralProfile = bundle.StructuralProfile;
        recipe.FactionSource = SpectralShipGen::ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        recipe.Faction = SpectralShipGen::ShipFactionType::SHIP_FACTION_TYPE_END;
        recipe.FactionProfile = bundle.FactionProfile;
        recipe.PaletteConfiguration = bundle.PaletteConfiguration;
    }

    SpectralShipGen::ValidationResult validateConfigurationBundle(const ConfigurationBundle& bundle)
    {
        SpectralShipGen::ShipResolvedGenerationConfiguration configuration;
        configuration.Generation.Dimensions = { 64u, 64u };
        configuration.Generation.PaletteConfiguration = bundle.PaletteConfiguration;
        configuration.StructuralProfile = bundle.StructuralProfile;
        configuration.FactionProfile = bundle.FactionProfile;
        configuration.Provenance.PaletteSource = bundle.PaletteConfiguration.Mode;
        return SpectralShipGen::validateShipGenerationConfiguration(configuration);
    }
}
