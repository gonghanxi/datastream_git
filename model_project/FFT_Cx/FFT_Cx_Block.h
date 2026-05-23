#ifndef FFT_CX_BLOCK_H
#define FFT_CX_BLOCK_H

#include "Block.h"
#include "FFT_Cx.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API FFT_Cx_Block : public SystemVueModelBuilder::Block
{
public:
	FFT_Cx_Block(const std::string& name);
	~FFT_Cx_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	FFT_Cx::SelectedDirection ConvertStringToDirection(const std::string& value);
	FFT_Cx::SelectedFreqSequence ConvertStringToFreqSequence(const std::string& value);
	void SetDefaultParamters();
	void SetParameters(int fftSize, int size, FFT_Cx::SelectedDirection dir, FFT_Cx::SelectedFreqSequence seq);

	int m_fftSize;
	int m_size;
	FFT_Cx::SelectedDirection m_direction;
	FFT_Cx::SelectedFreqSequence m_freqSequence;

	std::unique_ptr<FFT_Cx> m_fftCx;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(FFT_Cx_Block);
#endif // FFT_CX_BLOCK_H
