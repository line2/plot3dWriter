#ifndef PLOT3D_WRITER_CORE_H
#define PLOT3D_WRITER_CORE_H

#include "Plot3dWriterExport.h"
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

// C API
PLOT3D_WRITER_API int plot3d_write_structured(
    const char* filename,
    const Plot3dStructuredBlock* block,
    const Plot3dWriteOptions* options);

PLOT3D_WRITER_API int plot3d_write_solution(
    const char* filename,
    const Plot3dStructuredBlock* block,
    const double* q_data, // Interleaved or planar? Let's decide on planar for 5 variables: rho, rhou, rhov, rhow, E
    int num_vars,
    const Plot3dReferenceConditions* ref,
    const Plot3dWriteOptions* options);

PLOT3D_WRITER_API const char* plot3d_get_last_error();
PLOT3D_WRITER_API const char* plot3d_writer_version();

#ifdef __cplusplus
}
#endif

// C++ API
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

} // namespace plot3d_writer

#endif // PLOT3D_WRITER_CORE_H
