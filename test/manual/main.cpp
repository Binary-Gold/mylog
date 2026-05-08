#include "logger/internal_log.hpp"
#include "logger/compress/zlib_compress.hpp"

int main() {

    // LOG_INFO("aaa\n");
    logger::compress::ZlibCompress tmp;
    std::string s = "aabbcccddddccadad";

    std::string ret;
    // tmp.compress(s, s.size(), ret, ret.size());
    return 0;
}