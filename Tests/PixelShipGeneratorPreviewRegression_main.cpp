#include "RegressionRunner.h"
#include "RegressionSuites.h"

#include <iostream>

int main(int argc, char** argv)
{
    const std::vector<PixelShipGeneratorTests::RegressionSuite> suites = PixelShipGeneratorTests::createPreviewRegressionSuites();
    return PixelShipGeneratorTests::runRegressionRunner(argc, argv, suites, "PixelShipGeneratorPreviewRegression", std::cout, std::cerr);
}
