#include "DiagnosticsAppController.h"

#include <fstream>
#include <stdexcept>
#include <utility>

#include "DiagnosticsResultSerializer.h"

namespace PixelShipGeneratorDiagnosticsApp
{
    const char* getDiagnosticsAppRunStateName(DiagnosticsAppRunState state)
    {
        switch (state)
        {
        case DiagnosticsAppRunState::READY: return "READY";
        case DiagnosticsAppRunState::RUNNING: return "RUNNING";
        case DiagnosticsAppRunState::CANCELLING: return "CANCELLING";
        case DiagnosticsAppRunState::COMPLETED: return "COMPLETED";
        case DiagnosticsAppRunState::CANCELLED: return "CANCELLED";
        case DiagnosticsAppRunState::ERROR: return "ERROR";
        default: return "UNKNOWN";
        }
    }

    bool validateDiagnosticsConfiguration(const PixelShipGeneratorDiagnostics::DiagnosticsRunConfiguration& configuration, std::string& errorMessage)
    {
        if (configuration.Dimensions.empty())
        {
            errorMessage = "Select at least one dimension.";
            return false;
        }
        if (configuration.Styles.empty())
        {
            errorMessage = "Select at least one style.";
            return false;
        }
        if (configuration.Factions.empty())
        {
            errorMessage = "Select at least one faction.";
            return false;
        }
        if (configuration.SamplesPerConfiguration == 0u)
        {
            errorMessage = "Samples per combination must be greater than zero.";
            return false;
        }
        for (const PixelShipGenerator::ShipDimensions dimensions : configuration.Dimensions)
        {
            if (dimensions.Width == 0u || dimensions.Height == 0u)
            {
                errorMessage = "Diagnostic dimensions must be non-zero.";
                return false;
            }
        }
        errorMessage.clear();
        return true;
    }

    DiagnosticsAppController::~DiagnosticsAppController()
    {
        requestCancel();
        wait();
    }

    bool DiagnosticsAppController::start(const PixelShipGeneratorDiagnostics::DiagnosticsRunConfiguration& configuration, std::string& errorMessage)
    {
        joinFinishedWorker();
        if (!validateDiagnosticsConfiguration(configuration, errorMessage))
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_StatusMessage = errorMessage;
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            if (m_State == DiagnosticsAppRunState::RUNNING || m_State == DiagnosticsAppRunState::CANCELLING)
            {
                errorMessage = "Diagnostics are already running.";
                return false;
            }
            m_State = DiagnosticsAppRunState::RUNNING;
            m_Progress = {};
            m_LiveSummary = {};
            m_Result.reset();
            m_ErrorMessage.clear();
            m_StatusMessage = "Diagnostics started.";
            m_TotalGenerationMilliseconds = 0.0;
            m_TotalHullAttempts = 0u;
            m_TotalHullRetries = 0u;
            m_NegativeSpaceAttempts = 0u;
            m_NegativeSpaceSuccesses = 0u;
        }

        m_CancelRequested.store(false, std::memory_order_release);
        m_Worker = std::thread(&DiagnosticsAppController::workerMain, this, configuration);
        errorMessage.clear();
        return true;
    }

    void DiagnosticsAppController::requestCancel()
    {
        m_CancelRequested.store(true, std::memory_order_release);
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (m_State == DiagnosticsAppRunState::RUNNING)
        {
            m_State = DiagnosticsAppRunState::CANCELLING;
            m_StatusMessage = "Cancellation requested; finishing current sample.";
        }
    }

    void DiagnosticsAppController::wait()
    {
        if (m_Worker.joinable())
        {
            m_Worker.join();
        }
    }

    void DiagnosticsAppController::reset()
    {
        requestCancel();
        wait();
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_State = DiagnosticsAppRunState::READY;
        m_Progress = {};
        m_LiveSummary = {};
        m_Result.reset();
        m_ErrorMessage.clear();
        m_StatusMessage.clear();
        m_TotalGenerationMilliseconds = 0.0;
        m_TotalHullAttempts = 0u;
        m_TotalHullRetries = 0u;
        m_NegativeSpaceAttempts = 0u;
        m_NegativeSpaceSuccesses = 0u;
        m_CancelRequested.store(false, std::memory_order_release);
    }

    DiagnosticsAppSnapshot DiagnosticsAppController::getSnapshot() const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        DiagnosticsAppSnapshot snapshot;
        snapshot.State = m_State;
        snapshot.Progress = m_Progress;
        snapshot.LiveSummary = m_LiveSummary;
        snapshot.CompletedSamples = m_Progress.CompletedWorkItems;
        snapshot.ScheduledSamples = m_Progress.TotalWorkItems;
        snapshot.ErrorMessage = m_ErrorMessage;
        snapshot.StatusMessage = m_StatusMessage;
        snapshot.HasResult = static_cast<bool>(m_Result);
        if (m_Result)
        {
            snapshot.ResultCompleted = m_Result->Completed;
            snapshot.ResultCancelled = m_Result->Cancelled;
            snapshot.CompletedSamples = m_Result->CompletedWorkItems;
            snapshot.ScheduledSamples = m_Result->ScheduledWorkItems;
            snapshot.FinalSummary = m_Result->OverallSummary;
        }
        return snapshot;
    }

    std::shared_ptr<const PixelShipGeneratorDiagnostics::DiagnosticsResult> DiagnosticsAppController::getResult() const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Result;
    }

    bool DiagnosticsAppController::exportCsv(const std::filesystem::path& path, std::string& errorMessage) const
    {
        std::shared_ptr<const PixelShipGeneratorDiagnostics::DiagnosticsResult> result;
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            result = m_Result;
        }
        if (!result)
        {
            errorMessage = "No completed or partial diagnostics result is available.";
            return false;
        }

        std::ofstream stream(path);
        if (!stream)
        {
            errorMessage = "Unable to open CSV output path: " + path.string();
            return false;
        }
        PixelShipGeneratorDiagnostics::writeDiagnosticsResultCsv(stream, *result);
        if (!stream.good())
        {
            errorMessage = "Failed while writing diagnostics CSV: " + path.string();
            return false;
        }
        errorMessage.clear();
        return true;
    }


    bool DiagnosticsAppController::saveRun(const std::filesystem::path& path, std::string& errorMessage) const
    {
        std::shared_ptr<const PixelShipGeneratorDiagnostics::DiagnosticsResult> result;
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            result = m_Result;
        }
        if (!result)
        {
            errorMessage = "No completed or partial diagnostics result is available.";
            return false;
        }
        return PixelShipGeneratorDiagnostics::saveDiagnosticsResultJson(path, *result, errorMessage);
    }

    bool DiagnosticsAppController::loadRun(const std::filesystem::path& path, std::string& errorMessage)
    {
        joinFinishedWorker();
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            if (m_State == DiagnosticsAppRunState::RUNNING || m_State == DiagnosticsAppRunState::CANCELLING)
            {
                errorMessage = "Cancel the active diagnostics run before loading another result.";
                return false;
            }
        }
        PixelShipGeneratorDiagnostics::DiagnosticsResultLoadResult loaded = PixelShipGeneratorDiagnostics::loadDiagnosticsResultJson(path);
        if (!loaded.Success)
        {
            errorMessage = loaded.Error;
            return false;
        }
        auto result = std::make_shared<PixelShipGeneratorDiagnostics::DiagnosticsResult>(std::move(loaded.Result));
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Result = std::move(result);
        m_Progress = {};
        m_Progress.TotalWorkItems = m_Result->ScheduledWorkItems;
        m_Progress.CompletedWorkItems = m_Result->CompletedWorkItems;
        m_Progress.ProgressPercent = m_Result->ScheduledWorkItems == 0u ? 100.0 : 100.0 * static_cast<double>(m_Result->CompletedWorkItems) / static_cast<double>(m_Result->ScheduledWorkItems);
        m_Progress.ElapsedNanoseconds = m_Result->ElapsedNanoseconds;
        m_LiveSummary = {};
        m_LiveSummary.SampleCount = m_Result->CompletedWorkItems;
        m_LiveSummary.AverageGenerationMilliseconds = m_Result->OverallSummary.GenerationTimeMilliseconds.Mean;
        m_LiveSummary.AverageHullAttempts = m_Result->OverallSummary.HullAttempts.Mean;
        m_LiveSummary.HullRetryRatePercent = m_Result->OverallSummary.HullRetryRatePercent;
        m_LiveSummary.NegativeSpaceAttemptRatePercent = m_Result->OverallSummary.StructuralNegativeSpaceAttemptRatePercent;
        m_LiveSummary.NegativeSpaceSuccessRatePercent = m_Result->OverallSummary.StructuralNegativeSpaceSuccessRatePercent;
        m_State = m_Result->Cancelled ? DiagnosticsAppRunState::CANCELLED : (m_Result->Completed ? DiagnosticsAppRunState::COMPLETED : DiagnosticsAppRunState::ERROR);
        m_ErrorMessage = m_State == DiagnosticsAppRunState::ERROR ? "Loaded diagnostics result is incomplete and not marked cancelled." : std::string();
        m_StatusMessage = "Loaded diagnostics run: " + path.string();
        errorMessage.clear();
        return true;
    }

    bool DiagnosticsAppController::hasActiveWorker() const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_State == DiagnosticsAppRunState::RUNNING || m_State == DiagnosticsAppRunState::CANCELLING;
    }

    void DiagnosticsAppController::workerMain(PixelShipGeneratorDiagnostics::DiagnosticsRunConfiguration configuration)
    {
        try
        {
            PixelShipGeneratorDiagnostics::DiagnosticsRunner runner;
            PixelShipGeneratorDiagnostics::DiagnosticsResult result = runner.run(
                configuration,
                [this](const PixelShipGeneratorDiagnostics::DiagnosticsProgress& progress) { updateProgress(progress); },
                [this]() { return m_CancelRequested.load(std::memory_order_acquire); },
                [this](const PixelShipGeneratorDiagnostics::DiagnosticsRawSampleResult& sample) { observeSample(sample); });

            auto sharedResult = std::make_shared<PixelShipGeneratorDiagnostics::DiagnosticsResult>(std::move(result));
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_Result = std::move(sharedResult);
            m_Progress.TotalWorkItems = m_Result->ScheduledWorkItems;
            m_Progress.CompletedWorkItems = m_Result->CompletedWorkItems;
            m_Progress.ProgressPercent = m_Result->ScheduledWorkItems == 0u ? 100.0 :
                100.0 * static_cast<double>(m_Result->CompletedWorkItems) / static_cast<double>(m_Result->ScheduledWorkItems);
            if (m_Result->Cancelled)
            {
                m_State = DiagnosticsAppRunState::CANCELLED;
                m_StatusMessage = "Diagnostics cancelled; partial result retained.";
            }
            else if (m_Result->Completed)
            {
                m_State = DiagnosticsAppRunState::COMPLETED;
                m_StatusMessage = "Diagnostics completed.";
            }
            else
            {
                m_State = DiagnosticsAppRunState::ERROR;
                m_ErrorMessage = "Diagnostics ended without completing the configured work.";
                m_StatusMessage = m_ErrorMessage;
            }
        }
        catch (const std::exception& exception)
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_State = DiagnosticsAppRunState::ERROR;
            m_ErrorMessage = exception.what();
            m_StatusMessage = "Diagnostics worker failed: " + m_ErrorMessage;
        }
        catch (...)
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_State = DiagnosticsAppRunState::ERROR;
            m_ErrorMessage = "Unknown diagnostics worker error.";
            m_StatusMessage = m_ErrorMessage;
        }
    }

    void DiagnosticsAppController::updateProgress(const PixelShipGeneratorDiagnostics::DiagnosticsProgress& progress)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Progress = progress;
    }

    void DiagnosticsAppController::observeSample(const PixelShipGeneratorDiagnostics::DiagnosticsRawSampleResult& sample)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        ++m_LiveSummary.SampleCount;
        m_TotalGenerationMilliseconds += static_cast<double>(sample.TotalGenerationNanoseconds) / 1000000.0;
        m_TotalHullAttempts += sample.HullAttemptCount;
        if (sample.HullAttemptCount > 1u)
        {
            m_TotalHullRetries += static_cast<uint64_t>(sample.HullAttemptCount - 1u);
        }
        m_NegativeSpaceAttempts += sample.StructuralNegativeSpaceAttemptCount;
        m_NegativeSpaceSuccesses += sample.StructuralNegativeSpaceSuccessCount;

        const double count = static_cast<double>(m_LiveSummary.SampleCount);
        m_LiveSummary.AverageGenerationMilliseconds = m_TotalGenerationMilliseconds / count;
        m_LiveSummary.AverageHullAttempts = static_cast<double>(m_TotalHullAttempts) / count;
        m_LiveSummary.HullRetryRatePercent = 100.0 * static_cast<double>(m_TotalHullRetries) / count;
        m_LiveSummary.NegativeSpaceAttemptRatePercent = 100.0 * static_cast<double>(m_NegativeSpaceAttempts) / count;
        m_LiveSummary.NegativeSpaceSuccessRatePercent = m_NegativeSpaceAttempts == 0u ? 0.0 :
            100.0 * static_cast<double>(m_NegativeSpaceSuccesses) / static_cast<double>(m_NegativeSpaceAttempts);
    }

    void DiagnosticsAppController::joinFinishedWorker()
    {
        bool shouldJoin = false;
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            shouldJoin = m_Worker.joinable() && m_State != DiagnosticsAppRunState::RUNNING && m_State != DiagnosticsAppRunState::CANCELLING;
        }
        if (shouldJoin)
        {
            m_Worker.join();
        }
    }
}
