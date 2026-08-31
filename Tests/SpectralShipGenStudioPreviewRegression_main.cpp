#include "RegressionRunner.h"
#include "PreviewRegressionSuites.h"

#include <iostream>

int main(int argc, char** argv)
{
    const std::vector<SpectralShipGenStudioTests::RegressionSuite> suites = SpectralShipGenStudioTests::createPreviewRegressionSuites();
    return SpectralShipGenStudioTests::runRegressionRunner(argc, argv, suites, "SpectralShipGenStudioPreviewRegression", std::cout, std::cerr);
}
