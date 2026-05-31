#ifndef TIMESYNCHRONIZER_BLOCK_H
#define TIMESYNCHRONIZER_BLOCK_H


#include "Block.h"
#include "TimeSynchronizer.h"
#include <memory>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API TimeSynchronizer_Block : public SystemVueModelBuilder::Block
{
public:
    TimeSynchronizer_Block(const std::string& name);
    ~TimeSynchronizer_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    TimeSynchronizer::ModeEnum ConvertStringToModeEnum(const std::string& value);

    void SetDefaultParameters();
    void SetParameters();

    std::unique_ptr<TimeSynchronizer> m_TimeSynchronizer;
    TimeSynchronizer::ModeEnum m_Mode;

    struct SampleD { double v; double t; };

    std::vector<std::deque<SampleD>> fifos_;
    std::vector<double> lastValue_;
    int N_ = 0;

    static inline double eps() { return 1e-15; }
};

RegAlgo(TimeSynchronizer_Block);
#endif // TIMESYNCHRONIZER_BLOCK_H
