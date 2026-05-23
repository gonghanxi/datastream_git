#include "DataStreamVerification.h"
#include "Block.h"
#include <iostream>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <complex>
#include <queue>

using namespace SystemVueModelBuilder;

DataStreamVerification::DataStreamVerification()
    :m_feasible(true),m_nextVariableId(1)
{
    //初始化
}

DataStreamVerification::~DataStreamVerification()
{
    //清空
    clear();
}

DataStreamVerification::BlockVariable *DataStreamVerification::registerBlock(Block *block)
{
    if(!block)
        return nullptr;

    //检查是否已注册
    auto it = m_blockToVariable.find(block);
    if(it != m_blockToVariable.end()) {
        return it->second;
    }

    //创建变量并注册
    BlockVariable* var = new BlockVariable();
    var->block = block;
    var->blockName = block->GetName();
    var->variableId = m_nextVariableId++;
    var->description = "Block: " + block->GetName();

    m_variables.push_back(var);
    m_blockToVariable[block] = var;
    m_idToVariable[var->variableId] = var;

    qDebug() << "Registered block variable: x" << var->variableId
             << " for " << QString::fromStdString(var->blockName);

    return var;
}

void DataStreamVerification::addVerificationEquation(Block *upstream, Block *downstream, double upstreamCoeff, double downstreamCoeff, double constant, const std::string &description)
{
    //获取注册的BlockVariable
    BlockVariable* upstreamVar = registerBlock(upstream);
    BlockVariable* downstreamVar = registerBlock(downstream);

    //创建校验方程结构体变量，并添加
    VerificationParam vparams;
    vparams.equationId = static_cast<int>(m_vparams.size()) + 1;
    vparams.upstreamVar = upstreamVar;
    vparams.downstreamVar = downstreamVar;
    vparams.upstreamCoeff = upstreamCoeff;
    vparams.downstreamCoeff = downstreamCoeff;
    vparams.constant = constant;
    vparams.description = description;

    //将参数加入容器
    m_vparams.push_back(vparams);

    // 记录双向连接关系
    m_upstreamToDownstreamMap.insert({upstream, downstream});
    m_downstreamToUpstreamMap.insert({downstream, upstream});

    qDebug() << "Added VParams" << vparams.equationId << ": "
             << QString::fromStdString(vparams.toString());
}

bool DataStreamVerification::CheckFeasibility()
{
    qDebug() << "=== Checking DataStream Verification System ===";
    qDebug() << "Total variables:" << m_variables.size();
    qDebug() << "Total params:" << m_vparams.size();

    debugPrintEquations();

    // 检查是否有bus类型的block
    bool hasBusTypeBlock = false;
    std::vector<Block*> busTypeBlocks;

    // 遍历所有注册的block，检查是否是bus类型
    for (const auto& var : m_variables) {
        if (var->block) {
            bool isBus = false;
            // 遍历block的所有输入端口检查是否是bus类型
            for(size_t i = 0; i < var->block->GetInputPortCount(); i++) {
                std::string portName = var->block->GetInputPortName(i);
                BufferReader* inputReader = var->block->GetInputPort(portName);

                if (inputReader) {
                    DataType inputDataType = inputReader->GetDataType();
                    if (inputReader->IsBusType(inputDataType)) {
                        isBus = true;
                        break;
                    }
                }
            }

            if (isBus) {
                hasBusTypeBlock = true;
                busTypeBlocks.push_back(var->block);
                qDebug() << "Found bus-type block: " << QString::fromStdString(var->blockName);
            }
        }
    }

    // 如果没有bus类型block，直接返回true通过校验
    if (!hasBusTypeBlock) {
        qDebug() << "No bus-type blocks found. System is trivially feasible (skipping verification).";
        m_feasible = true;
        m_errorMessage = "No bus-type blocks - system is trivially feasible";
        return true;
    }

    qDebug() << "Found " << busTypeBlocks.size() << " bus-type block(s). Proceeding with verification.";

    // 没有校验方程，直接返回true
    if(m_vparams.empty()) {
        m_feasible = true;
        m_errorMessage = "No verification params to check";
        qDebug() << "No verification params - system is trivially feasible";
        return true;
    }

    // 添加连接一致性约束（只对bus类型）
//    addConnectionConsistencyConstraints();

    // 构建方程系统
    buildEquationSystem();

    // 打印矩阵信息
    printMatrixInfo();

    // 检查并求解
    bool canSolve = solveLinearSystem();

    if(canSolve) {
        // 验证解
        bool valid = verifySolution();

        if(valid) {
            m_feasible = true;
            m_errorMessage = "System is feasible"; // 有解
            qDebug() << "Verification System is feasible";

            // 打印解
            printSolutions();
            return true;
        }
        else {
            // 解不满足所有方程
            m_feasible = false;
            m_errorMessage = "Solution does not satisfy all equations";
            qDebug() << "Solution Verification failed";
            return false;
        }
    }
    else {
        // 无解
        m_feasible = false;
        qDebug() << "Verification System is infeasible";
        qDebug() << "Error: " << m_errorMessage;
        return false;
    }
}

const std::vector<DataStreamVerification::VerificationParam> &DataStreamVerification::getVParams() const
{
    return m_vparams;
}

const std::vector<DataStreamVerification::BlockVariable *> &DataStreamVerification::getVariables() const
{
    return m_variables;
}

void DataStreamVerification::clear()
{
    for(auto var : m_variables) {
        delete var;
    }
    m_variables.clear();
    m_blockToVariable.clear();
    m_vparams.clear();
    m_idToVariable.clear();
    m_nextVariableId = 1;
    m_feasible = true;
    m_errorMessage.clear();

    //清空连接映射
    m_upstreamToDownstreamMap.clear();
    m_downstreamToUpstreamMap.clear();

    //清空矩阵
    m_coefficientMatrix.init(0,0);
    m_constantVector.init(0,0);
    m_solution.init(0,0);
}

void DataStreamVerification::printVerification() const
{
    qDebug() << "=== DataStream Verification System ===";
    qDebug() << "Variables: ";
    //打印变量
    for(const auto& var : m_variables) {
        qDebug() << "  x" << var->variableId << ": "
                 << QString::fromStdString(var->blockName);
    }
    qDebug() << "Verification Params:";
    //打印检验参数
    for (const auto& param : m_vparams) {
        qDebug() << "  Eq" << param.equationId << ": "
                 << QString::fromStdString(param.toString()) << "  ("
                 << QString::fromStdString(param.description) << ")";
    }
}

void DataStreamVerification::printSolutions() const
{
    int solutionSize = m_solution.rows();
    qDebug() << "Solution vector x (" << solutionSize << "x1):";

    //打印未知数
    if(solutionSize <= 10) {
        for(int i = 0; i < solutionSize; ++i) {
            auto val = m_solution.get(i, 0);
            qDebug() << "x[" << i << "] = (" << val.real() << ", " << val.imag() << ")";
        }
    }

    qDebug() << "Variable solutions:";
    //打印变量结果
    for(const auto& var : m_variables) {
        double value = getVariableSolution(var->variableId);
        qDebug() << " x" << var->variableId << " ("
                 << QString::fromStdString(var->blockName) << ") = " << value;
    }
}

bool DataStreamVerification::isFeasible() const
{
    return m_feasible;
}

const QString &DataStreamVerification::getErrorMessage() const
{
    return m_errorMessage;
}

double DataStreamVerification::getVariableSolution(int variableId) const
{
    if(variableId < 1 || variableId > m_solution.rows()) {
        return 0.0;
    }
    return m_solution.getReal(variableId - 1, 0);
}

double DataStreamVerification::getVariableSolution(Block *block) const
{
    auto it = m_blockToVariable.find(block);
    if(it == m_blockToVariable.end()) {
        return 0.0;
    }
    return getVariableSolution(it->second->variableId);
}

void DataStreamVerification::buildEquationSystem()
{
    //获取方程数与未知数
    int numVars = static_cast<int>(m_variables.size());
    int numEq = static_cast<int>(m_vparams.size());

    qDebug() << "Building Equation System: " << numEq << " equations, "
             << numVars << " variables";

    if(numEq == 0 || numVars == 0) {
        m_coefficientMatrix.init(0, numVars);
        m_constantVector.init(0, 1);
        return;
    }

    //初始化矩阵
    m_coefficientMatrix.init(numEq, numVars);
    m_constantVector.init(numEq, 1);
    m_solution.init(numVars, 1);

    //初始化为零
    m_coefficientMatrix.initZero(numEq, numVars);
    m_constantVector.initZero(numEq, 1);
    m_solution.initZero(numVars, 1);

    //填充系数矩阵和常数向量
    for(int eqIndex = 0; eqIndex < numEq; eqIndex++) {
        const auto& params = m_vparams[eqIndex];

        //设置上游变量系数（列索引从0开始，变量ID从1开始）
        int upstreamCol = params.upstreamVar->variableId - 1;
        m_coefficientMatrix.setReal(eqIndex, upstreamCol, params.upstreamCoeff);

        //设置下游变量系数
        int downstreamCol = params.downstreamVar->variableId - 1;
        m_coefficientMatrix.setReal(eqIndex, downstreamCol, params.downstreamCoeff);

        //设置常数项（实数）
        m_constantVector.setReal(eqIndex, 0, params.constant);
    }
}

bool DataStreamVerification::solveLinearSystem()
{
    int numEq = m_coefficientMatrix.rows();
    int numVars = m_coefficientMatrix.cols();

    qDebug() << "Solving linear system: " << numEq << "x" << numVars;

    // 检查是否为齐次方程组
    bool isHomogeneous = true;
    for(int i = 0; i < numEq && isHomogeneous; ++i) {
        if(std::abs(m_constantVector.getReal(i, 0)) > 1e-10) {
            isHomogeneous = false;
        }
    }

    // 如果是齐次方程组，直接寻找非零解
    if(isHomogeneous) {
        qDebug() << "Homogeneous system detected, looking for non-zero solution";
        return findNonZeroSolution();
    }

    // 情况1：方程数少于变量数 - 无穷多解
    if(numEq < numVars) {
        qDebug() << "Underdetermined system: more variables than equations";

        // 尝试找到满足所有方程的特解
        // 方法1：使用最小二乘法求解
        try {
            // 计算 A^T * A 和 A^T * b
            LapackMatWrapper AT(numVars, numEq);
            m_coefficientMatrix.transpose(AT);

            LapackMatWrapper ATA(numVars, numVars);
            AT.multiply(m_coefficientMatrix, ATA);

            LapackMatWrapper ATb(numVars, 1);
            AT.multiply(m_constantVector, ATb);

            // 求解最小范数解 (A^T * A) * x = A^T * b
            LapackMatWrapper x(numVars, 1);
            bool success = LapackMatWrapper::solveLinearSystem(ATA, ATb, x);

            if(success) {
                // 检查是否为全零解
                bool allZero = true;
                for(int i = 0; i < numVars; ++i) {
                    if(std::abs(x.getReal(i, 0)) > 1e-10) {
                        allZero = false;
                        break;
                    }
                }

                if(allZero) {
                    qDebug() << "Solution is all zero - trying to find non-trivial solution";

                    // 方法：添加额外的约束，使解不为零
                    // 例如，强制第一个变量为1
                    return findNonZeroSolution();
                } else {
                    m_solution = std::move(x);
                    qDebug() << "Found minimum-norm solution for underdetermined system";
                    return true;
                }
            } else {
                // 如果ATA奇异，尝试其他方法
                qDebug() << "ATA is singular, trying alternative approach";
                return findNonZeroSolution(); // 改为调用非零解查找
            }
        }
        catch(const std::exception& e) {
            m_errorMessage = QString("Error solving underdetermined system: ") + e.what();
            return false;
        }
    }

    // 情况2：方程数等于变量数 - 尝试求解
    if(numEq == numVars) {
        qDebug() << "Square system: " << numEq << " equations = " << numVars << " variables";

        // 尝试直接求解
        try {
            m_solution = m_constantVector;
            bool success = LapackMatWrapper::solveLinearSystem(m_coefficientMatrix, m_constantVector, m_solution);

            if(success) {
                // 检查是否为全零解
                bool allZero = true;
                for(int i = 0; i < numVars; ++i) {
                    if(std::abs(m_solution.getReal(i, 0)) > 1e-10) {
                        allZero = false;
                        break;
                    }
                }

                if(!allZero) {
                    qDebug() << "Successfully solved linear system";
                    return true;
                } else {
                    // 全零解，但可能有非零解（奇异矩阵情况）
                    qDebug() << "Got zero solution for singular matrix, looking for non-zero solution";
                    return findNonZeroSolution();
                }
            }
            else {
                // 奇异矩阵，寻找非零解
                qDebug() << "Linear system is singular, looking for non-zero solution";
                return findNonZeroSolution();
            }
        }
        catch(const std::exception& e) {
            m_errorMessage = QString("Error solving linear system: ") + e.what();
            return false;
        }
    }

    // 情况3：方程数多于变量数 - 最小二乘法
    if(numEq > numVars) {
        qDebug() << "Overdetermined system: more equations than variables";

        try {
            // 方法1：尝试使用正规方程 + 正则化
            LapackMatWrapper AT(numVars, numEq);
            m_coefficientMatrix.transpose(AT);

            LapackMatWrapper ATA(numVars, numVars);
            AT.multiply(m_coefficientMatrix, ATA);

            LapackMatWrapper ATb(numVars, 1);
            AT.multiply(m_constantVector, ATb);

            // 添加正则化项 (岭回归) 避免奇异矩阵
            double lambda = 1e-8;  // 小的正则化参数
            for(int i = 0; i < numVars; ++i) {
                double current = ATA.getReal(i, i);
                ATA.setReal(i, i, current + lambda);
            }

            LapackMatWrapper x(numVars, 1);
            bool success = LapackMatWrapper::solveLinearSystem(ATA, ATb, x);

            if(success) {
                // 验证解是否合理
                double maxResidual = 0.0;
                for(int eq = 0; eq < numEq; ++eq) {
                    double left = 0.0;
                    for(int var = 0; var < numVars; ++var) {
                        left += m_coefficientMatrix.getReal(eq, var) * x.getReal(var, 0);
                    }
                    double right = m_constantVector.getReal(eq, 0);
                    double residual = std::abs(left - right);
                    maxResidual = std::max(maxResidual, residual);
                }

                qDebug() << "Max residual after regularization:" << maxResidual;

                // 如果残差太大，可能正则化不够，尝试其他方法
                if(maxResidual > 1e-6) {
                    qDebug() << "Residual too large, trying non-zero solution approach";
                    return findNonZeroSolution();
                }

                m_solution = std::move(x);
                qDebug() << "Found regularized least squares solution";
                return true;
            }
            else {
                qDebug() << "Regularization failed, trying non-zero solution";
                return findNonZeroSolution();
            }
        }
        catch(const std::exception& e) {
            m_errorMessage = QString("Error in least squares: ") + e.what();
            return findNonZeroSolution();
        }
    }

    //未知情况
    qDebug() << "Unknown situation, return false";
    return false;
}

bool DataStreamVerification::verifySolution()
{
    if(m_solution.rows() == 0)
        return false;

    //获取方程个数
    int numEq = m_coefficientMatrix.rows();
    double maxError = 0.0;

    for(int eq = 0; eq < numEq; eq++) {
        // 计算方程左边：Σ(a_i * x_i)
        std::complex<double> leftSide = {0.0, 0.0};

        for(int var = 0; var < m_coefficientMatrix.cols(); var++) {
            std::complex<double> coeff = m_coefficientMatrix.get(eq, var);
            std::complex<double> x = m_solution.get(var, 0);

            leftSide += coeff * x;
        }

        //右边常数项
        std::complex<double> rightSide = m_constantVector.get(eq, 0);

        //计算误差
        double error = std::abs(leftSide - rightSide);
        if(error > maxError) {
            maxError = error;
        }

        //如果误差太大，说明解不满足方程
        if(error > 1e-6) {
            qDebug() << "Equation" << eq + 1 << "not satisfied: error =" << error;
            qDebug() << "Left side: " << leftSide.real() << " Right side: " << rightSide.real();
            return false;
        }
    }
    // 检查解是否为整数（或接近整数）
    bool allIntegral = true;
    for(int i = 0; i < m_solution.rows(); ++i) {
        double val = m_solution.getReal(i, 0);
        double rounded = std::round(val);
        if(std::abs(val - rounded) > 1e-6) {
            allIntegral = false;
            qDebug() << "Warning: x" << i+1 << " =" << val << " is not an integer";
        }
    }

    if(!allIntegral) {
        qDebug() << "Warning: Solution contains non-integer values";
    }

    qDebug() << "Solution verification passed. Max error:" << maxError;
    return true;
}

bool DataStreamVerification::findNonZeroSolution()
{
    int numVars = static_cast<int>(m_variables.size());
    int numEq = m_coefficientMatrix.rows();

    if(numVars == 0) return false;

    qDebug() << "Looking for non-zero INTEGER solution...";

    // 首先尝试寻找最小正整数解
    // 方法：通过图论方法构建等价类

    // 构建变量关系图
    std::vector<std::vector<std::pair<int, double>>> graph(numVars);
    std::vector<bool> visited(numVars, false);

    // 从方程中提取关系
    for(int eq = 0; eq < numEq; ++eq) {
        std::vector<int> vars;
        std::vector<double> coeffs;

        for(int var = 0; var < numVars; ++var) {
            double coeff = m_coefficientMatrix.getReal(eq, var);
            if(std::abs(coeff) > 1e-10) {
                vars.push_back(var);
                coeffs.push_back(coeff);
            }
        }

        // 只处理二元方程（你的大部分方程是这种形式）
        if(vars.size() == 2) {
            int v1 = vars[0];
            int v2 = vars[1];
            double c1 = coeffs[0];
            double c2 = coeffs[1];

            // 方程: c1*x1 + c2*x2 = 0 => x1/x2 = -c2/c1
            // 根据系数符号确定关系
            if(c1 * c2 < 0) {
                // 异号：x1 和 x2 成正比
                double ratio = -c2 / c1;
                graph[v1].push_back({v2, ratio});
                graph[v2].push_back({v1, 1.0/ratio});
                qDebug() << "Equation" << eq << ": x" << v1+1 << " =" << ratio << "* x" << v2+1;
            }
            else {
                // 同号：x1 和 x2 成反比
                double ratio = -c2 / c1;
                graph[v1].push_back({v2, ratio});
                graph[v2].push_back({v1, 1.0/ratio});
                qDebug() << "Equation" << eq << ": x" << v1+1 << " =" << ratio << "* x" << v2+1 << " (inverse)";
            }
        }
    }

    // 使用BFS为每个连通分量分配值
    std::vector<double> values(numVars, 0.0);
    std::vector<double> assignments(numVars, 0.0);

    for(int start = 0; start < numVars; ++start) {
        if(!visited[start]) {
            // BFS遍历连通分量
            std::queue<int> q;
            q.push(start);
            visited[start] = true;
            assignments[start] = 1.0;  // 赋值为1

            while(!q.empty()) {
                int current = q.front();
                q.pop();

                for(const auto& neighbor : graph[current]) {
                    int next = neighbor.first;
                    double ratio = neighbor.second;

                    if(!visited[next]) {
                        visited[next] = true;
                        assignments[next] = assignments[current] * ratio;
                        q.push(next);
                    }
                }
            }

            // 缩放这个连通分量，使其值尽可能为整数
            double minVal = std::numeric_limits<double>::max();
            for(int i = 0; i < numVars; ++i) {
                if(visited[i] && std::abs(assignments[i]) > 1e-10) {
                    if(std::abs(assignments[i]) < minVal) {
                        minVal = std::abs(assignments[i]);
                    }
                }
            }

            // 尝试找到使所有值为整数的最小缩放因子
            if(minVal < std::numeric_limits<double>::max()) {
                // 计算最小公倍数倒数
                double scale = 1.0 / minVal;

                // 检查缩放后是否接近整数
                bool allIntegral = true;
                for(int i = 0; i < numVars; ++i) {
                    if(visited[i]) {
                        double scaled = assignments[i] * scale;
                        double rounded = std::round(scaled);
                        if(std::abs(scaled - rounded) > 1e-6) {
                            allIntegral = false;
                            break;
                        }
                    }
                }

                if(allIntegral) {
                    // 使用缩放后的值
                    for(int i = 0; i < numVars; ++i) {
                        if(visited[i]) {
                            values[i] = std::round(assignments[i] * scale);
                        }
                    }
                } else {
                    // 如果不能得到整数，至少保证是正数
                    for(int i = 0; i < numVars; ++i) {
                        if(visited[i]) {
                            values[i] = std::abs(assignments[i]);
                        }
                    }
                }
            }
        }
    }

    // 创建解向量
    LapackMatWrapper x(numVars, 1);
    bool hasSolution = false;

    for(int i = 0; i < numVars; ++i) {
        if(std::abs(values[i]) > 1e-10) {
            x.setReal(i, 0, values[i]);
            hasSolution = true;
        } else {
            // 对于未分配值的变量，赋值为1
            x.setReal(i, 0, 1.0);
        }
    }

    if(hasSolution) {
        // 验证解
        if(verifySolutionWithMatrix(x)) {
            m_solution = std::move(x);

            // 输出调试信息
            qDebug() << "Found integer solution using graph method:";
            for(int i = 0; i < numVars; ++i) {
                qDebug() << "  x" << i+1 << " =" << m_solution.getReal(i, 0);
            }

            return true;
        }
    }

    // 如果图论方法失败，回退到原来的方法但改进整数性
    qDebug() << "Graph method failed, trying augmented system with integer constraint...";

    // 方法：添加约束使解为最小正整数
    try {
        LapackMatWrapper A_aug(numEq + 2, numVars);
        LapackMatWrapper b_aug(numEq + 2, 1);

        // 复制原始方程
        for(int i = 0; i < numEq; ++i) {
            for(int j = 0; j < numVars; ++j) {
                A_aug.set(i, j, m_coefficientMatrix.get(i, j));
            }
            b_aug.set(i, 0, m_constantVector.get(i, 0));
        }

        // 约束1：所有变量和为最小正整数
        for(int j = 0; j < numVars; ++j) {
            A_aug.set(numEq, j, 1.0);
        }
        b_aug.setReal(numEq, 0, numVars);  // 和为变量个数（保证每个变量至少为1）

        // 约束2：所有变量 ≥ 0.5（引导向正整数）
        for(int j = 0; j < numVars; ++j) {
            A_aug.set(numEq + 1, j, 1.0);
        }
        b_aug.setReal(numEq + 1, 0, 0.5 * numVars);

        LapackMatWrapper x_temp(numVars, 1);
        bool success = LapackMatWrapper::solveLinearSystem(A_aug, b_aug, x_temp);

        if(success) {
            // 尝试舍入到最接近的整数
            for(int i = 0; i < numVars; ++i) {
                double val = x_temp.getReal(i, 0);
                double rounded = std::round(val);

                // 如果接近整数，使用整数
                if(std::abs(val - rounded) < 0.1) {
                    x.setReal(i, 0, rounded);
                } else {
                    x.setReal(i, 0, val);
                }

                // 确保至少为1
                if(x.getReal(i, 0) < 0.5) {
                    x.setReal(i, 0, 1.0);
                }
            }

            if(verifySolutionWithMatrix(x)) {
                m_solution = std::move(x);
                qDebug() << "Found solution with integer rounding";
                return true;
            }
        }
    }
    catch(...) {
        // 忽略异常
    }

    // 最后的手段：使用特征值方法
    return findNonZeroSolutionFallback();
}

bool DataStreamVerification::findNonZeroSolutionFallback()
{
    // 使用原来的方法但最后尝试整数化
    int numVars = static_cast<int>(m_variables.size());

    // 构建 A^T * A
    LapackMatWrapper AT(numVars, m_coefficientMatrix.rows());
    m_coefficientMatrix.transpose(AT);

    LapackMatWrapper ATA(numVars, numVars);
    AT.multiply(m_coefficientMatrix, ATA);

    // 添加小扰动避免奇异
    for(int i = 0; i < numVars; ++i) {
        double current = ATA.getReal(i, i);
        ATA.setReal(i, i, current + 1e-8);
    }

    // 右边是 [1, 1, ..., 1]^T
    LapackMatWrapper b(numVars, 1);
    for(int i = 0; i < numVars; ++i) {
        b.setReal(i, 0, 1.0);
    }

    LapackMatWrapper x(numVars, 1);
    bool success = LapackMatWrapper::solveLinearSystem(ATA, b, x);

    if(success) {
        // 归一化到最小正整数
        double minPos = std::numeric_limits<double>::max();
        for(int i = 0; i < numVars; ++i) {
            double val = x.getReal(i, 0);
            if(val > 1e-10 && val < minPos) {
                minPos = val;
            }
        }

        if(minPos < std::numeric_limits<double>::max()) {
            double scale = 1.0 / minPos;
            for(int i = 0; i < numVars; ++i) {
                double scaled = x.getReal(i, 0) * scale;
                // 四舍五入到最接近的整数
                x.setReal(i, 0, std::round(scaled));
            }
        }

        if(verifySolutionWithMatrix(x)) {
            m_solution = std::move(x);
            qDebug() << "Found solution using fallback method";
            return true;
        }
    }

    m_errorMessage = "Cannot find non-zero solution";
    return false;
}

bool DataStreamVerification::verifySolutionWithMatrix(const LapackMatWrapper &x) const
{
    int numEq = m_coefficientMatrix.rows();
    int numVars = m_coefficientMatrix.cols();

    double maxError = 0.0;

    for(int eq = 0; eq < numEq; ++eq) {
        double left = 0.0;
        for(int var = 0; var < numVars; ++var) {
            left += m_coefficientMatrix.getReal(eq, var) * x.getReal(var, 0);
        }
        double right = m_constantVector.getReal(eq, 0);
        double error = std::abs(left - right);

        if(error > 1e-6) {
            qDebug() << "Equation" << eq << "not satisfied, error =" << error;
            return false;
        }
        maxError = std::max(maxError, error);
    }

    // 检查是否为全零解
    bool allZero = true;
    for(int i = 0; i < numVars; ++i) {
        if(std::abs(x.getReal(i, 0)) > 1e-10) {
            allZero = false;
            break;
        }
    }

    if(allZero) {
        qDebug() << "Solution is all zero (rejected)";
        return false;
    }

    qDebug() << "Solution verified with max error:" << maxError;
    return true;
}

void DataStreamVerification::printMatrixInfo() const
{
    int rows = m_coefficientMatrix.rows();
    int cols = m_coefficientMatrix.cols();

    qDebug() << "Verification Matrix A (" << rows << "x" << cols << "):";

    //打印矩阵的系数矩阵
    if(rows <= 10 && cols <= 10) {
        for(int i = 0; i < rows; i++) {
            QString rowStr;
            for(int j = 0; j < cols; j++) {
                auto val = m_coefficientMatrix.get(i, j);
                rowStr += QString("(%1, %2) ").arg(val.real()).arg(val.imag());
            }
            qDebug() << rowStr;
        }
    }

    int bRows = m_constantVector.rows();
    qDebug() << "Constant Vector b (" << bRows << "x1):";

    //打印矩阵的常数项
    if(bRows <= 10) {
        for(int i = 0; i < bRows; i++) {
            auto val = m_constantVector.get(i, 0);
            qDebug() << "(" << val.real() << ", " << val.imag() << ")";
        }
    }
}

void DataStreamVerification::debugPrintEquations() const
{
    qDebug() << "=== Debug: All Verification Equations ===";

    for(const auto& param : m_vparams) {
        qDebug() << "Eq" << param.equationId << ":";
        qDebug() << "  " << QString::fromStdString(param.toString());
        qDebug() << "  Upstream: x" << param.upstreamVar->variableId
                 << " (" << QString::fromStdString(param.upstreamVar->blockName) << ")";
        qDebug() << "  Downstream: x" << param.downstreamVar->variableId
                 << " (" << QString::fromStdString(param.downstreamVar->blockName) << ")";
        qDebug() << "  Coefficients: upstream =" << param.upstreamCoeff
                 << ", downstream =" << param.downstreamCoeff;
    }
}

void DataStreamVerification::addConnectionConsistencyConstraints()
{
    size_t originalParamCount = m_vparams.size();
    int addedConstraints = 0;

    qDebug() << "==== addConnectionConsistencyConstraints begin ====";

    // ========== 情况2：多个上游连接到同一个下游 ==========
    // 统计每个下游Block连接的所有上游Block
    std::map<Block*, std::vector<Block*>> downstreamConnections;

    // 下游 - 上游映射
    for (const auto& [downstream, upstream] : m_downstreamToUpstreamMap) {
        downstreamConnections[downstream].push_back(upstream);
    }

    // 对于每个有多个上游连接的下游Block
    for (const auto& [downstream, upstreamBlocks] : downstreamConnections) {
        // 只有有多个上游连接时才需要检查
        if (upstreamBlocks.size() <= 1) {
            continue;
        }

        // 检查下游Block是否为bus类型
        bool isDownstreamBusType = false;

        qDebug() << "Checking downstream block: " << QString::fromStdString(downstream->GetName());

        if (downstream) {
            // 遍历所有输入端口检查是否是bus类型
            for(size_t i = 0; i < downstream->GetInputPortCount(); i++) {
                std::string portName = downstream->GetInputPortName(i);
                BufferReader* inputReader = downstream->GetInputPort(portName);

                if (!inputReader) {
                    qDebug() << "  Warning: Can not find inputReader for port: " << QString::fromStdString(portName);
                    continue;
                }

                // 检查输入端是否为总线类型
                DataType inputDataType = inputReader->GetDataType();
                bool portIsBus = inputReader->IsBusType(inputDataType);

                qDebug() << "  Port " << QString::fromStdString(portName)
                         << " is bus type: " << (portIsBus ? "true" : "false");

                // 如果有一个端口是bus类型，就认为是bus类型
                if (portIsBus) {
                    isDownstreamBusType = true;
                    break; // 找到一个bus端口就足够
                }
            }

            qDebug() << "Downstream block " << QString::fromStdString(downstream->GetName())
                     << " overall is bus type: " << (isDownstreamBusType ? "true" : "false");
        }

        // 只有下游Block是bus类型时才添加约束
        if (!isDownstreamBusType) {
            qDebug() << "  Skipping constraints for non-bus block: " << QString::fromStdString(downstream->GetName());
            continue;
        }

        // 添加约束
        qDebug() << "Bus-type constraint: Block " << QString::fromStdString(downstream->GetName())
                 << " has " << upstreamBlocks.size()
                 << " upstream blocks. Adding equality constraints...";

        // 确保这些上游Block的所有解都相等
        for (size_t i = 0; i < upstreamBlocks.size(); i++) {
            for (size_t j = i + 1; j < upstreamBlocks.size(); j++) {
                Block* block_i = upstreamBlocks[i];
                Block* block_j = upstreamBlocks[j];

                // 获取变量
                auto it_i = m_blockToVariable.find(block_i);
                auto it_j = m_blockToVariable.find(block_j);

                if (it_i == m_blockToVariable.end() ||
                        it_j == m_blockToVariable.end()) {
                    continue;
                }

                BlockVariable* var_i = it_i->second;
                BlockVariable* var_j = it_j->second;

                // 添加方程：x_i - x_j = 0
                std::string desc = "Same bus downstream constraint: " +
                        block_i->GetName() +
                        " == " + block_j->GetName() +
                        " (both to bus-type " + downstream->GetName() + ")";

                // 创建新的VerificationParam
                VerificationParam newParam;
                newParam.equationId = static_cast<int>(m_vparams.size()) + 1;
                newParam.upstreamVar = var_i;        // x_i
                newParam.downstreamVar = var_j;      // x_j
                newParam.upstreamCoeff = 1.0;        // 1*x_i
                newParam.downstreamCoeff = -1.0;     // -1*x_j
                newParam.constant = 0.0;             // = 0
                newParam.description = desc;

                m_vparams.push_back(newParam);
                addedConstraints++;

                qDebug() << "  Added bus-type constraint: x" << var_i->variableId
                         << " - x" << var_j->variableId << " = 0";
            }
        }
    }

    if (addedConstraints > 0) {
        qDebug() << "Added " << addedConstraints << " bus-type connection consistency constraints";

        // 重新构建方程系统（因为添加了新的方程）
        if (m_vparams.size() > originalParamCount) {
            buildEquationSystem();
        }
    } else {
        qDebug() << "No bus-type constraints added";
    }
}
