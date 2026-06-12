#include "RADAR_PlotsCentroid_Block.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <string>

// ============================================================================
// 构造函数
// ============================================================================

RADAR_PlotsCentroid_Block::RADAR_PlotsCentroid_Block(const std::string& name)
    : Block(name)
    , m_Type(Type_2D)
    , m_SampleNum(1024)
    , m_RangeBinNum(512)
    , m_DopplerBinNum(128)
    , m_effectiveSampleNum(1024)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_PlotsCentroid_Block::SetDefaultParameters()
{
    m_Type        = Type_2D;
    m_SampleNum   = 1024;
    m_RangeBinNum = 512;
    m_DopplerBinNum = 128;
}

// ============================================================================
// SetParameters
// ============================================================================

void RADAR_PlotsCentroid_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->Type          = static_cast<RADAR_PlotsCentroid::CentroidTypeEnum>(m_Type);
    m_algo->SampleNum     = m_SampleNum;
    m_algo->RangeBinNum   = m_RangeBinNum;
    m_algo->DopplerBinNum = m_DopplerBinNum;
}

// ============================================================================
// validateAndPrepare
// ============================================================================

bool RADAR_PlotsCentroid_Block::validateAndPrepare()
{
    if (m_Type == Type_1D)
    {
        if (m_SampleNum <= 0)
        {
            LOG_ERROR("SampleNum must be greater than 0.");
            return false;
        }
        m_effectiveSampleNum = m_SampleNum;
        return true;
    }

    // Type_2D
    if (m_RangeBinNum <= 0)
    {
        LOG_ERROR("RangeBinNum must be greater than 0 when Type is 2D.");
        return false;
    }
    if (m_DopplerBinNum <= 0)
    {
        LOG_ERROR("DopplerBinNum must be greater than 0 when Type is 2D.");
        return false;
    }

    const long long total =
        static_cast<long long>(m_RangeBinNum) * static_cast<long long>(m_DopplerBinNum);
    if (total <= 0 || total > 2147483647LL)
    {
        LOG_ERROR("RangeBinNum * DopplerBinNum is invalid or too large.");
        return false;
    }

    m_effectiveSampleNum = static_cast<int>(total);

    if (m_SampleNum != m_effectiveSampleNum)
    {
        LOG_WARN("For 2D mode, SampleNum should be equal to RangeBinNum * DopplerBinNum. "
                    "The model will use RangeBinNum * DopplerBinNum as the effective port rate.");
    }

    return true;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_PlotsCentroid_Block::Setup()
{
    Block::Setup();

    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    return true;
}

// ============================================================================
// Run
// ============================================================================

bool RADAR_PlotsCentroid_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_PlotsCentroid_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_PlotsCentroid>();

    SetDefaultParameters();

    try { m_Type        = std::stoi(getParameter("Type").Value);        } catch (...) {}
    try { m_SampleNum   = std::stoi(getParameter("SampleNum").Value);   } catch (...) {}
    try { m_RangeBinNum = std::stoi(getParameter("RangeBinNum").Value); } catch (...) {}
    try { m_DopplerBinNum = std::stoi(getParameter("DopplerBinNum").Value); } catch (...) {}

    SetParameters();

    if (!validateAndPrepare()) {
        return false;
    }

    AddInputPort("input",  m_algo->input,  m_effectiveSampleNum, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_algo->output, m_effectiveSampleNum, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

// ============================================================================
// DataStreamRun — 数据流模式：一次处理整个 PRI 的 CFAR 数据
// ============================================================================

bool RADAR_PlotsCentroid_Block::DataStreamRun()
{
    auto inputData = ReadInputData<double>(GetInputPortName(0));
    if (inputData.empty()) return true;

    const int L = m_effectiveSampleNum;
    std::vector<double> outVec(static_cast<size_t>(L), 0.0);

    if (m_Type == Type_1D)
        run1D(inputData, outVec);
    else
        run2D(inputData, outVec);

    WriteOutputData(GetOutputPortName(0), outVec);

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式：累积 → 处理一个块 → 出队
// ============================================================================

bool RADAR_PlotsCentroid_Block::TimeDrivenRun()
{
    // ① 累积输入
    {
        auto inputData = ReadInputData<double>(GetInputPortName(0));
        for (auto& v : inputData) m_inputBuffer.push_back(v);
    }

    // ② 当累积足够时，处理一个块
    if (static_cast<int>(m_inputBuffer.size()) >= m_effectiveSampleNum)
    {
        const int L = m_effectiveSampleNum;
        std::vector<double> outVec(static_cast<size_t>(L), 0.0);

        if (m_Type == Type_1D)
            run1D(m_inputBuffer, outVec);
        else
            run2D(m_inputBuffer, outVec);

        for (const auto& v : outVec) m_outputQueue.push(v);
        m_inputBuffer.clear();
    }

    // ③ 出队写入
    if (!m_outputQueue.empty())
    {
        double v = m_outputQueue.front(); m_outputQueue.pop();
        WriteOutputData(GetOutputPortName(0), std::vector<double>{v});
    }

    return true;
}

// ============================================================================
// 1D 点迹质心
//
// 连续 input[i] > 0 的区间视为一个 plot；
// 用 input[i] 作为权重计算质心，四舍五入索引，写入该 plot 最大值。
// ============================================================================

void RADAR_PlotsCentroid_Block::run1D(const std::vector<double>& src, std::vector<double>& dst)
{
    const int L = m_effectiveSampleNum;

    int i = 0;
    while (i < L)
    {
        while (i < L && !isPositive(src[static_cast<size_t>(i)]))
            ++i;

        if (i >= L) break;

        const int start = i;
        double sumW = 0.0;
        double sumIW = 0.0;
        double maxVal = src[static_cast<size_t>(i)];

        while (i < L && isPositive(src[static_cast<size_t>(i)]))
        {
            const double w = src[static_cast<size_t>(i)];
            sumW  += w;
            sumIW += static_cast<double>(i) * w;
            if (w > maxVal) maxVal = w;
            ++i;
        }

        const int end = i - 1;

        if (sumW > 0.0)
        {
            int ci = roundToNearestIndex(sumIW / sumW);
            ci = clampInt(ci, start, end);
            ci = clampInt(ci, 0, L - 1);
            if (maxVal > dst[static_cast<size_t>(ci)])
                dst[static_cast<size_t>(ci)] = maxVal;
        }
    }
}

// ============================================================================
// 2D 点迹质心
//
// BFS 4-邻域连通域搜索；对每个 plot 计算 Range/Doppler 幅度加权质心。
// ============================================================================

void RADAR_PlotsCentroid_Block::run2D(const std::vector<double>& src, std::vector<double>& dst)
{
    const int R = m_RangeBinNum;
    const int D = m_DopplerBinNum;
    const int L = m_effectiveSampleNum;

    std::vector<unsigned char> visited(static_cast<size_t>(L), 0u);

    // 4 邻域：上下左右（不含对角线）
    const int dr[4] = { 0, -1, 1,  0 };
    const int dd[4] = { -1,  0, 0,  1 };

    for (int d0 = 0; d0 < D; ++d0)
    {
        for (int r0 = 0; r0 < R; ++r0)
        {
            const int seed = idx2D(r0, d0);

            if (visited[static_cast<size_t>(seed)] != 0u)
                continue;

            visited[static_cast<size_t>(seed)] = 1u;

            if (!isPositive(src[static_cast<size_t>(seed)]))
                continue;

            double sumW  = 0.0;
            double sumRW = 0.0;
            double sumDW = 0.0;
            double maxVal = src[static_cast<size_t>(seed)];

            std::queue<int> q;
            q.push(seed);

            while (!q.empty())
            {
                const int idx = q.front();
                q.pop();

                const int d = idx / R;
                const int r = idx - d * R;

                const double w = src[static_cast<size_t>(idx)];
                sumW  += w;
                sumRW += static_cast<double>(r) * w;
                sumDW += static_cast<double>(d) * w;
                if (w > maxVal) maxVal = w;

                for (int k = 0; k < 4; ++k)
                {
                    const int rn = r + dr[k];
                    const int dn = d + dd[k];

                    if (rn < 0 || rn >= R || dn < 0 || dn >= D)
                        continue;

                    const int ni = idx2D(rn, dn);
                    const size_t nsz = static_cast<size_t>(ni);

                    if (visited[nsz] != 0u)
                        continue;

                    visited[nsz] = 1u;

                    if (isPositive(src[nsz]))
                        q.push(ni);
                }
            }

            if (sumW > 0.0)
            {
                int rc = roundToNearestIndex(sumRW / sumW);
                int dc = roundToNearestIndex(sumDW / sumW);

                rc = clampInt(rc, 0, R - 1);
                dc = clampInt(dc, 0, D - 1);

                const int outIdx = idx2D(rc, dc);

                if (maxVal > dst[static_cast<size_t>(outIdx)])
                    dst[static_cast<size_t>(outIdx)] = maxVal;
            }
        }
    }
}

// ============================================================================
// 工具函数
// ============================================================================

int RADAR_PlotsCentroid_Block::idx2D(int rangeIndex, int dopplerIndex) const
{
    return dopplerIndex * m_RangeBinNum + rangeIndex;
}

bool RADAR_PlotsCentroid_Block::isPositive(double x) const
{
    return x > 0.0;
}

int RADAR_PlotsCentroid_Block::roundToNearestIndex(double x)
{
    if (!(x == x)) return 0;  // NaN guard
    return static_cast<int>(std::floor(x + 0.5));
}

int RADAR_PlotsCentroid_Block::clampInt(int x, int lo, int hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}
