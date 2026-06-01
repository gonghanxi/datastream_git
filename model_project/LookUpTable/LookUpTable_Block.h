#ifndef LOOKUPTABLE_BLOCK_H
#define LOOKUPTABLE_BLOCK_H

#include "Block.h"
#include "LookUpTable.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API LookUpTable_Block : public SystemVueModelBuilder::Block
{
public:
    LookUpTable_Block(const std::string& name);
    ~LookUpTable_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<LookUpTable> m_LookUpTable;

    // Block 内部的查表数据（从参数解析）
    std::vector<double> m_values;

    // 时间驱动缓冲
    std::vector<int>   m_inputBuffer;
    std::queue<double> m_outputQueue;
};

RegAlgo(LookUpTable_Block);

#endif // LOOKUPTABLE_BLOCK_H
