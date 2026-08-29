#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "DiagnosticsRunner.h"

namespace PixelShipGeneratorDiagnosticsApp
{
    enum class DiagnosticsAppRunState : uint32_t
    {
        READY = 0u,
        RUNNING,
        CANCELLING,
        COMPLETED,
        CANCELLED,
        ERROR
    };

    struct DiagnosticsAppLiveSummary
    {
        uint64_t SampleCount = 0u;
        double AverageGenerationMilliseconds = 0.0;
        double AverageHullAttempts = 0.0;
        double HullRetryRatePercent = 0.0;
        double NegativeSpaceAttemptRatePercent = 0.0;
        double NegativeSpaceSuccessRatePercent = 0.0;
    };

    struct DiagnosticsAppSnapshot
    {
        DiagnosticsAppRunState State = DiagnosticsAppRunState::READY;
        PixelShipGeneratorDiagnostics::DiagnosticsProgress Progress;
        DiagnosticsAppLiveSummary LiveSummary;
        PixelShipGeneratorDiagnostics::DiagnosticsAggregateSummary FinalSummary;
        uint64_t CompletedSamples = 0u;
        uint64_t ScheduledSamples = 0u;
        bool HasResult = false;
        bool ResultCompleted = false;
        bool ResultCancelled = false;
        std::string ErrorMessage;
        std::string StatusMessage;
    };

    const char* getDiagnosticsAppRunStateName(DiagnosticsAppRunState state);
    bool validateDiagnosticsConfiguration(const PixelShipGeneratorDiagnostics::DiagnosticsRunConfiguration& configuration, std::string& errorMessage);

    class DiagnosticsAppController
    {
    public:
        DiagnosticsAppController() = default;
        ~DiagnosticsAppController();

        DiagnosticsAppController(const DiagnosticsAppController&) = delete;
        DiagnosticsAppController& operator=(const DiagnosticsAppController&) = delete;

        bool start(const PixelShipGeneratorDiagnostics::DiagnosticsRunConfiguration& configuration, std::string& errorMessage);
        void requestCancel();
        void wait();
        void reset();

        DiagnosticsAppSnapshot getSnapshot() const;
        std::shared_ptr<const PixelShipGeneratorDiagnostics::DiagnosticsResult> getResult() const;
        bool exportCsv(const std::filesystem::path& path, std::string& errorMessage) const;
        bool saveRun(const std::filesystem::path& path, std::string& errorMessage) const;
        bool loadRun(const std::filesystem::path& path, std::string& errorMessage);
        bool hasActiveWorker() const;

    private:
        void workerMain(PixelShipGeneratorDiagnostics::DiagnosticsRunConfiguration configuration);
        void updateProgress(const PixelShipGeneratorDiagnostics::DiagnosticsProgress& progress);
        void observeSample(const PixelShipGeneratorDiagnostics::DiagnosticsRawSampleResult& sample);
        void joinFinishedWorker();

    private:
        mutable std::mutex m_Mutex;
        std::thread m_Worker;
        std::atomic<bool> m_CancelRequested = false;
        DiagnosticsAppRunState m_State = DiagnosticsAppRunState::READY;
        PixelShipGeneratorDiagnostics::DiagnosticsProgress m_Progress;
        DiagnosticsAppLiveSummary m_LiveSummary;
        std::shared_ptr<PixelShipGeneratorDiagnostics::DiagnosticsResult> m_Result;
        std::string m_ErrorMessage;
        std::string m_StatusMessage;
        double m_TotalGenerationMilliseconds = 0.0;
        uint64_t m_TotalHullAttempts = 0u;
        uint64_t m_TotalHullRetries = 0u;
        uint64_t m_NegativeSpaceAttempts = 0u;
        uint64_t m_NegativeSpaceSuccesses = 0u;
    };
}
