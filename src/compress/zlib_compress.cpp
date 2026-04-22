#include <zlib.h>
#include <string>

#include "compress/zlib_compress.hpp"

// todo 研究一下
namespace compression {
    struct ZlibCompress::Imp
    {
        std::unique_ptr<z_stream, ZStreamDeflateDeleter> compress_stream_;
        std::unique_ptr<z_stream, ZStreamInflateDeleter> decompress_stream_;
    };

    ZlibCompress::~ZlibCompress() = default;

    size_t ZlibCompress::Compress(const void* input, size_t input_size, void* output, size_t output_size) {
        if (!input || !output) {
            return 0;
        }
        if (!imp_) {
            imp_ = std::make_unique<Imp>();
        }
        if (!imp_->compress_stream_) {
            return 0;
        }
        imp_->compress_stream_->next_in = (Bytef*)input;
        imp_->compress_stream_->avail_in = static_cast<uInt>(input_size);

        imp_->compress_stream_->next_out = (Bytef*)output;
        imp_->compress_stream_->avail_out = static_cast<uInt>(output_size);

        int ret = Z_OK;
        do {
            ret = deflate(imp_->compress_stream_.get(), Z_SYNC_FLUSH);
            if (ret != Z_OK && ret != Z_BUF_ERROR && ret != Z_STREAM_END) {
                return 0;
            }
        } while (ret == Z_BUF_ERROR);
        size_t out_len = output_size - imp_->compress_stream_->avail_out;
        return out_len;
    }
    size_t ZlibCompress::CompressedBound(size_t input_size) {
        return input_size + 10;
    }
    std::string ZlibCompress::Decompress(const void* data, size_t size) {
        if (!data || size == 0) {
            return {};
        }

        if (!imp_) {
            imp_ = std::make_unique<Imp>();
        }
        if (!imp_->decompress_stream_) {
            ResetUncompressStream_();
        }
        if (!imp_->decompress_stream_) {
            return {};
        }

        std::string output;
        imp_->decompress_stream_->next_in = reinterpret_cast<Bytef*>(const_cast<void*>(data));
        imp_->decompress_stream_->avail_in = static_cast<uInt>(size);

        while (imp_->decompress_stream_->avail_in > 0) {
            char buffer[4096] = {0};
            imp_->decompress_stream_->next_out = reinterpret_cast<Bytef*>(buffer);
            imp_->decompress_stream_->avail_out = sizeof(buffer);
            int ret = inflate(imp_->decompress_stream_.get(), Z_SYNC_FLUSH);
            if (ret != Z_OK && ret != Z_STREAM_END) {
                return {};
            }
            output.append(buffer, sizeof(buffer) - imp_->decompress_stream_->avail_out);
        }

        return output;
    }
    void ZlibCompress::ResetStream() {
        if (!imp_) {
            imp_ = std::make_unique<Imp>();
        }

        imp_->compress_stream_ = std::unique_ptr<z_stream, ZStreamDeflateDeleter>(new z_stream());
        imp_->compress_stream_->zalloc = Z_NULL;
        imp_->compress_stream_->zfree = Z_NULL;
        imp_->compress_stream_->opaque = Z_NULL;

        int32_t ret = deflateInit2(imp_->compress_stream_.get(),
                                   Z_BEST_COMPRESSION,
                                   Z_DEFLATED,
                                   MAX_WBITS,
                                   MAX_MEM_LEVEL,
                                   Z_DEFAULT_STRATEGY);
        if (ret != Z_OK) {
            imp_->compress_stream_.reset();
        }
    }

    void ZlibCompress::ResetUncompressStream_() {
        if (!imp_) {
            imp_ = std::make_unique<Imp>();
        }

        imp_->decompress_stream_ = std::unique_ptr<z_stream, ZStreamInflateDeleter>(new z_stream());
        imp_->decompress_stream_->zalloc = Z_NULL;
        imp_->decompress_stream_->zfree = Z_NULL;
        imp_->decompress_stream_->opaque = Z_NULL;
        imp_->decompress_stream_->avail_in = 0;
        imp_->decompress_stream_->next_in = Z_NULL;

        int32_t ret = inflateInit2(imp_->decompress_stream_.get(), MAX_WBITS);
        if (ret != Z_OK) {
            imp_->decompress_stream_.reset();
        }
    }
    
    
}