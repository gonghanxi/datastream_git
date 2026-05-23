#pragma once
#include "ModelBuilder.h"
#include "DFModel.h"
#include "TimedCircularBuffer.h"
#include "EnvelopeSignal.h"
#include "SystemVue.h"
#include <complex>
#include <cmath>
#include <algorithm>


class SYSTEMVUEMODELBUILDER_API FreqMpyDiv : public SystemVueModelBuilder::DFModel {
public:
	enum MultDivEnum { MD_Multiplier = 0, MD_Divider = 1 };
	enum OperatorTypeEnum { OP_Full = 0, OP_PhaseOnly = 1 };

	DECLARE_MODEL_INTERFACE(FreqMpyDiv);
	FreqMpyDiv();

	bool Setup() override;
	bool Run() override;
	bool PropagateCharacterizationFrequency();

	SystemVueModelBuilder::EnvelopeCircularBuffer input;
	SystemVueModelBuilder::EnvelopeCircularBuffer control;
	SystemVueModelBuilder::EnvelopeCircularBuffer output;

	MultDivEnum MultDiv;
	double NominalX;
	double MaxX;
	double MinX;
	OperatorTypeEnum OperatorType;

private:
	double fc_in_ = 0.0;
	double x_nom_ = 1.0;
	double fc_out_ = 0.0;

	static inline double clamp(double x, double lo, double hi) {
		return (x < lo ? lo : (x > hi ? hi : x));
	}
	inline double g(double X) const {
		if (MultDiv == MD_Multiplier) return fc_in_ * X;
		return (X > 0.0 ? fc_in_ / X : 0.0);
	}
};
