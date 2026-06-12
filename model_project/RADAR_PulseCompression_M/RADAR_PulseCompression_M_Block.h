#ifndef RADAR_PULSECOMPRESSION_M_BLOCK_H
#define RADAR_PULSECOMPRESSION_M_BLOCK_H

#include "Block.h"
#include "RADAR_PulseCompression_M.h"

#include <complex>
#include <deque>
#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_PulseCompression_M_Block : public Block
{
public:
    RADAR_PulseCompression_M_Block(const std::string& name);
    ~RADAR_PulseCompression_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    using Cx = std::complex<double>;
    using CxMatrix = Matrix<Cx>;

    void SetDefaultParameters();
    void SetParameters();
    bool validateAndPrepare();
    bool DataStreamRun();
    bool TimeDrivenRun();

    // 核心处理（内联，原算法方法均为 private）
    CxMatrix processOne(const CxMatrix& refMat, const CxMatrix& sigMat);
    void buildWindowSequence(Matrix<Cx>& windowSeq, int fftSize);

    // 递归 FFT / IFFT
    static void fft(Matrix<Cx>& a, int n, int invert);

    // Kaiser 窗辅助
    static int factorial(int n);
    static double I0(int n, double x);

    // Reference 矩阵尺寸 / 值读取
    int getReferenceFFTSize(const CxMatrix& ref) const;
    Cx  getReferenceValue(const CxMatrix& ref, int row, int k) const;

    // 字符串 → 枚举转换
    static RADAR_PulseCompression_M::SelectedWindowType ConvertStringToWindowType(const std::string& value);

    // ---- algorithm instance（端口注册用） ----
    std::unique_ptr<RADAR_PulseCompression_M> m_algo;

    // ---- 参数 ----
    RADAR_PulseCompression_M::SelectedWindowType m_WindowType;
    double     m_WindowParameter;

    // ---- TimeDrivenRun 缓冲区 ----
    std::deque<CxMatrix> m_refBuffer;
    std::deque<CxMatrix> m_sigBuffer;
    std::queue<CxMatrix> m_outputQueue;
};

RegAlgo(RADAR_PulseCompression_M_Block);

#endif // RADAR_PULSECOMPRESSION_M_BLOCK_H
