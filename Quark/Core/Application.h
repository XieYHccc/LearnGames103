#pragma once
#include "Core/Timer.h"
#include "Events/ApplicationEvent.h"
#include "Graphic/Device.h"


struct AppInitSpecs {
    std::string title = "Quark Application";
    std::uint32_t width = 1200;
    std::uint32_t height = 800;
    std::uint32_t uiInitFlags = 0;
    bool isFullScreen = false;
};  

class Application {
public:
    static Application& Instance() { return *singleton_; }
private:
    static Application* singleton_;
public:
    Application(const AppInitSpecs& specs);
    virtual ~Application();
    Application(const Application&) = delete;
    const Application& operator=(const Application&) = delete;

    void Run();
    graphic::Device* GetGraphicDevice() { return m_GraphicDevice.get();}

private:
    // Update Game Logic per frame
    virtual void Update(f32 deltaTime) = 0;

    // Render per frame : All rendering cmd list recording in this func
    virtual void Render(f32 deltaTime) = 0;

    // Prepare UI data
    virtual void UpdateUI() {};

    // Callback functions for events
    void OnWindowClose(const WindowCloseEvent& event);
    void OnWindowResize(const WindowResizeEvent& event) {}

protected:
    struct AppStatus 
    {
        f32 fps { 0 };
        bool isRunning { true };
        f64 lastFrameDuration { 0 };
    };

    Timer m_Timer;
    AppStatus m_Status;
    Scope<graphic::Device> m_GraphicDevice;
};

// To be defined in CLIENT
Application* CreateApplication();