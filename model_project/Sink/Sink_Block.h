#ifndef SINK_BLOCK_H
#define SINK_BLOCK_H

#include "Sink.h"
#include "Block.h"

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
    #if defined(_WIN32) || defined(__cpp_lib_filesystem)
        #include <filesystem>
        namespace fs = std::filesystem;
    #else
        #include <experimental/filesystem>
        namespace fs = std::experimental::filesystem;
    #endif
#endif

using namespace SystemVueModelBuilder;

// 缓冲区中单个数据点的结构：时间戳 + 数值
struct DataPoint {
    double time;
    double value;
};

class SYSTEMVUEMODELBUILDER_API Sink_Block : public SystemVueModelBuilder::Block
{
public:
    Sink_Block(const std::string& name);
    ~Sink_Block();

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
    bool Done() override;
    bool Flush() override;
    bool IsCollectionComplete() override;

    void SetParameters(int SampleStart = 0, int SampleStop = 1,
                       double TimeStart = 0.0, double TimeStop = 1.0,
                       Sink::SelectedStartStopOption StartStopOption = Sink::Auto,
                       char* Filename = nullptr);

private:
    Sink::SelectedStartStopOption ConvertStringToSelected(const std::string& value);
    void CopyStringToCharPtr(const std::string& src, char*& dest);
    char* CopyStringToCharPtr(const std::string& src);
    char* combinePathWithJsonSuffix(const fs::path& linkKeyFolder, const char* m_fileName);
    void SetDefaultParameters();

    // 文件操作方法
    bool openFileForWrite();          // 首次创建文件并写入 [
    bool openFileForAppend();         // 追加模式打开
    void closeFileProperly();         // 补全 ] 并关闭文件
    void cleanup();

    // 写入单个数据点（使用缓冲区中的时间和值）
    void writeDataPointToStream(size_t bufferIndex, unsigned long long dataIndex);

    // 缓冲区满时的批量写入
    void RunDealData();

    // 时间驱动中途刷新（仍保留，但遵循新结构）
    void flushToFile();
    bool isTimeDrivenMode() const;
    void setTimeDrivenMode(bool enabled);
    double GetCurrentSimulationTime() const;

    // 成员变量
    QFile m_qfile;
    QTextStream m_stream;

    std::unique_ptr<Sink> m_sink;

    Sink::SelectedStartStopOption m_StartStopOption;
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

    QString m_WritePath;               // 后端路径，格式 /01/xxx.json

    // 驱动模式相关
    bool m_isTimeDrivenMode = false;
    bool m_fileOpenedForAppend = false;
    int m_flushCounter = 0;
    int m_flushInterval = 100;
    double m_currentSimulationTime = 0.0;
};

RegAlgo(Sink_Block);

#endif // SINK_BLOCK_H
