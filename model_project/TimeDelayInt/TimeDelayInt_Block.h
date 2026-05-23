#ifndef TIMEDELAYINT_BLOCK_H
#define TIMEDELAYINT_BLOCK_H

#include "Block.h"
#include "TimeDelayInt.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API TimeDelayInt_Block : public SystemVueModelBuilder::Block
{
public:
	TimeDelayInt_Block(const std::string& name);
	~TimeDelayInt_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	TimeDelayInt::UnitEnum ConvertStringToUnitEnum(const std::string& value);
	void SetDefaultParamters();
	void SetParameters();
	bool UpdateLatency();

	TimeDelayInt::UnitEnum m_unit;
	double m_T;
	int m_N;
	double m_delaySeconds;
	bool m_latencyReady;

	std::unique_ptr<TimeDelayInt> m_timeDelayInt;
	SimuParameter simulator_param;
};
RegAlgo(TimeDelayInt_Block);
#endif // TIMEDELAYINT_BLOCK_H
