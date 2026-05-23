#ifndef INTEGRATORCX_BLOCK_H
#define INTEGRATORCX_BLOCK_H

#include "Block.h"
#include "IntegratorCx.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API IntegratorCx_Block : public Block
{
public:
    IntegratorCx_Block(const std::string& name);
    ~IntegratorCx_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    IntegratorCx::IntegrationMethodEnum ConvertStringToIntegrationMethodEnum(const std::string& value);
    IntegratorCx::LimitOutputEnum ConvertStringToLimitOutputEnum(const std::string& value);
    IntegratorCx::WindowEnum ConvertStringToWindowEnum(const std::string& value);

    void SetDefaultParameters();

    SimuParameter simulator_param;

    std::unique_ptr<IntegratorCx> m_IntegratorCx;

    IntegratorCx::IntegrationMethodEnum m_IntegrationMethod;
    IntegratorCx::LimitOutputEnum       m_LimitOutput;
    double                m_Top;
    double                m_Bottom;
    double                m_InitialState;
    IntegratorCx::WindowEnum            m_UseIntegrationWindow;
    double                m_FeedbackGain;
    double                m_IntegrationTime;
    int                   m_IntegrationSamples;

    bool DataStreamRun();
    bool TimeDrivenRun();
    void ProcessData(std::complex<double> x, bool rActive);
    // ========== 时间驱动缓冲队列 ==========
    std::vector<int> m_resetBuffer;   // 多输入累积缓冲区
    std::vector<std::complex<double>> m_dataBuffer;
    std::queue<std::complex<double>> m_outputQueue;
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(IntegratorCx_Block);
#endif // INTEGRATORCX_BLOCK_H
