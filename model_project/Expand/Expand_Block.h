#ifndef EXPAND_BLOCK_H
#define EXPAND_BLOCK_H
#include "Expand.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Expand_Block : public Block
{
public:
    Expand_Block(const std::string& name);
    ~Expand_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    Expand::SelectedCompressionType ConvertStringToSelectedCompressionType(const std::string& value);
    void SetDefaultParameters();

    std::unique_ptr<Expand> m_Expand;
    Expand::SelectedCompressionType m_CompressionType;
    double m_CompressionK;
    double m_Max;

};
RegAlgo(Expand_Block);

#endif // EXPAND_BLOCK_H
