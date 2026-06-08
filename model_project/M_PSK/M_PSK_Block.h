#ifndef M_PSK_BLOCK_H
#define M_PSK_BLOCK_H
#include "M_PSK.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API M_PSK_Block : public Block
{
public:
    M_PSK_Block(const std::string& name);
    ~M_PSK_Block() = default;

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    void SetDefaultParameters();
    M_PSK::ModType ConvertStringToModType(const std::string& value);
    M_PSK::BitOrder ConvertStringToBitOrder(const std::string& value);
    bool ModelSetup();

    std::unique_ptr<M_PSK> m_psk;

    M_PSK::ModType  m_modType;
    M_PSK::BitOrder m_bitOrder;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<int> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(M_PSK_Block)
#endif // M_PSK_BLOCK_H
