#ifndef MUX_BLOCK_H
#define MUX_BLOCK_H

#include "Block.h"
#include "Mux.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Mux_Block : public SystemVueModelBuilder::Block
{
public:
    Mux_Block(const std::string& name);
    ~Mux_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<Mux> m_Mux;

    int m_BlockSize;

    // 时间驱动缓冲
    std::vector<double> m_inputBuffer;
    std::vector<int>    m_controlBuffer;
    std::queue<double>  m_outputQueue;
};

RegAlgo(Mux_Block);

#endif // MUX_BLOCK_H
