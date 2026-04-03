#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> 
#include <sys/mman.h>

#include <stdexcept>

#include <utils/filestream_linux.hpp>

namespace fs {
    Fd::Fd() {};
    Fd::Fd(const std::filesystem::path& path) {
        std::u8string u8str = path.u8string();
        const char* path_cstr = reinterpret_cast<const char*>(u8str.c_str());
        fd_ = ::open(path_cstr, O_RDWR | O_CREAT, 0644);
        if (fd_ == -1) {
            throw std::runtime_error("open failed");
        }
    }
    Fd::~Fd() {
        if (fd_ != -1) {
            ::close(fd_);
        }
    }

    void Fd::Open(const std::filesystem::path& path) {
        if (-1 != fd_) {
            throw std::runtime_error("double open");
        }
        std::u8string u8str = path.u8string();
        const char* path_cstr = reinterpret_cast<const char*>(u8str.c_str());
        fd_ = ::open(path_cstr, O_RDWR | O_CREAT, 0644);
        if (fd_ == -1) {
            throw std::runtime_error("open failed");
        }
    }
    int Fd::GetFd() const {
        if (-1 == fd_) {
            throw std::runtime_error("GetFd failed");
        }
        return fd_;
    }
    uint64_t Fd::GetFileSize() const{
        if (-1 == fd_) {
            throw std::runtime_error("GetFileSize failed");
        }
        struct stat sb;
        if (fstat(fd_, &sb) == -1) {
            throw std::runtime_error("fstat failed");
        }
        if (sb.st_size < 0) {
            throw std::runtime_error("sb.st_size failed");
        }
        return static_cast<uint64_t>(sb.st_size);
    }

    MMap::MMap() {};
    MMap::MMap(const Fd& fd, uint64_t size) {
        if (::ftruncate(fd.GetFd(), size) == -1) {
            throw std::runtime_error("ftruncate failed");
        }
        char *tmp_ptr_ = static_cast<char*>(::mmap(NULL, size,  PROT_READ | PROT_WRITE, MAP_SHARED, fd.GetFd(), 0));
        if (tmp_ptr_ == MAP_FAILED) {
            throw std::runtime_error("mmap failed"); 
        }
        size_ = size;
        mmap_ptr_ = tmp_ptr_;
    }
    MMap::~MMap() {
        if (mmap_ptr_) {
            if (::munmap(mmap_ptr_, size_) == -1) {
                //TODO: 日志记录/有序退出
            }
        }
    }

    void MMap::Request(const Fd& fd, uint64_t size) {
        if (mmap_ptr_) {
            throw std::runtime_error("double mmap");
        }
        if (::ftruncate(fd.GetFd(), size) == -1) {
            throw std::runtime_error("ftruncate failed");
        }
        char *tmp_ptr_ = static_cast<char*>(::mmap(NULL, size,  PROT_READ | PROT_WRITE, MAP_SHARED, fd.GetFd(), 0));
        if (tmp_ptr_ == MAP_FAILED) {
            throw std::runtime_error("mmap failed"); 
        }
        size_ = size;
        mmap_ptr_ = tmp_ptr_;
    }

    void* MMap::Data() const {
        if (!mmap_ptr_) {
            throw std::runtime_error("Data failed");
        }
        return  static_cast<void*>(mmap_ptr_);
    }

    uint64_t GetPageSize() {
        long ps = ::sysconf(_SC_PAGESIZE);
        if (ps == -1) {
            throw std::runtime_error("sysconf failed"); 
        }
        return static_cast<uint64_t>(ps);
    }
}