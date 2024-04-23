#include "VulkanApp.h"
#include <GLFW/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <imgui.h>
#include <Application/Window/Input.h>
#include <Graphics/Vulkan/RendererVulkan.h>
#include <Scene/SceneMngr.h>

Application* CreateApplication()
{
    auto application = new VulkanApp("test"," ", 1300, 800);

    // load scene
    application->scene = SceneMngr::Instance().LoadGltfScene("/Users/xieyhccc/develop/XEngine/src/Assets/Gltf/structure.glb");

    // add a camera
    auto cam = application->scene->AddGameObject("MainCamera");
    cam->transformCmpt->SetPosition(glm::vec3(0, 0, 100));
    application->scene->SetMainCamera(cam->AddComponent<CameraCmpt>());
    application->yaw = 0;
    application->pitch = 0;

    return application;
}


VulkanApp::~VulkanApp()
{
    
}

void VulkanApp::Update()
{
    CameraCmpt* cam = scene->GetMainCamera();
    TransformCmpt& camTrans = *(cam->GetOwner()->transformCmpt);
    float moveSpeed = 40;
    float mouseSensitivity = 0.3;

    // 1. process mouse inputs
    MousePosition pos = Input::GetMousePosition();
    if (Input::first_mouse_) {
        Input::last_position_ = pos;
        Input::first_mouse_ = false;
    }

    float xoffset = pos.x_pos - Input::last_position_.x_pos;
    float yoffset = pos.y_pos - Input::last_position_.y_pos;

    pitch -= (glm::radians(yoffset) * mouseSensitivity);
    yaw -= (glm::radians(xoffset) * mouseSensitivity);
    // make sure that when pitch is out of bounds, screen doesn't get flipped
    pitch = std::clamp(pitch, -1.5f, 1.5f);

    Input::last_position_ = pos;

    // 2. process keyboard inputs
    glm::vec3 move {0.f};
    if (Input::IsKeyPressed(W))
        move.z = -1;
    if (Input::IsKeyPressed(S))
        move.z = 1;
    if (Input::IsKeyPressed(A))
        move.x = -1;
    if (Input::IsKeyPressed(D))
        move.x = 1;
    move = move * moveSpeed * deltaTime_;

    // 3.update camera's transform
    camTrans.SetEuler(glm::vec3(pitch, yaw, 0));
    glm::mat4 rotationMatrix = glm::toMat4(camTrans.GetQuat());
    camTrans.SetPosition(camTrans.GetPosition() + glm::vec3(rotationMatrix * glm::vec4(move, 0.f)));


    scene->Update();
}

void VulkanApp::Render()
{   
    // 1. prepare imgui data (not executing render commands here)
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ImGui::ShowDemoWindow();
    if (ImGui::Begin("Stats")) {
        ImGui::End();
    }

    ImGui::Render();

    // 2. render frame
    RendererVulkan::GetInstance()->BeginFrame();

    GpuImageVulkan& drawImage = RendererVulkan::GetInstance()->GetBuiltInDrawImage();
    GpuImageVulkan& depthmage = RendererVulkan::GetInstance()->GetBuiltInDepthImage();

    VkImage currentPresentImage = RendererVulkan::GetInstance()->GetCurrentPresentImage();
    VkImageView currentPresentImageView = RendererVulkan::GetInstance()->GetCurrentPresentImageView();
    VkExtent2D swapchainExtent = RendererVulkan::GetInstance()->GetSwapCainExtent();


    VkExtent2D drawExtent;
    drawExtent.height = std::min(swapchainExtent.height, drawImage.imageExtent.height);
	drawExtent.width= std::min(swapchainExtent.width, drawImage.imageExtent.width);


    // draw background
    RendererVulkan::GetInstance()->TransitionImageLayout(drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    RendererVulkan::GetInstance()->DrawBackGround();
    // draw geometry
    RendererVulkan::GetInstance()->TransitionImageLayout(drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    RendererVulkan::GetInstance()->TransitionImageLayout(depthmage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR);
    RendererVulkan::GetInstance()->DrawGeometry(drawImage.imageView, depthmage.imageView, drawExtent, scene->GetDrawContext());
    // copy image to present image
    RendererVulkan::GetInstance()->TransitionImageLayout(drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    RendererVulkan::GetInstance()->TransitionImageLayout(currentPresentImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    RendererVulkan::GetInstance()->CopyImagetoImage(drawImage.image, currentPresentImage, drawExtent, swapchainExtent);
    // draw imgui
    RendererVulkan::GetInstance()->TransitionImageLayout(currentPresentImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    RendererVulkan::GetInstance()->DrawImgui(currentPresentImageView, swapchainExtent);
    RendererVulkan::GetInstance()->TransitionImageLayout(currentPresentImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    RendererVulkan::GetInstance()->EndFrame();
}