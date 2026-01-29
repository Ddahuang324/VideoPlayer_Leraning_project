#pragma once

#include "logger/LogInfo.h"
#include "common/Result.h"
#include "common/Error.h"
#include <fstream>
#include <filesystem>
#include <chrono>

namespace yibo {

class LogWriter {
public:
    explicit LogWriter(const std::filesystem::path& log_dir);
    ~LogWriter();

    Result<void, Error> Write(const LogInfo& info);
    Result<void, Error> Flush();
    
    // Get the current log file path (useful for testing)
    const std::filesystem::path& GetCurrentFile() const { return m_current_file; }


private:
    Result<void, Error> Rotate();
    Buffer FormatLog(const LogInfo& info);
    Buffer FormatDump(BufferView data);
    Buffer FormatTime(const std::chrono::system_clock::time_point& tp);

    std::filesystem::path m_log_dir;
    std::ofstream m_file;
    std::filesystem::path m_current_file;
    size_t m_current_size{0};
    std::chrono::system_clock::time_point m_last_rotation;
};

} // namespace yibo
