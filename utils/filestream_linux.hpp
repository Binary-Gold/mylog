#ifndef FILESTREAM_LINUX_HPP
#define FILESTREAM_LINUX_HPP

#include <cstdint>
#include <filesystem>

namespace fs {
    class Fd{
    public:
        Fd();
        explicit Fd(const std::filesystem::path& path);
        ~Fd();

        void Open(const std::filesystem::path& path);
        int GetFd() const;
        uint64_t GetFileSize() const;

        Fd(const Fd&) = delete;
        Fd& operator=(const Fd&) = delete;
    private:
        int fd_ = -1;
    };
    
    class MMap {
    public:
        MMap();
        MMap(const Fd& fd, uint64_t size);
        ~MMap();

        void Request(const Fd& fd, uint64_t size);
        void* Data();
        const void* Data() const;
    private:
        char* mmap_ptr_{nullptr};
        uint64_t size_{0};
    };

    uint64_t GetPageSize();
}

#endif