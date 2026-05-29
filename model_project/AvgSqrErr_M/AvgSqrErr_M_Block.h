#ifndef AVGSQRERR_M_BLOCK_H
#define AVGSQRERR_M_BLOCK_H

#include "Block.h"
#include "AvgSqrErr_M.h"

#include <memory>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API AvgSqrErr_M_Block : public SystemVueModelBuilder::Block
{
public:
    AvgSqrErr_M_Block(const std::string& name);
    ~AvgSqrErr_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool DataStreamRun();

    std::unique_ptr<AvgSqrErr_M> m_AvgSqrErr_M;
    int m_NumInputsToAverage;

    // 矩阵尺寸状态（用于滑动窗口）
    int m_rows;
    int m_cols;
    bool m_shapeInit;

    // 滑动窗口状态
    std::vector<double> m_ring;
    int m_head;
    double m_accumSSE;
    int m_count;
};

RegAlgo(AvgSqrErr_M_Block);

#endif // AVGSQRERR_M_BLOCK_H
