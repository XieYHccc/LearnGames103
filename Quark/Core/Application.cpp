#include "Quark/qkpch.h"

#include <nfd.hpp>

#include "Quark/Core/Application.h"
#include "Quark/Core/KeyMouseCodes.h"
#include "Quark/Core/Window.h"
#include "Quark/Core/Input.h"
#include "Quark/Events/EventManager.h"
#include "Quark/Events/ApplicationEvent.h"
#include "Quark/Asset/AssetManager.h"

#ifdef USE_VULKAN_DRIVER
#include "Quark/Graphic/Vulkan/Device_Vulkan.h"
#endif

namespace quark {

Application* Application::s_Instance = nullptr;

Application::Application(const AppInitSpecs& specs) 
{
    s_Instance = this;
    
    Logger::Init();

    AssetManager::CreateSingleton();
    
    EventManager::Instance().Init();

    Window::Create();
    Window::Instance()->Init(specs.title,specs.isFullScreen,  specs.width, specs.height);

    CORE_ASSERT(NFD::Init() == NFD_OKAY)

    Input::CreateSingleton();
    Input::Get()->Init();

    // Init graphic device
#ifdef  USE_VULKAN_DRIVER
    m_GraphicDevice = CreateScope<graphic::Device_Vulkan>();
    m_GraphicDevice->Init();
#endif

    // Init UI system
    UI::CreateSingleton();
    UI::Get()->Init(m_GraphicDevice.get(), specs.uiSpecs);

    // Register application callback functions
    EventManager::Instance().Subscribe<WindowCloseEvent>([this](const WindowCloseEvent& event) { OnWindowClose(event);});
}

Application::~Application() {

    // Destroy UI system
    UI::Get()->Finalize();
    UI::FreeSingleton();
    
#ifdef  USE_VULKAN_DRIVER
    m_GraphicDevice->ShutDown();
#endif

    // Destroy InputManager
    Input::Get()->Finalize();
    Input::FreeSingleton();

    // Destroy window
    Window::Instance()->Finalize();
    Window::Destroy();

    // destroy event manager
    EventManager::Instance().Finalize();
}

void Application::Run()
{
    while (m_Status.isRunning) {
        f64 start_frame = m_Timer.ElapsedSeconds();

        // Poll events
        Input::Get()->Update();
        
        // TODO: Multithreading
        // Update each moudule (including processing inputs)
        OnUpdate(m_Status.lastFrameDuration);

        // Render Scene
        OnRender(m_Status.lastFrameDuration);

        // Dispatch events
        EventManager::Instance().DispatchEvents();

        m_Status.lastFrameDuration = m_Timer.ElapsedSeconds() - start_frame;
        m_Status.fps = 1.f / m_Status.lastFrameDuration;
    }
}

void Application::OnWindowClose(const WindowCloseEvent& e)
{
    m_Status.isRunning = false;
}

}