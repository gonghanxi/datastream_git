#ifndef SINKFXP_BLOCK_H
#define SINKFXP_BLOCK_H

#include "SinkFxp.h"
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

// DataPoint struct already defined in Sink_Block.h; guard for standalone compilation
#ifndef DATAPOINT_DEFINED
#define DATAPOINT_DEFINED
struct DataPoint {
    double time;
    double value;
};
#endif

class SYSTEMVUEMODELBUILDER_API SinkFxp_Block : public SystemVueModelBuilder::Block
{
public:
    SinkFxp_Block(const std::string& name);
    ~SinkFxp_Block();

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
    bool Done() override;
    bool Flush() override;
    bool IsCollectionComplete() override;

    void SetParameters(int SampleStart = 0, int SampleStop = 1,
                       double TimeStart = 0.0, double TimeStop = 1.0,
                       SinkFxp::SelectedStartStopOption StartStopOption = SinkFxp::Auto,
                       char* Filename = nullptr);

private:
    SinkFxp::SelectedStartStopOption ConvertStringToSelected(const std::string& value);
    void CopyStringToCharPtr(const std::string& src, char*& dest);
    char* CopyStringToCharPtr(const std::string& src);
    char* combinePathWithJsonSuffix(const fs::path& linkKeyFolder, const char* m_fileName);
    void SetDefaultParameters();

    bool openFileForWrite();
    bool openFileForAppend();
    void closeFileProperly();
    void cleanup();

    void writeDataPointToStream(size_t bufferIndex, unsigned long long dataIndex);
    void RunDealData();
    void flushToFile();

    bool isTimeDrivenMode() const;
    void setTimeDrivenMode(bool enabled);
    double GetCurrentSimulationTime() const;

    QFile m_qfile;
    QTextStream m_stream;

    std::unique_ptr<SinkFxp> m_sinkFxp;

    SinkFxp::SelectedStartStopOption m_StartStopOption;
    int m_SampleStart;
    int m_SampleStop;
    double m_TimeStart;
    double m_TimeStop;
    double m_sampleRate;
    char* m_fileName;

    QString m_fullPath;
    unsigned long long Index;
    size_t m_iBuffer;
    DataPoint* m_pdBuffer;
    char* FileName;
    std::string m_UserId;

    QString m_WritePath;

    bool m_isTimeDrivenMode = false;
    bool m_fileOpenedForAppend = false;
    int m_flushCounter = 0;
    int m_flushInterval = 100;
    double m_currentSimulationTime = 0.0;

    int m_fxpPos;
    double m_fxpFactor;

    // SINK输出截断控制
    unsigned long long m_sinkTargetSamples = ULLONG_MAX;
    unsigned long long m_sinkSkipSamples = 0; // 头部跳过的采样点数，默认0
};

RegAlgo(SinkFxp_Block);

#endif // SINKFXP_BLOCK_H
