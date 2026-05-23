// Copyright  2011 - 2016 Keysight Technologies, Inc   
#pragma once

#include "CircularBuffer.h"
#include "Matrix.h"

namespace SystemVueModelBuilder
{
	/* ********************************************************************** */
	// Bool
	/* ********************************************************************** */
	/// Circular buffer for BoolMatrix data type
	typedef CircularBuffer<BoolMatrix> BoolMatrixCircularBuffer;
	/// Circular buffer bus for BoolMatrix data type
	typedef CircularBufferBusT<BoolMatrixCircularBuffer> BoolMatrixCircularBufferBus;


	/* ********************************************************************** */
	// Char
	/* ********************************************************************** */
	/// Circular buffer for CharMatrix data type
	typedef CircularBuffer<CharMatrix> CharMatrixCircularBuffer;
	/// Circular buffer bus for CharMatrix data type
	typedef CircularBufferBusT<CharMatrixCircularBuffer> CharMatrixCircularBufferBus;


	/* ********************************************************************** */
	// Integer
	/* ********************************************************************** */
	/// Circular buffer for IntMatrix data type
	typedef CircularBuffer<IntMatrix> IntMatrixCircularBuffer;
	/// Circular buffer bus for IntMatrix data type
	typedef CircularBufferBusT<IntMatrixCircularBuffer> IntMatrixCircularBufferBus;


	/* ********************************************************************** */
	// Double
	/* ********************************************************************** */
	/// Circular buffer for DoubleMatrix data type
	typedef CircularBuffer<DoubleMatrix> DoubleMatrixCircularBuffer;
	/// Circular buffer bus for DoubleMatrix data type
	typedef CircularBufferBusT<DoubleMatrixCircularBuffer> DoubleMatrixCircularBufferBus;


	/* ********************************************************************** */
	// Complex double
	/* ********************************************************************** */
	/// Circular buffer for DComplexMatrix data type
	typedef CircularBuffer<DComplexMatrix> DComplexMatrixCircularBuffer;
	/// Circular buffer bus for DComplexMatrix data type
	typedef CircularBufferBusT<DComplexMatrixCircularBuffer> DComplexMatrixCircularBufferBus;

	/* ********************************************************************** */
	// Float
	/* ********************************************************************** */
	/// Circular buffer for FloatMatrix data type
	typedef CircularBuffer<FloatMatrix> FloatMatrixCircularBuffer;
	/// Circular buffer bus for FloatMatrix data type
	typedef CircularBufferBusT<FloatMatrixCircularBuffer> FloatMatrixCircularBufferBus;

	/* ********************************************************************** */
	// Complex float 
	/* ********************************************************************** */
	/// Circular buffer for FComplexMatrix data type
	typedef CircularBuffer<FComplexMatrix> FComplexMatrixCircularBuffer;
	/// Circular buffer bus for FComplexMatrix data type
	typedef CircularBufferBusT<FComplexMatrixCircularBuffer> FComplexMatrixCircularBufferBus;
}
