#include "qkpch.h"
#include "Core/Window.h"

#if defined(QK_PLATFORM_WINDOWS) || defined(QK_PLATFORM_MACOS)
#include "Platform/MacOS/WindowGLFW.h"
#endif

Window* Window::singleton_ = nullptr;

Window* Window::Instance()
{
    return singleton_;
}

void Window::Create()
{
#if defined(QK_PLATFORM_WINDOWS) || defined(QK_PLATFORM_MACOS)
    Window::singleton_ = new WindowGLFW();
#else
    #error "Platform doesn't support window"
#endif

}

void Window::Destroy()
{
    if(singleton_ != nullptr)
    {
        singleton_->Finalize();
        delete singleton_;
    }
}

