#ifndef UNPACK_M_BLOCK_H
#define UNPACK_M_BLOCK_H

#include "Block.h"
#include "Unpack_M.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Unpack_M_Block : public Block
{
public:
    Unpack_M_Block(const std::string& name);
    ~Unpack_M_Block() = default;

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    Unpack_M::SelectedFormat ConvertStringToSelectedFormat(const std::string& value);
    void SetDefaultParameters();

    std::unique_ptr<Unpack_M> m_Unpack_M;

    int m_NumRows;
    int m_NumCols;
    Unpack_M::SelectedFormat m_Format;
};
RegAlgo(Unpack_M_Block);
#endif // UNPACK_M_BLOCK_H
