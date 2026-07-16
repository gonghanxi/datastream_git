#ifndef RADAR_MULTICH_TX_BLOCK_H
#define RADAR_MULTICH_TX_BLOCK_H

#include "Block.h"
#include "RADAR_MultiCH_Tx.h"

#include <complex>
#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_MultiCH_Tx_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_MultiCH_Tx_Block(const std::string& name);
    ~RADAR_MultiCH_Tx_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;
    int  GetBusChannelCount() const override { return m_NumOfCH; }

    void SetDefaultParameters();
    void SetParameters();

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    // 解析 ImbalanceCoef 字符串
    static bool parseComplexArray(const std::string& str, std::vector<std::complex<double>>& out);

    void rebuildCache();

    std::unique_ptr<RADAR_MultiCH_Tx> m_algo;

    // ===== 参数 =====
    int     m_NumOfCH;
    double  m_TStep;
    double  m_FCarrier;

    // ===== 缓存 =====
    int m_nChExpected;
    std::vector<std::complex<double>> m_imbCache;

    // ===== TimeDrivenRun 逐点累积 =====
    std::vector<std::complex<double>> m_inputBuffer;
    std::queue<EnvelopeSignal> m_outputQueue;
};

RegAlgo(RADAR_MultiCH_Tx_Block);

#endif // RADAR_MULTICH_TX_BLOCK_H
