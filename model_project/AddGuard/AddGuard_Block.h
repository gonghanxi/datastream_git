#ifndef ADDGUARD_BLOCK_H
#define ADDGUARD_BLOCK_H
#include "AddGuard.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API AddGuard_Block : public Block
{
public:
    AddGuard_Block(const std::string& name);
    ~AddGuard_Block();
    bool Run() override;
    bool Setup() override;
    bool Initialize() override;
    void SetParameters();
private:
    void SetDefaultParameters();
    void ClearCplxBuffer();
    bool ModelSetup();

    std::unique_ptr<AddGuard> m_AddGuard;


    int m_iIFFTSize;
    int m_iPreGuard;
    int m_iPostGuard;
    int m_iIntersection;

    size_t m_iNout;
    size_t m_iNwin;
    size_t m_iNperiod;

    std::complex<double>* m_cplxBuffer;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_InBuffer;   // 多输入累积缓冲区
    std::vector<double> m_WindowBuffer;   // 多输入累积缓冲区

    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(AddGuard_Block)
#endif // ADDGUARD_BLOCK_H
