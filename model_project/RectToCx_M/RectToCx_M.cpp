#include "RectToCx_M.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RectToCx_M )
{	
	SET_MODEL_DESCRIPTION("Convert real(I) and imaginary(Q) parts to complex signal");
	SET_MODEL_SYMBOL("SYM_RectToCx");
	SET_MODEL_CATEGORY("Matrix Scalar");
	SET_MODEL_CATEGORY("Type Converters");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(real);
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(imag);
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}
	return true;
}
#endif

RectToCx_M::RectToCx_M()
{
	
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RectToCx_M::Run()
{
	// 分别获取两个输入端口的矩阵大小，若端口未连接则视作1x1
	int NRowReal = real.IsConnected() ? real[0].NumRows() : 1;
	int NColReal = real.IsConnected() ? real[0].NumColumns() : 1;
	int NRowImag = imag.IsConnected() ? imag[0].NumRows() : 1;
	int NColImag = imag.IsConnected() ? imag[0].NumColumns() : 1;

	// 行列取两者最大值作为输出矩阵大小
	int NRow = NRowReal > NRowImag ? NRowReal : NRowImag;
	int NCol = NColReal > NColImag ? NColReal : NColImag;

	output[0].Resize(NRow, NCol);

	for (int row = 0; row < NRow; row++)
	{
		for (int col = 0; col < NCol; col++)
		{
			// 输出默认值补0
			output[0](row, col) = 0.0;

			// 若实部输入端口已连接，则将实部数据填入输出矩阵实部的前 NRowReal 行 NColReal 列
			if (real.IsConnected())
			{
				if (row < NRowReal && col < NColReal)
				{
					output[0](row, col).real(real[0](row, col));
				}
			}

			// 若虚部输入端口已连接，则将虚部数据填入输出矩阵虚部的前 NRowImag 行 NColImag 列
			if (imag.IsConnected())
			{
				if (row < NRowImag && col < NColImag)
				{
					output[0](row, col).imag(imag[0](row, col));
				}
			}
		}
	}
	return true;
}
