#ifndef AUTOCORR_BLOCK_H
#define AUTOCORR_BLOCK_H
#include "AutoCorr.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API AutoCorr_Block : public Block
{
public:
    AutoCorr_Block(const std::string& name);
    ~AutoCorr_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    AutoCorr::CorrelationType ConvertStringToCorrelationType(const std::string& value);
    AutoCorr::Normalization ConvertStringToNormalization(const std::string& value);
    void SetDefaultParameters();
    bool ModelsSetup();

    std::unique_ptr<AutoCorr> m_AutoCorr;

    AutoCorr::CorrelationType m_CorrelationType;
    int m_CorrelationLength;
    int m_StartLag;
    int m_StopLag;
    AutoCorr::Normalization m_Normalization;

    std::vector<double> m_samples;
    int m_numLags;

    double nonCircularAutoCorrelation(int lag);

    double circularAutoCorrelation(int lag);

    bool DataStreamRun();
    bool TimeDrivenRun();

    //
    size_t m_maxBlock = 0;
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<double> m_outputQueue;    // 输出分发队列
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(AutoCorr_Block);

#endif // AUTOCORR_BLOCK_H
