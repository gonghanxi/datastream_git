// Copyright  2011 - 2015 Keysight Technologies, Inc   
#pragma once
#include <stdint.h>

namespace SystemVueModelBuilder
{
	struct FixedPointStruct
	{
		uint64_t** val;
		size_t size;
		bool* isNegative;

		FixedPointStruct(size_t wl, size_t arraySize)
		{
			isNegative = 0;
			val = 0;
			size = (wl + 63)/64;
			val = new uint64_t*[arraySize];
			isNegative = new bool[arraySize];
			for (size_t i = 0; i < arraySize; i++)
				val[i] = new uint64_t[size]();
		}

		~FixedPointStruct()
		{
			if (isNegative)
				delete[] isNegative;

			if (val)
			{
				size_t arraySize = sizeof(val) / (sizeof(*val));
				for (size_t i = 0; i < arraySize; i++)
					delete[] val[i];
				delete[] val;
			}
		}
	};

}


