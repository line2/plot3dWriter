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
#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkIdList.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <map>
#include <set>
#include <queue>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <sstream>

namespace fs = std::filesystem;

// Helper structure for (i,j,k) coordinates
struct IJK {
    int i, j, k;
    bool operator<(const IJK& other) const {
        if (i != other.i) return i < other.i;
        if (j != other.j) return j < other.j;
        return k < other.k;
    }
};

static void DebugLog(const char* location, const char* message, const std::string& dataJson, const char* runId, const char* hypothesisId) {
    auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    long long ts = now.time_since_epoch().count();
    std::ofstream f("c:\\Users\\line2\\CLionProjects\\plot3dWriter\\.cursor\\debug.log", std::ios::app);
    if (!f) return;
    f << "{\"sessionId\":\"debug-session\",\"runId\":\"" << runId
      << "\",\"hypothesisId\":\"" << hypothesisId
      << "\",\"location\":\"" << location
      << "\",\"message\":\"" << message
      << "\",\"data\":" << dataJson
      << ",\"timestamp\":" << ts << "}\n";
}

static vtkStructuredGrid* TryRebuildStructuredGridFromUnstructured(vtkUnstructuredGrid* ug) {
    if (!ug) return nullptr;

    vtkIdType numPoints = ug->GetNumberOfPoints();
    vtkIdType numCells = ug->GetNumberOfCells();

    if (numPoints == 0 || numCells == 0) return nullptr;

    // #region agent log
    DebugLog("file_reader_c_api_example.cpp:43", "TryRebuildStructuredGridFromUnstructured enter",
             std::string("{\"numPoints\":") + std::to_string(numPoints) + ",\"numCells\":" + std::to_string(numCells) + "}",
             "debug-run", "H1");
    // #endregion

    // 1. Basic Topology Check: All cells must be Hexahedrons or Voxels
    for (vtkIdType i = 0; i < numCells; ++i) {
        int cellType = ug->GetCellType(i);
        if (cellType != VTK_HEXAHEDRON && cellType != VTK_VOXEL) {
            // #region agent log
            DebugLog("file_reader_c_api_example.cpp:51", "Non-hex cell encountered",
                     std::string("{\"cellId\":") + std::to_string(i) + ",\"cellType\":" + std::to_string(cellType) + "}",
                     "debug-run", "H2");
            // #endregion
            std::cerr << "Rebuild failed: Cell " << i << " is not a hexahedron/voxel." << std::endl;
            return nullptr;
        }
    }

    // 2. Build Point Adjacency Graph
    std::vector<std::set<vtkIdType>> adj(numPoints);
    for (vtkIdType i = 0; i < numCells; ++i) {
        vtkCell* cell = ug->GetCell(i);
        // Hex/Voxel has 12 edges. We'll just use the cell connectivity to find neighbors.
        // For a structured hex, each point has neighbors along the i, j, k axes.
        vtkIdList* ptIds = cell->GetPointIds();
        
        auto addEdge = [&](int localId1, int localId2) {
            vtkIdType id1 = ptIds->GetId(localId1);
            vtkIdType id2 = ptIds->GetId(localId2);
            adj[id1].insert(id2);
            adj[id2].insert(id1);
        };

        if (ug->GetCellType(i) == VTK_HEXAHEDRON || ug->GetCellType(i) == VTK_VOXEL) {
            // Standard VTK Hex/Voxel edge connectivity
            addEdge(0, 1); addEdge(1, 2); addEdge(2, 3); addEdge(3, 0);
            addEdge(4, 5); addEdge(5, 6); addEdge(6, 7); addEdge(7, 4);
            addEdge(0, 4); addEdge(1, 5); addEdge(2, 6); addEdge(3, 7);
        }
    }

    // 3. Find a Corner Point (valence = 3 in the point graph for a structured grid)
    vtkIdType startNode = -1;
    for (vtkIdType i = 0; i < numPoints; ++i) {
        if (adj[i].size() == 3) {
            startNode = i;
            break;
        }
    }

    if (startNode == -1) {
        // #region agent log
        DebugLog("file_reader_c_api_example.cpp:83", "No corner point found",
                 std::string("{\"numPoints\":") + std::to_string(numPoints) + "}",
                 "debug-run", "H3");
        // #endregion
        std::cerr << "Rebuild failed: No corner point found (points with 3 neighbors)." << std::endl;
        return nullptr;
    }

    // 4. BFS to assign (i,j,k)
    std::map<vtkIdType, IJK> pointToIJK;
    std::map<IJK, vtkIdType> ijkToPoint;
    
    std::queue<vtkIdType> q;
    q.push(startNode);
    pointToIJK[startNode] = {0, 0, 0};
    ijkToPoint[{0, 0, 0}] = startNode;

    // To determine directions, we use the first cell connected to the startNode
    vtkSmartPointer<vtkIdList> cellIds = vtkSmartPointer<vtkIdList>::New();
    ug->GetPointCells(startNode, cellIds);
    if (cellIds->GetNumberOfIds() == 0) return nullptr;
    
    vtkCell* firstCell = ug->GetCell(cellIds->GetId(0));
    vtkIdList* firstCellPts = firstCell->GetPointIds();
    int localIdx = -1;
    for(int i=0; i<8; ++i) if(firstCellPts->GetId(i) == startNode) localIdx = i;

    // Map local neighbors of startNode in the first cell to i, j, k directions
    // VTK Hex order: 0-1 is i, 0-3 is j, 0-4 is k (roughly)
    std::map<vtkIdType, IJK> directions;
    auto getDir = [&](int localNeighbor, IJK dir) {
        directions[firstCellPts->GetId(localNeighbor)] = dir;
    };

    auto addEdge = [&](int localA, int localB) {
        // #region agent log
        DebugLog("file_reader_c_api_example.cpp:112", "Unexpected addEdge call in direction mapping",
                 std::string("{\"localA\":") + std::to_string(localA) + ",\"localB\":" + std::to_string(localB) + "}",
                 "debug-run", "H4");
        // #endregion
        (void)localA;
        (void)localB;
    };

    if (localIdx == 0) { getDir(1, {1,0,0}); getDir(3, {0,1,0}); getDir(4, {0,0,1}); }
    else if (localIdx == 1) { getDir(0, {-1,0,0}); getDir(2, {0,1,0}); getDir(5, {0,0,1}); }
    else if (localIdx == 2) { getDir(3, {-1,0,0}); getDir(1, {0,-1,0}); getDir(6, {0,0,1}); }
    else if (localIdx == 3) { getDir(0, {0,-1,0}); getDir(2, {1,0,0}); getDir(7, {0,0,1}); }
    else if (localIdx == 4) { getDir(5, {1,0,0}); getDir(7, {0,-1,0}); getDir(0, {0,0,-1}); }
    else if (localIdx == 5) { getDir(4, {-1,0,0}); getDir(6, {0,1,0}); addEdge(1, 5); getDir(1, {0,0,-1}); }
    else if (localIdx == 6) { getDir(7, {-1,0,0}); getDir(5, {0,-1,0}); getDir(2, {0,0,-1}); }
    else if (localIdx == 7) { getDir(4, {0,1,0}); getDir(6, {1,0,0}); getDir(3, {0,0,-1}); }

    while (!q.empty()) {
        vtkIdType curr = q.front();
        q.pop();
        IJK currIJK = pointToIJK[curr];

        for (vtkIdType neighbor : adj[curr]) {
            if (pointToIJK.find(neighbor) == pointToIJK.end()) {
                // Determine direction from curr to neighbor
                // We look for a cell that contains both curr and neighbor
                vtkSmartPointer<vtkIdList> commonCells = vtkSmartPointer<vtkIdList>::New();
                ug->GetCellNeighbors(curr, vtkSmartPointer<vtkIdList>::New(), commonCells); // This is not quite right for points
                
                // Correct way to find common cells for two points
                vtkSmartPointer<vtkIdList> cells1 = vtkSmartPointer<vtkIdList>::New();
                ug->GetPointCells(curr, cells1);
                vtkSmartPointer<vtkIdList> cells2 = vtkSmartPointer<vtkIdList>::New();
                ug->GetPointCells(neighbor, cells2);
                
                vtkIdType commonCellId = -1;
                for(vtkIdType c1=0; c1<cells1->GetNumberOfIds(); ++c1) {
                    for(vtkIdType c2=0; c2<cells2->GetNumberOfIds(); ++c2) {
                        if(cells1->GetId(c1) == cells2->GetId(c2)) {
                            commonCellId = cells1->GetId(c1);
                            break;
                        }
                    }
                    if(commonCellId != -1) break;
                }

                if (commonCellId != -1) {
                    vtkCell* cell = ug->GetCell(commonCellId);
                    vtkIdList* pts = cell->GetPointIds();
                    int i1 = -1, i2 = -1;
                    for(int i=0; i<8; ++i) {
                        if(pts->GetId(i) == curr) i1 = i;
                        if(pts->GetId(i) == neighbor) i2 = i;
                    }

                    // Use VTK Hex/Voxel topology to find relative direction
                    IJK diff = {0,0,0};
                    // Simplified direction logic based on VTK_HEXAHEDRON/VTK_VOXEL standard numbering
                    int rel = -1;
                    auto isEdge = [&](int a, int b) { return (i1 == a && i2 == b) || (i1 == b && i2 == a); };
                    int sign = (i2 > i1) ? 1 : -1;
                    if (isEdge(0,1) || isEdge(3,2) || isEdge(4,5) || isEdge(7,6)) diff = {1,0,0};
                    else if (isEdge(0,3) || isEdge(1,2) || isEdge(4,7) || isEdge(5,6)) diff = {0,1,0};
                    else if (isEdge(0,4) || isEdge(1,5) || isEdge(2,6) || isEdge(3,7)) diff = {0,0,1};
                    
                    if (i2 < i1) { diff.i *= -1; diff.j *= -1; diff.k *= -1; }

                    IJK nextIJK = {currIJK.i + diff.i, currIJK.j + diff.j, currIJK.k + diff.k};
                    if (ijkToPoint.find(nextIJK) == ijkToPoint.end()) {
                        pointToIJK[neighbor] = nextIJK;
                        ijkToPoint[nextIJK] = neighbor;
                        q.push(neighbor);
                    }
                }
            }
        }
    }

    if (pointToIJK.size() != numPoints) {
        // #region agent log
        DebugLog("file_reader_c_api_example.cpp:177", "Incomplete IJK assignment",
                 std::string("{\"assigned\":") + std::to_string(pointToIJK.size()) + ",\"numPoints\":" + std::to_string(numPoints) + "}",
                 "debug-run", "H5");
        // #endregion
        std::cerr << "Rebuild failed: Could not assign IJK to all points. Grid might be non-structured." << std::endl;
        return nullptr;
    }

    // 5. Determine Dimensions and Shift to 0,0,0
    int minI = 0, maxI = 0, minJ = 0, maxJ = 0, minK = 0, maxK = 0;
    for (auto const& [id, ijk] : pointToIJK) {
        minI = std::min(minI, ijk.i); maxI = std::max(maxI, ijk.i);
        minJ = std::min(minJ, ijk.j); maxJ = std::max(maxJ, ijk.j);
        minK = std::min(minK, ijk.k); maxK = std::max(maxK, ijk.k);
    }

    int ni = maxI - minI + 1;
    int nj = maxJ - minJ + 1;
    int nk = maxK - minK + 1;

    if ((size_t)ni * nj * nk != (size_t)numPoints) {
        // #region agent log
        DebugLog("file_reader_c_api_example.cpp:194", "Dimensions do not match numPoints",
                 std::string("{\"ni\":") + std::to_string(ni) + ",\"nj\":" + std::to_string(nj) + ",\"nk\":" + std::to_string(nk) +
                 ",\"numPoints\":" + std::to_string(numPoints) + "}",
                 "debug-run", "H5");
        // #endregion
        std::cerr << "Rebuild failed: ni*nj*nk (" << ni << "*" << nj << "*" << nk << "=" << ni*nj*nk 
                  << ") != numPoints (" << numPoints << ")" << std::endl;
        return nullptr;
    }

    // 6. Create vtkStructuredGrid
    auto sg = vtkStructuredGrid::New();
    sg->SetDimensions(ni, nj, nk);
    
    vtkSmartPointer<vtkPoints> newPts = vtkSmartPointer<vtkPoints>::New();
    newPts->SetNumberOfPoints(numPoints);

    for (auto const& [id, ijk] : pointToIJK) {
        int i = ijk.i - minI;
        int j = ijk.j - minJ;
        int k = ijk.k - minK;
        vtkIdType newId = i + ni * (j + nj * k);
        newPts->SetPoint(newId, ug->GetPoint(id));
    }
    sg->SetPoints(newPts);

    return sg;
}

static vtkStructuredGrid* ExtractStructuredGrid(vtkDataObject* obj);
static vtkUnstructuredGrid* ExtractUnstructuredGrid(vtkDataObject* obj);

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
        vtkDataObject* output = reader->GetOutput();
        if (auto mb = vtkMultiBlockDataSet::SafeDownCast(output)) {
            if (auto sg = ExtractStructuredGrid(mb)) {
                return sg;
            }
            if (auto ug = ExtractUnstructuredGrid(mb)) {
                return ug;
            }
        }
        return output;
    }

    // #region agent log
    DebugLog("file_reader_c_api_example.cpp:309", "ReadFile unsupported extension",
             std::string("{\"ext\":\"") + ext + "\"}",
             "debug-run", "H6");
    // #endregion

    return nullptr;
}

static vtkStructuredGrid* ExtractStructuredGrid(vtkDataObject* obj) {
    if (!obj) return nullptr;

    if (auto sg = vtkStructuredGrid::SafeDownCast(obj)) {
        return sg;
    }

    if (auto mb = vtkMultiBlockDataSet::SafeDownCast(obj)) {
        std::ostringstream blockTypes;
        blockTypes << "[";
        for (unsigned int i = 0; i < mb->GetNumberOfBlocks(); ++i) {
            auto block = mb->GetBlock(i);
            const char* cls = block ? block->GetClassName() : "null";
            if (i > 0) blockTypes << ",";
            blockTypes << "\"" << cls << "\"";
        }
        blockTypes << "]";
        // #region agent log
        DebugLog("file_reader_c_api_example.cpp:319", "ExtractStructuredGrid multiblock",
                 std::string("{\"numBlocks\":") + std::to_string(mb->GetNumberOfBlocks()) +
                 ",\"blockTypes\":" + blockTypes.str() + "}",
                 "debug-run", "H7");
        // #endregion
        for (unsigned int i = 0; i < mb->GetNumberOfBlocks(); ++i) {
            auto block = mb->GetBlock(i);
            if (!block) continue;
            if (auto sg = vtkStructuredGrid::SafeDownCast(block)) {
                // #region agent log
                DebugLog("file_reader_c_api_example.cpp:337", "ExtractStructuredGrid found block",
                         std::string("{\"blockIndex\":") + std::to_string(i) +
                         ",\"blockClass\":\"" + block->GetClassName() + "\"}",
                         "debug-run", "H8");
                // #endregion
                return sg;
            }
            if (auto nested = vtkMultiBlockDataSet::SafeDownCast(block)) {
                if (auto sg = ExtractStructuredGrid(nested)) {
                    return sg;
                }
            }
        }
    }

    return nullptr;
}

static vtkUnstructuredGrid* ExtractUnstructuredGrid(vtkDataObject* obj) {
    if (!obj) return nullptr;

    if (auto ug = vtkUnstructuredGrid::SafeDownCast(obj)) {
        return ug;
    }

    if (auto mb = vtkMultiBlockDataSet::SafeDownCast(obj)) {
        std::ostringstream blockTypes;
        blockTypes << "[";
        for (unsigned int i = 0; i < mb->GetNumberOfBlocks(); ++i) {
            auto block = mb->GetBlock(i);
            const char* cls = block ? block->GetClassName() : "null";
            if (i > 0) blockTypes << ",";
            blockTypes << "\"" << cls << "\"";
        }
        blockTypes << "]";
        // #region agent log
        DebugLog("file_reader_c_api_example.cpp:357", "ExtractUnstructuredGrid multiblock",
                 std::string("{\"numBlocks\":") + std::to_string(mb->GetNumberOfBlocks()) +
                 ",\"blockTypes\":" + blockTypes.str() + "}",
                 "debug-run", "H9");
        // #endregion
        for (unsigned int i = 0; i < mb->GetNumberOfBlocks(); ++i) {
            auto block = mb->GetBlock(i);
            if (!block) continue;
            if (auto ug = vtkUnstructuredGrid::SafeDownCast(block)) {
                // #region agent log
                DebugLog("file_reader_c_api_example.cpp:367", "ExtractUnstructuredGrid found block",
                         std::string("{\"blockIndex\":") + std::to_string(i) +
                         ",\"blockClass\":\"" + block->GetClassName() + "\"}",
                         "debug-run", "H9");
                // #endregion
                return ug;
            }
            if (auto nested = vtkMultiBlockDataSet::SafeDownCast(block)) {
                if (auto ug = ExtractUnstructuredGrid(nested)) {
                    return ug;
                }
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
    // #region agent log
    DebugLog("file_reader_c_api_example.cpp:396", "ReadFile output type",
             std::string("{\"class\":\"") + dataObj->GetClassName() + "\"}",
             "debug-run", "H10");
    // #endregion

    // Check if it's a structured grid
    vtkStructuredGrid* sg = ExtractStructuredGrid(dataObj);
    bool wasRebuilt = false;

    if (!sg) {
        if (auto ug = ExtractUnstructuredGrid(dataObj)) {
            std::cout << "Input is an Unstructured Grid. Attempting to rebuild structured topology..." << std::endl;
            sg = TryRebuildStructuredGridFromUnstructured(ug);
            if (sg) {
                std::cout << "Successfully rebuilt structured topology from unstructured grid!" << std::endl;
                wasRebuilt = true;
            } else {
                std::cerr << "Error: Input file contains an Unstructured Grid that could not be rebuilt as a Structured Grid." << std::endl;
                std::cerr << "Plot3D only supports Structured Grids. Please provide a Structured Grid or resample your data." << std::endl;
                return EXIT_FAILURE;
            }
        } else {
            std::cerr << "Error: Input file does not contain a supported Structured Grid or a rebuildable Unstructured Grid." << std::endl;
            return EXIT_FAILURE;
        }
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

    if (wasRebuilt) {
        sg->Delete();
    }

    return EXIT_SUCCESS;
}
