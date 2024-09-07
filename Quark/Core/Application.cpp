#include "Quark/qkpch.h"

#include <nfd.hpp>

#include "Quark/Core/Application.h"
#include "Quark/Core/KeyMouseCodes.h"
#include "Quark/Core/Window.h"
#include "Quark/Core/Input.h"
#include "Quark/Events/EventManager.h"
#include "Quark/Events/ApplicationEvent.h"
#include "Quark/Asset/AssetManager.h"
#include "Quark/Renderer/GpuResourceManager.h"
#include "Quark/Renderer/ShaderManager.h"

#ifdef USE_VULKAN_DRIVER
#include "Quark/Graphic/Vulkan/Device_Vulkan.h"
#endif

namespace quark {

Application* Application::s_Instance = nullptr;

Application::Application(const AppInitSpecs& specs) 
{
    s_Instance = this;
    
    // Init moudules, the order is important
    Logger::Init();

    EventManager::CreateSingleton();

    Window::Create();
    Window::Instance()->Init(specs.title, specs.isFullScreen, specs.width, specs.height);

    CORE_ASSERT(NFD::Init() == NFD_OKAY)

    Input::CreateSingleton();
    Input::Get()->Init();

#ifdef  USE_VULKAN_DRIVER
    m_GraphicDevice = CreateScope<graphic::Device_Vulkan>();
    m_GraphicDevice->Init();
#endif

    GpuResourceManager::CreateSingleton();
    GpuResourceManager::Get().Init();

    ShaderManager::CreateSingleton();

    AssetManager::CreateSingleton();

    // Init UI system
    UI::CreateSingleton();
    UI::Get()->Init(m_GraphicDevice.get(), specs.uiSpecs);

    // Register application callback functions
    EventManager::Get().Subscribe<WindowCloseEvent>([this](const WindowCloseEvent& event) { OnWindowClose(event);});
    EventManager::Get().Subscribe<WindowResizeEvent>([this](const WindowResizeEvent& event) { OnWindowResize(event); });

}

Application::~Application() {

    UI::Get()->Finalize();
    UI::FreeSingleton();

    AssetManager::FreeSingleton();

    ShaderManager::FreeSingleton();

    GpuResourceManager::Get().Shutdown();
    GpuResourceManager::FreeSingleton();

    m_GraphicDevice->ShutDown();
    m_GraphicDevice.reset();

    // Destroy InputManager
    Input::Get()->Finalize();
    Input::FreeSingleton();

    // Destroy window
    Window::Instance()->Finalize();
    Window::Destroy();

    EventManager::FreeSingleton();

}

void Application::Run()
{
    while (m_Status.isRunning) 
    {
        f64 start_frame = m_Timer.ElapsedSeconds();

        // Poll events
        Input::Get()->OnUpdate();
        
        if (!m_Status.isMinimized)
        {
            // TODO: Multithreading
            OnUpdate(m_Status.lastFrameDuration);

            OnImGuiUpdate();

            OnRender(m_Status.lastFrameDuration);
        }

        // Dispatch events
        EventManager::Get().DispatchEvents();

        m_Status.lastFrameDuration = m_Timer.ElapsedSeconds() - start_frame;
        m_Status.fps = 1.f / m_Status.lastFrameDuration;
    }
}

void Application::OnWindowClose(const WindowCloseEvent& e)
{
    m_Status.isRunning = false;
}

void Application::OnWindowResize(const WindowResizeEvent& event)
{
    if (event.width == 0 || event.height == 0)
    {
        m_Status.isMinimized = true;
        return;
    }

    m_Status.isMinimized = false;
}

}