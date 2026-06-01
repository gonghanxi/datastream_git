#ifndef ORDERTWOINT_BLOCK_H
#define ORDERTWOINT_BLOCK_H

#include "Block.h"
#include "OrderTwoInt.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API OrderTwoInt_Block : public SystemVueModelBuilder::Block
{
public:
    OrderTwoInt_Block(const std::string& name);
    ~OrderTwoInt_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<OrderTwoInt> m_OrderTwoInt;

    // 时间驱动缓冲
    std::vector<bool> m_upperBuffer;
    std::vector<bool> m_lowerBuffer;
    std::queue<bool>  m_greaterQueue;
    std::queue<bool>  m_lesserQueue;
};

RegAlgo(OrderTwoInt_Block);

#endif // ORDERTWOINT_BLOCK_H
