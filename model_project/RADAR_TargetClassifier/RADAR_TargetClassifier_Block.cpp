#include "RADAR_TargetClassifier_Block.h"
#include <vector>
#include <limits>
#include <random>

RADAR_TargetClassifier_Block::RADAR_TargetClassifier_Block(const std::string &name)
    :Block(name)
{

}

bool RADAR_TargetClassifier_Block::Setup()
{
    Block::Setup();
    while(!m_predictOutQueue.empty()) m_predictOutQueue.pop();
    while(!m_centroidQueue.empty()) m_centroidQueue.pop();
    return true;
}

bool RADAR_TargetClassifier_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool RADAR_TargetClassifier_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_classifier = std::make_unique<RADAR_TargetClassifier>();
    SetDefaultParameters();

    try { ClassifierType = std::stoi(getParameter("ClassifierType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ClassifierType', using default value."); }
    try { K = std::stoi(getParameter("K").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'K', using default value."); }
    try { TrainSize = std::stoi(getParameter("TrainSize").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'TrainSize', using default value."); }
    try { PredictSize = std::stoi(getParameter("PredictSize").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PredictSize', using default value."); }
    try { MaxIteration = std::stoi(getParameter("MaxIteration").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'MaxIteration', using default value."); }

    SetParameters();

    if(!ModelSetup()) return false;

    AddInputPort("trainIn", m_classifier->trainIn, static_cast<size_t>(TrainSize), DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddInputPort("predictIn", m_classifier->predictIn, static_cast<size_t>(PredictSize), DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("predictOut", m_classifier->predictOut, static_cast<size_t>(PredictSize), DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("centroid", m_classifier->centroid, static_cast<size_t>(K), DataType::CIRCULAR_BUFFER_DCOMPLEX);
    return true;
}

void RADAR_TargetClassifier_Block::SetDefaultParameters()
{
    ClassifierType = 0;
    K = 2;
    TrainSize = 100;
    PredictSize = 100;
    MaxIteration = 200;
}

bool RADAR_TargetClassifier_Block::ModelSetup()
{
    bool bStatus = true;

    if(K < 2)
    {
        LOG_ERROR("K must be >= 2");
        bStatus = false;
    }
    if(K > TrainSize)
    {
        LOG_ERROR("K must be <= TrainSize");
        bStatus = false;
    }
    if(TrainSize <= 0)
    {
        LOG_ERROR("TrainSize must be > 0");
        bStatus = false;
    }
    if(PredictSize <= 0)
    {
        LOG_ERROR("PredictSize must be > 0");
        bStatus = false;
    }

    return bStatus;
}

void RADAR_TargetClassifier_Block::SetParameters()
{
    if(!m_classifier) return;
    m_classifier->ClassifierType = static_cast<RADAR_TargetClassifier::SelectedClassifierType>(ClassifierType);
    m_classifier->K = K;
    m_classifier->TrainSize = TrainSize;
    m_classifier->PredictSize = PredictSize;
    m_classifier->MaxIteration = MaxIteration;
}

// ============================================================
// DataStreamRun
// ============================================================

bool RADAR_TargetClassifier_Block::DataStreamRun()
{
    auto trainData = ReadInputData<std::complex<double>>("trainIn");
    auto predictData = ReadInputData<std::complex<double>>("predictIn");

    if(trainData.empty() || predictData.empty()) return true;

    // ========== 训练阶段：K-means ==========
    std::vector<int> labels(TrainSize);
    std::vector<std::complex<double>> centroids(K);

    std::mt19937 rng(42);
    std::vector<int> idx(TrainSize);
    for(int i = 0; i < TrainSize; ++i) idx[i] = i;
    std::shuffle(idx.begin(), idx.end(), rng);
    for(int i = 0; i < K; ++i) centroids[i] = trainData[idx[i]];

    for(int iter = 0; iter < MaxIteration; ++iter)
    {
        bool changed = false;
        for(int i = 0; i < TrainSize; ++i)
        {
            double bestDist = std::numeric_limits<double>::max();
            int bestIdx = 0;
            for(int j = 0; j < K; ++j)
            {
                double dist = std::abs(trainData[i] - centroids[j]);
                if(dist < bestDist)
                {
                    bestDist = dist;
                    bestIdx = j;
                }
                if(labels[i] != bestIdx)
                {
                    labels[i] = bestIdx;
                    changed = true;
                }
            }
            if(!changed) break;

            for(int j = 0; j < K; ++j)
            {
                centroids[j] = 0;
                int cnt = 0;
                for(int i = 0; i < TrainSize; ++i)
                {
                    if(labels[i] != j) continue;
                    centroids[j] += trainData[j];
                    ++cnt;
                }
                if(cnt > 0)
                {
                    centroids[j] /= cnt;
                }
            }
        }
    }

    // ========== 预测阶段 ==========
    std::vector<int> predictResult(PredictSize);
    for(int i = 0; i < PredictSize; ++i)
    {
        double bestDist = std::numeric_limits<double>::max();
        for(int j = 0; j < K; ++j)
        {
            double dist = std::abs(predictData[i] - centroids[j]);
            if(dist < bestDist)
            {
                bestDist = dist;
                predictResult[i] = j;
            }
        }
    }

    WriteOutputData("predictOut", predictResult);
    WriteOutputData("centroid", centroids);

    return true;
}

// ============================================================
// TimeDrivenRun
// ============================================================

bool RADAR_TargetClassifier_Block::TimeDrivenRun()
{
    auto trainData = ReadInputData<std::complex<double>>("trainIn");
    auto predictData = ReadInputData<std::complex<double>>("predictIn");

    for(const auto& val : trainData) m_trainBuffer.push_back(val);
    m_trainCount += static_cast<int>(trainData.size());

    for(const auto& val : predictData) m_predictBuffer.push_back(val);
    m_predictCount += static_cast<int>(predictData.size());

    // 当两个输入都积累足够数据时执行算法
    if(m_trainCount >= TrainSize && m_predictCount >= PredictSize) {
        // ========== 训练阶段：K-means ==========
        std::vector<int> labels(TrainSize);
        std::vector<std::complex<double>> centroids(K);

        std::mt19937 rng(42);
        std::vector<int> idx(TrainSize);
        for(int i = 0; i < TrainSize; ++i) idx[i] = i;
        std::shuffle(idx.begin(), idx.end(), rng);
        for(int i = 0; i < K; ++i) centroids[i] = m_trainBuffer[idx[i]];

        for(int iter = 0; iter < MaxIteration; ++iter)
        {
            bool changed = false;
            for(int i = 0; i < TrainSize; ++i)
            {
                double bestDist = std::numeric_limits<double>::max();
                int bestIdx = 0;
                for(int j = 0; j < K; ++j)
                {
                    double dist = std::abs(m_trainBuffer[i] - centroids[j]);
                    if(dist < bestDist)
                    {
                        bestDist = dist;
                        bestIdx = j;
                    }
                    if(labels[i] != bestIdx)
                    {
                        labels[i] = bestIdx;
                        changed = true;
                    }
                }
                if(!changed) break;

                for(int j = 0; j < K; ++j)
                {
                    centroids[j] = 0;
                    int cnt = 0;
                    for(int i = 0; i < TrainSize; ++i)
                    {
                        if(labels[i] != j) continue;
                        centroids[j] += m_trainBuffer[j];
                        ++cnt;
                    }
                    if(cnt > 0)
                    {
                        centroids[j] /= cnt;
                    }
                }
            }
        }

        // ========== 预测阶段 ==========
        for(int i = 0; i < PredictSize; ++i)
        {
            double bestDist = std::numeric_limits<double>::max();
            for(int j = 0; j < K; ++j)
            {
                double dist = std::abs(m_predictBuffer[i] - centroids[j]);
                if(dist < bestDist)
                {
                    bestDist = dist;
                    m_predictOutQueue.push(j);
                }
            }
        }

        for(int j = 0; j < K; ++j)
        {
            m_centroidQueue.push(centroids[j]);
        }

        m_trainBuffer.clear();
        m_predictBuffer.clear();
        m_trainCount = 0;
        m_predictCount = 0;
    }

    // 分发 predictOut
    if(!m_predictOutQueue.empty()) {
        auto outputValue = m_predictOutQueue.front();
        m_predictOutQueue.pop();
        m_outputCount++;
        WriteOutputData("predictOut", std::vector<int>{outputValue});
    }

    // 分发 centroid
    if(!m_centroidQueue.empty()) {
        auto centroidValue = m_centroidQueue.front();
        m_centroidQueue.pop();
        WriteOutputData("centroid", std::vector<std::complex<double>>{centroidValue});
    }

    return true;
}
