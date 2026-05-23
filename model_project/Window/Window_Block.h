#ifndef WINDOW_BLOCK_H
#define WINDOW_BLOCK_H

#include "Block.h"
#include "Window.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Window_Block : public SystemVueModelBuilder::Block
{
public:
    Window_Block(const std::string& name);
    ~Window_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();
    void SetParameters();

    Window::SelectedWindowType ConvertStringToWindowType(const std::string& value);
    Window::SelectedShowAdvancedParams ConvertStringToShowAdvancedParams(const std::string& value);
    Window::SelectedSampleRateOption ConvertStringToSampleRateOption(const std::string& value);

    std::unique_ptr<Window> m_window;

    Window::SelectedWindowType m_windowType;
    int m_length;
    int m_zeroPad;
    double m_kaiserParameter;
    Window::SelectedShowAdvancedParams m_showAdvancedParams;
    Window::SelectedSampleRateOption m_sampleRateOption;
    double m_sampleRate;
    int m_initialDelay;

    SimuParameter simulator_param;
};

RegAlgo(Window_Block);

#endif // WINDOW_BLOCK_H
