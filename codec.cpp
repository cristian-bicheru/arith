#include "arith.h"
#include "huffman.h"

const std::string usage("usage:  codec [option] algorithm [option] infile outfile");
const std::string help0("--algorithm  specify codec algorithm (arith or huffman)");
const std::string help1("--encode  encode infile to outfile");
const std::string help2("--decode  decode infile to outfile");

void print_help() {
    std::cout << usage << std::endl;
    std::cout << "options:" << std::endl;
    std::cout << help0 << std::endl;
    std::cout << help1 << std::endl;
    std::cout << help2 << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        print_help();
    } else if (strcmp(argv[1], "--algorithm") != 0) {
        print_help();
    } else {
        if (strcmp(argv[3], "--encode") == 0) {
            if (strcmp(argv[2], "arith") == 0) {
                arith::CompressFile(std::string(argv[4]), std::string(argv[5]));
            } else if (strcmp(argv[2], "huffman") == 0) {
                huffman::CompressFile(std::string(argv[4]), std::string(argv[5]));
            } else {
                print_help();
            }
        } else {
            if (strcmp(argv[2], "arith") == 0) {
                arith::UncompressFile(std::string(argv[4]), std::string(argv[5]));
            } else if (strcmp(argv[2], "huffman") == 0) {
                huffman::UncompressFile(std::string(argv[4]), std::string(argv[5]));
            } else {
                print_help();
            }
        }
    }
    exit(0);
}