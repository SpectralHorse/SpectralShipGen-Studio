#include "RegressionSuites.h"

namespace PixelShipGeneratorTests
{
    std::vector<RegressionSuite> createPreviewRegressionSuites()
    {
        std::vector<RegressionSuite> suites = {
            { "preview-preferences", "Preview Preferences", RegressionCategory::PERSISTENCE, runPreviewPreferencesRegression, false },
            { "diagnostics-app", "Diagnostics Application Controller", RegressionCategory::TOOLING, runDiagnosticsAppRegression, false },
            { "calibration", "Calibration Lab", RegressionCategory::TOOLING, runGenerationCalibrationRegression, true }
        };
#if PIXEL_SHIP_GENERATOR_PREVIEW_HAS_SFML
        suites.push_back({ "attribute-reroll-studio", "Attribute Reroll Studio", RegressionCategory::TOOLING, runAttributeRerollStudioRegression, false });
#endif
        return suites;
    }
}
