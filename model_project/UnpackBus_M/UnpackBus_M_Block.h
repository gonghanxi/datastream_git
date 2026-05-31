#ifndef UNPACKBUS_M_BLOCK_H
#define UNPACKBUS_M_BLOCK_H

#include "Block.h"
#include "UnpackBus_M.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API UnpackBus_M_Block : public SystemVueModelBuilder::Block
{
public:
    UnpackBus_M_Block(const std::string& name);
    ~UnpackBus_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    UnpackBus_M::SelectedFormat ConvertStringToFormat(const std::string& value);

    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<UnpackBus_M> m_UnpackBus_M;

    int m_NumRows;
    int m_NumCols;
    UnpackBus_M::SelectedFormat m_Format;

    // 时间驱动缓冲
    std::vector<SystemVueModelBuilder::DoubleMatrix> m_inputBuffer;
    std::queue<double> m_outputQueue;
};

RegAlgo(UnpackBus_M_Block);

#endif // UNPACKBUS_M_BLOCK_H
