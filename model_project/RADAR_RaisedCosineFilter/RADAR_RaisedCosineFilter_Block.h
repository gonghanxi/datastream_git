#ifndef RADAR_RAISEDCOSINEFILTER_BLOCK_H
#define RADAR_RAISEDCOSINEFILTER_BLOCK_H

#include "Block.h"
#include "RADAR_RaisedCosineFilter.h"

#include <complex>
#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_RaisedCosineFilter_Block : public Block
{
public:
	typedef std::complex<double> Cx;

	RADAR_RaisedCosineFilter_Block(const std::string& name);
	~RADAR_RaisedCosineFilter_Block() = default;

	bool Setup()      override;
	bool Run()        override;
	bool Initialize() override;

private:
	void SetDefaultParameters();
	void SetParameters();
	bool validateAndPrepare();
	bool DataStreamRun();
	bool TimeDrivenRun();

	// ---- algorithm instance ----
	std::unique_ptr<RADAR_RaisedCosineFilter> m_algo;

	// ---- parameters ----
	double m_Alpha;
	double m_PRI;
	int    m_FilterLen;
	double m_SampleRate;

	// ---- derived ----
	int m_numPRI;

	// ---- TimeDrivenRun buffers ----
	std::vector<EnvelopeSignal> m_inputBuffer;
	std::queue<EnvelopeSignal>  m_outputQueue;
	int                         m_inputCount;
};

RegAlgo(RADAR_RaisedCosineFilter_Block);

#endif // RADAR_RAISEDCOSINEFILTER_BLOCK_H
