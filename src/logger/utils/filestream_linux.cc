#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdexcept>

#include "logger/utils/filestream_linux.hpp"

namespace logger::fs {
Fd::Fd(const std::filesystem::path& path) {
    std::string path_str = path.string();
    fd_ = ::open(path_str.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd_ == -1) {
        throw std::runtime_error("open failed");
    }
}
Fd::~Fd() {
    ::close(fd_);
}

int Fd::GetFd() const {
    return fd_;
}
uint64_t Fd::GetFileSize() const {
    struct stat sb;
    if (fstat(fd_, &sb) == -1) {
        throw std::runtime_error("fstat failed");
    }
    if (sb.st_size < 0) {
        throw std::runtime_error("sb.st_size failed");
    }
    return static_cast<uint64_t>(sb.st_size);
}

MMap::MMap(const Fd& fd, uint64_t size) {
    if (::ftruncate(fd.GetFd(), size) == -1) {
        throw std::runtime_error("ftruncate failed");
    }
    void* tmp_ptr_ = ::mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd.GetFd(), 0);
    if (tmp_ptr_ == MAP_FAILED) {
        throw std::runtime_error("mmap failed");
    }
    size_ = size;
    mmap_ptr_ = tmp_ptr_;
}
MMap::~MMap() {
    if (mmap_ptr_) {
        if (::munmap(mmap_ptr_, size_) == -1) {
            // TODO 极端情况可能失败,暂不考虑
        }
    }
}

void* MMap::Data() const {
    if (!mmap_ptr_) {
        throw std::runtime_error("Data failed");
    }
    return mmap_ptr_;
}

uint64_t GetPageSize() {
    long ps = ::sysconf(_SC_PAGESIZE);
    if (ps == -1) {
        throw std::runtime_error("sysconf failed");
    }
    return static_cast<uint64_t>(ps);
}
}  // namespace logger::fs
