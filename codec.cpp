#include "arith.h"

const std::string usage("usage:  codec [option] infile outfile");
const std::string help1("--encode  encode infile to outfile");
const std::string help2("--decode  decode infile to outfile");

void print_help() {
    std::cout << usage << std::endl;
    std::cout << "options:" << std::endl;
    std::cout << help1 << std::endl;
    std::cout << help2 << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        print_help();
    } else if (strcmp(argv[1], "--encode") != 0 && strcmp(argv[1], "--decode") != 0) {
        print_help();
    } else {
        if (strcmp(argv[1], "--encode") == 0) {
            arith::CompressFile(argv[2], argv[3]);
        } else {
            arith::UncompressFile(argv[2], argv[3]);
        }
    }
}