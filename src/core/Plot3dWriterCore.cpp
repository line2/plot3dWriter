#ifndef PLOT3D_WRITER_DLL_EXPORTS
#define PLOT3D_WRITER_DLL_EXPORTS
#endif

#include "Plot3dWriterCore.h"
#include <fstream>
#include <ostream>
#include <vector>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <array>
#include <type_traits>

namespace {

thread_local std::string g_last_error;

void SetLastError(const std::string& msg) {
    g_last_error = msg;
}

bool IsLittleEndian() {
    const std::uint16_t one = 1;
    return reinterpret_cast<const std::uint8_t*>(&one)[0] == 1;
}

template <typename T>
T ByteSwap(T v) {
    static_assert(std::is_trivially_copyable<T>::value, "ByteSwap requires trivially copyable type");
    std::array<std::uint8_t, sizeof(T)> bytes;
    std::memcpy(bytes.data(), &v, sizeof(T));
    std::reverse(bytes.begin(), bytes.end());
    T out;
    std::memcpy(&out, bytes.data(), sizeof(T));
    return out;
}

template <typename T>
void WriteValue(std::ostream& out, T v, bool swap) {
    if (swap) v = ByteSwap(v);
    out.write(reinterpret_cast<const char*>(&v), sizeof(T));
}

class RecordWriter {
    std::ostream& out_;
    bool useFortran_;
    bool swap_;
    std::streampos startPos_;

public:
    RecordWriter(std::ostream& out, bool useFortran, bool swap)
        : out_(out), useFortran_(useFortran), swap_(swap) {
        if (useFortran_) {
            startPos_ = out_.tellp();
            std::uint32_t placeholder = 0;
            WriteValue(out_, placeholder, swap_);
        }
    }

    ~RecordWriter() {
        if (useFortran_) {
            std::streampos endPos = out_.tellp();
            std::uint32_t length = static_cast<std::uint32_t>(endPos - startPos_ - (std::streampos)sizeof(std::uint32_t));
            out_.seekp(startPos_);
            WriteValue(out_, length, swap_);
            out_.seekp(endPos);
            WriteValue(out_, length, swap_);
        }
    }
};

} // namespace

namespace plot3d_writer {

template <typename RealT>
int WriteStructuredInternal(
    const std::string& filename,
    const StructuredBlock& block,
    const WriteOptions& options) {

    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        SetLastError("Failed to open file: " + filename);
        return -1;
    }

    bool swap = !IsLittleEndian();
    bool useFortran = options.useFortranFormat;

    // Header record
    {
        RecordWriter rw(out, useFortran, swap);
        std::int32_t nBlocks = 1;
        WriteValue(out, nBlocks, swap);
    }

    // Dimensions record
    {
        RecordWriter rw(out, useFortran, swap);
        WriteValue(out, (std::int32_t)block.ni, swap);
        WriteValue(out, (std::int32_t)block.nj, swap);
        WriteValue(out, (std::int32_t)block.nk, swap);
    }

    // Coordinates record
    {
        RecordWriter rw(out, useFortran, swap);
        size_t nPts = (size_t)block.ni * block.nj * block.nk;
        
        for (size_t i = 0; i < nPts; ++i) WriteValue(out, (RealT)block.x[i], swap);
        for (size_t i = 0; i < nPts; ++i) WriteValue(out, (RealT)block.y[i], swap);
        for (size_t i = 0; i < nPts; ++i) WriteValue(out, (RealT)block.z[i], swap);
    }

    return 0;
}

template <typename RealT>
int WriteSolutionInternal(
    const std::string& filename,
    const StructuredBlock& block,
    const double* q_data,
    int num_vars,
    const ReferenceConditions& ref,
    const WriteOptions& options) {

    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        SetLastError("Failed to open file: " + filename);
        return -1;
    }

    bool swap = !IsLittleEndian();
    bool useFortran = options.useFortranFormat;

    // Header record
    {
        RecordWriter rw(out, useFortran, swap);
        std::int32_t nBlocks = 1;
        WriteValue(out, nBlocks, swap);
    }

    // Dimensions record
    {
        RecordWriter rw(out, useFortran, swap);
        WriteValue(out, (std::int32_t)block.ni, swap);
        WriteValue(out, (std::int32_t)block.nj, swap);
        WriteValue(out, (std::int32_t)block.nk, swap);
    }

    // Reference conditions and Data record
    {
        RecordWriter rw(out, useFortran, swap);
        WriteValue(out, (RealT)ref.mach, swap);
        WriteValue(out, (RealT)ref.alpha, swap);
        WriteValue(out, (RealT)ref.reynolds, swap);
        WriteValue(out, (RealT)ref.time, swap);

        size_t nPts = (size_t)block.ni * block.nj * block.nk;
        for (int v = 0; v < num_vars; ++v) {
            for (size_t i = 0; i < nPts; ++i) {
                WriteValue(out, (RealT)q_data[v * nPts + i], swap);
            }
        }
    }

    return 0;
}

int WriteStructured(
    const std::string& filename,
    const StructuredBlock& block,
    const WriteOptions& options) {
    if (options.precision == WriteOptions::Precision::Float64) {
        return WriteStructuredInternal<double>(filename, block, options);
    } else {
        return WriteStructuredInternal<float>(filename, block, options);
    }
}

int WriteSolution(
    const std::string& filename,
    const StructuredBlock& block,
    const double* q_data,
    int num_vars,
    const ReferenceConditions& ref,
    const WriteOptions& options) {
    if (options.precision == WriteOptions::Precision::Float64) {
        return WriteSolutionInternal<double>(filename, block, q_data, num_vars, ref, options);
    } else {
        return WriteSolutionInternal<float>(filename, block, q_data, num_vars, ref, options);
    }
}

const std::string& GetLastError() {
    return g_last_error;
}

} // namespace plot3d_writer
