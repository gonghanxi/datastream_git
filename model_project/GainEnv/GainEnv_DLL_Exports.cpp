#include "GainEnv_Block.h"

extern "C" __declspec(dllexport) SystemVueModelBuilder::Block* createAlgorithm() {
    return new GainEnv_Block("GainEnv_Block");
}

extern "C" __declspec(dllexport) const char* getAlgorithmName() {
    return "GainEnv_Block";
}
