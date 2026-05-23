#ifndef LOGAMP_BLOCK_H
#define LOGAMP_BLOCK_H

#include "Block.h"
#include "LogAmp.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API LogAmp_Block : public Block
{
public:
    LogAmp_Block(const std::string& name);
    ~LogAmp_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<LogAmp> m_LogAmp;

    double m_Sensitivity;
    double m_PMin;
    double m_E;
    double m_Ec;
    double m_RefR;
};
RegAlgo(LogAmp_Block);
#endif // LOGAMP_BLOCK_H
