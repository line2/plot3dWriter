#ifndef PLOT3D_WRITER_VTK_H
#define PLOT3D_WRITER_VTK_H

#include "Plot3dWriterCore.h"
#include <string>
#include <vector>

class vtkDataObject;

struct Plot3dWriterOptions {
    plot3d_writer::WriteOptions coreOptions;
    std::string densityArrayName = "Density";
    std::string velocityArrayName = "Velocity";
    std::string pressureArrayName = "Pressure";
    double gamma = 1.4;
    plot3d_writer::ReferenceConditions refConditions;
};

class Plot3dWriter {
public:
    static void Write(vtkDataObject* input, const std::string& fileName, const Plot3dWriterOptions& opt = {});
    
    // Convenience for separate grid and solution
    static void WriteGrid(vtkDataObject* input, const std::string& fileName, const Plot3dWriterOptions& opt = {});
    static void WriteSolution(vtkDataObject* input, const std::string& fileName, const Plot3dWriterOptions& opt = {});
};

#endif // PLOT3D_WRITER_VTK_H
