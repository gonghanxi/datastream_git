#ifndef PAM_DEMAPPER_BLOCK_H
#define PAM_DEMAPPER_BLOCK_H
#include "PAM_Demapper.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API PAM_Demapper_Block : public Block
{
public:
    PAM_Demapper_Block(const std::string& name);
    ~PAM_Demapper_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    PAM_Demapper::BitOrderE ConvertStringToBitOrderE(const std::string& value);
    void SetDefaultParameters();

    std::unique_ptr<PAM_Demapper> m_pam;

    int NumBits;
    PAM_Demapper::BitOrderE BitOrder;
    double HighLevel;
    double LowLevel;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<int> m_outputQueue;    // 输出分发队列
    int m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(PAM_Demapper_Block);
#endif // PAM_DEMAPPER_BLOCK_H
