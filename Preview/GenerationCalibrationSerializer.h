#pragma once

#include <filesystem>
#include <string>

#include "GenerationCalibration.h"

namespace PixelShipGeneratorPreview
{
    struct GenerationCalibrationSessionLoadResult
    {
        GenerationCalibrationSession Session;
        bool Success = false;
        std::string Error;
    };

    std::string serializeGenerationCalibrationSession(const GenerationCalibrationSession& session);
    GenerationCalibrationSessionLoadResult deserializeGenerationCalibrationSession(const std::string& text);
    bool saveGenerationCalibrationSession(const GenerationCalibrationSession& session, const std::filesystem::path& path, std::string& error);
    GenerationCalibrationSessionLoadResult loadGenerationCalibrationSession(const std::filesystem::path& path);
    bool exportGenerationCalibrationCsv(const GenerationCalibrationSession& session, const std::filesystem::path& path, std::string& error);
    bool exportGenerationTuningProfile(const PixelShipGenerator::GenerationTuningProfile& profile, const std::filesystem::path& path, std::string& error);
}
