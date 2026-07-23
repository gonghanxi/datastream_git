#ifndef SINK_M_BLOCK_H
#define SINK_M_BLOCK_H
#include "Sink_M.h"
#include "Block.h"
using namespace SystemVueModelBuilder;

#ifdef __has_include
    #if __has_include(<filesystem>)
        #include <filesystem>
        namespace fs = std::filesystem;
    #elif __has_include(<experimental/filesystem>)
        #include <experimental/filesystem>
        namespace fs = std::experimental::filesystem;
    #else
        #error "Missing filesystem support"
    #endif
#else
    // 对于不支持 __has_include 的编译器
    #if defined(_WIN32) || defined(__cpp_lib_filesystem)
        #include <filesystem>
        namespace fs = std::filesystem;
    #else
        #include <experimental/filesystem>
        namespace fs = std::experimental::filesystem;
    #endif
#endif

// 缓冲区中单个数据点的结构：时间戳 + 数值
struct DataPoint {
    double time; // 时间戳（秒）
    DoubleMatrix value;// 数据值
};

class SYSTEMVUEMODELBUILDER_API Sink_M_Block : public Block
{
public:
    Sink_M_Block(const std::string& name);
    ~Sink_M_Block();

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
    bool Done() override;
    bool Flush() override;
    bool IsCollectionComplete() override;

    void SetParameters();

private:

    Sink_M::SelectedStartStopOption ConvertStringToSelected(const std::string& value);
    void CopyStringToCharPtr(const std::string& src, char*& dest);
    char* CopyStringToCharPtr(const std::string& src);
    char* combinePathWithJsonSuffix(const fs::path& linkKeyFolder, const char* m_fileName);
    void SetDefaultParameters();

    // 文件操作
    bool openFileForWrite();          // 首次创建文件并写入 [
    bool openFileForAppend();         // 追加模式打开（用于Flush中途写入）
    void closeFileProperly();         // 补全 ] 并关闭文件
    void cleanup();

    // 核心写入方法：将缓冲区中第 bufferIndex 个点写入流，使用 dataIndex 作为序号
    void writeDataPointToStream(size_t bufferIndex, unsigned long long dataIndex);
    void RunDealData(); // 缓冲区满时批量写入
    void flushToFile(); // 时间驱动模式下定期刷新

    bool isTimeDrivenMode() const;
    void setTimeDrivenMode(bool enabled);
    double GetCurrentSimulationTime() const;

    // 修改成员变量
    QFile m_qfile;  // 使用QFile替代std::ofstream
    QTextStream m_stream;

    std::unique_ptr<Sink_M> m_sink;

    Sink_M::SelectedStartStopOption m_StartStopOption;
    int m_SampleStart;
    int m_SampleStop;
    double m_TimeStart;
    double m_TimeStop;
    double m_sampleRate;
    char* m_fileName;


    QString m_fullPath;
    unsigned long long Index;          // 实际成功记录的数据点序号（每写入一条递增）
    size_t m_iBuffer;                  // 当前缓冲区中数据点个数
    DataPoint* m_pdBuffer;             // 缓冲区（存储时间和数值）
    char* FileName;
    std::string m_UserId;
    int numCols;
    int numRows;

    //后端需要的写入路径 格式为 /01/xxx.json
    QString m_WritePath;

    // 驱动模式相关
    bool m_isTimeDrivenMode = false;
    bool m_fileOpenedForAppend = false;
    int m_flushCounter = 0;
    int m_flushInterval = 100;
    double m_currentSimulationTime = 0.0;

    // SINK输出截断控制
    unsigned long long m_sinkTargetSamples = ULLONG_MAX;
    unsigned long long m_sinkSkipSamples = 0; // 头部跳过的采样点数，默认0
};

RegAlgo(Sink_M_Block);

#endif // SINK_M_BLOCK_H
