#ifndef INTEGRATORINT_BLOCK_H
#define INTEGRATORINT_BLOCK_H

#include "Block.h"
#include "IntegratorInt.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API IntegratorInt_Block : public Block
{
public:
    IntegratorInt_Block(const std::string& name);
    ~IntegratorInt_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    IntegratorInt::LimitOutputEnum ConvertStringToLimitOutputEnum(const std::string& value);
    IntegratorInt::WindowEnum ConvertStringToWindowEnum(const std::string& value);

    void SetDefaultParameters();

    SimuParameter simulator_param;

    std::unique_ptr<IntegratorInt> m_IntegratorInt;

    IntegratorInt::LimitOutputEnum       m_LimitOutput;
    int                m_Top;
    int                m_Bottom;
    int                m_InitialState;
    IntegratorInt::WindowEnum            m_UseIntegrationWindow;
    double                m_IntegrationTime;
    int                   m_IntegrationSamples;

    bool DataStreamRun();
    bool TimeDrivenRun();
    void ProcessData(int x, bool rActive);
    // ========== 时间驱动缓冲队列 ==========
    std::vector<int> m_resetBuffer;   // 多输入累积缓冲区
    std::vector<int> m_dataBuffer;
    std::queue<int> m_outputQueue;
    int m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(IntegratorInt_Block);

#endif // INTEGRATORINT_BLOCK_H
