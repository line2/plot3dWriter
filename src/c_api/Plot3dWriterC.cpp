#ifndef PLOT3D_WRITER_DLL_EXPORTS
#define PLOT3D_WRITER_DLL_EXPORTS
#endif

#include "Plot3dWriterC.h"
#include "Plot3dWriterCore.h"

extern "C" {

PLOT3D_WRITER_API int plot3d_write_structured(
    const char* filename,
    const Plot3dStructuredBlock* block,
    const Plot3dWriteOptions* options) {
    
    plot3d_writer::StructuredBlock cppBlock;
    cppBlock.ni = block->ni;
    cppBlock.nj = block->nj;
    cppBlock.nk = block->nk;
    cppBlock.x = block->x;
    cppBlock.y = block->y;
    cppBlock.z = block->z;

    plot3d_writer::WriteOptions cppOptions;
    cppOptions.precision = (options->precision == 1) ? 
        plot3d_writer::WriteOptions::Precision::Float64 : 
        plot3d_writer::WriteOptions::Precision::Float32;
    cppOptions.useFortranFormat = (options->useFortranFormat != 0);

    return plot3d_writer::WriteStructured(filename, cppBlock, cppOptions);
}

PLOT3D_WRITER_API int plot3d_write_solution(
    const char* filename,
    const Plot3dStructuredBlock* block,
    const double* q_data,
    int num_vars,
    const Plot3dReferenceConditions* ref,
    const Plot3dWriteOptions* options) {

    plot3d_writer::StructuredBlock cppBlock;
    cppBlock.ni = block->ni;
    cppBlock.nj = block->nj;
    cppBlock.nk = block->nk;
    cppBlock.x = block->x;
    cppBlock.y = block->y;
    cppBlock.z = block->z;

    plot3d_writer::ReferenceConditions cppRef;
    cppRef.mach = ref->mach;
    cppRef.alpha = ref->alpha;
    cppRef.reynolds = ref->reynolds;
    cppRef.time = ref->time;

    plot3d_writer::WriteOptions cppOptions;
    cppOptions.precision = (options->precision == 1) ? 
        plot3d_writer::WriteOptions::Precision::Float64 : 
        plot3d_writer::WriteOptions::Precision::Float32;
    cppOptions.useFortranFormat = (options->useFortranFormat != 0);

    return plot3d_writer::WriteSolution(filename, cppBlock, q_data, num_vars, cppRef, cppOptions);
}

PLOT3D_WRITER_API const char* plot3d_get_last_error() {
    return plot3d_writer::GetLastError().c_str();
}

PLOT3D_WRITER_API const char* plot3d_writer_version() {
    return "1.0.0";
}

} // extern "C"
