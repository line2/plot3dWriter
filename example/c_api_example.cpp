#include "Plot3dWriterC.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static size_t idx3(int i, int j, int k, int ni, int nj) {
    return (size_t)(i + ni * (j + nj * k));
}

int main(void) {
    const int ni = 11, nj = 6, nk = 2;
    const size_t nPts = (size_t)ni * (size_t)nj * (size_t)nk;

    // 1) Prepare mesh data
    double* x = (double*)malloc(nPts * sizeof(double));
    double* y = (double*)malloc(nPts * sizeof(double));
    double* z = (double*)malloc(nPts * sizeof(double));
    if (!x || !y || !z) {
        fprintf(stderr, "malloc failed\n");
        return EXIT_FAILURE;
    }

    for (int k = 0; k < nk; ++k) {
        for (int j = 0; j < nj; ++j) {
            for (int i = 0; i < ni; ++i) {
                const size_t id = idx3(i, j, k, ni, nj);
                const double xx = (double)i / (double)(ni - 1);
                const double yy = (double)j / (double)(nj - 1);
                const double zz = (double)k;

                x[id] = xx;
                y[id] = yy;
                z[id] = zz + 0.02 * sin(2.0 * 3.141592653589793 * xx) * sin(2.0 * 3.141592653589793 * yy);
            }
        }
    }

    Plot3dStructuredBlock block;
    block.ni = ni; block.nj = nj; block.nk = nk;
    block.x = x; block.y = y; block.z = z;

    Plot3dWriteOptions opt;
    opt.precision = 0;          /* 0: float32, 1: float64 */
    opt.useFortranFormat = 0;   /* 0: raw binary, 1: fortran unformatted */

    // 2) Write grid (.xyz)
    if (plot3d_write_structured("demo_c.xyz", &block, &opt) != 0) {
        fprintf(stderr, "plot3d_write_structured failed: %s\n", plot3d_get_last_error());
        free(x); free(y); free(z);
        return EXIT_FAILURE;
    }

    // 3) Prepare solution data: 5 variables (rho, rhou, rhov, rhow, E)
    const int numVars = 5;
    double* q = (double*)calloc((size_t)numVars * nPts, sizeof(double));
    if (!q) {
        fprintf(stderr, "calloc failed\n");
        free(x); free(y); free(z);
        return EXIT_FAILURE;
    }

    const double gamma = 1.4;
    const double gm1 = gamma - 1.0;

    for (size_t id = 0; id < nPts; ++id) {
        const double rho = 1.0;
        const double u = 1.0, v = 0.2, w = 0.0;
        const double p = 1.0;

        /* planar layout: q[var * nPts + id] */
        q[0 * nPts + id] = rho;
        q[1 * nPts + id] = rho * u;
        q[2 * nPts + id] = rho * v;
        q[3 * nPts + id] = rho * w;
        q[4 * nPts + id] = p / gm1 + 0.5 * rho * (u * u + v * v + w * w);
    }

    Plot3dReferenceConditions ref;
    ref.mach = 0.5f;
    ref.alpha = 0.0f;
    ref.reynolds = 1e6f;
    ref.time = 0.0f;

    // 4) Write solution (.q)
    if (plot3d_write_solution("demo_c.q", &block, q, numVars, &ref, &opt) != 0) {
        fprintf(stderr, "plot3d_write_solution failed: %s\n", plot3d_get_last_error());
        free(q); free(x); free(y); free(z);
        return EXIT_FAILURE;
    }

    free(q);
    free(x); free(y); free(z);

    printf("Successfully wrote demo_c.xyz and demo_c.q (C API)\n");
    return EXIT_SUCCESS;
}
