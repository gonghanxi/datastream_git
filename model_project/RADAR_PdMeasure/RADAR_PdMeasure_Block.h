#ifndef RADAR_PDMEASURE_BLOCK_H
#define RADAR_PDMEASURE_BLOCK_H

#include "Block.h"
#include "RADAR_PdMeasure.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_PdMeasure_Block : public Block
{
public:
    RADAR_PdMeasure_Block(const std::string& name);
    ~RADAR_PdMeasure_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool validateAndPrepare();
    bool DataStreamRun();
    bool TimeDrivenRun();

    static int    roundToInt(double x);
    static double clamp01(double x);

    // ---- algorithm instance ----
    std::unique_ptr<RADAR_PdMeasure> m_algo;

    // ---- 参数 ----
    double m_PRI;
    double m_SampleRate;
    int    m_SimulationNumber;

    // ---- 派生量 ----
    int m_rangeBinNum;
    int m_inputRate;
    int m_outputRate;

    // ---- TimeDrivenRun 缓冲区 ----
    std::vector<int>    m_inputBuffer;
    std::queue<double>  m_outputQueue;
};

RegAlgo(RADAR_PdMeasure_Block);

#endif // RADAR_PDMEASURE_BLOCK_H
