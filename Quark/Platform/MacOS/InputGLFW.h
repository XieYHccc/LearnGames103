#pragma once
#include "Core/Input.h"
#include <GLFW/glfw3.h>

class InputGLFW : public Input {
public:
    void Init() override;
    void Update() override;
    void Finalize() override;

private:
    friend class WindowGLFW;
    friend class MakeSingletonPtr<Input>;

    InputGLFW() = default;

    void RecordKey(int key, int action);
    void RecordMousePosition(float xpos, float ypos);

    GLFWwindow* window_ = nullptr;
};