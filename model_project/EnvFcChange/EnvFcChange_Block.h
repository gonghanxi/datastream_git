#ifndef ENVFCCHANGE_BLOCK_H
#define ENVFCCHANGE_BLOCK_H

#include "Block.h"
#include "EnvFcChange.h"
#include <limits>

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API EnvFcChange_Block : public SystemVueModelBuilder::Block
{
public:
	EnvFcChange_Block(const std::string& name);
	~EnvFcChange_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters(double outputFc, double bandwidth);
	void UpdateCharacterizationFrequency();

	static inline double clip(double x, double lo, double hi)
	{
		return (x < lo ? lo : (x > hi ? hi : x));
	}

	double m_outputFc;
	double m_bandwidth;

	double m_ts;
	double m_lastTime;
	double m_alpha;
	double m_iLp;
	double m_qLp;

	std::unique_ptr<EnvFcChange> m_envFcChange;
};
RegAlgo(EnvFcChange_Block);
#endif // ENVFCCHANGE_BLOCK_H
