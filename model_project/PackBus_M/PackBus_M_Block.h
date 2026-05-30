#ifndef PACKBUS_M_BLOCK_H
#define PACKBUS_M_BLOCK_H

#include "Block.h"
#include "PackBus_M.h"

#include <memory>
#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API PackBus_M_Block : public SystemVueModelBuilder::Block
{
public:
    PackBus_M_Block(const std::string& name);
    ~PackBus_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    PackBus_M::SelectedFormat ConvertStringToFormat(const std::string& value);

    int m_NumRows;
    int m_NumCols;
    PackBus_M::SelectedFormat m_Format;  // 0=ColumnMajor, 1=RowMajor

    std::unique_ptr<PackBus_M> m_PackBus_M;
};

RegAlgo(PackBus_M_Block);

#endif // PACKBUS_M_BLOCK_H
