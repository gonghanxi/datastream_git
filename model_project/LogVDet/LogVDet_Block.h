#ifndef LOGVDET_BLOCK_H
#define LOGVDET_BLOCK_H
#include "LogVDet.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API LogVDet_Block : public Block
{
public:
    LogVDet_Block(const std::string& name);
    ~LogVDet_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<LogVDet> m_log;

    double Sensitivity;
    double PMin;
    double E;
    double Ec;
    double RefR;
};

#endif // LOGVDET_BLOCK_H
