#ifndef SINK_BLOCK_H
#define SINK_BLOCK_H

#include "Block.h"
#include "EnvelopeSignal.h"
#include <fstream>
#include "json.hpp"

class Sink_Block : public Block
{
public:
    Sink_Block(const std::string& name);
    ~Sink_Block();

    void Setup() override;
    void Run() override;
    void SetCharacterizationFrequency(double fc);

    void PrintParameters() const;

private:
    double m_fc;
    std::vector<SystemVueModelBuilder::EnvelopeSignal> m_envelopeData;
    std::vector<std::chrono::microseconds> m_timestamps;
    size_t m_totalSamplesProcessed;
    bool m_fcExtracted;

    void Initialize();
    void ProcessEnvelopeData();
    void WriteJsonOutput();
};

#endif // SINK_BLOCK_H
