#include "RegressionSuites.h"

namespace PixelShipGeneratorTests
{
    std::vector<RegressionSuite> createPreviewRegressionSuites()
    {
        std::vector<RegressionSuite> suites = {
            { "preview-preferences", "Preview Preferences", RegressionCategory::PERSISTENCE, runPreviewPreferencesRegression, false },
            { "preview-favorites", "Preview Favorites Persistence", RegressionCategory::PERSISTENCE, runPreviewFavoritesPersistenceRegression, false },
            { "preview-session", "Preview Session Controllers", RegressionCategory::TOOLING, runPreviewSessionRegression, false },
            { "diagnostics-app", "Diagnostics Application Controller", RegressionCategory::TOOLING, runDiagnosticsAppRegression, false },
            { "configuration-editor", "Preview Configuration Editor", RegressionCategory::TOOLING, runPreviewConfigurationEditorRegression, false },
            { "faction-profile-editor", "Preview Faction Profile Editor", RegressionCategory::TOOLING, runPreviewFactionProfileEditorRegression, false },
            { "palette-editor", "Preview Palette Editor", RegressionCategory::TOOLING, runPreviewPaletteEditorRegression, false },
            { "calibration", "Calibration Lab", RegressionCategory::TOOLING, runGenerationCalibrationRegression, true }
        };
#if PIXEL_SHIP_GENERATOR_PREVIEW_HAS_SFML
        suites.push_back({ "attribute-reroll-studio", "Attribute Reroll Studio", RegressionCategory::TOOLING, runAttributeRerollStudioRegression, false });
#endif
        return suites;
    }
}
