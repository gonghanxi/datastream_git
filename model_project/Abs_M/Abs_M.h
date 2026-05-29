#pragma once
#include "ModelBuilder.h"
#include "Matrix.h"

namespace SystemVueModelBuilder
{
    class SYSTEMVUEMODELBUILDER_API Abs_M : public SystemVueModelBuilder::DFModel
	{
	public:
		CircularBuffer< Matrix<double> > input;
		CircularBuffer< Matrix<double> > output;

		DECLARE_MODEL_INTERFACE(Abs_M);

		Abs_M() = default;

		bool Initialize();
		bool Run();
		bool Finalize();
	};
}
