#include <string>

#include "ShipGeneratorPreviewApp.h"

int main(int argc, char** argv)
{
    SpectralShipGenStudioPreview::ShipGeneratorPreviewApp app(argc > 1 ? std::string(argv[1]) : std::string());
    return app.run();
}
