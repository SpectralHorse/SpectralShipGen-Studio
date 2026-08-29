#include "DiagnosticsApp.h"

#include <filesystem>
#include <string>

int main(int argc, char** argv)
{
    PixelShipGeneratorDiagnosticsApp::DiagnosticsAppLaunchOptions options;
    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];
        if (argument == "--smoke") { options.AutomatedSmoke = true; }
        else if (argument == "--cancel-smoke") { options.AutomatedSmoke = true; options.CancelSmoke = true; }
        else if (argument == "--screenshot" && index + 1 < argc) { options.ScreenshotPath = std::filesystem::path(argv[++index]); }
        else if (argument == "--csv" && index + 1 < argc) { options.SmokeCsvPath = std::filesystem::path(argv[++index]); }
    }
    PixelShipGeneratorDiagnosticsApp::DiagnosticsApp app(options);
    return app.run();
}
