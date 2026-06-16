#ifndef RADAR_TARGETCLASSIFIER_BLOCK_H
#define RADAR_TARGETCLASSIFIER_BLOCK_H
#include "RADAR_TargetClassifier.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_TargetClassifier_Block : public Block
{
public:
    RADAR_TargetClassifier_Block(const std::string& name);
    ~RADAR_TargetClassifier_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    bool ModelSetup();
    void SetDefaultParameters();

    std::unique_ptr<RADAR_TargetClassifier> m_classifier;

    int ClassifierType;
    int K;
    int TrainSize;
    int PredictSize;
    int MaxIteration;

    bool DataStreamRun();
    bool TimeDrivenRun();

    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_trainBuffer;
    std::vector<std::complex<double>> m_predictBuffer;

    std::queue<int> m_predictOutQueue;
    std::queue<std::complex<double>> m_centroidQueue;

    int m_trainCount;
    int m_predictCount;
    int m_outputCount;
};
RegAlgo(RADAR_TargetClassifier_Block);
#endif // RADAR_TARGETCLASSIFIER_BLOCK_H
