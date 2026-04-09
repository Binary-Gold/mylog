#ifndef MMAP_HPP
#define MMAP_HPP

#include <cstdint>
#include <filesystem>
#include <memory>
#include <utility>

namespace mmap {
struct MMapHeader {
    static constexpr uint64_t kMagic = 0x4D4D4150;  // "MMAP"
    uint64_t magic_{kMagic};
    uint64_t data_size_{0};
};

class MMapAux {
public:
    using fpath = std::filesystem::path;

    explicit MMapAux(const fpath& file_path);
    ~MMapAux();

    void Resize(uint64_t new_size);
    uint64_t Size() const;
    uint8_t* Data();
    void Clear();
    void Push(const void* data, uint64_t size);
    bool Empty() const { return Size() == 0; }
    double Ratio() const;

    MMapAux(const MMapAux&) = delete;
    MMapAux& operator=(const MMapAux&) = delete;

private:
    void Reserve_(uint64_t new_size);
    void Init_();
    static uint64_t GetValidCapacity_(uint64_t size);
    uint8_t* Handle_();
    MMapHeader* Header_() { return std::as_const(*this).Header_(); }
    MMapHeader* Header_() const;
    void Unmap_();
    void Trymap_(uint64_t valid_new_size);

    struct Imp;
    std::unique_ptr<Imp> imp_;
};
}  // namespace mmap

#endif
