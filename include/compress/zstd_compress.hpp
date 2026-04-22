#include <zstd.h>

#include "compress/compress.hpp"

namespace compression {
    struct ZStreamCompressDeleter {
        void operator()(ZSTD_CStream* stream) {
            if (stream) {
                ZSTD_freeCStream(stream);
            }
        }
    };
    
    struct ZStreamDecompressDeleter {
        void operator()(ZSTD_DStream* stream) {
            if (stream) {
                ZSTD_freeDStream(stream);
            }
        }
    };
    
    class ZstdCompress final : public Compression {
    public:
        ~ZstdCompress();

        size_t Compress(const void* input, size_t input_size, void* output, size_t output_size) override;
        size_t CompressedBound(size_t input_size) override;
        std::string Decompress(const void* data, size_t size) override;
        void ResetStream() override;
    private:
        void ResetUncompressStream_();
        
        struct Imp;
        std::unique_ptr<Imp> imp_;
    }
}