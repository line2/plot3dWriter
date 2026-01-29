#include "Plot3dWriterC.h"
#include <vtkSmartPointer.h>
#include <vtkXMLStructuredGridReader.h>
#include <vtkStructuredGridReader.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkCGNSReader.h>
#include <vtkStructuredGrid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkPoints.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkDataSet.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

static vtkSmartPointer<vtkDataObject> ReadFile(const std::string& filename) {
    std::string ext = fs::path(filename).extension().string();
    for (auto& c : ext) c = std::tolower(c);

    if (ext == ".vts") {
        auto reader = vtkSmartPointer<vtkXMLStructuredGridReader>::New();
        reader->SetFileName(filename.c_str());
        reader->Update();
        return reader->GetOutput();
    } else if (ext == ".vtk") {
        auto reader = vtkSmartPointer<vtkStructuredGridReader>::New();
        reader->SetFileName(filename.c_str());
        reader->Update();
        return reader->GetOutput();
    } else if (ext == ".vtu") {
        auto reader = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
        reader->SetFileName(filename.c_str());
        reader->Update();
        return reader->GetOutput();
    } else if (ext == ".cgns") {
        auto reader = vtkSmartPointer<vtkCGNSReader>::New();
        reader->SetFileName(filename.c_str());
        reader->Update();
        return reader->GetOutput();
    }

    return nullptr;
}

static vtkStructuredGrid* ExtractStructuredGrid(vtkDataObject* obj) {
    if (!obj) return nullptr;

    if (auto sg = vtkStructuredGrid::SafeDownCast(obj)) {
        return sg;
    }

    if (auto mb = vtkMultiBlockDataSet::SafeDownCast(obj)) {
        for (unsigned int i = 0; i < mb->GetNumberOfBlocks(); ++i) {
            auto block = mb->GetBlock(i);
            if (auto sg = vtkStructuredGrid::SafeDownCast(block)) {
                return sg;
            }
        }
    }

    return nullptr;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: file_reader_c_api_example <input_mesh_file>\n";
        std::cout << "Supported formats: .vts, .vtk (Structured), .vtu, .cgns\n";
        return EXIT_FAILURE;
    }

    std::string inputFile = argv[1];
    std::cout << "Reading file: " << inputFile << "..." << std::endl;

    auto dataObj = ReadFile(inputFile);
    if (!dataObj) {
        std::cerr << "Error: Could not read file or unsupported format: " << inputFile << std::endl;
        return EXIT_FAILURE;
    }

    // Check if it's a structured grid
    vtkStructuredGrid* sg = ExtractStructuredGrid(dataObj);
    if (!sg) {
        if (vtkUnstructuredGrid::SafeDownCast(dataObj)) {
            std::cerr << "Error: Input file contains an Unstructured Grid (.vtu or similar)." << std::endl;
            std::cerr << "Plot3D only supports Structured Grids. Please provide a Structured Grid or resample your data." << std::endl;
        } else {
            std::cerr << "Error: Input file does not contain a supported Structured Grid." << std::endl;
        }
        return EXIT_FAILURE;
    }

    int dims[3];
    sg->GetDimensions(dims);
    std::cout << "Structured Grid dimensions: " << dims[0] << " x " << dims[1] << " x " << dims[2] << std::endl;

    size_t nPts = (size_t)dims[0] * dims[1] * dims[2];
    std::vector<double> x(nPts), y(nPts), z(nPts);

    for (size_t i = 0; i < nPts; ++i) {
        double p[3];
        sg->GetPoint(static_cast<vtkIdType>(i), p);
        x[i] = p[0];
        y[i] = p[1];
        z[i] = p[2];
    }

    // 1) Prepare the C-style block structure
    Plot3dStructuredBlock block;
    block.ni = dims[0];
    block.nj = dims[1];
    block.nk = dims[2];
    block.x = x.data();
    block.y = y.data();
    block.z = z.data();

    Plot3dWriteOptions opt;
    opt.precision = 1;          // 0: float32, 1: float64
    opt.useFortranFormat = 0;   // 0: raw binary, 1: fortran unformatted

    // 2) Write grid (.xyz) using C API
    std::string outputGrid = "converted_grid.xyz";
    if (plot3d_write_structured(outputGrid.c_str(), &block, &opt) != 0) {
        std::cerr << "Error: plot3d_write_structured failed: " << plot3d_get_last_error() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Successfully wrote " << outputGrid << " (C API)" << std::endl;

    // 3) Optionally write solution if arrays exist
    auto pd = sg->GetPointData();
    if (pd->GetNumberOfArrays() > 0) {
        // For demonstration, we'll try to find some common arrays or just use the first one as Density
        // In a real application, you'd map VTK arrays to Plot3D variables (rho, rhou, rhov, rhow, E)
        std::cout << "Found " << pd->GetNumberOfArrays() << " point data arrays. Solution writing could be implemented here." << std::endl;
        
        // Example of how you might write a single-variable solution if it were density
        /*
        vtkDataArray* densityArr = pd->GetArray(0);
        if (densityArr) {
            std::vector<double> q(nPts);
            for(size_t i = 0; i < nPts; ++i) q[i] = densityArr->GetComponent(i, 0);
            Plot3dReferenceConditions ref = {0.5f, 0.0f, 1e6f, 0.0f};
            plot3d_write_solution("converted_solution.q", &block, q.data(), 1, &ref, &opt);
        }
        */
    }

    return EXIT_SUCCESS;
}
