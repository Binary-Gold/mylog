#include <algorithm>
#include <cstring>
#include <stdexcept>

#include <mmap/mmap.hpp>
#include <utils/filestream_linux.hpp>


namespace mmap {
    // 最小容量为 512KB
    static constexpr uint64_t kDefaultCapacity = 512 * 1024;

    struct MMapAux::Imp {
        // 容量
        uint64_t capacity_{0};
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

    MMapAux::~MMapAux() = default;

    uint64_t MMapAux::Size() const {
        return Header_()->data_size_;
    }
    
    void MMapAux::Resize(uint64_t new_size) {
        Reserve_(sizeof(MMapHeader) + new_size);
        Header_()->data_size_ = new_size;
    }

    uint8_t* MMapAux::Data() {
        return Handle_() + sizeof(MMapHeader);
    }

    void MMapAux::Clear() {
        Header_()->data_size_ = 0;
    }

    void MMapAux::Push(const void* data, uint64_t size) {
        Reserve_(sizeof(MMapHeader) + Size() + size);
        memcpy(Data() + Size(), data, size);
        Header_()->data_size_ = Size() + size;
    }

    double MMapAux::Ratio() const {
        if (imp_->capacity_ - sizeof(MMapHeader) == 0) {
            throw std::runtime_error("Ratio 0 error");
        }
        return static_cast<double>(Size()) / (imp_->capacity_ - sizeof(MMapHeader));
    }

    void MMapAux::Init_() {
        MMapHeader* header = static_cast<MMapHeader*>(Header_());
        if (header->magic_ != MMapHeader::kMagic) {
            header->magic_ = MMapHeader::kMagic;
            header->data_size_ = 0;
        }
    }

    uint64_t MMapAux::GetValidCapacity_(uint64_t size) {
        uint64_t page_size = fs::GetPageSize();
        return (size + page_size - 1) / page_size * page_size;
    }

    void MMapAux::Reserve_(uint64_t new_size) {
        if (new_size <= imp_->capacity_) {
            return;
        }
        uint64_t valid_new_size = GetValidCapacity_(new_size);
        Unmap_();
        Trymap_(valid_new_size);
        imp_->capacity_ = valid_new_size;
    }

    uint8_t* MMapAux::Handle_() {
        return static_cast<uint8_t*>(imp_->mmap_->Data());
    }

    MMapHeader* MMapAux::Header_() const {
        MMapHeader* header = static_cast<MMapHeader*>(imp_->mmap_->Data());
        if (header->magic_ != MMapHeader::kMagic) {
            throw std::runtime_error("Header_() failed");
        }
        return header;
    }

    void MMapAux::Unmap_() {
        imp_->mmap_.reset(); 
    }

    void MMapAux::Trymap_(uint64_t valid_new_size ) {
        imp_->mmap_ = std::make_unique<fs::MMap>(*imp_->fd_, valid_new_size);
    }

} 

