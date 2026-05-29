#ifndef TIMESYNCHRONIZER_BLOCK_H
#define TIMESYNCHRONIZER_BLOCK_H

#include "Block.h"
#include "TimeSynchronizer.h"

#include <memory>
#include <string>
#include <vector>
#include <deque>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API TimeSynchronizer_Block : public SystemVueModelBuilder::Block
{
public:
    TimeSynchronizer_Block(const std::string& name);
    ~TimeSynchronizer_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool DataStreamRun();

    TimeSynchronizer::ModeEnum ConvertStringToModeEnum(const std::string& value);

    std::unique_ptr<TimeSynchronizer> m_TimeSynchronizer;
    TimeSynchronizer::ModeEnum m_Mode;

    SimuParameter m_simuParam;
    int           m_sampleCount;
    double        m_sampleRate;

    struct SampleD { double v; double t; };
    std::vector<std::deque<SampleD>> fifos_;
    std::vector<double> lastValue_;
    int N_ = 0;

    static inline double eps() { return 1e-15; }
};

RegAlgo(TimeSynchronizer_Block);

#endif // TIMESYNCHRONIZER_BLOCK_H
