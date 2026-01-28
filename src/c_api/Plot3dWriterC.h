#ifndef PLOT3D_WRITER_C_H
#define PLOT3D_WRITER_C_H

#include "Plot3dWriterExport.h"

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
    const double* q_data,
    int num_vars,
    const Plot3dReferenceConditions* ref,
    const Plot3dWriteOptions* options);

PLOT3D_WRITER_API const char* plot3d_get_last_error();
PLOT3D_WRITER_API const char* plot3d_writer_version();

#ifdef __cplusplus
}
#endif

#endif // PLOT3D_WRITER_C_H
