#ifndef TIMEDELAYENV_BLOCK_H
#define TIMEDELAYENV_BLOCK_H

#include "Block.h"
#include "TimeDelayEnv.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API TimeDelayEnv_Block : public SystemVueModelBuilder::Block
{
public:
	TimeDelayEnv_Block(const std::string& name);
	~TimeDelayEnv_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	TimeDelayEnv::UnitEnum ConvertStringToUnitEnum(const std::string& value);
	void SetDefaultParamters();
	void SetParameters();
	bool UpdateLatency();
	void UpdateCharacterizationFrequency();

	TimeDelayEnv::UnitEnum m_unit;
	double m_T;
	int m_N;
	double m_delaySeconds;
	bool m_latencyReady;

	std::unique_ptr<TimeDelayEnv> m_timeDelayEnv;
	SimuParameter simulator_param;
};
RegAlgo(TimeDelayEnv_Block);
#endif // TIMEDELAYENV_BLOCK_H
