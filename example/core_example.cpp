#include "Plot3dWriterCore.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

static size_t idx3(int i, int j, int k, int ni, int nj) {
    return static_cast<size_t>(i + ni * (j + nj * k));
}

int main() {
    const int ni = 11, nj = 6, nk = 2;
    const size_t nPts = static_cast<size_t>(ni) * nj * nk;

    // 1) Prepare mesh data
    std::vector<double> x(nPts), y(nPts), z(nPts);
    for (int k = 0; k < nk; ++k) {
        for (int j = 0; j < nj; ++j) {
            for (int i = 0; i < ni; ++i) {
                const size_t id = idx3(i, j, k, ni, nj);
                const double xx = double(i) / double(ni - 1);
                const double yy = double(j) / double(nj - 1);
                const double zz = double(k);

                x[id] = xx;
                y[id] = yy;
                z[id] = zz + 0.02 * std::sin(2.0 * 3.141592653589793 * xx) * std::sin(2.0 * 3.141592653589793 * yy);
            }
        }
    }

    plot3d_writer::StructuredBlock block;
    block.ni = ni; block.nj = nj; block.nk = nk;
    block.x = x.data(); block.y = y.data(); block.z = z.data();

    plot3d_writer::WriteOptions opt;
    opt.precision = plot3d_writer::WriteOptions::Precision::Float32;
    opt.useFortranFormat = false;

    // 2) Write grid (.xyz)
    if (plot3d_writer::WriteStructured("demo_cpp.xyz", block, opt) != 0) {
        std::cerr << "WriteStructured failed: " << plot3d_writer::GetLastError() << "\n";
        return EXIT_FAILURE;
    }

    // 3) Prepare solution data: 5 variables (rho, rhou, rhov, rhow, E)
    const int numVars = 5;
    std::vector<double> q(static_cast<size_t>(numVars) * nPts, 0.0);

    const double gamma = 1.4;
    const double gm1 = gamma - 1.0;

    for (size_t id = 0; id < nPts; ++id) {
        const double rho = 1.0;
        const double u = 1.0, v = 0.2, w = 0.0;
        const double p = 1.0;

        // planar layout: q[var * nPts + id]
        q[0 * nPts + id] = rho;
        q[1 * nPts + id] = rho * u;
        q[2 * nPts + id] = rho * v;
        q[3 * nPts + id] = rho * w;
        q[4 * nPts + id] = p / gm1 + 0.5 * rho * (u*u + v*v + w*w);
    }

    plot3d_writer::ReferenceConditions ref;
    ref.mach = 0.5f;
    ref.alpha = 0.0f;
    ref.reynolds = 1e6f;
    ref.time = 0.0f;

    // 4) Write solution (.q)
    if (plot3d_writer::WriteSolution("demo_cpp.q", block, q.data(), numVars, ref, opt) != 0) {
        std::cerr << "WriteSolution failed: " << plot3d_writer::GetLastError() << "\n";
        return EXIT_FAILURE;
    }

    std::cout << "Successfully wrote demo_cpp.xyz and demo_cpp.q (C++ API)\n";
    return EXIT_SUCCESS;
}
