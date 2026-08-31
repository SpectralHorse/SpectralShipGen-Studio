#include "ConfigurationBundle.h"

#include <utility>

#include <PixelShipGenerator/ShipFactionProfile.h>
#include <PixelShipGenerator/ShipGenerationProfile.h>
#include <PixelShipGenerator/ShipResolvedGenerationConfiguration.h>

namespace PixelShipGeneratorPreview
{
    ConfigurationBundle makeConfigurationBundle(const PixelShipGenerator::ShipGenerationRecipe& recipe,
        std::string structuralDisplayName,
        std::string factionDisplayName,
        std::string paletteDisplayName)
    {
        ConfigurationBundle bundle;
        bundle.StructuralDisplayName = std::move(structuralDisplayName);
        bundle.FactionDisplayName = std::move(factionDisplayName);
        bundle.PaletteDisplayName = std::move(paletteDisplayName);
        bundle.StructuralProfile = recipe.StructuralSource == PixelShipGenerator::ShipGenerationRecipeProfileSource::BUILT_IN_PRESET
            ? PixelShipGenerator::getShipGenerationProfile(recipe.Style)
            : recipe.StructuralProfile;
        bundle.FactionProfile = recipe.FactionSource == PixelShipGenerator::ShipGenerationRecipeProfileSource::BUILT_IN_PRESET
            ? PixelShipGenerator::getShipFactionProfile(recipe.Faction)
            : recipe.FactionProfile;
        bundle.PaletteConfiguration = recipe.PaletteConfiguration;
        return bundle;
    }

    void applyConfigurationBundle(const ConfigurationBundle& bundle, PixelShipGenerator::ShipGenerationRecipe& recipe)
    {
        recipe.StructuralSource = PixelShipGenerator::ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        recipe.Style = PixelShipGenerator::ShipStyle::SHIP_STYLE_END;
        recipe.StructuralProfile = bundle.StructuralProfile;
        recipe.FactionSource = PixelShipGenerator::ShipGenerationRecipeProfileSource::EMBEDDED_CUSTOM;
        recipe.Faction = PixelShipGenerator::ShipFactionType::SHIP_FACTION_TYPE_END;
        recipe.FactionProfile = bundle.FactionProfile;
        recipe.PaletteConfiguration = bundle.PaletteConfiguration;
    }

    PixelShipGenerator::ValidationResult validateConfigurationBundle(const ConfigurationBundle& bundle)
    {
        PixelShipGenerator::ShipResolvedGenerationConfiguration configuration;
        configuration.Generation.Dimensions = { 64u, 64u };
        configuration.Generation.PaletteConfiguration = bundle.PaletteConfiguration;
        configuration.StructuralProfile = bundle.StructuralProfile;
        configuration.FactionProfile = bundle.FactionProfile;
        configuration.Provenance.PaletteSource = bundle.PaletteConfiguration.Mode;
        return PixelShipGenerator::validateShipGenerationConfiguration(configuration);
    }
}
