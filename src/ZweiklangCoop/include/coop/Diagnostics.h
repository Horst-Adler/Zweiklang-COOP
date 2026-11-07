#pragma once
#include <string>
#include <mutex>
#include <cstdio>
namespace zweiklang {
class Diagnostics {
public:
    ~Diagnostics();
    void Open(const std::string& directory, unsigned int maxBytes = 8 * 1024 * 1024);
    void Configure(int level, const std::string& name);
    void Role(const std::string& role);
    void Write(int level, const std::string& event, const std::string& detail);
    void Flush();
    std::string Path() const { return path_; }
    static std::string Escape(const std::string& text);
private:
    void Rotate();
    std::mutex mutex_;
    FILE* file_ = nullptr;
    std::string path_, name_, role_ = "OFFLINE";
    int level_ = 1;
    unsigned int maxBytes_ = 8 * 1024 * 1024;
    unsigned long long lastFlush_ = 0;
    unsigned long long sequence_ = 0;
};
}
