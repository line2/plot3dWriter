#include "Plot3dWriterCore.h"
#include <vector>
#include <iostream>

int main() {
    // Block 1: 2x2x2
    std::vector<double> x1 = {0, 1, 0, 1, 0, 1, 0, 1};
    std::vector<double> y1 = {0, 0, 1, 1, 0, 0, 1, 1};
    std::vector<double> z1 = {0, 0, 0, 0, 1, 1, 1, 1};
    
    plot3d_writer::StructuredBlock b1;
    b1.ni = 2; b1.nj = 2; b1.nk = 2;
    b1.x = x1.data(); b1.y = y1.data(); b1.z = z1.data();

    // Block 2: 3x2x1
    std::vector<double> x2 = {2, 3, 4, 2, 3, 4};
    std::vector<double> y2 = {0, 0, 0, 1, 1, 1};
    std::vector<double> z2 = {0, 0, 0, 0, 0, 0};

    plot3d_writer::StructuredBlock b2;
    b2.ni = 3; b2.nj = 2; b2.nk = 1;
    b2.x = x2.data(); b2.y = y2.data(); b2.z = z2.data();

    plot3d_writer::StructuredBlock blocks[2] = {b1, b2};
    
    plot3d_writer::WriteOptions options;
    options.precision = plot3d_writer::WriteOptions::Precision::Float64;
    options.useFortranFormat = true;

    int ret = plot3d_writer::WriteStructuredMulti("multi_block_test.xyz", blocks, 2, options);
    
    if (ret == 0) {
        std::cout << "Successfully wrote multi-block Plot3D file: multi_block_test.xyz" << std::endl;
    } else {
        std::cerr << "Failed to write: " << plot3d_writer::GetLastError() << std::endl;
    }

    return ret;
}
