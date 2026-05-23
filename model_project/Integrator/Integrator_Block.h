#ifndef INTEGRATOR_BLOCK_H
#define INTEGRATOR_BLOCK_H

#include "Block.h"
#include "Integrator.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Integrator_Block : public Block
{
public:
    Integrator_Block(const std::string& name);
    ~Integrator_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    Integrator::IntegrationMethodEnum ConvertStringToIntegrationMethodEnum(const std::string& value);
    Integrator::LimitOutputEnum ConvertStringToLimitOutputEnum(const std::string& value);
    Integrator::WindowEnum ConvertStringToWindowEnum(const std::string& value);

    void SetDefaultParameters();

    SimuParameter simulator_param;

    std::unique_ptr<Integrator> m_Integrator;

    Integrator::IntegrationMethodEnum m_IntegrationMethod;
    Integrator::LimitOutputEnum       m_LimitOutput;
    double                m_Top;
    double                m_Bottom;
    double                m_InitialState;
    Integrator::WindowEnum            m_UseIntegrationWindow;
    double                m_FeedbackGain;
    double                m_IntegrationTime;
    int                   m_IntegrationSamples;

    bool DataStreamRun();
    bool TimeDrivenRun();
    void ProcessData(double x, bool rActive);
    // ========== 时间驱动缓冲队列 ==========
    std::vector<int> m_resetBuffer;   // 多输入累积缓冲区
    std::vector<double> m_dataBuffer;
    std::queue<double> m_outputQueue;
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(Integrator_Block);
#endif // INTEGRATOR_BLOCK_H
