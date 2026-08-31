#include "PreviewRegressionSuites.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "DiagnosticsAppController.h"

namespace
{
    using SpectralShipGenStudioDiagnosticsApp::DiagnosticsAppController;
    using SpectralShipGenStudioDiagnosticsApp::DiagnosticsAppRunState;

    bool expect(bool condition, const char* message)
    {
        if (!condition) { std::cerr << "Diagnostics app regression failed: " << message << '\n'; }
        return condition;
    }

    SpectralShipGenDiagnostics::DiagnosticsRunConfiguration makeConfiguration(uint64_t samples, uint32_t resolution = 32u)
    {
        SpectralShipGenDiagnostics::DiagnosticsRunConfiguration configuration;
        configuration.Dimensions = { { resolution, resolution } };
        configuration.Styles = { SpectralShipGen::ShipStyle::FIGHTER };
        configuration.Factions = { SpectralShipGen::ShipFactionType::MILITARY };
        configuration.SamplesPerConfiguration = samples;
        configuration.DiagnosticSeed = 0x65D1A60000000001ull;
        configuration.DetailedPerformanceInstrumentation = true;
        return configuration;
    }

    bool waitForTerminalState(DiagnosticsAppController& controller, std::chrono::milliseconds timeout)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
            const auto snapshot = controller.getSnapshot();
            if (snapshot.State == DiagnosticsAppRunState::COMPLETED || snapshot.State == DiagnosticsAppRunState::CANCELLED || snapshot.State == DiagnosticsAppRunState::ERROR)
            {
                controller.wait();
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        return false;
    }
}

namespace SpectralShipGenStudioTests
{
    int runDiagnosticsAppRegression()
    {
        {
            auto invalid = makeConfiguration(1u);
            invalid.Dimensions.clear();
            std::string error;
            if (!expect(!SpectralShipGenStudioDiagnosticsApp::validateDiagnosticsConfiguration(invalid, error), "empty dimensions should be rejected")) { return 1; }
            invalid = makeConfiguration(1u);
            invalid.Styles.clear();
            if (!expect(!SpectralShipGenStudioDiagnosticsApp::validateDiagnosticsConfiguration(invalid, error), "empty styles should be rejected")) { return 1; }
            invalid = makeConfiguration(1u);
            invalid.Factions.clear();
            if (!expect(!SpectralShipGenStudioDiagnosticsApp::validateDiagnosticsConfiguration(invalid, error), "empty factions should be rejected")) { return 1; }
            invalid = makeConfiguration(0u);
            if (!expect(!SpectralShipGenStudioDiagnosticsApp::validateDiagnosticsConfiguration(invalid, error), "zero samples should be rejected")) { return 1; }
        }

        const std::filesystem::path csvPath = std::filesystem::temp_directory_path() / "spectral_ship_gen_diagnostics_app_regression.csv";
        const std::filesystem::path runPath = std::filesystem::temp_directory_path() / "spectral_ship_gen_diagnostics_app_regression.shipdiag.json";
        std::filesystem::remove(csvPath);
        std::filesystem::remove(runPath);

        {
            DiagnosticsAppController controller;
            std::string error;
            if (!expect(controller.start(makeConfiguration(4u), error), "valid configuration should start")) { return 1; }
            const auto running = controller.getSnapshot();
            if (!expect(running.State == DiagnosticsAppRunState::RUNNING || running.State == DiagnosticsAppRunState::COMPLETED, "start should transition to running/completed")) { return 1; }
            if (!expect(waitForTerminalState(controller, std::chrono::seconds(20)), "completed run timed out")) { return 1; }
            const auto snapshot = controller.getSnapshot();
            if (!expect(snapshot.State == DiagnosticsAppRunState::COMPLETED, "small run should complete")) { return 1; }
            if (!expect(snapshot.HasResult && snapshot.ResultCompleted && !snapshot.ResultCancelled, "completed result ownership invalid")) { return 1; }
            if (!expect(snapshot.CompletedSamples == 4u && snapshot.ScheduledSamples == 4u, "completed sample counts invalid")) { return 1; }
            if (!expect(snapshot.Progress.ProgressPercent >= 99.999, "completed progress should reach 100 percent")) { return 1; }
            if (!expect(snapshot.LiveSummary.SampleCount == 4u, "live summary should ingest every completed sample")) { return 1; }
            if (!expect(snapshot.FinalSummary.GenerationTimeMilliseconds.Count == 4u, "final summary should retain completed samples")) { return 1; }
            if (!expect(controller.exportCsv(csvPath, error), "completed result should export CSV")) { return 1; }
            std::ifstream csv(csvPath);
            std::string header;
            std::getline(csv, header);
            if (!expect(header.find("generation_time_mean_ms") != std::string::npos, "CSV should use Task-64 backend schema")) { return 1; }
            if (!expect(controller.saveRun(runPath, error), "completed result should save .shipdiag.json")) { return 1; }
            controller.reset();
            if (!expect(controller.loadRun(runPath, error), "saved .shipdiag.json should load back into controller")) { return 1; }
            const auto loadedSnapshot = controller.getSnapshot();
            if (!expect(loadedSnapshot.State == DiagnosticsAppRunState::COMPLETED && loadedSnapshot.HasResult, "loaded completed run should restore completed state")) { return 1; }
            if (!expect(loadedSnapshot.CompletedSamples == 4u && loadedSnapshot.FinalSummary.GenerationTimeMilliseconds.Count == 4u, "loaded run summary/sample ownership invalid")) { return 1; }
        }
        std::filesystem::remove(csvPath);
        std::filesystem::remove(runPath);

        {
            DiagnosticsAppController controller;
            std::string error;
            if (!expect(controller.start(makeConfiguration(200u, 160u), error), "cancellation run should start")) { return 1; }
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
            while (std::chrono::steady_clock::now() < deadline)
            {
                const auto snapshot = controller.getSnapshot();
                if (snapshot.CompletedSamples >= 1u || snapshot.State == DiagnosticsAppRunState::COMPLETED) { break; }
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            controller.requestCancel();
            const auto cancelling = controller.getSnapshot();
            if (!expect(cancelling.State == DiagnosticsAppRunState::CANCELLING || cancelling.State == DiagnosticsAppRunState::CANCELLED || cancelling.State == DiagnosticsAppRunState::COMPLETED, "cancel should transition coherently")) { return 1; }
            if (!expect(waitForTerminalState(controller, std::chrono::seconds(30)), "cancelled run timed out")) { return 1; }
            const auto snapshot = controller.getSnapshot();
            if (!expect(snapshot.State == DiagnosticsAppRunState::CANCELLED || snapshot.State == DiagnosticsAppRunState::COMPLETED, "cancel should end in cancelled unless work already completed")) { return 1; }
            if (snapshot.State == DiagnosticsAppRunState::CANCELLED)
            {
                if (!expect(snapshot.HasResult && snapshot.ResultCancelled && !snapshot.ResultCompleted, "cancelled result should be retained as partial")) { return 1; }
                if (!expect(snapshot.CompletedSamples < snapshot.ScheduledSamples, "cancelled result should be incomplete")) { return 1; }
            }
        }

        {
            DiagnosticsAppController controller;
            std::string error;
            if (!expect(controller.start(makeConfiguration(300u, 160u), error), "destructor-cleanup run should start")) { return 1; }
            // Scope exit exercises cooperative cancellation and join; no detached worker is permitted.
        }

        std::cout << "Diagnostics app controller regression passed.\n";
        return 0;
    }
}
