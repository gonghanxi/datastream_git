#include "Buffer.h"
#include "BufferReader.h"
#include "Block.h"
#include "DFModel.h"
#include "DataStreamVerification.h"
#include <iostream>
#include <vector>
#include <thread>
#include <windows.h>

#include <QSerialPortInfo>
#include <QCoreApplication>
#include <QTimer>
#include <QTextCodec>
#include <QLibrary>
#include <QDebug>
#include <iostream>

//#include "AddCx_Block.h"
//#include "CxToEnv_Block.h"
//#include "CxToRect_Block.h"
//#include "EnvToCx_Block.h"
//#include "Gain_Block.h"
//#include "RADAR_CW_Block.h"
//#include "RADAR_PULSE_Block.h"
//#include "RectToCx_Block.h"
//#include "Reverse_Block.h"
//#include "UpSampleEnv_Block.h"

using namespace std;
using namespace SystemVueModelBuilder;

int GeneralWork(Block* currentBlock)
{
    if (!currentBlock || currentBlock->IsDone()) {
        return -1;
    }

    if (!currentBlock->CanProcess()) {
        return 0;
    }

    // 调用现有处理逻辑
    currentBlock->Run();

    // 只要Run()成功执行，就认为处理了数据
    std::cout << "Block '" << currentBlock->GetName() << "': GeneralWork executed successfully" << std::endl;
    return 1;
}

void SimpleScheduler(std::vector<Block*>& blocks) {

    qDebug() << "\n=== 开始连接约束校验 ===";

    // 1. 创建约束系统
    auto VerificationSystem = std::make_shared<DataStreamVerification>();

    // 2. 注册所有块的约束变量
    for (auto block : blocks) {
        VerificationSystem->registerBlock(block);
        qDebug() << "已注册块:" << QString::fromStdString(block->GetName());
    }
    // 3. 检查约束系统可行性
    bool constraintsFeasible = VerificationSystem->CheckFeasibility();

    if (!constraintsFeasible) {
        qDebug() << "链路校验失败!";
        qDebug() << "错误信息:" << VerificationSystem->getErrorMessage();
        qDebug() << "数据流将被终止。";
        return;
    }
    qDebug() << "链路校验通过!";
    qDebug() << "数据流可以继续执行";


    // 初始化所有块
    for (auto block : blocks) {
        block->SetDone(false);
        block->Setup();
        std::cout << "Initialized block: " << block->GetName() << std::endl;
    }

    // 调度状态变量（移到所有goto之前）
    unsigned int bi = 0;
    unsigned int nalive = blocks.size();
    unsigned int blocks_count = nalive;
    bool made_progress_last_pass = true;
    bool making_progress = false;
    int processCount = 0;
    const int MAX_PROCESS_COUNT = 5000 * 3;
    int iteration = 0;

    // 死锁检测计数器
    int noProgressCount = 0;
    const int MAX_NO_PROGRESS_ITERATIONS = 1000;  // 可根据需要调整

    // 在循环开始前声明所有可能用到的变量
//    DFModel* currentBlock = nullptr;
    Block* currentBlock = nullptr;
    bool output_ready = false;
    bool input_done = false;
    int max_items_avail = 0;
    bool input_ready = false;
    bool upstream_done = false;
    int result = 0;
    int i = 0;
    std::string portName;
    int available = 0;
    int freeSpace = 0;

    int maxProcessCount = 60;

    std::cout << "\n=== Initial Block States ===" << std::endl;

    while(nalive > 0) {
//    while(nalive > 0 && processCount < MAX_PROCESS_COUNT && iteration <= 600) {
        iteration++;
        if (iteration >= 0) { // 每次迭代输出一次状态
            std::cout << "\n=== Iteration " << iteration << " ===" << std::endl;
            std::cout << "Active blocks: " << nalive
                      << ", Process count: " << processCount
                      << "/" << MAX_PROCESS_COUNT << std::endl;
        }

        currentBlock = blocks[bi];

        if (currentBlock->IsDone()) {
            goto next_block;
        }

        if (currentBlock->GetBlockType() == Block::BlockType::SOURCE) {
            qDebug() << (made_progress_last_pass ? "true" : "false");
            if (noProgressCount / blocks_count > 10) {
               qDebug() << "Too many iterations without progress for source, forcing exit";
                goto were_done;
            }
            if(processCount >= maxProcessCount) {
                goto were_done;
            }
            if (made_progress_last_pass) {
                goto next_block;
            }

            // 检查下游是否完成
            if (currentBlock->IsDownstreamDone()) {
                std::cout << "  Downstream is done, marking source as done" << std::endl;
                goto were_done;
            }

            // 检查输出空间
            output_ready = true;
            for (size_t i = 0; i < currentBlock->GetOutputPortCount(); i++) {
                portName = currentBlock->GetOutputPortName(i);
                freeSpace = currentBlock->GetOutputPort(portName)->GetBufferFreeSpace();
                if (iteration % 10 == 0) {
                    std::cout << "  Output port " << portName << " free space: " << freeSpace << std::endl;
                }

                if (freeSpace == 0) {
                    output_ready = false;
                    if (iteration % 10 == 0)
                        std::cout << "  Output blocked on port: " << portName << std::endl;
                    break;
                }
            }

            if (!output_ready) {
                goto next_block;
            }

            if (iteration % 10 == 0) {
                std::cout << "  Source: Executing work..." << std::endl;
            }

            // 执行source块的工作
            result = GeneralWork(currentBlock);
            if (result > 0) {
                making_progress = true;
                processCount++;
                if (iteration >= 0) {
                    std::cout << "信号源处理次数: " << processCount << "/" << MAX_PROCESS_COUNT << std::endl;
                }

            }else {
                if (iteration % 10 == 0) {
                    std::cout << "  Source: Work returned " << result << " (no progress)" << std::endl;
                }
            }
            goto next_block;
        }
        else if (currentBlock->GetBlockType() == Block::BlockType::SINK) {
            if (noProgressCount / blocks_count > 10) {
                qDebug() << "Too many iterations without progress for sink, forcing exit";
                goto were_done;
            }
            max_items_avail = 0;
            input_done = false;
            bool all_upstream_done = true;  //检查上游是否全部完成

            for (size_t i = 0; i < currentBlock->GetInputPortCount(); i++) {
                portName = currentBlock->GetInputPortName(i);
                BufferReader* reader = currentBlock->GetInputPort(portName);
                available = reader->GetAvailableDataCount();

                if(available < 1) {
                    // 只有当上游完成且无数据时，才认为输入完成
                    if (reader->IsUpstreamDone()) {
                        input_done = true;
                    } else {
                        // 上游未完成，只是暂时没有数据
                        input_done = false;
                    }
                } else {
                    input_done = false;  // 有数据，输入未完成
                }

                // 检查上游完成状态
                if (!reader->IsUpstreamDone()) {
                    all_upstream_done = false;
                }

                if(max_items_avail < available) {
                    max_items_avail = available;
                }
            }

            // 只有当所有上游都完成且无数据时，才标记sink块完成
            if (input_done && all_upstream_done) {
                std::cout << "  Sink block: all upstream done and no data, marking as done" << std::endl;
                goto were_done;
            }

            // 如果上游未完成但暂时无数据，跳过处理
            if (max_items_avail < 1) {
                if (iteration % 100 == 0) {
                    std::cout << "  Sink block: no data available but upstream not done, skipping" << std::endl;
                }
                goto next_block;
            }

            // 执行sink块的工作
            result = GeneralWork(currentBlock);
            if (result > 0) {
                making_progress = true;
            }
            goto next_block;
        }
        else {
            if (noProgressCount / blocks_count > 10) {
                qDebug() << "Too many iterations without progress for processor, forcing exit";
                goto were_done;
            }
            input_ready = true;
            upstream_done = false;
            bool hasActiveInputPorts = false;

            for (size_t i = 0; i < currentBlock->GetInputPortCount(); i++) {
                portName = currentBlock->GetInputPortName(i);
                if(!currentBlock->GetInputPort(portName)->HasValidConnection()
                        && !currentBlock->GetInputPort(portName)->IsBusType(currentBlock->GetInputPort(portName)->GetDataType())) {
                    continue;
                }
                hasActiveInputPorts = true;
                BufferReader* reader = currentBlock->GetInputPort(portName);
                // 检查是否有足够的所需数据
                if (!reader->HasDataAvailable()) {
                    input_ready = false;

                    // 检查上游是否完成
                    if (reader->IsUpstreamDone()
                            && reader->HasValidConnection()) {
                        upstream_done = true;
                    }
                }
            }

            if (!hasActiveInputPorts) {
                qDebug() << "Processing block has no active input ports, marking as done";
                noProgressCount++;
                goto next_block;
            }

            if (upstream_done) {
                std::cout << "  Upstream is done, marking block as done" << std::endl;
                goto were_done;
            }

            if (!input_ready) {
                goto next_block;
            }

            output_ready = true;
            for (size_t i = 0; i < currentBlock->GetOutputPortCount(); i++) {
                portName = currentBlock->GetOutputPortName(i);
                freeSpace = currentBlock->GetOutputPort(portName)->GetBufferFreeSpace();

                if (freeSpace <= 0) {
                    output_ready = false;
                    std::cout << "  Output blocked on port: " << portName << std::endl;
                    break;
                }
            }

            if (!output_ready) {
                goto next_block;
            }

            // 所有条件满足，执行处理
            result = GeneralWork(currentBlock);
            if (result > 0) {
                noProgressCount = 0;
                making_progress = true;
            }
            goto next_block;
        }

    were_done:
        currentBlock->SetDone(true);
        currentBlock->Stop();
        nalive--;

    next_block:
        if (++bi >= blocks.size()) {
            bi = 0;
            made_progress_last_pass = making_progress;
            making_progress = false;

            // 每轮结束检查状态
            if (iteration % 100 == 0) {
                std::cout << "=== Round completed ===" << std::endl;
                std::cout << "Made progress last pass: " << made_progress_last_pass << std::endl;
                std::cout << "Active blocks: " << nalive << std::endl;

                // 检查是否有死锁
                if (!made_progress_last_pass && nalive > 0) {
                    std::cout << "WARNING: No progress made in last round, possible deadlock!" << std::endl;
                    // 输出详细状态信息
                    for (auto block : blocks) {
                        if (!block->IsDone()) {
                            std::cout << "  Active block: " << block->GetName()
                            << ", CanProcess: " << block->CanProcess() << std::endl;
                        }
                    }
                }
            }
        }


    }

    std::cout << "\n=== Scheduler finished ===" << std::endl;
    std::cout << "Total iterations: " << iteration << std::endl;
    std::cout << "Source processing count: " << processCount << std::endl;

    // 完成处理
    for (auto block : blocks) {
        if (!block->IsDone()) {
            block->Done();
        }
    }

    std::cout << "Scheduler finished. Total iterations: " << iteration
              << ", Source processing count: " << processCount << std::endl;
}




// 定义函数指针类型
typedef SystemVueModelBuilder::DFModel* (*CreateAlgoFunc)();
typedef const char* (*GetAlgoNameFunc)();

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    auto verificationSystem = std::make_shared<DataStreamVerification>();
    Block::SetVerificationSystem(verificationSystem);

    qDebug() << "=== Loader Test ===";

    // 1. 创建 QLibrary 对象并加载 DLL
    QLibrary R1_library("C:/Users/shi/Desktop/test/radar_cw/radar_cw.dll");
    QLibrary C1_library("C:/Users/shi/Desktop/test/CxToEnv/CxToEnv.dll");

    R1_library.load();
    C1_library.load();

    // 2. 解析导出函数
    CreateAlgoFunc createAlgo_R1 = (CreateAlgoFunc)R1_library.resolve("createAlgo");
    if (!createAlgo_R1) {
        qCritical() << "Failed to resolve createAlgo function:" << R1_library.errorString();
        R1_library.unload();
        return -1;
    }
    qDebug() << "radar_cw createAlgo function resolved";

    GetAlgoNameFunc getAlgoName_R1 = (GetAlgoNameFunc)R1_library.resolve("getAlgoName");
    if (!getAlgoName_R1) {
        qCritical() << "Failed to resolve getAlgoName function:" << R1_library.errorString();
        R1_library.unload();
        return -1;
    }
    qDebug() << "radar_cw getAlgoName function resolved";

    CreateAlgoFunc createAlgo_C1 = (CreateAlgoFunc)C1_library.resolve("createAlgo");
    if (!createAlgo_C1) {
        qCritical() << "Failed to resolve createAlgo function:" << R1_library.errorString();
        C1_library.unload();
        return -1;
    }
    qDebug() << "cxToenv createAlgo function resolved";

    GetAlgoNameFunc getAlgoName_C1 = (GetAlgoNameFunc)C1_library.resolve("getAlgoName");
    if (!getAlgoName_C1) {
        qCritical() << "Failed to resolve getAlgoName function:" << R1_library.errorString();
        C1_library.unload();
        return -1;
    }
    qDebug() << "cxToenv getAlgoName function resolved";


    // 3. 获取算法名称
    const char* algoName_R1 = getAlgoName_R1();
    qDebug() << "Algorithm name:" << algoName_R1;
    const char* algoName_C1 = getAlgoName_C1();
    qDebug() << "Algorithm name:" << algoName_C1;

    // 4. 创建算法实例
    SystemVueModelBuilder::DFModel* model_R1 = createAlgo_R1();
    if (!model_R1) {
        qCritical() << "Failed to create algorithm instance";
        R1_library.unload();
        return -1;
    }
    qDebug() << "Algorithm instance created at:" << model_R1;
    SystemVueModelBuilder::DFModel* model_C1 = createAlgo_C1();
    if (!model_C1) {
        qCritical() << "Failed to create algorithm instance";
        C1_library.unload();
        return -1;
    }
    qDebug() << "Algorithm instance created at:" << model_C1;

    // 5. 调用 Block/DFModel 的基本方法
    qDebug() << "\n--- Testing Block/DFModel interface ---";

    // Setup
    if (model_R1->Setup()) {
        qDebug() << "Setup() succeeded";
    } else {
        qWarning() << "Setup() failed";
    }
    if (model_C1->Setup()) {
        qDebug() << "Setup() succeeded";
    } else {
        qWarning() << "Setup() failed";
    }

    // Initialize
    if (model_R1->Initialize()) {
        qDebug() << "Initialize() succeeded";
    } else {
        qWarning() << "Initialize() failed";
    }
    if (model_C1->Initialize()) {
        qDebug() << "Initialize() succeeded";
    } else {
        qWarning() << "Initialize() failed";
    }

    Block::Connect(model_R1, model_R1->GetOutputPortName(0), model_C1, model_C1->GetInputPortName(0));

//    //运行几次
//    std::vector<Block*> blocks = {&model_R1, &model_C1};
//    qDebug() << "\n>>> 启动调度测试 <<<";
//    SimpleScheduler(blocks);
//    qDebug() << "=== 测试完成 ===";
    GeneralWork(model_R1);
    GeneralWork(model_C1);

    // Finalize
    if (model_R1->Finalize()) {
        qDebug() << "\n Finalize() succeeded";
    } else {
        qWarning() << "\n Finalize() failed";
    }
    if (model_C1->Finalize()) {
        qDebug() << "\n Finalize() succeeded";
    } else {
        qWarning() << "\n Finalize() failed";
    }

    // 7. 测试 UpdateDynamicParameters
    if (model_R1->UpdateDynamicParameters()) {
        qDebug() << " UpdateDynamicParameters() succeeded";
    } else {
        qDebug() << "UpdateDynamicParameters() returned false (may not be implemented)";
    }
    if (model_C1->UpdateDynamicParameters()) {
        qDebug() << " UpdateDynamicParameters() succeeded";
    } else {
        qDebug() << "UpdateDynamicParameters() returned false (may not be implemented)";
    }

    // 8. 清理
    qDebug() << "\n--- Cleaning up ---";
    delete model_R1;
    delete model_C1;
    qDebug() << " Algorithm instance deleted";

    R1_library.unload();
    C1_library.unload();
    qDebug() << " DLL unloaded";

    qDebug() << "\n=== Test completed successfully ===";

    return 0;
    // return a.exec(); // 如果不需要事件循环，可以直接返回
}

