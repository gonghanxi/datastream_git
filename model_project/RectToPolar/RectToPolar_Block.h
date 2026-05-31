#ifndef RECTTOPOLAR_BLOCK_H
#define RECTTOPOLAR_BLOCK_H
#include "RectToPolar.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API RectToPolar_Block : public Block
{
public:
    RectToPolar_Block(const std::string& name);
    ~RectToPolar_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
private:
    std::unique_ptr<RectToPolar> m_RectToPolar;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_xBuffer;   // 多输入累积缓冲区
    std::vector<double> m_yBuffer;
    std::queue<double> m_phaseQueue;    // 输出分发队列
    std::queue<double> m_magnitudeQueue;

    double m_lastphase;                 // 上次输出值（用于保持）
    double m_lastmagnitude;
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(RectToPolar_Block);

#endif // RECTTOPOLAR_BLOCK_H
