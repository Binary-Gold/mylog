#ifndef FILESTREAM_LINUX_HPP
#define FILESTREAM_LINUX_HPP

#include <cstdint>
#include <filesystem>

namespace logger::fs {
class Fd {
public:
    explicit Fd(const std::filesystem::path& path);
    ~Fd();
    int GetFd() const;
    uint64_t GetFileSize() const;

    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;

private:
    int fd_ = -1;
};

class MMap {
public:
    MMap(const Fd& fd, uint64_t size);
    ~MMap();

    void* Data() const;

private:
    void* mmap_ptr_{nullptr};
    uint64_t size_{0};
};

uint64_t GetPageSize();
size_t GetFileSize(const std::filesystem::path& file_path);
}  // namespace logger::fs
#endif
