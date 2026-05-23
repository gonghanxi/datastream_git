#ifndef LIMIT_BLOCK_H
#define LIMIT_BLOCK_H
#include "Limit.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Limit_Block : public Block
{
public:
    Limit_Block(const std::string& name);
    ~Limit_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    Limit::SelectedLimiterType ConvertStringToSelectedLimiterType(const std::string& value);
    void SetDefaultParameters();

    std::unique_ptr<Limit> m_Limit;
    double K;
    double Bottom;
    double Top;
    Limit::SelectedLimiterType LimiterType;

};
RegAlgo(Limit_Block);


#endif // LIMIT_BLOCK_H
