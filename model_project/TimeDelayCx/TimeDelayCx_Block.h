#ifndef TIMEDELAYCX_BLOCK_H
#define TIMEDELAYCX_BLOCK_H

#include "Block.h"
#include "TimeDelayCx.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API TimeDelayCx_Block : public SystemVueModelBuilder::Block
{
public:
	TimeDelayCx_Block(const std::string& name);
	~TimeDelayCx_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	TimeDelayCx::UnitEnum ConvertStringToUnitEnum(const std::string& value);
	void SetDefaultParamters();
	void SetParameters();
	bool UpdateLatency();

	TimeDelayCx::UnitEnum m_unit;
	double m_T;
	int m_N;
	double m_delaySeconds;
	bool m_latencyReady;

	std::unique_ptr<TimeDelayCx> m_timeDelayCx;
	SimuParameter simulator_param;
};
RegAlgo(TimeDelayCx_Block);
#endif // TIMEDELAYCX_BLOCK_H
