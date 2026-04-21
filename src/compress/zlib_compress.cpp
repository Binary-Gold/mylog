#include "compress/zlib_compress.hpp"

namespace compress {
    struct ZlibCompress::Imp
    {
        std::unique_ptr<z_stream, ZStreamDeflateDeleter> compress_stream_;
    };

    ZlibCompress::~ZlibCompress() = default;

    
    
}