#ifndef MMAP_HPP
#define MMAP_HPP

#include <cstdint>
#include <filesystem>
#include <memory>
#include <utility>

namespace logger::mmap {
struct MMapHeader {
    static constexpr uint64_t kMagic = 0x50414D4D;  // "MMAP" (little-endian)
    uint64_t magic_{kMagic};
    uint64_t data_size_{0};
};

class MMapAux {
public:
    using fpath = std::filesystem::path;

    explicit MMapAux(const fpath& file_path);
    ~MMapAux();

    // 使用逻辑空占位置
    void Resize(uint64_t new_size);
    // 通过MMapHeader获取文件逻辑大小
    uint64_t Size() const;
    // 获取MMapHeader后首地址
    uint8_t* Data() const;
    // 通过设置MMapHeader使文件被视为空
    void Clear();
    // 文件尾部追加数据
    void Push(const void* data, uint64_t size);
    bool Empty() const { return Size() == 0; }
    double Ratio() const;

    MMapAux(const MMapAux&) = delete;
    MMapAux& operator=(const MMapAux&) = delete;

private:
    // 每次插入判断是否扩容
    void Reserve_(uint64_t new_size);
    // 通过MMapHeader判断数据合法,进而决定逻辑文件大小
    void Init_();
    static uint64_t GetValidCapacity_(uint64_t size);
    uint8_t* Handle_() { return std::as_const(*this).Handle_(); }
    uint8_t* Handle_() const;

    MMapHeader* Header_() { return std::as_const(*this).Header_(); }
    MMapHeader* Header_() const;
    void Unmap_();
    void Trymap_(uint64_t valid_new_size);

    struct Imp;
    std::unique_ptr<Imp> imp_;
};
}  // namespace logger::mmap

#endif
