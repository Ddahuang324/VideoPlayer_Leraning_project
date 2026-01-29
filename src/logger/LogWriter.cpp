#include "logger/LogWriter.h"
#include <iomanip>
#include <sstream>
#include <ctime>

namespace yibo {

LogWriter::LogWriter(const std::filesystem::path& log_dir)
    : m_log_dir(log_dir), m_last_rotation(std::chrono::system_clock::now()) {

    std::filesystem::create_directories(log_dir);
    (void)Rotate();
}

LogWriter::~LogWriter() {
    if (m_file.is_open()) {
        m_file.close();
    }
}

Result<void, Error> LogWriter::Write(const LogInfo& info) {
    if (!m_file.is_open()) {
        return Result<void, Error>::Err(
            Error(ErrorCode::FileWriteError, "Log file not open"));
    }

    Buffer line = FormatLog(info);
    m_file << line;
    m_current_size += line.size();

    if (info.level == LogLevel::FATAL) {
        m_file.flush();
    }

    // Check rotation (100MB or 24 hours)
    if (m_current_size > 100 * 1024 * 1024 ||
        (std::chrono::system_clock::now() - m_last_rotation) > std::chrono::hours(24)) {
        (void)Rotate();
    }

    return Result<void, Error>::Ok();
}

Result<void, Error> LogWriter::Flush() {
    if (m_file.is_open()) {
        m_file.flush();
    }
    return Result<void, Error>::Ok();
}

Result<void, Error> LogWriter::Rotate() {
    if (m_file.is_open()) {
        m_file.close();
    }

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    localtime_r(&time_t_now, &tm_now);

    std::ostringstream filename;
    filename << "server_"
             << std::put_time(&tm_now, "%Y%m%d_%H%M%S")
             << ".log";

    m_current_file = m_log_dir / filename.str();
    m_file.open(m_current_file, std::ios::app);

    if (!m_file.is_open()) {
        return Result<void, Error>::Err(
            Error(ErrorCode::FileWriteError, "Failed to open log file"));
    }

    m_current_size = 0;
    m_last_rotation = now;

    return Result<void, Error>::Ok();
}

Buffer LogWriter::FormatLog(const LogInfo& info) {
    std::ostringstream oss;

    oss << "[" << FormatTime(info.timestamp) << "] "
        << "[" << LevelToString(info.level) << "] "
        << "[" << info.file << ":" << info.line << "] "
        << info.content << "\n";

    if (info.dump_data.HasValue()) {
        oss << FormatDump(info.dump_data.Value());
    }

    return oss.str();
}

Buffer LogWriter::FormatDump(BufferView data) {
    std::ostringstream oss;

    for (size_t i = 0; i < data.size(); i += 16) {
        oss << std::setfill('0') << std::setw(8) << std::hex << i << "  ";

        for (size_t j = 0; j < 16; ++j) {
            if (i + j < data.size()) {
                oss << std::setw(2) << std::hex
                    << static_cast<unsigned>(static_cast<uint8_t>(data[i + j])) << " ";
            } else {
                oss << "   ";
            }
            if (j == 7) oss << " ";
        }

        oss << " |";
        for (size_t j = 0; j < 16 && i + j < data.size(); ++j) {
            char c = data[i + j];
            oss << (c >= 32 && c <= 126 ? c : '.');
        }
        oss << "|\n";
    }

    return oss.str();
}

Buffer LogWriter::FormatTime(const std::chrono::system_clock::time_point& tp) {
    auto time_t_val = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_val;
    localtime_r(&time_t_val, &tm_val);

    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

} // namespace yibo
