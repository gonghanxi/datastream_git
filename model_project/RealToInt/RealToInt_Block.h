#ifndef REALTOINT_BLOCK_H
#define REALTOINT_BLOCK_H

#include "Block.h"
#include "RealToInt.h"

#include <memory>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RealToInt_Block : public SystemVueModelBuilder::Block
{
public:
    RealToInt_Block(const std::string& name);
    ~RealToInt_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool DataStreamRun();

    RealToInt::SelectedConvertType ConvertStringToConvertType(const std::string& value);

    // ---- algorithm instance ----
    std::unique_ptr<RealToInt> m_algo;

    // ---- parameters ----
    RealToInt::SelectedConvertType m_convertType;
};

RegAlgo(RealToInt_Block);

#endif // REALTOINT_BLOCK_H
