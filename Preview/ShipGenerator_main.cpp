#include <string>

#include "ShipGeneratorPreviewApp.h"

int main(int argc, char** argv)
{
    PixelShipGeneratorPreview::ShipGeneratorPreviewApp app(argc > 1 ? std::string(argv[1]) : std::string());
    return app.run();
}
