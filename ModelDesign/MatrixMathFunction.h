// Copyright 2011 - 2014 Keysight Technologies, Inc   
#pragma once

#include "Matrix.h"
#include "CircularBuffer.h"
  
 
namespace SystemVueModelBuilder 
{
	
	template <typename T>
	bool Matrix_Inverse(const Matrix<T>& source,  Matrix<T>& result)
	{
		_ASSERT( source.IsMatrix());
		size_t nRows = source.NumRows();
		size_t nCols = source.NumColumns();

		if(nRows != nCols || source.NumElements() == 0) 
		{
			_ASSERT( false); // this should have been caught by the caller
			return false;	
		}

		result.Resize(nRows,nCols);

		Matrix<T> work;
		work = source;
		size_t row, col;
		T temp;

		// set result to be the identity matrix
		result.identity();

		for(size_t i = 0; i < nRows; i++) {
			// check that the element in (i,i) is not zero
			if(work(i,i) == T(0)) {
				// swap with a row below this one that has a non-zero element
				// in the same column
				for(row = i+1; row < nRows; row++)
					if(work(row,i) != (T)0)
						break;
				if(row == nRows) {
					return false;
				}
				// swap rows
				for(col = 0; col < nRows; col++) {
					temp = work(i,col);
					work(i,col) = work(row,col);
					work(row,col) = temp;
					temp = result(i,col);
					result(i,col) = result(row,col);
					result(row,col) = temp;
				}
			}
			// divide every element in the row by element (i,i)
			temp = work(i,i);
			for(col = 0; col < nRows; col++) {
				work(i,col) /= temp;
				result(i,col) /= temp;
			}
			// zero out the rest of column i
			for(row = 0; row < nRows; row++) {
				if(row != i) {
					temp = work(row,i);
					col = nRows;
					_ASSERT(col > 0);
					do
					{
						--col;
						work(row,col) -= (temp * work(i,col));
						result(row,col) -= (temp * result(i,col));
					} while (col > 0);	// stop after processing col==0
				}
			}
		}
		return true;
	}

	template <typename T> void transpose(const Matrix<T>& input, Matrix<T>& output)
	{
		output.Resize( input.NumColumns(), input.NumRows());

		if ( input.NumElements())
		{
			size_t i, j;

			for (i=0; i < input.NumRows(); i++)
				for (j=0; j < input.NumColumns(); j++)
					output(j,i) = input(i,j);
		}
	}

	
	template <typename T>
	bool Sum_Matrix_Elements(const Matrix<T>& source,  T& result)
	{
		_ASSERT( source.IsMatrix());
				
		size_t NumElements = source.NumElements();

		result = 0;

		for(size_t i = 0; i < NumElements; i++)
		{
			result += source(i);
		}

		return true;
	}

	template <typename T>
	bool Elementwise_Matrix_Product(const Matrix<T>& mat1,  const Matrix<T>& mat2, Matrix<T>& result)
	{
		_ASSERT( mat1.IsMatrix());
		_ASSERT( mat2.IsMatrix());
		

		size_t nRows = mat1.NumRows();
		size_t nCols = mat1.NumColumns();				
		size_t NumElements = nRows * nCols;		

		result.Resize(nRows,nCols);

		for(size_t i = 0; i < NumElements; i++)
		{
			result(i) = mat1(i)*mat2(i);
		}

		return true;
	}

		
		

}
