#include "simrunner.h"
#include <QDebug>
#include <QDir>
#include "unitconvert.h"
#include "dataflowcheck.h"
#include "ConnectionValidator.h"

#include "VariableAnalysis/LinkParser.h"
#include "FMUManager.h"
#include "cfunction/CFunctionBlock.h"
#include "cfunction/CFunctionModelInfo.h"

#include <QTextCodec>
#include <QVariant>
#include <queue>


SimRunner::SimRunner(const char *appPath, const char **linkFiles, int fileCount, const char *outPutPath)
{
    // 使用fromUtf8将const char*转换为QString
    m_appPath = QString::fromUtf8(appPath);
    m_outPutPath = QString::fromUtf8(outPutPath);

    QTextCodec* gbkCodec = QTextCodec::codecForName("GBK");
    if (gbkCodec) {
        gbkCodec->toUnicode(outPutPath);
    }

    QTextCodec* utf8Codec = QTextCodec::codecForName("UTF-8");
    if (utf8Codec) {
        utf8Codec->toUnicode(outPutPath);
    }
    // m_paused: 暂停标志，0=正常运行，1=已暂停
    // 使用 QAtomicInt 保证跨线程访问的原子性
    m_paused = 0;

    // m_stopRequested: 停止请求标志，0=正常运行，1=已请求停止
    // 由 stdin 监听线程通过信号槽设置，主调度循环检查
    m_stopRequested = 0;

    // 处理文件列表
    for (int i = 0; i < fileCount; i++) {
        QString file = QString::fromUtf8(linkFiles[i]);
        m_linkFiles.append(file);
    }

    m_simuParamsCache = AlgorithmManager::createInstance()->getSimuParameters();
}

SimRunner::~SimRunner()
{
    qDebug() << "~SimRunner";
    // ========== 确保析构时不处于暂停等待状态 ==========
    // 如果析构时调度线程还在暂停等待中（虽然正常流程不会出现），
    // 强制清除暂停标志并唤醒，避免线程永远阻塞
    if (m_paused) {
        m_paused = 0;
        QMutexLocker locker(&m_pauseMutex);
        m_pauseCond.wakeAll();
    }
    AlgorithmManager::createInstance()->clear();
}

bool SimRunner::start()
{
    //    LOG_INFO("开始解析链路文件.");
    if(!AnalysisFiles())
    {
        //        LOG_INFO("链路文件解析成功.");
        //        LOG_INFO("初始化模型实例.");
        LOG_ERROR("链路文件解析失败.");
        return false;
    }
    //将所有要执行的block地址按链路保存
    ManageAllBlocks();
    if(!InitializeBlocks())
    {
        //            LOG_INFO("模型实例初始化完成.");
        LOG_INFO("初始化模型实例失败.");
        return false;
    }
    //初始化矩阵校验
    m_verificationSystem = std::make_shared<DataStreamVerification>();
    Block::SetVerificationSystem(m_verificationSystem);
    if(!ManageConnection())
    {
        //                LOG_INFO("模型实例建立连接完成.");
        LOG_INFO("模型实例建立连接失败");
        return false;
    }
    if(!SetupBlocks())
    {
        //                LOG_INFO("模型实例Setup完成");
        LOG_INFO("模型实例Setup失败");
        return false;
    }
    return true;
}

bool SimRunner::run()
{
    //    LOG_INFO("数据流引擎调度开始.");
    bool result = RunBlocks();

    // 仿真结束后，清理CFunction引擎生成的临时文件夹（以链路值命名的目录）
    QDir appDir(m_appPath);
    for (const QString& linkKey : m_simuParamsCache.keys()) {
        QDir cfuncDir(appDir.absoluteFilePath(linkKey));
        if (cfuncDir.exists()) {
            cfuncDir.removeRecursively();
        }
    }

    return result;
}

void SimRunner::setWriter(ILogWriter *write)
{
    std::ignore = write;
    //    mWrite=write;
    //    for (const auto& modelId : mSortedModels) {
    //        auto algo= AlgorithmManager::createInstance()->getAlgorithmById(modelId.toInt());
    //        algo->setLogWriter(write);
    //    }
}

void SimRunner::setSimCfg(SimCfgData *data)
{
    mSimCfgData=data;
}

SimuParameter SimRunner::getSimulationParameters() const
{
    SimuParameter params;

    if (!m_simuParamsCache.isEmpty()) {
        // 取第一个链路的仿真参数（通常主链路是第一个）
        SimuParameter simuPara = m_simuParamsCache.first();

        params.startTime = simuPara.startTime;
        params.stopTime = simuPara.stopTime;
        params.time_Interval = simuPara.time_Interval;
        params.samplingRate = simuPara.samplingRate;
        params.num_Samples = simuPara.num_Samples;

        params.simuName = simuPara.simuName.c_str();
        params.linkName = simuPara.linkName.c_str();
    }

    return params;
}

void SimRunner::pause()
{
    // ======================================================================
    // 功能：暂停仿真引擎
    // 调用时机：由 main.cpp 中的 StdinListener 收到 "pause" 命令后通过信号槽调用
    // 实现原理：
    //   1. 设置 m_paused 原子标志为 1
    //   2. SimpleScheduler 主循环在每轮迭代开始时会检查这个标志
    //   3. 检测到 m_paused == 1 后，主循环会进入 QWaitCondition::wait() 阻塞等待
    //   4. 阻塞期间不消耗 CPU，仿真进度完全冻结
    // ======================================================================
    if (!m_paused) {
        m_paused = 1;  // 设置暂停标志，原子操作
        LOG_INFO("仿真已收到暂停命令...");
        qDebug() << "[SimRunner] 暂停标志已设置，等待调度循环响应...";
    } else {
        qDebug() << "[SimRunner] 当前已在暂停状态，忽略重复暂停命令";
    }
}

void SimRunner::resume()
{
    // ======================================================================
    // 功能：继续仿真引擎（从暂停状态恢复）
    // 调用时机：由 main.cpp 中的 StdinListener 收到 "continue"/"resume" 命令后调用
    // 实现原理：
    //   1. 清除 m_paused 标志
    //   2. 使用 QWaitCondition::wakeAll() 唤醒主循环中的等待线程
    //   3. 被唤醒后，主循环重新检查 m_paused，发现为 0，继续执行调度
    // ======================================================================
    if (m_paused) {
        m_paused = 0;
        // 获取互斥锁后唤醒等待线程
        // 必须先获取锁再唤醒，保证 wait() 正确返回
        {
            QMutexLocker locker(&m_pauseMutex);
            m_pauseCond.wakeAll();  // 唤醒所有等待的线程（实际只有一个调度线程）
        }

        LOG_INFO("仿真已收到继续命令，恢复执行...");
        qDebug() << "[SimRunner] 继续标志已设置，调度循环已唤醒";
    } else {
        qDebug() << "[SimRunner] 当前未处于暂停状态，忽略继续命令";
    }
}

void SimRunner::requestStop()
{
    // ======================================================================
    // 功能：请求停止仿真引擎（通过 stdin 命令方式）
    // 调用时机：由 main.cpp 中的 StdinListener 收到 "stop" 命令后调用
    // 实现原理：
    //   1. 设置 m_stopRequested 原子标志为 1
    //   2. 如果当前处于暂停状态，需要唤醒等待线程
    //      因为暂停时主循环阻塞在 wait() 中，无法检查 m_stopRequested
    //      唤醒后主循环会先检查 m_stopRequested，然后 break 退出
    //   3. 如果当前处于运行状态，主循环会在下一次迭代开头检查到标志并退出
    // ======================================================================
    m_stopRequested = 1;
    qDebug() << "[SimRunner] 停止请求标志已设置";
    // 如果当前处于暂停状态，必须唤醒等待中的主循环
    // 否则主循环会永远阻塞在 wait() 中，无法响应停止命令
    if (m_paused) {
        m_paused = 0;  // 先清除暂停标志，让主循环跳出 wait()
        {
            QMutexLocker locker(&m_pauseMutex);
            m_pauseCond.wakeAll();  // 唤醒主循环
        }
        LOG_INFO("暂停状态下收到停止命令，正在终止仿真...");
        qDebug() << "[SimRunner] 已唤醒暂停等待中的调度循环";
    } else {
        LOG_INFO("仿真已收到停止命令，将在当前迭代完成后停止...");
    }
}

bool SimRunner::isPaused() const
{
    return m_paused == 1;
}

bool SimRunner::AnalysisFiles()
{
    LinkParser parser;

    ParseResult result = parser.parseLinkFiles(
                m_appPath,
                m_linkFiles,
                m_outPutPath
                );

    if (!result.success) {
        LOG_ERROR(result.errorMessage.toStdString());
        return false;
    }

    // 存储blocksInfo、connections、simuParams
    auto blocksInfo = parser.getBlocksInfo();
    auto connections = parser.getConnections();
    auto simuParams = parser.getSimuParameters();
    m_currentMainLinkKey = result.mainLinkKey;

    // ========== 处理短路和开路 ==========
    ShortOpenProcessor processor;
    if (!processor.processAllLinks(blocksInfo, connections)) {
        LOG_ERROR("短路/开路处理失败");
        return false;
    }
    // 存储Blocks
    qDebug() << "处理短路/开路后 blocksInfo: ";
    for (auto it = blocksInfo.begin(); it != blocksInfo.end(); ++it) {
        const QVector<BlockInfo>& blocks = it.value();
        for (const BlockInfo& blockInfo : blocks) {
            qDebug() << blockInfo.instanceName;
        }
    }

    // 存储连接关系
    qDebug() << "处理短路/开路后 connections: ";
    for (auto it = connections.begin(); it != connections.end(); ++it) {
        const QVector<Connection>& conns = it.value();
        qDebug() << conns.size();
    }

    // 获取短路模型列表（信号源和数据收集器）
    m_shortCircuitedSourcesAndSinks = processor.getShortCircuitedSourcesAndSinks();

    // 存储Blocks
    for (auto it = blocksInfo.begin(); it != blocksInfo.end(); ++it) {
        const QString& linkKey = it.key();
        const QVector<BlockInfo>& blocks = it.value();

        for (const BlockInfo& blockInfo : blocks) {
            AlgorithmManager::createInstance()->addBlocksInfo(linkKey, blockInfo);
        }
    }

    // 存储连接关系
    for (auto it = connections.begin(); it != connections.end(); ++it) {
        const QString& linkKey = it.key();
        const QVector<Connection>& conns = it.value();

        for (const Connection& conn : conns) {
            AlgorithmManager::createInstance()->addConnection(linkKey, conn);
        }
    }

    // 存储仿真参数
    for (auto it = simuParams.begin(); it != simuParams.end(); ++it) {
        AlgorithmManager::createInstance()->addSimuParameters(it.key(), it.value());
    }

    m_simuParamsCache = simuParams;

    // ========== 新增：收集FMU信息并加载到FMUManager ==========
    std::vector<fmuCreateInfo> fmuInfoList;

    for (auto it = blocksInfo.begin(); it != blocksInfo.end(); ++it) {
        const QVector<BlockInfo>& blocks = it.value();

        for (const BlockInfo& blockInfo : blocks) {
            if (blockInfo.isFmuModel) {
                qDebug() << "发现FMU模型:" << blockInfo.instanceName
                         << "GUID:" << blockInfo.guid;

                // 构建FMU配置
                fmuConfig config;
                config.guid = blockInfo.guid;
                config.type = fmiType::CS;  // 默认使用CS模式

                // 选择合适平台的库路径，其余作为依赖库路径
                for (const QString& path : blockInfo.dllOrSoPaths) {
#ifdef _WIN32
                    if (path.contains("win64") || path.contains("windows") || path.contains(".dll")) {
                        config.path = path;
                        config.libname = QFileInfo(path).baseName();
                        break;
                    }
#else
                    if (path.contains("linux") || path.contains(".so")) {
                        config.path = path;
                        config.libname = QFileInfo(path).baseName();
                        break;
                    }
#endif
                }

                if (config.path.isEmpty() && !blockInfo.dllOrSoPaths.isEmpty()) {
                    config.path = blockInfo.dllOrSoPaths.first();
                    config.libname = QFileInfo(config.path).baseName();
                }

                // 收集依赖库路径（排除主库路径本身）
                for (const QString& path : blockInfo.dllOrSoPaths) {
                    if (path != config.path) {
                        config.depPaths.append(path);
                    }
                }

                qDebug() << "FMU库路径:" << config.path
                         << "库名称:" << config.libname
                         << "依赖库数:" << config.depPaths.size();

                // 构建FmuVar列表
                std::vector<FmuVar> fmuVars;

                // 添加端口变量
                for (auto portIt = blockInfo.portsMsg.begin();
                     portIt != blockInfo.portsMsg.end(); ++portIt) {
                    const PortMsg& port = portIt.value();
                    int valueRef = blockInfo.portValueReferences.value(port.id, -1);

                    if (valueRef != -1) {
                        FmuVar var;
                        var.varname = port.name;
                        var.vr = valueRef;

                        // 设置类型和初值
                        switch (port.dataType) {
                        case PortMsg::PortDataType::REAL:
                            var.type = Real;
                            var.startValue = 0.0;
                            break;
                        case PortMsg::PortDataType::INT:
                            var.type = Integer;
                            var.startValue = static_cast<double>(0);
                            break;
                            //                            case PortMsg::PortDataType::BOOLEAN:
                            //                                var.type = Boolean;
                            //                                var.startValue = false;
                            //                                break;
                        default:
                            var.type = Real;
                            var.startValue = 0.0;
                            break;
                        }

                        // 设置属性
                        if (port.putType == "in") {
                            var.causality = Input;
                        } else if (port.putType == "out") {
                            var.causality = Output;
                        } else {
                            var.causality = Local;
                        }

                        fmuVars.push_back(var);
                        qDebug() << "  添加FMU变量:" << var.varname
                                 << "vr:" << var.vr
                                 << "causality:" << var.causality;
                    }
                }

                // 添加参数变量
                for (auto paramIt = blockInfo.parameters.begin();
                     paramIt != blockInfo.parameters.end(); ++paramIt) {
                    const std::string& paramName = paramIt->first;
                    const SystemVueModelBuilder::Parameter& param = paramIt->second;
                    int valueRef = blockInfo.paramValueReferences.value(
                                QString::fromStdString(paramName), -1);

                    if (valueRef != -1) {
                        FmuVar var;
                        var.varname = QString::fromStdString(paramName);
                        var.vr = valueRef;
                        var.causality = VarcausalityType::Parameter;

                        // 尝试解析参数类型
                        QString paramValue = QString::fromStdString(param.Value);
                        bool ok;
                        double dval = paramValue.toDouble(&ok);
                        if (ok) {
                            var.type = Real;
                            var.startValue = dval;
                        } else if (paramValue == "true" || paramValue == "false") {
                            var.type = Boolean;
                            var.startValue = (paramValue == "true");
                        } else {
                            int ival = paramValue.toInt(&ok);
                            if (ok) {
                                var.type = Integer;
                                var.startValue = static_cast<double>(ival);
                            } else {
                                var.type = String;
                                var.startValue = paramValue;
                            }
                        }

                        fmuVars.push_back(var);
                        qDebug() << "  添加FMU参数:" << var.varname
                                 << "vr:" << var.vr
                                 << "值:" << paramValue;
                    }
                }

                // 创建FMU创建信息
                fmuCreateInfo createInfo;
                createInfo.config = config;
                createInfo.fmuVec = fmuVars;
                fmuInfoList.push_back(createInfo);

                qDebug() << "FMU信息收集完成:" << blockInfo.instanceName
                         << "变量数:" << fmuVars.size();
            }
        }
    }

    // 加载所有FMU到FMUManager
    if (!fmuInfoList.empty()) {
        FMUManager* fmuMgr = FMUManager::getInstance();
        if (!fmuMgr->load(fmuInfoList)) {
            LOG_ERROR("FMU加载失败");
            return false;
        }
        qDebug() << "FMU加载成功，数量:" << fmuInfoList.size();

        // 验证加载结果
        for (const auto& info : fmuInfoList) {
            if (fmuMgr->hasInstance(info.config.guid)) {
                qDebug() << "FMU实例已加载:" << info.config.guid;
            } else {
                LOG_ERROR("FMU实例加载失败:", info.config.guid.toStdString());
            }
        }
    } else {
        qDebug() << "没有FMU模型需要加载";
    }

    return true;
}

void SimRunner::ManageAllBlocks()
{
    qDebug() << "ManageAllBlocks():" << AlgorithmManager::createInstance()->getSimuParameters().keys();

    //取仿真器参数（只有主链路有仿真器参数）
    for(auto e:AlgorithmManager::createInstance()->getSimuParameters().keys())
    {
        //保存实例地址给调度器
        QVector<Block *> blocks;
        QVector<BlockInfo>& blocksInfo = AlgorithmManager::createInstance()->getBlocksInfo()[e];
        recursiveReadBlock(blocksInfo, blocks);
        AlgorithmManager::createInstance()->addRunBlocks(e,blocks);
        qDebug() << "e: "<< e.size();
    }
}

bool SimRunner::InitializeBlocks()
{
    for (const QString& linkKey : AlgorithmManager::createInstance()->getRunBlocks().keys())
    {
        auto blocks=AlgorithmManager::createInstance()->getRunBlocks().value(linkKey);

        qDebug() << "InitializeBlocks():" << AlgorithmManager::createInstance()->getRunBlocks();
        for (const auto block:blocks)
        {
            if(block->Initialize())
            {
                qDebug() <<  "链路：" << linkKey << "实例：" << QString::fromStdString(block->GetName()) << "初始化成功.";
            }
            else
            {
                LOG_ERROR("链路：",linkKey.toStdString(),"，实例：",block->GetName(),"初始化失败.");
                return false;
            }
        }

        //时间驱动
//        AlgorithmManager::createInstance()->setSchedulerType(AlgorithmManager::SchedulerType::TIME_DRIVEN);
//        AlgorithmManager::SchedulerType type = AlgorithmManager::createInstance()->getSchedulerType();
//        if(type == AlgorithmManager::SchedulerType::TIME_DRIVEN) {
//            for (const auto& block:blocks)
//            {
//                block->SetVariableStepMode(true);
//                for(const auto& output : block->GetOutputPorts() ) {
//                    output.second->SetVariableMode(true);
//                }
//            }
//        }
    }
    return true;
}
bool SimRunner::SetupBlocks()
{
    for (const QString& linkKey : AlgorithmManager::createInstance()->getRunBlocks().keys())
    {
        auto blocks=AlgorithmManager::createInstance()->getRunBlocks().value(linkKey);
        qDebug() << "SetupBlocks --blocks: " << blocks.size();
        for (const auto block:blocks)
        {
            if(block->Setup())
            {
                qDebug() <<  "链路：" << linkKey << "实例：" << QString::fromStdString(block->GetName()) << "SetUp成功.";
            }
            else
            {
                LOG_ERROR("链路：",linkKey.toStdString(),"，实例：",block->GetName(),"Setup失败.");
                return false;
            }
        }
    }
    return true;
}
bool SimRunner::ManageConnection()
{
    AlgorithmManager* algoMgr = AlgorithmManager::createInstance();
    if (nullptr == algoMgr) { return false; }

    if (!validateShortCircuitedSourcesAndSinks()) {
        return false;
    }

    //取仿真器参数（只有主链路有仿真器参数）
    const auto& simuKeys = algoMgr->getSimuParameters().keys();
    qDebug() << "getSimuParameters().keys()" << algoMgr->getSimuParameters().keys();
    for (const QString& linkKey : simuKeys)
    {
        if(linkKey.isEmpty()) continue;

        // ========== 连接校验器 ==========
        auto blocksInfo = algoMgr->getBlocksInfo().value(linkKey);
        auto connections = algoMgr->getConnection().value(linkKey);

        ConnectionValidator validator(blocksInfo, connections, linkKey);
        auto validationResult = validator.validateAll();

        if (!validationResult.isValid) {
            LOG_ERROR("链路：", linkKey.toStdString(), "连接校验失败：", validationResult.errorMessage.toStdString());
            return false;
        }

        // 输出警告信息
        for (const QString& warning : validationResult.warnings) {
            LOG_WARN("链路：", linkKey.toStdString(), "警告：", warning.toStdString());
        }

        // 获取实际连接用于调试
        auto actualConnections = validator.getActualConnections();
        qDebug() << "=== 实际连接（穿透子系统）===";
        for (const auto& ac : actualConnections) {
            qDebug() << "实际连接:" << ac.srcBlockId << ":" << ac.srcPortId
                     << "->" << ac.dstBlockId << ":" << ac.dstPortId
                     << "[" << ac.originalPath << "]";
        }
        // =======================================

        mConnections.clear();
        // 树的根节点遍历：初始父上下文为空（主链路无上游）
        if(!dfsTraverseLink(BlockInfo(), PortMsg(), BlockInfo(), PortMsg(), linkKey))
            return false;
    }
    qDebug() << "ManageConnection --ManageConnection end";
    return true;
}

bool SimRunner::validateLinkAndSubLinks(const QString &linkKey)
{
    AlgorithmManager* algoMgr = AlgorithmManager::createInstance();

    auto blocksInfo = algoMgr->getBlocksInfo().value(linkKey);
    auto connections = algoMgr->getConnection().value(linkKey);

    qDebug() << "\n========== 校验链路:" << linkKey << "==========";

    // 创建当前链路的校验器
    ConnectionValidator validator(blocksInfo, connections, linkKey);
    auto validationResult = validator.validateAll();

    if (!validationResult.isValid) {
        LOG_ERROR("链路：", linkKey.toStdString(),
                  "连接校验失败：", validationResult.errorMessage.toStdString());
        return false;
    }

    // 输出警告信息
    for (const QString& warning : validationResult.warnings) {
        LOG_WARN("链路：", linkKey.toStdString(), "警告：", warning.toStdString());
    }

    // 递归校验所有子链路
    for (const BlockInfo& blockInfo : blocksInfo) {
        if (blockInfo.isSubSystem && !blockInfo.childTopoId.isEmpty()) {
            qDebug() << "递归校验子系统:" << blockInfo.instanceName
                     << "子链路:" << blockInfo.childTopoId;

            if (!validateLinkAndSubLinks(blockInfo.childTopoId)) {
                return false;
            }
        }
    }

    return true;
}

bool SimRunner::validateShortCircuitedSourcesAndSinks()
{
    if (m_shortCircuitedSourcesAndSinks.isEmpty()) {
        return true;  // 没有需要校验的模型
    }

    qDebug() << "========== 开始校验短路模型（信号源/数据收集器）==========";

    for (const ShortCircuitedModel& shortModel : m_shortCircuitedSourcesAndSinks) {
        const BlockInfo& blockInfo = *(shortModel.blockInfo);
        qDebug() << "validateShortCircuitedSourcesAndSinks - m_shortCircuitedSourcesAndSinks: "
                 << m_shortCircuitedSourcesAndSinks.size();
        qDebug() << "validateShortCircuitedSourcesAndSinks - blockInfo: "
                 << blockInfo.instanceName;

        if (blockInfo.cmpCategory == "Sources") {
            // 信号源短路：检查是否存在未连接的输出端口
            bool hasUnconnectedPort = false;
            QStringList unconnectedPorts;

            // 获取当前链路的所有连接
            auto connections = AlgorithmManager::createInstance()
                ->getConnection().value(shortModel.linkKey);

            for (auto it = blockInfo.portsMsg.begin(); it != blockInfo.portsMsg.end(); ++it) {
                const PortMsg& port = it.value();
                if (port.putType == "out") {
                    // 检查该输出端口是否有连接
                    bool isConnected = false;
                    QString srcModelId = QString::number(blockInfo.cmpId);
                    QString srcPortId = QString::number(port.id);

                    for (const Connection& conn : connections) {
                        if (conn.fromModelId() == srcModelId &&
                            conn.fromPort() == srcPortId) {
                            isConnected = true;
                            break;
                        }
                    }

                    if (!isConnected) {
                        hasUnconnectedPort = true;
                        unconnectedPorts << port.name;
                    }
                }
            }

            if (hasUnconnectedPort) {
                LOG_ERROR("特殊情况：信号源模型",
                         blockInfo.instanceName.toStdString(),
                         "短路，存在未连接端口:",
                         unconnectedPorts.join(",").toStdString());
                return false;
            } else {
                qDebug() << "信号源短路校验通过:" << blockInfo.instanceName;
            }
        }
        else if (blockInfo.cmpCategory == "Sinks") {
            // 数据收集器短路：检查是否存在未连接的输入端口
            bool hasUnconnectedPort = false;
            QStringList unconnectedPorts;

            auto connections = AlgorithmManager::createInstance()
                ->getConnection().value(shortModel.linkKey);

            for (auto it = blockInfo.portsMsg.begin(); it != blockInfo.portsMsg.end(); ++it) {
                const PortMsg& port = it.value();
                if (port.putType == "in") {
                    bool isConnected = false;
                    QString dstModelId = QString::number(blockInfo.cmpId);
                    QString dstPortId = QString::number(port.id);

                    for (const Connection& conn : connections) {
                        if (conn.toModelId() == dstModelId &&
                            conn.toPort() == dstPortId) {
                            isConnected = true;
                            break;
                        }
                    }

                    if (!isConnected) {
                        hasUnconnectedPort = true;
                        unconnectedPorts << port.name;
                    }
                }
            }

            if (hasUnconnectedPort) {
                LOG_ERROR("特殊情况：数据收集器模型",
                         blockInfo.instanceName.toStdString(),
                         "短路，存在未连接端口:",
                         unconnectedPorts.join(",").toStdString());
                return false;
            } else {
                qDebug() << "数据收集器短路校验通过:" << blockInfo.instanceName;
            }
        }
    }

    qDebug() << "========== 短路模型校验完成 ==========";
    return true;
}

// simrunner.cpp 中修改 recursiveReadBlock 的FMU部分
void SimRunner::recursiveReadBlock(QVector<BlockInfo> &blocksInfo, QVector<Block *> &blocks)
{
    qDebug() << "recursiveReadBlock: " << blocksInfo.size();

    for (BlockInfo& blockInfo : blocksInfo)
    {
        //        qDebug() << "before current blockInfo block '"<< blockInfo.instanceName <<"' ptr: " << (blockInfo.block ? "true" : "false");
        if (blockInfo.isSubSystem)
        {
            auto subBlocks = AlgorithmManager::createInstance()->getBlocksInfo().value(blockInfo.childTopoId);
            recursiveReadBlock(subBlocks, blocks);  // 传递引用
        }
        else
        {
            bool topProtIdExit = false;
            for (auto port : blockInfo.portsMsg)
            {
                if (port.topProtId != -1)
                {
                    topProtIdExit = true;
                }
            }

            if (!topProtIdExit)
            {
                Block* block = nullptr;

                // 判断是否为FMU模型
                if (blockInfo.isFmuModel) {
                    // 从blockInfo重建FMUModelInfo
                    FMUModelInfo modelInfo;
                    modelInfo.guid = blockInfo.guid;
                    modelInfo.instanceName = blockInfo.instanceName;
                    modelInfo.cmpId = blockInfo.cmpId;
                    modelInfo.dllOrSoPaths = blockInfo.dllOrSoPaths;
                    modelInfo.m_Sima = getSimulationParameters();

                    // 重建端口信息
                    for (auto portIt = blockInfo.portsMsg.begin();
                         portIt != blockInfo.portsMsg.end(); ++portIt) {
                        const PortMsg& port = portIt.value();
                        int valueRef = blockInfo.portValueReferences.value(port.id, -1);
                        if (valueRef != -1) {
                            FMUPortMsg fmuPort;
                            fmuPort.id = port.id;
                            fmuPort.name = port.name;
                            fmuPort.putType = port.putType;
                            fmuPort.valueReference = valueRef;
                            fmuPort.dataType = UnitConvert::dataTypeToString(port.dataType);
                            fmuPort.isOptional = port.isOptional;
                            fmuPort.portRate = port.portRate;
                            fmuPort.topProtId = port.topProtId;
                            modelInfo.fmuPorts[port.id] = fmuPort;
                        }
                    }

                    // 重建参数信息
                    for (auto paramIt = blockInfo.parameters.begin();
                         paramIt != blockInfo.parameters.end(); ++paramIt) {
                        const std::string& paramName = paramIt->first;
                        const SystemVueModelBuilder::Parameter& param = paramIt->second;
                        int valueRef = blockInfo.paramValueReferences.value(
                                    QString::fromStdString(paramName), -1);
                        if (valueRef != -1) {
                            FMUParameter fmuParam;
                            fmuParam.name = QString::fromStdString(paramName);
                            fmuParam.value = QString::fromStdString(param.Value);
                            fmuParam.valueReference = valueRef;
                            modelInfo.fmuParameters[fmuParam.name] = fmuParam;
                        }
                    }
                    qDebug() << "FMU模型参数有" << modelInfo.fmuParameters.size();

                    Block* fmuBlock = modelInfo.createBlock();
                    block = fmuBlock;
                    blockInfo.block = block;
                    qDebug() << "创建FMUBlock:" << QString::fromStdString(blockInfo.block->GetName())
                             << "GUID:" << blockInfo.guid;
                } else if (blockInfo.isCFunctionModel) {
                    // 从blockInfo重建CFunctionBlock
                    CFunctionBlock* cfuncBlock = new CFunctionBlock(blockInfo.instanceName.toStdString());
                    cfuncBlock->setCFunctionConfig(blockInfo.instanceName, blockInfo.cmpId);

                    // 重建configData
                    CFunctionConfigData configData;
                    configData.language = blockInfo.cfunctionLanguage;
                    for (int i = 0; i < blockInfo.cfunctionLibFileNames.size() && i < blockInfo.cfunctionLibFilePaths.size(); ++i) {
                        configData.libFiles.append({blockInfo.cfunctionLibFilePaths[i], blockInfo.cfunctionLibFileNames[i]});
                    }
                    for (int i = 0; i < blockInfo.cfunctionHeaderFileNames.size() && i < blockInfo.cfunctionHeaderFilePaths.size(); ++i) {
                        configData.headerFiles.append({blockInfo.cfunctionHeaderFilePaths[i], blockInfo.cfunctionHeaderFileNames[i]});
                    }
                    for (int i = 0; i < blockInfo.cfunctionCFileNames.size() && i < blockInfo.cfunctionCFilePaths.size(); ++i) {
                        configData.cFiles.append({blockInfo.cfunctionCFilePaths[i], blockInfo.cfunctionCFileNames[i]});
                    }
                    cfuncBlock->setConfigData(configData);
                    cfuncBlock->setEquations(blockInfo.cfunctionEquations);
                    cfuncBlock->setGeneratedJsonPath(blockInfo.cfunctionGeneratedJsonPath);
                    cfuncBlock->setSimuParams(getSimulationParameters());

                    // 设置端口信息
                    for (auto portIt = blockInfo.portsMsg.begin();
                         portIt != blockInfo.portsMsg.end(); ++portIt) {
                        cfuncBlock->addPortInfo(portIt.value());
                    }

                    // 设置参数信息（attribute参数）
                    for (auto paramIt = blockInfo.parameters.begin();
                         paramIt != blockInfo.parameters.end(); ++paramIt) {
                        cfuncBlock->addParameterInfo(
                            QString::fromStdString(paramIt->first),
                            QString::fromStdString(paramIt->second.Value));
                    }

                    block = cfuncBlock;
                    blockInfo.block = block;
                    qDebug() << "创建CFunctionBlock:" << blockInfo.instanceName
                             << "JSON路径:" << blockInfo.cfunctionGeneratedJsonPath;
                } else {
                    // 普通模型，使用AlgorithmManager创建
                    block = blockInfo.block;

                    // 添加模型参数
                    for (auto e : blockInfo.parameters)
                    {
                        if (e.second.Name.empty() || e.second.Value.empty()) {
                            qDebug() << "WARNING: Empty parameter found!";
                            continue;
                        }

                        if (block) {
                            block->setParameter(e.second);
                            block->setUserId(m_UserId.toStdString());
                        }
                    }
                }

                if (block) {
                    blocks.push_back(block);
                } else {
                    LOG_WARN("模型实例为空:", blockInfo.instanceName.toStdString());
                }
            }
        }
//        qDebug() << "after current blockInfo block '"<< blockInfo.instanceName <<"' ptr: " << (blockInfo.block ? "true" : "false");
    }
}

/**
 * @brief DFS
 * @param upstreamBlock  上游节点
 * @param upstreamPort   上游节点的端口 (兼容in/out)
 * @param parentBlock    父节点
 * @param parentPort 父节点端口
 * @return bool 遍历状态
 */
bool SimRunner::dfsTraverseLink(const BlockInfo& upstreamBlock, const PortMsg& upstreamPort, const BlockInfo& parentBlock, const PortMsg& parentPort, const QString &linkKey)
{
    qDebug() << "=== 进入 dfsTraverseLink ===";
    qDebug() << "linkKey:" << linkKey;
    qDebug() << "upstreamBlock:" << (upstreamBlock.block ? QString::fromStdString(upstreamBlock.block->GetName()) : "null")
             << "upstreamPort:" << upstreamPort.name << "(id:" << upstreamPort.id << ", topProtId:" << upstreamPort.topProtId << ")";
    qDebug() << "parentBlock:" << (parentBlock.block ? QString::fromStdString(parentBlock.block->GetName()) : "null")
             << "parentPort:" << parentPort.name << "(id:" << parentPort.id << ", topProtId:" << parentPort.topProtId << ")";
    AlgorithmManager* algoMgr = AlgorithmManager::createInstance();
    if (nullptr == algoMgr || linkKey.isEmpty()) { qDebug() << "错误：algoMgr为空或linkKey为空";return false; }
    QVector<Connection> connections = algoMgr->getConnection().value(linkKey);
    QVector<BlockInfo> allBlocks = algoMgr->getBlocksInfo().value(linkKey);
    qDebug() << "连接数量:" << connections.size() << ", 块数量:" << allBlocks.size();
    if (allBlocks.isEmpty() || connections.isEmpty()) { qDebug() << "链路中没有块或连接，返回true";return true; }

    // ========== 先识别并处理空子系统 ==========
    // 找出所有的空子系统
    QMap<int, bool> emptySubSystems;  // key: 子系统cmpId, value: 是否为空

    for (const BlockInfo& block : allBlocks) {
        if (block.isSubSystem && !block.childTopoId.isEmpty()) {
            if (isSubSystemEmpty(block.childTopoId)) {
                emptySubSystems[block.cmpId] = true;
                qDebug() << "发现空子系统:" << block.instanceName << "(ID:" << block.cmpId << ")";
            }
        }
    }

    // 处理空子系统相关的连接
    if (!emptySubSystems.isEmpty()) {
        qDebug() << "=== 处理空子系统 ===";

        // 先建立所有空子系统前后的直连
        for (const Connection& conn : connections) {
            bool bValid = false;
            int srcBlockId = conn.fromModelId().toInt(&bValid);
            int dstBlockId = conn.toModelId().toInt(&bValid);

            if (!bValid) continue;

            // 场景1: 源是空子系统
            if (emptySubSystems.contains(srcBlockId)) {
                qDebug() << "场景1: 源是空子系统:" << srcBlockId;

                // 找到谁连接到这个空子系统（输入连接）
                for (const Connection& inputConn : connections) {
                    int inputSrcId = inputConn.fromModelId().toInt(&bValid);
                    int inputDstId = inputConn.toModelId().toInt(&bValid);

                    if (inputDstId == srcBlockId) {
                        // 找到空子系统的输入源和目标
                        qDebug() << "  找到输入连接:" << inputSrcId << "->" << srcBlockId;

                        // 直接连接输入源到当前连接的目标
                        BlockInfo* srcBlock = nullptr;
                        BlockInfo* dstBlock = nullptr;
                        PortMsg srcPort, dstPort;

                        // 查找输入源的块和端口
                        for (BlockInfo& block : allBlocks) {
                            if (block.cmpId == inputSrcId) {
                                // 找到输入源的输出端口
                                for (const auto& port : block.portsMsg) {
                                    if (port.putType == "out") {
                                        srcBlock = &block;
                                        srcPort = port;
                                        break;
                                    }
                                }
                            }
                            // 查找当前连接的目标块和端口
                            if (block.cmpId == dstBlockId) {
                                for (const auto& port : block.portsMsg) {
                                    if (port.putType == "in") {
                                        dstBlock = &block;
                                        dstPort = port;
                                        break;
                                    }
                                }
                            }
                        }

                        if (srcBlock && dstBlock && srcBlock->block && dstBlock->block) {
                            QString connection = QString::number(srcBlock->cmpId) + ":" +
                                    QString::number(srcPort.id) + ":" +
                                    QString::number(dstBlock->cmpId) + ":" +
                                    QString::number(dstPort.id);

                            if (!mConnections.contains(connection)) {
                                Block::Connect(srcBlock->block, srcPort.name.toStdString(),
                                               dstBlock->block, dstPort.name.toStdString());
                                mConnections.push_back(connection);
                                qDebug() << "空子系统穿透成功：" <<
                                            srcBlock->instanceName << ":" <<
                                            srcPort.name << " -> " <<
                                            dstBlock->instanceName << ":" <<
                                            dstPort.name;
                            }
                        }
                    }
                }
            }

            // 场景2: 目标是空子系统
            if (emptySubSystems.contains(dstBlockId)) {
                qDebug() << "场景2: 目标是空子系统:" << dstBlockId;

                // 找到从这个空子系统出去的连接（输出连接）
                for (const Connection& outputConn : connections) {
                    int outputSrcId = outputConn.fromModelId().toInt(&bValid);
                    int outputDstId = outputConn.toModelId().toInt(&bValid);

                    if (outputSrcId == dstBlockId) {
                        // 找到空子系统的输出目标
                        qDebug() << "  找到输出连接:" << dstBlockId << "->" << outputDstId;

                        // 直接连接当前连接的源到空子系统的输出目标
                        BlockInfo* srcBlock = nullptr;
                        BlockInfo* dstBlock = nullptr;
                        PortMsg srcPort, dstPort;

                        // 查找当前连接的源块和端口
                        for (BlockInfo& block : allBlocks) {
                            if (block.cmpId == srcBlockId) {
                                for (const auto& port : block.portsMsg) {
                                    if (port.putType == "out") {
                                        srcBlock = &block;
                                        srcPort = port;
                                        break;
                                    }
                                }
                            }
                            // 查找输出目标的块和端口
                            if (block.cmpId == outputDstId) {
                                for (const auto& port : block.portsMsg) {
                                    if (port.putType == "in") {
                                        dstBlock = &block;
                                        dstPort = port;
                                        break;
                                    }
                                }
                            }
                        }

                        if (srcBlock && dstBlock && srcBlock->block && dstBlock->block) {
                            QString connection = QString::number(srcBlock->cmpId) + ":" +
                                    QString::number(srcPort.id) + ":" +
                                    QString::number(dstBlock->cmpId) + ":" +
                                    QString::number(dstPort.id);

                            if (!mConnections.contains(connection)) {
                                Block::Connect(srcBlock->block, srcPort.name.toStdString(),
                                               dstBlock->block, dstPort.name.toStdString());
                                mConnections.push_back(connection);
                                qDebug() << "空子系统穿透成功：" <<
                                            srcBlock->instanceName << ":" <<
                                            srcPort.name << " -> " <<
                                            dstBlock->instanceName << ":" <<
                                            dstPort.name;
                            }
                        }
                    }
                }
            }
        }
    }
    // =============================================

    for (const Connection& conn : connections)
    {
        qDebug() << "\n--- 处理新连接 ---";
        qDebug() << "原始连接信息:" << conn.fromModelId() << ":" << conn.fromPort()
                 << "->" << conn.toModelId() << ":" << conn.toPort();

        bool bValid = false;
        int srcBlockId  = conn.fromModelId().toInt(&bValid);
        int srcPortId   = conn.fromPort().toInt(&bValid);
        int dstBlockId  = conn.toModelId().toInt(&bValid);
        int dstPortId   = conn.toPort().toInt(&bValid);

        qDebug() << "解析后的连接:" << srcBlockId << ":" << srcPortId << "->" << dstBlockId << ":" << dstPortId;

        if(!bValid || srcBlockId<0 || srcPortId<0 || dstBlockId<0 || dstPortId<0)  {qDebug() << "连接解析无效，跳过";continue;}

        BlockInfo srcNode, dstNode;
        PortMsg srcPort, dstPort;
        bool srcFound = false, dstFound = false;
        for(const BlockInfo& block : allBlocks)
        {
            qDebug() << "检查块:" << block.instanceName << "(id:" << block.cmpId << ")";

            // 查找源块
            if(block.cmpId == srcBlockId && block.portsMsg.contains(srcPortId))
            {
                //前端有可能connect连反的情况，所以要根据端口puttype来区分
                if(block.portsMsg[srcPortId].putType=="in")
                {
                    dstNode = block; dstPort = block.portsMsg[srcPortId]; dstFound = true;
                    qDebug() << "  作为目标块找到:" << block.instanceName << "端口:" << dstPort.name;
                }
                else
                {
                    srcNode = block; srcPort = block.portsMsg[srcPortId]; srcFound = true;
                    qDebug() << "  作为源块找到:" << block.instanceName << "端口:" << srcPort.name;
                }
            }
            // 查找目标块
            if(block.cmpId == dstBlockId && block.portsMsg.contains(dstPortId))
            {
                if(block.portsMsg[dstPortId].putType=="in")
                {
                    dstNode = block; dstPort = block.portsMsg[dstPortId]; dstFound = true;
                    qDebug() << "  作为目标块找到:" << block.instanceName << "端口:" << dstPort.name;
                }
                else
                {
                    srcNode = block; srcPort = block.portsMsg[dstPortId]; srcFound = true;
                    qDebug() << "  作为源块找到:" << block.instanceName << "端口:" << srcPort.name;
                }
            }
        }

        if(!srcFound || !dstFound)
        {
            LOG_ERROR("链路：",linkKey.toStdString(),"，实例："+srcNode.instanceName.toStdString(),"和实例："+dstNode.instanceName.toStdString(),"(cmpId:",dstNode.cmpId,")连接关系有误.实例或端口不存在.");
            qDebug() << "错误：未找到源或目标块";
            return false;
        }
        // 只处理 OUT → IN 的合法连接 json文件可能有连线错误的情况，不能过滤
        //        if (srcPort.putType != "out" || dstPort.putType != "in") { continue; }

        qDebug() << "找到连接双方:";
        qDebug() << "  源:" << srcNode.instanceName << "(id:" << srcNode.cmpId << ", isSubSystem:" << srcNode.isSubSystem
                 << ", cmpType:" << srcNode.cmpType << ") 端口:" << srcPort.name
                 << "(id:" << srcPort.id << ", topProtId:" << srcPort.topProtId << ", putType:" << srcPort.putType << ")";
        qDebug() << "  目标:" << dstNode.instanceName << "(id:" << dstNode.cmpId << ", isSubSystem:" << dstNode.isSubSystem
                 << ", cmpType:" << dstNode.cmpType << ") 端口:" << dstPort.name
                 << "(id:" << dstPort.id << ", topProtId:" << dstPort.topProtId << ", putType:" << dstPort.putType << ")";

        // 检查是否是合法连接（out->in）
        if (!(srcPort.putType == "out" && dstPort.putType == "in")) {
            qDebug() << "警告：非标准连接方向 (src putType:" << srcPort.putType << ", dst putType:" << dstPort.putType << ")";
        }

        bool srcHasChild = !srcNode.childTopoId.isEmpty();
        bool dstHasChild = !dstNode.childTopoId.isEmpty();
        qDebug() << "子链路信息: srcHasChild=" << srcHasChild << "(" << srcNode.childTopoId << ")"
                 << ", dstHasChild=" << dstHasChild << "(" << dstNode.childTopoId << ")";

        //        // 源+目标都是容器
        //        if (srcNode.isSubSystem && dstNode.isSubSystem && srcHasChild && dstHasChild)
        //        {
        //            // 1. 递归穿透：源容器的子链路，执行子链路内所有连接逻辑
        //            dfsTraverseLink(dstNode, dstPort, srcNode, srcPort, srcNode.childTopoId);
        //            // 2. 递归穿透：目标容器的子链路，执行子链路内所有连接逻辑
        //            dfsTraverseLink(srcNode, srcPort, dstNode, dstPort, dstNode.childTopoId);
        //            continue;
        //        }

        // 场景A：源是容器/带子链路 → 穿透源的子链路
        if (srcNode.isSubSystem && srcHasChild)
        {
            qDebug() << "=== 场景A：源是容器/带子链路 ===";
            //目标端口是子链路的终点
            if(dstPort.topProtId!=-1)
            {
                qDebug() << "目标端口是锚点端口(topProtId=" << dstPort.topProtId << ")，递归处理上游连接";
                qDebug() << "场景A --srcNode.childTopoId: " << srcNode.childTopoId;
                if(!dfsTraverseLink(upstreamBlock, upstreamPort, srcNode, srcPort, srcNode.childTopoId))
                {
                    qDebug() << "场景A递归失败";
                    return false;
                }
                else
                {
                    qDebug() << "场景A递归成功，继续下一个连接";
                    continue;
                }
            }
            else
            {
                qDebug() << "目标端口不是锚点端口，递归处理下游连接";
                qDebug() << "场景A --srcNode.childTopoId: " << srcNode.childTopoId;
                if(!dfsTraverseLink(dstNode, dstPort, srcNode, srcPort, srcNode.childTopoId))
                {
                    qDebug() << "场景A递归失败";
                    return false;
                }
                else
                {
                    qDebug() << "场景A递归成功，继续下一个连接";
                    continue;
                }
            }
        }

        // 场景B：源不是容器，目标是容器/带子链路 → 穿透目标的子链路
        if (!srcNode.isSubSystem && dstNode.isSubSystem && dstHasChild)
        {
            qDebug() << "=== 场景B：目标是容器/带子链路 ===";
            //源端口是子链路的起点
            if(srcPort.topProtId!=-1)
            {
                qDebug() << "源端口是锚点端口(topProtId=" << srcPort.topProtId << ")，递归处理上游连接";
                if(!dfsTraverseLink(upstreamBlock, upstreamPort, dstNode, dstPort, dstNode.childTopoId))
                {
                    qDebug() << "场景B递归失败";
                    return false;
                }
                else
                {
                    qDebug() << "场景B递归成功，继续下一个连接";
                    continue;
                }
            }
            else
            {
                qDebug() << "源端口不是锚点端口，递归处理下游连接";
                if(!dfsTraverseLink(srcNode, srcPort, dstNode, dstPort, dstNode.childTopoId))
                {
                    qDebug() << "场景B递归失败";
                    return false;
                }
                else
                {
                    qDebug() << "场景B递归成功，继续下一个连接";
                    continue;
                }
            }
        }

        qDebug() << "=== 检查连接条件 ===";
        qDebug() << "srcNode.instance" << srcNode.instanceName << "dstNode.instanceName" << dstNode.instanceName;
        bool upstreamSubSystem= upstreamBlock.isSubSystem || !upstreamBlock.childTopoId.isEmpty();
        bool isSubLinkAnchor     = (srcPort.topProtId != -1) || (dstPort.topProtId != -1);
        qDebug() << "upstreamSubSystem: " << upstreamSubSystem;
        qDebug() << "dstPort.topProtId: " << dstPort.topProtId;
        qDebug() << "parentPort.id: " << parentPort.id;
        qDebug() << "srcPort.topProtId: " << srcPort.topProtId;

        // 入穿透：上游 → 子链路锚点目标 (上游非容器)
        if (!upstreamSubSystem && srcPort.topProtId == parentPort.id && dstPort.topProtId == -1)
        {

            qDebug() << "=== 场景：入穿透（上游→子链路锚点目标） ===";
            qDebug() << "连接：" << upstreamBlock.instanceName << ":" << upstreamPort.name
                     << " -> " << dstNode.instanceName << ":" << dstPort.name;
            //连接前检查，已连接的不再连接
            //            QString connection=QString::fromStdString(upstreamBlock.block->GetName())+":"+upstreamPort.name+":"+QString::fromStdString(dstNode.block->GetName())+":"+dstPort.name;
            QString connection=QString::number(upstreamBlock.cmpId)+":"+QString::number(upstreamPort.id)+":"+QString::number(dstNode.cmpId)+":"+QString::number(dstPort.id);
            if(mConnections.contains(connection)) {
                qDebug() << "连接已存在，跳过";
                continue;
            }
            Block::Connect(upstreamBlock.block, upstreamPort.name.toStdString(),
                           dstNode.block, dstPort.name.toStdString());
            qDebug() << "入穿透连接成功：" << upstreamBlock.instanceName << ":" << upstreamPort.name
                     << " -----> " << dstNode.instanceName << ":" << dstPort.name;

        }

        // 出穿透：子链路锚点源 → 上游（上游非容器）
        if (!upstreamSubSystem && dstPort.topProtId == parentPort.id && srcPort.topProtId == -1)
        {
            qDebug() << "=== 场景：出穿透（子链路锚点源→上游）===";
            qDebug() << "连接：" << srcNode.instanceName << ":" << srcPort.name
                     << " -> " << upstreamBlock.instanceName << ":" << upstreamPort.name;
            //连接前检查，已连接的不再连接
            //            QString connection=QString::fromStdString(srcNode.block->GetName())+":"+srcPort.name+":"+QString::fromStdString(upstreamBlock.block->GetName())+":"+upstreamPort.name;
            QString connection=QString::number(srcNode.cmpId)+":"+QString::number(srcPort.id)+":"+QString::number(upstreamBlock.cmpId)+":"+QString::number(upstreamPort.id);
            if(mConnections.contains(connection))
                continue;
            Block::Connect(srcNode.block, srcPort.name.toStdString(),
                           upstreamBlock.block, upstreamPort.name.toStdString());
            mConnections.push_back(connection);
            qDebug() << "出穿透连接成功：" << srcNode.instanceName << ":" << srcPort.name
                     << " -----> " << upstreamBlock.instanceName << ":" << upstreamPort.name;
        }

        // 出穿透：子链路锚点源  → 上游（上游容器）
        if (upstreamSubSystem && dstPort.topProtId == parentPort.id && srcPort.topProtId == -1)
        {
            qDebug() << "=== 场景：出穿透（子链路锚点源→上游容器） ===";
            qDebug() << "继续递归到父系统";
            if(!dfsTraverseLink(srcNode, srcPort, upstreamBlock, upstreamPort, upstreamBlock.childTopoId))
            {
                qDebug() << "递归失败";
                return false;
            }
            else
            {
                qDebug() << "递归成功";
                return true;
            }
        }

        // *****适配子链路只有起止节点的情况****

        //        qDebug() << "isSubLinkAnchor" << isSubLinkAnchor;
        // 基础业务连接，过滤锚点
        if (!isSubLinkAnchor)
        {
            //没有父节点或者子链路没有进行过基础连接才进行基础连接，否则会多次connect，导致报错
            qDebug() << "=== 场景：基础业务连接 ===";

            QString connection = QString("%1(%2) %3(%4) --> %5(%6) %7(%8)")
                    .arg(QString::fromStdString(srcNode.block->GetName()))
                    .arg(srcNode.cmpId)
                    .arg(srcPort.name)
                    .arg(srcPort.id)
                    .arg(QString::fromStdString(dstNode.block->GetName()))
                    .arg(dstNode.cmpId)
                    .arg(dstPort.name)
                    .arg(dstPort.id);
            qDebug() << "场景基础业务 --检查连接: connection: " << connection;
            if(mConnections.contains(connection))
                continue;
            Block::Connect(srcNode.block, srcPort.name.toStdString(),
                           dstNode.block, dstPort.name.toStdString());
            mConnections.push_back(connection);
            qDebug() << "基础连接成功：" << srcNode.instanceName << ":" << srcPort.name
                     << " -----> " << dstNode.instanceName << ":" << dstPort.name;

        }
    }
    qDebug() << "=== dfsTraverseLink 结束 ===";
    return true;
}

bool SimRunner::isSubSystemEmpty(const QString& subLinkKey)
{
    AlgorithmManager* algoMgr = AlgorithmManager::createInstance();
    if (!algoMgr) return false;

    QVector<BlockInfo> subBlocks = algoMgr->getBlocksInfo().value(subLinkKey);
    QVector<Connection> subConns = algoMgr->getConnection().value(subLinkKey);

    qDebug() << "检查子系统是否为空:" << subLinkKey;
    qDebug() << "  子块数量:" << subBlocks.size();
    qDebug() << "  连接数量:" << subConns.size();

    // 情况1：子链路中只有 inPort 和 outPort，且它们直接相连
    if (subBlocks.size() == 2 && subConns.size() == 1) {
        int inPortId = -1, outPortId = -1;
        QString inPortTopId, outPortTopId;

        // 识别 inPort 和 outPort
        for (const BlockInfo& block : subBlocks) {
            qDebug() << "  检查块:" << block.instanceName
                     << ", cmpType:" << block.cmpType
                     << ", cmpId:" << block.cmpId;  // block.cmpId 是 int 类型

            if (block.cmpType == "inPort") {
                inPortId = block.cmpId;
                // 获取 inPort 的 topProtId（从端口信息中）
                for (const auto& port : block.portsMsg) {
                    if (port.topProtId != -1) {
                        inPortTopId = QString::number(port.topProtId);
                        break;
                    }
                }
                qDebug() << "    inPort ID:" << inPortId << ", topProtId:" << inPortTopId;
            }
            else if (block.cmpType == "outPort") {
                outPortId = block.cmpId;
                // 获取 outPort 的 topProtId（从端口信息中）
                for (const auto& port : block.portsMsg) {
                    if (port.topProtId != -1) {
                        outPortTopId = QString::number(port.topProtId);
                        break;
                    }
                }
                qDebug() << "    outPort ID:" << outPortId << ", topProtId:" << outPortTopId;
            }
        }

        // 必须同时有 inPort 和 outPort
        if (inPortId == -1 || outPortId == -1) {
            qDebug() << "  缺少 inPort 或 outPort";
            return false;
        }

        // 检查连接 - 注意 Connection 中的 ID 是 QString 类型
        const Connection& conn = subConns.first();
        QString fromId = conn.fromModelId();  // 这是 QString，如 "cp_1"
        QString toId = conn.toModelId();      // 这是 QString，如 "cp_2"

        qDebug() << "  连接信息:" << fromId << ":" << conn.fromPort()
                 << " -> " << toId << ":" << conn.toPort();

        // 将 int 类型的 cmpId 转换为 QString 用于比较
        QString inPortIdStr = QString::number(inPortId);    // 如 "1"
        QString outPortIdStr = QString::number(outPortId);  // 如 "2"

        qDebug() << "  inPortIdStr:" << inPortIdStr << ", outPortIdStr:" << outPortIdStr;

        // 验证连接确实是在 inPort 和 outPort 之间
        // 两种情况：
        // 1. inPort -> outPort
        // 2. outPort -> inPort (方向可能相反)
        bool isValidConnection = false;

        if (fromId == inPortIdStr && toId == outPortIdStr) {
            isValidConnection = true;
            qDebug() << "  有效连接: inPort -> outPort";
        }
        else if (fromId == outPortIdStr && toId == inPortIdStr) {
            isValidConnection = true;
            qDebug() << "  有效连接: outPort -> inPort (方向相反)";
        }

        if (isValidConnection) {
            qDebug() << "  检测到空子系统（只有出入口且直接相连）";
            return true;
        }
        else {
            qDebug() << "  连接不在 inPort 和 outPort 之间";
            return false;
        }
    }

    // 情况2：子链路中没有任何块（理论上不应该发生）
    if (subBlocks.isEmpty()) {
        qDebug() << "  子链路为空（无任何块）";
        return true;
    }

    // 情况3：子链路中只有 inPort 和 outPort，但没有任何连接
    if (subBlocks.size() == 2 && subConns.isEmpty()) {
        bool hasInPort = false;
        bool hasOutPort = false;

        for (const BlockInfo& block : subBlocks) {
            if (block.cmpType == "inPort") hasInPort = true;
            if (block.cmpType == "outPort") hasOutPort = true;
        }

        if (hasInPort && hasOutPort) {
            LOG_WARN("子系统：", subLinkKey.toStdString(),
                     "，包含出入口但没有连接，视为空子系统");
            return true;
        }
    }

    qDebug() << "  不是空子系统";
    return false;
}

bool SimRunner::RunBlocks()
{
#if 1
//    return NewScheduler();
    return OldScheduler();
//    return TimeScheduler();
#else
    for (auto linkKey : mSimuParameters.keys())
    {
        int lastProgress = -1;
        for(int i=1;i<=mSimuParameters[linkKey].num_Samples;++i)
        {
            double j = (double)i / mSimuParameters[linkKey].num_Samples * 100;
            int currentProgress = (int)j;

            if(currentProgress % 10 == 0 && currentProgress != lastProgress)
            {
                LOG_INFO("当前进度：",j,"%");
                lastProgress = currentProgress; // 更新上一次进度
            }
        }
    }
    return true;
#endif
}

bool SimRunner::NewScheduler()
{
    // 使用新的调度器
    ReadyQueueScheduler scheduler;
    for (const QString& linkKey : AlgorithmManager::createInstance()->getRunBlocks().keys())
    {
        QVector<Block *> blocks = AlgorithmManager::createInstance()->getRunBlocks().value(linkKey);

        // 获取仿真参数
        SimuParameter simuParams;
        auto simuParamsMap = AlgorithmManager::createInstance()->getSimuParameters();
        if (simuParamsMap.contains(linkKey)) {
            simuParams = simuParamsMap.value(linkKey);
        }

        // 调用新调度器
        if (!scheduler.schedule(linkKey, blocks, m_verificationSystem, simuParams)) {
            LOG_ERROR("链路：", linkKey.toStdString(), "，调度失败");
            return false;
        }
    }
    return true;
}

//bool SimRunner::OldScheduler()
//{
//    //使用旧的调度器
//    for (const QString& linkKey : AlgorithmManager::createInstance()->getRunBlocks().keys())
//    {
//        QVector<Block *> blocks=AlgorithmManager::createInstance()->getRunBlocks().value(linkKey);
//        if(!SimpleScheduler(linkKey, blocks))
//            return false;
//    }
//    return true;
//}
bool SimRunner::OldScheduler()
{
    // 使用旧的调度器
    for (const QString& linkKey : AlgorithmManager::createInstance()->getRunBlocks().keys())
    {
        QVector<Block *> blocks = AlgorithmManager::createInstance()->getRunBlocks().value(linkKey);

        // 设置暂停控制成员
        m_simpleScheduler.setPauseControls(&m_paused, &m_stopRequested, &m_pauseMutex, &m_pauseCond);

        // 获取仿真参数
        SimuParameter simuParams;
        auto simuParamsMap = AlgorithmManager::createInstance()->getSimuParameters();
        if (simuParamsMap.contains(linkKey)) {
            simuParams = simuParamsMap.value(linkKey);
        }

        // 调用数据流调度器
        if(!m_simpleScheduler.schedule(linkKey, blocks, m_verificationSystem, simuParams))
            return false;
    }
    return true;
}

bool SimRunner::TimeScheduler()
{
    m_activeScheduler = ActiveScheduler::TIME_DRIVEN;

    for (const QString& linkKey : AlgorithmManager::createInstance()->getRunBlocks().keys())
    {
        QVector<Block *> blocks = AlgorithmManager::createInstance()->getRunBlocks().value(linkKey);

        // ========== 步骤1: 初始化 ==========
        if (!m_timeDrivenScheduler.InitializeScheduler(
                linkKey, blocks, &m_topologySorter))
        {
            LOG_ERROR("[SimRunner] TimeDrivenScheduler 初始化失败");
            m_activeScheduler = ActiveScheduler::NONE;
            return false;
        }

        // ========== 步骤2: 启动仿真 ==========
        // 将暂停/停止控制变量传递给调度器
        // 调度器内部会自行检测这些标志，与 SimpleScheduler 设计一致
        LOG_INFO("[SimRunner] 启动时间驱动仿真...");

        bool result = m_timeDrivenScheduler.RunSimulation(
            linkKey,
            &m_paused,           // 暂停标志
            &m_stopRequested,    // 停止标志
            &m_pauseMutex,       // 暂停互斥锁
            &m_pauseCond         // 暂停条件变量
        );

        if (!result) {
            qDebug() << "[SimRunner] TimeDrivenScheduler 仿真失败";
            m_activeScheduler = ActiveScheduler::NONE;
            return false;
        }
        qDebug() << "[SimRunner] 时间驱动仿真结束";
    }

    m_activeScheduler = ActiveScheduler::NONE;
    return true;
}

ISimRunner *createSimRunner(    const char* appPath,
                                const char** linkFiles,
                                int fileCount,
                                const char* outPutPath)
{
    // 直接转换为QString，使用fromUtf8
    QString qAppPath = QString::fromUtf8(appPath);
    QString qOutPutPath = QString::fromUtf8(outPutPath);

    return new SimRunner(appPath, linkFiles, fileCount, outPutPath);
}
