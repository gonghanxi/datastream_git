#ifndef MAXMIN_BLOCK_H
#define MAXMIN_BLOCK_H

#include "Block.h"
#include "MaxMin.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API MaxMin_Block : public SystemVueModelBuilder::Block
{
public:
    MaxMin_Block(const std::string& name);
    ~MaxMin_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

    void SetParameters();

private:
    void SetDefaultParameters();
    bool DataStreamRun();
    bool TimeDrivenRun();

    MaxMin::SelectedMaxOrMin  ConvertStringToMaxOrMin(const std::string& value);
    MaxMin::SelectedCompare   ConvertStringToCompare(const std::string& value);
    MaxMin::SelectedOutputType ConvertStringToOutputType(const std::string& value);

    std::unique_ptr<MaxMin> m_MaxMin;

    int m_N;
    MaxMin::SelectedMaxOrMin  m_MaxOrMin;
    MaxMin::SelectedCompare   m_Compare;
    MaxMin::SelectedOutputType m_OutputType;

    // 时间驱动缓冲
    std::vector<double> m_inputBuffer;
    std::queue<double>  m_outputQueue;
    std::queue<int>     m_indexQueue;
};

RegAlgo(MaxMin_Block);

#endif // MAXMIN_BLOCK_H
