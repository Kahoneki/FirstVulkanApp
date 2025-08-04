#include "Application.h"

#include "InputManager.h"
#include "TimeManager.h"

namespace Neki
{



Application::Application(const VKAppCreationDescription& _vkAppCreationDescription)
{
	vkApp = std::make_unique<VKApp>(_vkAppCreationDescription);
	camera = std::make_unique<PlayerCamera>(30.0f, 0.05f, *(vkApp->vulkanRenderManager), glm::vec3(0,0,3), glm::vec3(0,1,0), 0.0f, 0.0f, 0.1f, 100.0f, 90.0f);
}



Application::~Application(){}



void Application::Start()
{
	while (!vkApp->vulkanRenderManager->WindowShouldClose())
	{
		RunFrame();
	}
}



void Application::RunFrame()
{
	glfwPollEvents();
	TimeManager::NewFrame();
	InputManager::UpdateInput(vkApp->vulkanRenderManager->GetWindow());
	camera->Update();
	vkApp->DrawFrame(*camera);
}






}