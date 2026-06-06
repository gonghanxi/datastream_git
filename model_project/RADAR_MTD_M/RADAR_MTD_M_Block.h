#ifndef RADAR_MTD_M_BLOCK_H
#define RADAR_MTD_M_BLOCK_H

#include "Block.h"
#include "RADAR_MTD_M.h"

#include <memory>
#include <queue>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_MTD_M_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_MTD_M_Block(const std::string& name);
    ~RADAR_MTD_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    // ===== 字符串转换 =====
    RADAR_MTD_M::SelectedWindowType ConvertStringToWindowType(const std::string& value);

    // ===== 算法移植 =====
    void GenerateWindow(int size);
    void ProcessOneSlowTimeVector(std::vector<std::complex<double>>& x);
    void ProcessFrame(const SystemVueModelBuilder::DComplexMatrix& inMat,
                      SystemVueModelBuilder::DComplexMatrix& outMat);

    std::unique_ptr<RADAR_MTD_M> m_algo;

    // ===== 参数 =====
    RADAR_MTD_M::SelectedWindowType m_WindowType;
    int                 m_NumOfPulse;
    double*             m_Freq_Weight;
    int                 m_Freq_Weight_Size;
    double*             m_WindowParameters;
    int                 m_WindowParameters_Size;

    // ===== 参数数组存储 =====
    std::vector<double> m_Freq_WeightData;
    std::vector<double> m_WindowParametersData;

    // ===== 运行时状态 =====
    std::vector<double> m_window;

    // ===== TimeDrivenRun =====
    std::vector<SystemVueModelBuilder::DComplexMatrix> m_inputBuffer;
    std::queue<SystemVueModelBuilder::DComplexMatrix>  m_outputQueue;
};

RegAlgo(RADAR_MTD_M_Block);

#endif // RADAR_MTD_M_BLOCK_H
