#pragma once 

#include <memory>

#include <zlib.h>

#include "compress/compress.hpp"

namespace compress {
    struct ZStreamDeflateDeleter {
        void operator()(z_stream* stream) {
            if (stream) {
                deflateEnd(stream);
                delete stream;
            }
        }
    };
    
    class ZlibCompress final : public Compression {
    public:
        ~ZlibCompress();

        size_t Compress(const void* input, size_t input_size, void* output, size_t output_size) override;
        size_t CompressedBound(size_t input_size) override;
        std::string Decompress(const void* data, size_t size) override;
        void ResetStream() override;
    private:
        struct Imp;
        std::unique_ptr<Imp> imp_;
    };
}