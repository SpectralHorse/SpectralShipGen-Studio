#pragma once

#include "RegressionRunner.h"

#include <vector>

namespace PixelShipGeneratorTests
{
    int runPreviewPreferencesRegression();
    int runPreviewFavoritesPersistenceRegression();
    int runPreviewFavoritesBrowserRegression();
    int runPreviewUserPresetPersistenceRegression();
    int runPreviewConfigurationBundleRegression();
    int runPreviewSessionRegression();
    int runPreviewWorkspaceRegression();
    int runPreviewInspectionRegression();
    int runPreviewAnimationLabRegression();
    int runPreviewUiQolRegression();
    int runGenerationCalibrationRegression();
    int runAttributeRerollStudioRegression();
    int runDiagnosticsAppRegression();
    int runPreviewConfigurationEditorRegression();
    int runPreviewFactionProfileEditorRegression();
    int runPreviewPaletteEditorRegression();

    std::vector<RegressionSuite> createPreviewRegressionSuites();
}
