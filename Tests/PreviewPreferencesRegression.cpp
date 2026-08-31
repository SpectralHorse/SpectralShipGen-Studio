#include "PreviewRegressionSuites.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "PreviewPreferences.h"
#include "PreviewResolution.h"

int PixelShipGeneratorTests::runPreviewPreferencesRegression()
{
    using namespace PixelShipGeneratorPreview;
    using PixelShipGenerator::ShipDimensions;
    bool success = true;

    PreviewPreferences preferences;
    preferences.ResolutionBookmarks = {
        { 72u, 72u },
        { 40u, 56u },
        { 40u, 56u },
        { 31u, 48u },
        { 48u, 64u },
        { 256u, 128u },
        { 24u, 48u },
        { 112u, 80u },
        { 192u, 256u },
        { 300u, 128u },
        { 24u, 64u }
    };

    const std::string json = serializePreviewPreferences(preferences);
    const PreviewPreferencesLoadResult parsed = deserializePreviewPreferences(json);
    const std::vector<ShipDimensions> expected = {
        { 24u, 48u },
        { 40u, 56u },
        { 48u, 64u },
        { 72u, 72u },
        { 112u, 80u },
        { 192u, 256u }
    };

    if (!parsed.Success || parsed.Preferences.ResolutionBookmarks != expected)
    {
        success = false;
        std::cerr << "Preview preference sanitization/sorting failed.\n";
    }

    const auto legacy = deserializePreviewPreferences("{\"format_version\":1,\"resolution_bookmarks\":[40,72,40,22,256]}");
    const std::vector<ShipDimensions> expectedLegacy = { { 40u, 40u }, { 72u, 72u }, { 256u, 256u } };
    if (!legacy.Success || legacy.Preferences.ResolutionBookmarks != expectedLegacy)
    {
        success = false;
        std::cerr << "Legacy square bookmark migration failed.\n";
    }

    const auto invalidVersion = deserializePreviewPreferences("{\"format_version\":99,\"dimension_bookmarks\":[]}");
    if (invalidVersion.Success)
    {
        success = false;
        std::cerr << "Unsupported preferences version was not rejected.\n";
    }

    if (!isSelectablePreviewResolution(24u) || !isSelectablePreviewResolution(256u) || isSelectablePreviewResolution(22u) || isSelectablePreviewResolution(25u) || isSelectablePreviewResolution(258u))
    {
        success = false;
        std::cerr << "Preview dimension validation is incorrect.\n";
    }

    if (!isSelectablePreviewDimensions({ 32u, 64u }) || !isSelectablePreviewDimensions({ 64u, 32u }) || !isSelectablePreviewDimensions({ 48u, 64u }) || isSelectablePreviewDimensions({ 30u, 64u }) || isSelectablePreviewDimensions({ 64u, 30u }))
    {
        success = false;
        std::cerr << "Preview aspect-ratio validation is incorrect.\n";
    }

    if (clampPreviewResolution(23u) != 24u || clampPreviewResolution(41u) != 42u || clampPreviewResolution(257u) != 256u)
    {
        success = false;
        std::cerr << "Preview dimension snapping is incorrect.\n";
    }

    const std::filesystem::path path = std::filesystem::temp_directory_path() / "pixel_ship_generator_preview_preferences_regression.json";
    std::string error;
    if (!savePreviewPreferences(preferences, path, error))
    {
        success = false;
        std::cerr << error << '\n';
    }
    else
    {
        const PreviewPreferencesLoadResult loaded = loadPreviewPreferences(path);
        if (!loaded.Success || loaded.Preferences.ResolutionBookmarks != expected)
        {
            success = false;
            std::cerr << "Preview preferences file round-trip failed.\n";
        }
        std::error_code removeError;
        std::filesystem::remove(path, removeError);
    }

    return success ? 0 : 1;
}
