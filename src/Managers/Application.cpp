#include "Application.h"

#include "TimeManager.h"

namespace Neki
{



Application::Application(const VKAppCreationDescription& _vkAppCreationDescription)
{
	vkApp = std::make_unique<VKApp>(_vkAppCreationDescription);
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
	vkApp->DrawFrame();
}






}