#include <iostream>

#include "logger/mmap/mmap.hpp"

int main() {
    std::filesystem::path p("mmap_test.txt");
    logger::mmap::MMapAux mmap_aux(p);

    std::string line{"hello "};
    mmap_aux.Push(line.c_str(), line.size());
    std::cout << mmap_aux.Size() << std::endl;

    mmap_aux.Clear();
    uint8_t* ptr = mmap_aux.Data();
    std::cout << *ptr;

    return 0;
}