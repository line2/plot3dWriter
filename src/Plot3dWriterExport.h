#ifndef PLOT3D_WRITER_EXPORT_H
#define PLOT3D_WRITER_EXPORT_H

#if defined(_WIN32) || defined(__CYGWIN__)
  #ifdef PLOT3D_WRITER_DLL_EXPORTS
    #define PLOT3D_WRITER_API __declspec(dllexport)
  #else
    #define PLOT3D_WRITER_API __declspec(dllimport)
  #endif
#else
  #if __GNUC__ >= 4
    #define PLOT3D_WRITER_API __attribute__ ((visibility ("default")))
  #else
    #define PLOT3D_WRITER_API
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int ni, nj, nk;
    const double* x;
    const double* y;
    const double* z;
} Plot3dStructuredBlock;

typedef struct {
    float mach;
    float alpha;
    float reynolds;
    float time;
} Plot3dReferenceConditions;

typedef struct {
    int precision; // 0: float, 1: double
    int useFortranFormat; // 0: C-Binary, 1: Fortran unformatted
} Plot3dWriteOptions;

#ifdef __cplusplus
}
#endif

#endif // PLOT3D_WRITER_EXPORT_H
