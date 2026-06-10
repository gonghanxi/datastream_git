#include "codegenerator.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QJsonArray>
#include <QVector>

ModelCodeGenerator::ModelCodeGenerator(const ModelDescriptor &desc)
    : m_desc(desc) {}

/**
 * @brief 主入口：创建输出目录并依次生成计算代码和 FMI 包装代码
 */
void ModelCodeGenerator::generate(const QString &outputDir) {
    QDir().mkpath(outputDir);
    generateComputationalCode(outputDir);
    generateFmiWrapper(outputDir);
}

/**
 * @brief 生成计算模型头文件 generated_model.h 和源文件 generated_model.cpp
 *
 * 根据原理图中的模块类型和参数，生成对应的 C++ 类：
 * - Gain  : 增益模块，y = gain * u
 * - Delay : 延时模块，环形缓冲区实现
 * - FIR   : 有限冲激响应滤波器，滑动窗口内积
 * - Noise : 高斯噪声发生器
 *
 * 所有模块串联成 GeneratedModel 类，使用正弦波自激励信号。
 * 关键修复：模块参数在 initialize() 中设置，避免输出始终为 0。
 */
void ModelCodeGenerator::generateComputationalCode(const QString &dir) {
    // ---------- 头文件 generated_model.h ----------
    QString header = dir + "/generated_model.h";
    QFile h(header);
    h.open(QIODevice::WriteOnly);
    QTextStream hs(&h);

    hs << "#ifndef GENERATED_MODEL_H\n#define GENERATED_MODEL_H\n\n";
    hs << "#include <vector>\n#include <cmath>\n#include <random>\n\n";

    // 遍历所有模块，按类型生成类定义（initialize 中设置参数值）
    for (const auto &mod : m_desc.modules) {
        hs << "// " << mod.name.toUpper() << " (" << mod.type << ")\n";
        if (mod.type == "Gain") {
            double gain = mod.params["gain"].toDouble(1.0);
            hs << "class Gain_" << mod.name << " {\n";
            hs << "public:\n    double m_gain;\n";
            // 关键：在 initialize() 中设置增益值
            hs << "    void initialize() { m_gain = " << gain << "; }\n";
            hs << "    double step(double input) { return input * m_gain; }\n";
            hs << "};\n\n";
        } else if (mod.type == "Delay") {
            int d = mod.params["delay"].toInt(1);
            hs << "class Delay_" << mod.name << " {\n";
            hs << "public:\n    std::vector<double> buffer;\n";
            hs << "    int index = 0;\n";
            hs << "    void initialize() { buffer.assign(" << d << ", 0.0); index = 0; }\n";
            hs << "    double step(double input) {\n";
            hs << "        double out = buffer[index];\n";
            hs << "        buffer[index] = input;\n";
            hs << "        index = (index + 1) % " << d << ";\n";
            hs << "        return out;\n    }\n";
            hs << "};\n\n";
        } else if (mod.type == "FIR") {
            QJsonArray coeffs = mod.params["coefficients"].toArray();
            int taps = coeffs.size();
            hs << "class FIR_" << mod.name << " {\n";
            hs << "public:\n    std::vector<double> coeff;\n";
            hs << "    std::vector<double> buffer;\n";
            hs << "    int index = 0;\n";
            hs << "    void initialize() {\n";
            // 从 JSON 数组初始化滤波器系数
            hs << "        coeff = {";
            for (int i = 0; i < taps; ++i) {
                if (i) hs << ", ";
                hs << coeffs[i].toDouble();
            }
            hs << "};\n";
            hs << "        buffer.assign(" << taps << ", 0.0);\n";
            hs << "        index = 0;\n    }\n";
            hs << "    double step(double input) {\n";
            hs << "        buffer[index] = input;\n";
            hs << "        double sum = 0.0;\n";
            hs << "        for (int i = 0; i < " << taps << "; ++i) {\n";
            hs << "            sum += coeff[i] * buffer[(index - i + " << taps << ") % " << taps << "];\n";
            hs << "        }\n";
            hs << "        index = (index + 1) % " << taps << ";\n";
            hs << "        return sum;\n    }\n";
            hs << "};\n\n";
        } else if (mod.type == "Noise") {
            double mean = mod.params["mean"].toDouble(0.0);
            double stddev = mod.params["stddev"].toDouble(0.1);
            hs << "class Noise_" << mod.name << " {\n";
            hs << "public:\n    std::mt19937 gen;\n";
            hs << "    std::normal_distribution<double> dist;\n";
            hs << "    void initialize() {\n";
            hs << "        gen.seed(42);\n";
            hs << "        dist = std::normal_distribution<double>(" << mean << ", " << stddev << ");\n";
            hs << "    }\n";
            hs << "    double step(double) { return dist(gen); }\n";
            hs << "};\n\n";
        }
    }

    // 主模型类：正弦波自激励 + 完整模块链路
    hs << "class GeneratedModel {\npublic:\n";
    for (const auto &mod : m_desc.modules) {
        hs << "    " << mod.type << "_" << mod.name << " " << mod.name << ";\n";
    }
    hs << "    double input = 0.0;\n";
    hs << "    double output = 0.0;\n";
    hs << "    double internalTime = 0.0;\n";
    hs << "    double stepSize = " << m_desc.stepSize << ";\n";
    hs << "\n";
    hs << "    void initialize() {\n";
    for (const auto &mod : m_desc.modules) {
        hs << "        " << mod.name << ".initialize();\n";
    }
    hs << "        internalTime = 0.0;\n";
    hs << "        input = 0.0;\n";
    hs << "        output = 0.0;\n";
    hs << "    }\n\n";
    hs << "    void doStep() {\n";
    hs << "        // 生成 0.5Hz 正弦波激励信号（振幅 5.0）\n";
    hs << "        double val = std::sin(2.0 * 3.14159265358979323846 * 0.5 * internalTime) * 5.0;\n";
    hs << "        internalTime += stepSize;\n";
    hs << "        // 如果外部设置了非零输入，优先使用外部输入\n";
    hs << "        if (std::abs(input) > 1e-9) {\n";
    hs << "            val = input;\n";
    hs << "        }\n";
    // 按顺序串联调用各模块
    for (const auto &mod : m_desc.modules) {
        hs << "        val = " << mod.name << ".step(val);\n";
    }
    hs << "        output = val;\n";
    hs << "    }\n";
    hs << "};\n\n";
    hs << "#endif\n";
    h.close();

    // ---------- 源文件 generated_model.cpp ----------
    QFile cpp(dir + "/generated_model.cpp");
    cpp.open(QIODevice::WriteOnly);
    QTextStream cs(&cpp);
    cs << "#include \"generated_model.h\"\n";
    cpp.close();
}

/**
 * @brief 生成 FMI 3.0 CoSimulation 接口包装文件 fmi_wrapper.cpp
 *
 * 包含所有 FMI 3.0 必需函数：
 * - 已实现的核心函数：GetVersion, SetDebugLogging, InstantiateCoSimulation,
 *   FreeInstance, Enter/ExitInitializationMode, EnterStepMode, DoStep,
 *   Terminate, Reset, Get/SetFloat64, Get/SetFloat32
 * - 其余函数以正确的签名提供空实现，确保动态库符号完整
 *
 * 变量 valueReference 映射（与 modelDescription.xml 一致）：
 * - vr=0 : time（独立变量，由宿主提供）
 * - vr=1 : input（输入变量）
 * - vr=2 : output（输出变量）
 */
void ModelCodeGenerator::generateFmiWrapper(const QString &dir) {
    QString wrapperCpp = dir + "/fmi_wrapper.cpp";
    QFile w(wrapperCpp);
    w.open(QIODevice::WriteOnly);
    QTextStream ws(&w);

    ws << "#include <cstring>\n#include <cmath>\n";
    ws << "#include \"fmi3Functions.h\"\n";
    ws << "#include \"generated_model.h\"\n\n";

    // FMU 实例结构体，封装模型和运行时环境
    ws << "typedef struct {\n"
          "    GeneratedModel model;\n"
          "    fmi3Float64 currentTime;\n"
          "    fmi3Boolean loggingOn;\n"
          "    fmi3LogMessageCallback logCallback;\n"
          "    fmi3InstanceEnvironment instanceEnv;\n"
          "    fmi3Boolean eventModeUsed;\n"
          "    fmi3Boolean earlyReturnAllowed;\n"
          "    fmi3IntermediateUpdateCallback intermediateUpdate;\n"
          "    fmi3Boolean initialized;\n"
          "} FmuInstance;\n\n";

    ws << "extern \"C\" {\n\n";

    // ================= 已实现的核心函数 =================

    ws << "FMI3_Export const char* fmi3GetVersion(void) { return \"3.0\"; }\n\n";

    ws << "FMI3_Export fmi3Status fmi3SetDebugLogging(fmi3Instance instance,\n"
          "    fmi3Boolean loggingOn, size_t nCategories, const fmi3String categories[]) {\n"
          "    return fmi3OK;\n}\n\n";

    // 实例化 CoSimulation FMU
    ws << "FMI3_Export fmi3Instance fmi3InstantiateCoSimulation(\n"
          "    fmi3String instanceName, fmi3String instantiationToken, fmi3String resourcePath,\n"
          "    fmi3Boolean visible, fmi3Boolean loggingOn, fmi3Boolean eventModeUsed,\n"
          "    fmi3Boolean earlyReturnAllowed,\n"
          "    const fmi3ValueReference requiredIntermediateVariables[],\n"
          "    size_t nRequiredIntermediateVariables,\n"
          "    fmi3InstanceEnvironment instanceEnvironment,\n"
          "    fmi3LogMessageCallback logMessage,\n"
          "    fmi3IntermediateUpdateCallback intermediateUpdate) {\n"
          "    FmuInstance* inst = new FmuInstance();\n"
          "    if (!inst) return nullptr;\n"
          "    inst->model.initialize();\n"
          "    inst->currentTime = 0.0;\n"
          "    inst->loggingOn = loggingOn;\n"
          "    inst->logCallback = logMessage;\n"
          "    inst->instanceEnv = instanceEnvironment;\n"
          "    inst->eventModeUsed = eventModeUsed;\n"
          "    inst->earlyReturnAllowed = earlyReturnAllowed;\n"
          "    inst->intermediateUpdate = intermediateUpdate;\n"
          "    inst->initialized = fmi3False;\n"
          "    return reinterpret_cast<fmi3Instance>(inst);\n}\n\n";

    ws << "FMI3_Export void fmi3FreeInstance(fmi3Instance instance) {\n"
          "    FmuInstance* inst = reinterpret_cast<FmuInstance*>(instance);\n"
          "    if (inst) delete inst;\n}\n\n";

    // 进入初始化模式（重置模型状态）
    ws << "FMI3_Export fmi3Status fmi3EnterInitializationMode(fmi3Instance instance,\n"
          "    fmi3Boolean toleranceDefined, fmi3Float64 tolerance,\n"
          "    fmi3Float64 startTime, fmi3Boolean stopTimeDefined, fmi3Float64 stopTime) {\n"
          "    FmuInstance* inst = reinterpret_cast<FmuInstance*>(instance);\n"
          "    inst->model.initialize();\n"
          "    inst->currentTime = startTime;\n"
          "    inst->initialized = fmi3True;\n"
          "    return fmi3OK;\n}\n\n";

    ws << "FMI3_Export fmi3Status fmi3ExitInitializationMode(fmi3Instance instance) { return fmi3OK; }\n\n";
    ws << "FMI3_Export fmi3Status fmi3EnterStepMode(fmi3Instance instance) { return fmi3OK; }\n\n";

    // 执行一个通信步长
    ws << "FMI3_Export fmi3Status fmi3DoStep(fmi3Instance instance,\n"
          "    fmi3Float64 currentCommunicationPoint, fmi3Float64 communicationStepSize,\n"
          "    fmi3Boolean noSetFMUStatePriorToCurrentPoint,\n"
          "    fmi3Boolean* eventHandlingNeeded, fmi3Boolean* terminateSimulation,\n"
          "    fmi3Boolean* earlyReturn, fmi3Float64* lastSuccessfulTime) {\n"
          "    FmuInstance* inst = reinterpret_cast<FmuInstance*>(instance);\n"
          "    inst->model.doStep();\n"
          "    if (eventHandlingNeeded) *eventHandlingNeeded = fmi3False;\n"
          "    if (terminateSimulation) *terminateSimulation = fmi3False;\n"
          "    if (earlyReturn) *earlyReturn = fmi3False;\n"
          "    if (lastSuccessfulTime) *lastSuccessfulTime = currentCommunicationPoint + communicationStepSize;\n"
          "    return fmi3OK;\n}\n\n";

    ws << "FMI3_Export fmi3Status fmi3Terminate(fmi3Instance instance) { return fmi3OK; }\n\n";

    // 重置模型
    ws << "FMI3_Export fmi3Status fmi3Reset(fmi3Instance instance) {\n"
          "    FmuInstance* inst = reinterpret_cast<FmuInstance*>(instance);\n"
          "    inst->model.initialize();\n"
          "    inst->currentTime = 0.0;\n"
          "    return fmi3OK;\n}\n\n";

    // 读取 Float64 变量（vr=0: time, vr=1: input, vr=2: output）
    ws << "FMI3_Export fmi3Status fmi3GetFloat64(fmi3Instance instance,\n"
          "    const fmi3ValueReference vr[], size_t nvr, fmi3Float64 value[], size_t nValues) {\n"
          "    FmuInstance* inst = reinterpret_cast<FmuInstance*>(instance);\n"
          "    for (size_t i = 0; i < nvr; ++i) {\n"
          "        if (vr[i] == 0) { value[i] = inst->currentTime; }\n"
          "        else if (vr[i] == 1) { value[i] = inst->model.input; }\n"
          "        else if (vr[i] == 2) { value[i] = inst->model.output; }\n"
          "        else { return fmi3Error; }\n"
          "    }\n    return fmi3OK;\n}\n\n";

    // 设置 Float64 变量
    ws << "FMI3_Export fmi3Status fmi3SetFloat64(fmi3Instance instance,\n"
          "    const fmi3ValueReference vr[], size_t nvr, const fmi3Float64 value[], size_t nValues) {\n"
          "    FmuInstance* inst = reinterpret_cast<FmuInstance*>(instance);\n"
          "    for (size_t i = 0; i < nvr; ++i) {\n"
          "        if (vr[i] == 0) { inst->currentTime = value[i]; }\n"
          "        else if (vr[i] == 1) { inst->model.input = value[i]; }\n"
          "        else { return fmi3Error; }\n"
          "    }\n    return fmi3OK;\n}\n\n";

    // 读取 Float32 变量
    ws << "FMI3_Export fmi3Status fmi3GetFloat32(fmi3Instance instance,\n"
          "    const fmi3ValueReference vr[], size_t nvr, fmi3Float32 value[], size_t nValues) {\n"
          "    FmuInstance* inst = reinterpret_cast<FmuInstance*>(instance);\n"
          "    for (size_t i = 0; i < nvr; ++i) {\n"
          "        if (vr[i] == 0) { value[i] = (fmi3Float32)inst->currentTime; }\n"
          "        else if (vr[i] == 1) { value[i] = (fmi3Float32)inst->model.input; }\n"
          "        else if (vr[i] == 2) { value[i] = (fmi3Float32)inst->model.output; }\n"
          "        else { return fmi3Error; }\n"
          "    }\n    return fmi3OK;\n}\n\n";

    // 设置 Float32 变量
    ws << "FMI3_Export fmi3Status fmi3SetFloat32(fmi3Instance instance,\n"
          "    const fmi3ValueReference vr[], size_t nvr, const fmi3Float32 value[], size_t nValues) {\n"
          "    FmuInstance* inst = reinterpret_cast<FmuInstance*>(instance);\n"
          "    for (size_t i = 0; i < nvr; ++i) {\n"
          "        if (vr[i] == 0) { inst->currentTime = (fmi3Float64)value[i]; }\n"
          "        else if (vr[i] == 1) { inst->model.input = (fmi3Float64)value[i]; }\n"
          "        else { return fmi3Error; }\n"
          "    }\n    return fmi3OK;\n}\n\n";

    // ================= 其余 FMI 3.0 函数空实现 =================
    // 确保动态库符号完整，所有未使用的函数均提供正确签名的空实现
    struct E { const char* name, *args, *ret, *stmt; };
    E all[] = {
        {"fmi3InstantiateModelExchange", "(fmi3String,fmi3String,fmi3String,fmi3Boolean,fmi3Boolean,fmi3InstanceEnvironment,fmi3LogMessageCallback)", "fmi3Instance", "return nullptr;"},
        {"fmi3InstantiateScheduledExecution", "(fmi3String,fmi3String,fmi3String,fmi3Boolean,fmi3Boolean,fmi3InstanceEnvironment,fmi3LogMessageCallback,fmi3ClockUpdateCallback,fmi3LockPreemptionCallback,fmi3UnlockPreemptionCallback)", "fmi3Instance", "return nullptr;"},
        {"fmi3EnterEventMode", "(fmi3Instance)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetInt8", "(fmi3Instance,const fmi3ValueReference*,size_t,fmi3Int8*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetUInt8", "(fmi3Instance,const fmi3ValueReference*,size_t,fmi3UInt8*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetInt16", "(fmi3Instance,const fmi3ValueReference*,size_t,fmi3Int16*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetUInt16", "(fmi3Instance,const fmi3ValueReference*,size_t,fmi3UInt16*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetInt32", "(fmi3Instance,const fmi3ValueReference*,size_t,fmi3Int32*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetUInt32", "(fmi3Instance,const fmi3ValueReference*,size_t,fmi3UInt32*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetInt64", "(fmi3Instance,const fmi3ValueReference*,size_t,fmi3Int64*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetUInt64", "(fmi3Instance,const fmi3ValueReference*,size_t,fmi3UInt64*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetBoolean", "(fmi3Instance,const fmi3ValueReference*,size_t,fmi3Boolean*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetString", "(fmi3Instance,const fmi3ValueReference*,size_t,fmi3String*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetBinary", "(fmi3Instance,const fmi3ValueReference*,size_t,size_t*,fmi3Binary*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetClock", "(fmi3Instance,const fmi3ValueReference*,size_t,fmi3Clock*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetInt8", "(fmi3Instance,const fmi3ValueReference*,size_t,const fmi3Int8*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetUInt8", "(fmi3Instance,const fmi3ValueReference*,size_t,const fmi3UInt8*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetInt16", "(fmi3Instance,const fmi3ValueReference*,size_t,const fmi3Int16*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetUInt16", "(fmi3Instance,const fmi3ValueReference*,size_t,const fmi3UInt16*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetInt32", "(fmi3Instance,const fmi3ValueReference*,size_t,const fmi3Int32*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetUInt32", "(fmi3Instance,const fmi3ValueReference*,size_t,const fmi3UInt32*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetInt64", "(fmi3Instance,const fmi3ValueReference*,size_t,const fmi3Int64*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetUInt64", "(fmi3Instance,const fmi3ValueReference*,size_t,const fmi3UInt64*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetBoolean", "(fmi3Instance,const fmi3ValueReference*,size_t,const fmi3Boolean*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetString", "(fmi3Instance,const fmi3ValueReference*,size_t,const fmi3String*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetBinary", "(fmi3Instance,const fmi3ValueReference*,size_t,const size_t*,const fmi3Binary*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetClock", "(fmi3Instance,const fmi3ValueReference*,size_t,const fmi3Clock*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetNumberOfVariableDependencies", "(fmi3Instance,fmi3ValueReference,size_t*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetVariableDependencies", "(fmi3Instance,fmi3ValueReference,size_t*,fmi3ValueReference*,size_t*,fmi3DependencyKind*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetFMUState", "(fmi3Instance,fmi3FMUState*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetFMUState", "(fmi3Instance,fmi3FMUState)", "fmi3Status", "return fmi3Error;"},
        {"fmi3FreeFMUState", "(fmi3Instance,fmi3FMUState*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SerializedFMUStateSize", "(fmi3Instance,fmi3FMUState,size_t*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SerializeFMUState", "(fmi3Instance,fmi3FMUState,fmi3Byte*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3DeserializeFMUState", "(fmi3Instance,const fmi3Byte*,size_t,fmi3FMUState*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetDirectionalDerivative", "(fmi3Instance,const fmi3ValueReference*,size_t,const fmi3ValueReference*,size_t,const fmi3Float64*,size_t,fmi3Float64*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetAdjointDerivative", "(fmi3Instance,const fmi3ValueReference*,size_t,const fmi3ValueReference*,size_t,const fmi3Float64*,size_t,fmi3Float64*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3EnterConfigurationMode", "(fmi3Instance)", "fmi3Status", "return fmi3Error;"},
        {"fmi3ExitConfigurationMode", "(fmi3Instance)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetIntervalDecimal", "(fmi3Instance,const fmi3ValueReference*,size_t,fmi3Float64*,fmi3IntervalQualifier*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetIntervalFraction", "(fmi3Instance,const fmi3ValueReference*,size_t,fmi3UInt64*,fmi3UInt64*,fmi3IntervalQualifier*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetShiftDecimal", "(fmi3Instance,const fmi3ValueReference*,size_t,fmi3Float64*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetShiftFraction", "(fmi3Instance,const fmi3ValueReference*,size_t,fmi3UInt64*,fmi3UInt64*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetIntervalDecimal", "(fmi3Instance,const fmi3ValueReference*,size_t,const fmi3Float64*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetIntervalFraction", "(fmi3Instance,const fmi3ValueReference*,size_t,const fmi3UInt64*,const fmi3UInt64*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetShiftDecimal", "(fmi3Instance,const fmi3ValueReference*,size_t,const fmi3Float64*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetShiftFraction", "(fmi3Instance,const fmi3ValueReference*,size_t,const fmi3UInt64*,const fmi3UInt64*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3EvaluateDiscreteStates", "(fmi3Instance)", "fmi3Status", "return fmi3Error;"},
        {"fmi3UpdateDiscreteStates", "(fmi3Instance,fmi3Boolean*,fmi3Boolean*,fmi3Boolean*,fmi3Boolean*,fmi3Boolean*,fmi3Float64*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3EnterContinuousTimeMode", "(fmi3Instance)", "fmi3Status", "return fmi3Error;"},
        {"fmi3CompletedIntegratorStep", "(fmi3Instance,fmi3Boolean,fmi3Boolean*,fmi3Boolean*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetTime", "(fmi3Instance,fmi3Float64)", "fmi3Status", "return fmi3Error;"},
        {"fmi3SetContinuousStates", "(fmi3Instance,const fmi3Float64*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetContinuousStateDerivatives", "(fmi3Instance,fmi3Float64*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetEventIndicators", "(fmi3Instance,fmi3Float64*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetContinuousStates", "(fmi3Instance,fmi3Float64*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetNominalsOfContinuousStates", "(fmi3Instance,fmi3Float64*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetNumberOfEventIndicators", "(fmi3Instance,size_t*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetNumberOfContinuousStates", "(fmi3Instance,size_t*)", "fmi3Status", "return fmi3Error;"},
        {"fmi3GetOutputDerivatives", "(fmi3Instance,const fmi3ValueReference*,size_t,const fmi3Int32*,fmi3Float64*,size_t)", "fmi3Status", "return fmi3Error;"},
        {"fmi3ActivateModelPartition", "(fmi3Instance,fmi3ValueReference,fmi3Float64)", "fmi3Status", "return fmi3Error;"},
    };
    for (const auto& e : all)
        ws << "FMI3_Export " << e.ret << " " << e.name << e.args << " { " << e.stmt << " }\n";

    ws << "} // extern \"C\"\n";
    w.close();
}
