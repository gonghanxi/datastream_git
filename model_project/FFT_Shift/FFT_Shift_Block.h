#ifndef FFT_SHIFT_BLOCK_H
#define FFT_SHIFT_BLOCK_H

#include "Block.h"
#include "FFT_Shift.h"

#include <complex>
#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API FFT_Shift_Block : public SystemVueModelBuilder::Block
{
public:
    FFT_Shift_Block(const std::string& name);
    ~FFT_Shift_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

    void SetParameters();

private:
    FFT_Shift::SelectedDirection ConvertStringToDirection(const std::string& value);
    void SetDefaultParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<FFT_Shift> m_FFT_Shift;

    int m_FFTSize;
    FFT_Shift::SelectedDirection m_Direction;

    // 时间驱动缓冲
    std::vector<std::complex<double>> m_inputBuffer;
    std::queue<std::complex<double>>  m_outputQueue;
};

RegAlgo(FFT_Shift_Block);

#endif // FFT_SHIFT_BLOCK_H
