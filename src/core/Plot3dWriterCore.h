#ifndef PLOT3D_WRITER_CORE_H
#define PLOT3D_WRITER_CORE_H

#include "Plot3dWriterExport.h"
#include <string>

namespace plot3d_writer {

struct StructuredBlock {
    int ni, nj, nk;
    const double* x = nullptr;
    const double* y = nullptr;
    const double* z = nullptr;
};

struct WriteOptions {
    enum class Precision { Float32, Float64 } precision = Precision::Float32;
    bool useFortranFormat = false;
};

struct ReferenceConditions {
    float mach = 0.0f;
    float alpha = 0.0f;
    float reynolds = 0.0f;
    float time = 0.0f;
};

PLOT3D_WRITER_API int WriteStructured(
    const std::string& filename,
    const StructuredBlock& block,
    const WriteOptions& options = {});

PLOT3D_WRITER_API int WriteSolution(
    const std::string& filename,
    const StructuredBlock& block,
    const double* q_data,
    int num_vars,
    const ReferenceConditions& ref = {},
    const WriteOptions& options = {});

PLOT3D_WRITER_API const std::string& GetLastError();

} // namespace plot3d_writer

#endif // PLOT3D_WRITER_CORE_H
