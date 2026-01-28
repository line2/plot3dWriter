#include "Plot3dWriter.h"
#include <vtkStructuredGrid.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkPoints.h>
#include <vtkCompositeDataSet.h>
#include <vtkCompositeDataIterator.h>
#include <vtkSmartPointer.h>
#include <stdexcept>
#include <vector>

void Plot3dWriter::Write(vtkDataObject* input, const std::string& fileName, const Plot3dWriterOptions& opt) {
    WriteGrid(input, fileName + ".xyz", opt);
    WriteSolution(input, fileName + ".q", opt);
}

static vtkStructuredGrid* GetFirstStructuredGrid(vtkDataObject* input) {
    if (auto sg = vtkStructuredGrid::SafeDownCast(input)) {
        return sg;
    }
    if (auto composite = vtkCompositeDataSet::SafeDownCast(input)) {
        auto it = vtkSmartPointer<vtkCompositeDataIterator>::Take(composite->NewIterator());
        for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem()) {
            if (auto sg = vtkStructuredGrid::SafeDownCast(it->GetCurrentDataObject())) {
                return sg;
            }
        }
    }
    return nullptr;
}

void Plot3dWriter::WriteGrid(vtkDataObject* input, const std::string& fileName, const Plot3dWriterOptions& opt) {
    auto sg = GetFirstStructuredGrid(input);
    if (!sg) throw std::runtime_error("No vtkStructuredGrid found in input.");

    int dims[3];
    sg->GetDimensions(dims);
    size_t nPts = (size_t)dims[0] * dims[1] * dims[2];

    std::vector<double> x(nPts), y(nPts), z(nPts);
    for (size_t i = 0; i < nPts; ++i) {
        double p[3];
        sg->GetPoint(static_cast<vtkIdType>(i), p);
        x[i] = p[0];
        y[i] = p[1];
        z[i] = p[2];
    }

    plot3d_writer::StructuredBlock block;
    block.ni = dims[0];
    block.nj = dims[1];
    block.nk = dims[2];
    block.x = x.data();
    block.y = y.data();
    block.z = z.data();

    if (plot3d_writer::WriteStructured(fileName, block, opt.coreOptions) != 0) {
        throw std::runtime_error(::plot3d_get_last_error());
    }
}

void Plot3dWriter::WriteSolution(vtkDataObject* input, const std::string& fileName, const Plot3dWriterOptions& opt) {
    auto sg = GetFirstStructuredGrid(input);
    if (!sg) throw std::runtime_error("No vtkStructuredGrid found in input.");

    int dims[3];
    sg->GetDimensions(dims);
    size_t nPts = (size_t)dims[0] * dims[1] * dims[2];

    auto pd = sg->GetPointData();
    auto rhoArr = pd->GetArray(opt.densityArrayName.c_str());
    auto velArr = pd->GetArray(opt.velocityArrayName.c_str());
    auto pArr = pd->GetArray(opt.pressureArrayName.c_str());

    if (!rhoArr || !velArr || !pArr) {
        throw std::runtime_error("Missing required arrays for solution file.");
    }

    std::vector<double> q(nPts * 5);
    double gm1 = opt.gamma - 1.0;

    for (size_t i = 0; i < nPts; ++i) {
        double rho = rhoArr->GetComponent(static_cast<vtkIdType>(i), 0);
        double u = velArr->GetComponent(static_cast<vtkIdType>(i), 0);
        double v = velArr->GetComponent(static_cast<vtkIdType>(i), 1);
        double w = (velArr->GetNumberOfComponents() >= 3) ? velArr->GetComponent(static_cast<vtkIdType>(i), 2) : 0.0;
        double p = pArr->GetComponent(static_cast<vtkIdType>(i), 0);

        q[0 * nPts + i] = rho;
        q[1 * nPts + i] = rho * u;
        q[2 * nPts + i] = rho * v;
        q[3 * nPts + i] = rho * w;
        q[4 * nPts + i] = p / gm1 + 0.5 * rho * (u * u + v * v + w * w);
    }

    plot3d_writer::StructuredBlock block;
    block.ni = dims[0];
    block.nj = dims[1];
    block.nk = dims[2];
    // x, y, z are not needed for solution writing in my current Core API, 
    // but the API takes the block for dimensions.
    
    if (plot3d_writer::WriteSolution(fileName, block, q.data(), 5, opt.refConditions, opt.coreOptions) != 0) {
        throw std::runtime_error(::plot3d_get_last_error());
    }
}
