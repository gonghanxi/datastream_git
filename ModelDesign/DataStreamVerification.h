#ifndef DATASTREAMVERIFICATION_H
#define DATASTREAMVERIFICATION_H

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <sstream>
#include <iomanip>
#include "LapackMatWrapper.h"
#include <QDebug>
#include "../Common/LogExport.h"

namespace SystemVueModelBuilder {
class Block;

class DataStreamVerification
{
public:
    struct BlockVariable {
        Block* block;
        std::string blockName;
        int variableId; //x1,x2...的编号
        std::string description; //变量描述
    };

    struct VerificationParam {
        int equationId;
        BlockVariable* upstreamVar;
        BlockVariable* downstreamVar;
        double upstreamCoeff;   //a 写指针系数
        double downstreamCoeff; //b 读指针系数
        double constant;        //c 常数项 通常为0
        std::string description;

        //返回方程字符串表示 a * x1 + b * x2 = c
        std::string toString() const {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1);

            // 上游系数
            if (std::abs(upstreamCoeff - 1.0) > 0.0001) {
                ss << upstreamCoeff << " * ";
            }
            ss << "x" << upstreamVar->variableId;

            // 下游系数
            if (downstreamCoeff >= 0) {
                ss << " - ";
            } else {
                ss << " + ";
            }

            if (std::abs(std::abs(downstreamCoeff) - 1.0) > 0.0001) {
                ss << std::abs(downstreamCoeff) << " * ";
            }
            ss << "x" << downstreamVar->variableId;

            ss << " = " << constant;

            return ss.str();
        }
    };

    DataStreamVerification();
    ~DataStreamVerification();

    //注册新的Block变量
    BlockVariable* registerBlock(Block* block);

    //添加检验方程
    void addVerificationEquation(Block* upstream, Block* downstream,
                                 double upstreamCoeff, double downstreamCoeff,
                                 double constant, const std::string& description);


    //检查校验方程是否有解
    bool CheckFeasibility();

    //获取所有校验参数
    const std::vector<VerificationParam>& getVParams() const;

    //获取所有变量
    const std::vector<BlockVariable*>& getVariables() const;

    //清空
    void clear();

    //打印检验
    void printVerification() const;
    void printSolutions() const;

    //获取校验结果
    bool isFeasible() const;
    const QString& getErrorMessage() const;

    //获取变量的解
    double getVariableSolution(int variableId) const;
    double getVariableSolution(Block* block) const;

private:
    //构建线性方程组 Ax = b
    void buildEquationSystem();

    //使用LapackMat求解线性方程组
    bool solveLinearSystem();

    //验证解是否满足所有方程
    bool verifySolution();

    //找寻非零解
    bool findNonZeroSolution();
    bool findNonZeroSolutionFallback();
    bool verifySolutionWithMatrix(const LapackMatWrapper& x) const;

    //调试信息
    void printMatrixInfo() const;
    void debugPrintEquations() const;

    // 检测并添加连接一致性约束
    void addConnectionConsistencyConstraints();

    // 存储上游到下游的映射
    std::multimap<Block*, Block*> m_upstreamToDownstreamMap;
    // 存储下游到上游的映射
    std::multimap<Block*, Block*> m_downstreamToUpstreamMap;

    //未知数/Block容器
    std::vector<BlockVariable*> m_variables;
    //Block - 未知数 映射
    std::map<Block*, BlockVariable*> m_blockToVariable;
    //系数容器
    std::vector<VerificationParam> m_vparams;
    // id - 未知数 映射
    std::map<int, BlockVariable*> m_idToVariable;

    //方程系统
    LapackMatWrapper m_coefficientMatrix; //A 系数矩阵
    LapackMatWrapper m_constantVector;    //b 常数向量
    LapackMatWrapper m_solution;          //x 解向量

    bool m_feasible;
    QString m_errorMessage;
    int m_nextVariableId;
};
}
#endif // DATASTREAMVERIFICATION_H
