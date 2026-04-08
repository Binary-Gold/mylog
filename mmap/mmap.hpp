#ifndef MMAP_HPP
#define MMAP_HPP

#include <filesystem>
#include <memory>
#include <cstdint>
#include <utility>

namespace mmap {
    struct MMapHeader{
        static constexpr uint64_t kMagic = 0x4D4D4150; // "MMAP"
        uint64_t magic_ {kMagic};
        uint64_t data_size_{0};
    };

    class MMapAux {
    public:
        using fpath = std::filesystem::path;
        
        // 唯一入口
        explicit MMapAux(const fpath& file_path);
        // 析构
        ~MMapAux();

        // 设置大小
        void Resize(uint64_t new_size);
        // 获取大小
        uint64_t Size() const;
        // 获取数据指针
        uint8_t* Data();
        // 清空
        void Clear();
        // 追加
        void Push(const void* data, uint64_t size);
        // 判空
        bool Empty() const { return Size() == 0; }
        // 获取占用/空闲比例
        double Ratio() const;

        // 删除拷贝
        MMapAux(const MMapAux&) = delete;
        // 删除赋值
        MMapAux& operator=(const MMapAux&) = delete;
    private:
        // 申请内存
        void Reserve_(uint64_t new_size);
        // 初始化
        void Init_();
        // 向上取整倍数page字节数
        static uint64_t GetValidCapacity_(uint64_t size);
        // 获取整个数据段指针
        uint8_t* Handle_();
        // 获取头部信息指针
        MMapHeader* Header_() { return std::as_const(*this).Header_(); }
        MMapHeader* Header_() const;
        // 释放mamp
        void Unmap_();
        // 连接mmap,若未释放会先释放再赋值
        void Trymap_(uint64_t valid_new_size );
        
        struct Imp;
        std::unique_ptr<Imp> imp_;
    };
}

#endif