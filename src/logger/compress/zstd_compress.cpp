#include <cstring>
#include <string>

#include "logger/internal_log.hpp"
#include "logger/compress/zstd_compress.hpp"

namespace logger::compress {
    namespace {
        constexpr int kZstdCompressionLevel = 5;

        bool IsZSTDCompressed(const void* input, size_t input_size) {
            if (!input || input_size < 4) {
                return false;
            }

            const uint8_t kMagicNumberBigEndian[] = {0x28, 0xb5, 0x2f, 0xfd};
            const uint8_t kMagicNumberLittleEndian[] = {0xfd, 0x2f, 0xb5, 0x28};
            if (memcmp(input, kMagicNumberBigEndian, sizeof(kMagicNumberBigEndian)) == 0) {
                return true;
            }
            if (memcmp(input, kMagicNumberLittleEndian, sizeof(kMagicNumberLittleEndian)) == 0) {
                return true;
            }
            return false;
        }
    }  // namespace

    struct ZstdCompress::Imp {
        std::unique_ptr<ZSTD_CStream, ZStreamCompressDeleter> compress_stream_;
        std::unique_ptr<ZSTD_DStream, ZStreamDecompressDeleter> decompress_stream_;
    };

    ZstdCompress::ZstdCompress() : imp_(std::make_unique<Imp>()) {
        ResetStream_();
        ResetUncompressStream_();
    }
    ZstdCompress::~ZstdCompress() = default;

    size_t ZstdCompress::Compress(const void* input, size_t input_size, void* output, size_t output_size) {
        if (!input || !output || input_size == 0) {
            return 0;
        }

        ResetStream_();
        ZSTD_inBuffer in_buffer = {input, input_size, 0};
        ZSTD_outBuffer out_buffer = {output, output_size, 0};
        const size_t ret = ZSTD_compressStream2(imp_->compress_stream_.get(), &out_buffer, &in_buffer, ZSTD_e_end);
        if (ZSTD_isError(ret) != 0) {
            return 0;
        }
        return out_buffer.pos;
    }

    size_t ZstdCompress::CompressedBound(size_t input_size) {
        return ZSTD_compressBound(input_size);
    }

    std::string ZstdCompress::Decompress(const void* data, size_t size) {
        if (!data || size == 0) {
            return {};
        }

        if (!IsZSTDCompressed(data, size)) {
            throw std::runtime_error("Decompress error");
        }

        ResetUncompressStream_();
        ZSTD_inBuffer in_buffer = {data, size, 0};
        std::string output;
        output.reserve(10 * 1024);

        while (in_buffer.pos < in_buffer.size) {
            char buffer[4096] = {0};
            ZSTD_outBuffer out_buffer = {buffer, sizeof(buffer), 0};
            const size_t ret = ZSTD_decompressStream(imp_->decompress_stream_.get(), &out_buffer, &in_buffer);
            if (ZSTD_isError(ret) != 0) {
                return {};
            }
            output.append(buffer, out_buffer.pos);
            if (ret == 0) {
                break;
            }
        }

        return output;
    }

    void ZstdCompress::ResetStream_() {
        imp_->compress_stream_.reset(ZSTD_createCStream());
        const size_t ret = ZSTD_initCStream(imp_->compress_stream_.get(), kZstdCompressionLevel);
        if (ZSTD_isError(ret) != 0) {
            imp_->compress_stream_.reset();
        }
    }

    void ZstdCompress::ResetUncompressStream_() {
        imp_->decompress_stream_.reset(ZSTD_createDStream());
        const size_t ret = ZSTD_initDStream(imp_->decompress_stream_.get());
        if (ZSTD_isError(ret) != 0) {
            imp_->decompress_stream_.reset();
        }
    }
}  // namespace logger::compress
