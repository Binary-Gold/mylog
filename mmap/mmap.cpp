#include <algorithm>
#include <cassert>

#include <mmap/mmap.hpp>
#include <utils/filestream_linux.hpp>

namespace mmap {
    // 最小容量为 512KB
    static constexpr uint64_t kDefaultCapacity = 512 * 1024;

    struct MMapAux::Imp {
        // 容量
        uint64_t capacity_{0};
        // 当前字节数
        uint64_t size_{0};
        // 路径
        fpath filepath_{};
        // 文件描述符
        std::unique_ptr<fs::Fd> fd_ = std::make_unique<fs::Fd>();
        // mmap指针
        std::unique_ptr<fs::MMap> mmap_ = std::make_unique<fs::MMap>();
    };

    MMapAux::MMapAux(const fpath& file_path) : imp_(std::make_unique<Imp>()) {
        imp_->fd_->Open(file_path);
        Reserve_(std::max(kDefaultCapacity, imp_->fd_->GetFileSize()));
        Init_();
    }

    void MMapAux::Init_() {
        MMapHeader* header = Handle_();
        if (!header) {
            throw std::runtime_error("Handle_ failed");
        }
        if (header->magic_ != MMapHeader::kMagic) {
            header->magic_ = MMapHeader::kMagic;
            header->size_ = 0;
        }
    }

    uint64_t MMapAux::GetValidCapacity(uint64_t size) {
        uint64_t page_size = fs::GetPageSize();
        return (size + page_size - 1) / page_size * page_size;
    }

    void MMapAux::Reserve_(uint64_t new_size) {
        if (new_size <= imp_->capacity_) {
            return;
        }
        uint64_t valid_new_size = GetValidCapacity(new_size);
        Unmap_();
        Trymap_(valid_new_size);
        imp_->capacity_ = valid_new_size;
    }

    MMapHeader* MMapAux::Handle_() const {
        return static_cast<MMapHeader*>(imp_->mmap_->Data());
    }
    
    void MMapAux::Unmap_() {
        imp_->mmap_.reset(); 
    }

    void MMapAux::Trymap_(uint64_t valid_new_size ) {
        imp_->mmap_ = std::make_unique<fs::MMap>(*imp_->fd_, imp_->capacity_);
    }
} 

