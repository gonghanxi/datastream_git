#ifndef CROSSCORR_BLOCK_H
#define CROSSCORR_BLOCK_H
#include "CrossCorr.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API CrossCorr_Block : public Block
{
public:
    CrossCorr_Block(const std::string& name);
    ~CrossCorr_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    CrossCorr::CorrelationTypeEnum ConvertStringToCorrelationTypeEnum(const std::string& value);
    CrossCorr::NormalizationEnum ConvertStringToNormalizationEnum(const std::string& value);
    void SetDefaultParameters();
    bool ModelsSetup();

    std::unique_ptr<CrossCorr> m_CrossCorr;

    CrossCorr::CorrelationTypeEnum m_CorrelationType;
    int                 m_CorrelationLength;
    int                 m_StartLag;
    int                 m_StopLag;
    CrossCorr::NormalizationEnum   m_Normalization;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inputBuffer;   // 多输入累积缓冲区
    std::vector<double> m_input2Buffer;
    std::queue<double> m_outputQueue;    // 输出分发队列
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(CrossCorr_Block);
#endif // CROSSCORR_BLOCK_H
