#include "Core/Input.h"

#include <GLFW/glfw3.h>

#include "Core/Window.h"

// bool Input::first_mouse = true;
// MousePosition Input::last_position;

// bool Input::IsKeyPressed(Keycode key)
// {
//     auto* window = static_cast<GLFWwindow*>(Window::Instance()->GetNativeWindow());
//     auto state = glfwGetKey(window, key);
//     return state == GLFW_PRESS;
// }

// bool Input::IsKeyReleased(Keycode key)
// {
//     auto* window = static_cast<GLFWwindow*>(Window::Instance()->GetNativeWindow());
//     auto state = glfwGetKey(window, key);
//     return state == GLFW_RELEASE;
// }

// bool Input::IsMousePressed(MouseCode button)
// {
//     auto* window = static_cast<GLFWwindow*>(Window::Instance()->GetNativeWindow());
//     auto state = glfwGetMouseButton(window, button);
//     return state == GLFW_PRESS || state == GLFW_REPEAT;
// }

// MousePosition Input::GetMousePosition()
// {
//     auto* window = static_cast<GLFWwindow*>(Window::Instance()->GetNativeWindow());
//     double xPos, yPos;
//     MousePosition pos;
//     glfwGetCursorPos(window, &xPos, &yPos);
//     pos.x_pos = (float)xPos;
//     pos.y_pos = (float)yPos;
//     return pos;
// }

// float Input::GetMouseX()
// {
//     return GetMousePosition().x_pos;
// }

// float Input::GetMouseY()
// {
//     return GetMousePosition().y_pos;
// }

bool InputManager::IsKeyPressed(Keycode key, bool repeat) const
{
    if (repeat) {
        return keyMouseStatus_[key] == KeyAction::KEY_PRESSED
            || keyMouseStatus_[key] == KeyAction::KEY_KEEP_PRESSED;
    }
    else
        return keyMouseStatus_[key] == KeyAction::KEY_PRESSED;
}


bool InputManager::IsKeyReleased(Keycode key) const
{
    return keyMouseStatus_[key] == KeyAction::KEY_RELEASED;
}

bool InputManager::IsKeyKeepPressed(Keycode key) const
{
    return keyMouseStatus_[key] == KeyAction::KEY_KEEP_PRESSED;
}

bool InputManager::IsMousePressed(MouseCode button) const
{
    return keyMouseStatus_[button] == KeyAction::KEY_PRESSED;
}

bool InputManager::IsMouseReleased(MouseCode button) const
{
    return keyMouseStatus_[button] == KeyAction::KEY_RELEASED;
}



MousePosition InputManager::GetMousePosition() const
{
    return mousePosition_;
}



