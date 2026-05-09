#include <iostream>

#include "logger/internal_log.hpp"
#include "logger/compress/zlib_compress.hpp"

int main() {

    // LOG_INFO("aaa\n");
    logger::compress::ZlibCompress tmp;
    std::string s = "abcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabc";
    std::string ret;
    ret.resize(tmp.CompressedBound(s.size()));
    size_t real = tmp.Compress(s.data(), s.size(), ret.data(), ret.size());
    ret.resize(real);
    std::cout << s.size() << " " << real << std::endl;
    return 0;
}