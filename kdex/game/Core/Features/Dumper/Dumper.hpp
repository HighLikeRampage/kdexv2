#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Core::Features::Dumper {

struct VfsNode {
    std::string name;
    std::string fullPath;
    bool isDirectory = false;
    size_t size = 0;
    std::vector<uint8_t> content;
    std::vector<std::shared_ptr<VfsNode>> children;
};

enum class DumpState : int {
    Idle = 0,
    Detecting,
    FetchingConfig,
    Running,
    Done,
    Failed,
};

struct DumperStatus {
    DumpState state = DumpState::Idle;
    int totalResources = 0;
    int doneResources = 0;
    int failedResources = 0;
    std::string currentResource;
    std::string lastError;
    std::string serverUrl;
    uint64_t startedAt = 0;
    uint64_t finishedAt = 0;
};

class ResourceDumper {
public:
    void StartDumpAsync(std::string serverUrl);
    void Reset();
    void Cancel();
    void StartWatcher();
    void StopWatcher();

    DumperStatus GetStatus();
    std::shared_ptr<VfsNode> GetRoot();

    bool GetFileContent(const std::string &fullPath, std::vector<uint8_t> &out);

    bool IsRunning() const { return running_.load(); }

    static std::string DetectServerUrl();
    static bool LooksTextual(const std::vector<uint8_t> &content);
    static std::string GuessLanguage(const std::string &fileName);

    bool ExportZip(std::vector<uint8_t> &out);

private:
    std::mutex mutex_;
    std::shared_ptr<VfsNode> root_;
    DumperStatus status_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_{false};
    std::atomic<bool> watcher_started_{false};
    std::atomic<bool> watcher_stop_{false};
    std::string lastDumpedServer_;

    void RunAsync(std::string serverUrl);
    void RunWatcher();
};

extern ResourceDumper g_ResourceDumper;

}
