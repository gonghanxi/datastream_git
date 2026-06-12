#ifndef RADAR_PLOTSCENTROID_BLOCK_H
#define RADAR_PLOTSCENTROID_BLOCK_H

#include "Block.h"
#include "RADAR_PlotsCentroid.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_PlotsCentroid_Block : public Block
{
public:
    RADAR_PlotsCentroid_Block(const std::string& name);
    ~RADAR_PlotsCentroid_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool validateAndPrepare();
    bool DataStreamRun();
    bool TimeDrivenRun();

    // 算法核心（内联，原算法函数为 private）
    void run1D(const std::vector<double>& src, std::vector<double>& dst);
    void run2D(const std::vector<double>& src, std::vector<double>& dst);

    // 工具函数
    int  idx2D(int rangeIndex, int dopplerIndex) const;
    bool isPositive(double x) const;
    static int roundToNearestIndex(double x);
    static int clampInt(int x, int lo, int hi);

    // 枚举常量（匹配原算法）
    static constexpr int Type_1D = 0;
    static constexpr int Type_2D = 1;

    // ---- algorithm instance (用于端口注册) ----
    std::unique_ptr<RADAR_PlotsCentroid> m_algo;

    // ---- 参数 ----
    int m_Type;
    int m_SampleNum;
    int m_RangeBinNum;
    int m_DopplerBinNum;

    // ---- 派生量 ----
    int m_effectiveSampleNum;

    // ---- TimeDrivenRun 缓冲区 ----
    std::vector<double> m_inputBuffer;
    std::queue<double>  m_outputQueue;
};

RegAlgo(RADAR_PlotsCentroid_Block);

#endif // RADAR_PLOTSCENTROID_BLOCK_H
