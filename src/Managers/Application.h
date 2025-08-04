#ifndef APPLICATION_H
#define APPLICATION_H
#include "../Vulkan/VKApp.h"


namespace Neki
{



class Application final
{
public:
	explicit Application(const VKAppCreationDescription& _vkAppCreationDescription);
	~Application();

	//Start the main frame loop
	void Start();
	
private:
	void RunFrame();
	
	std::unique_ptr<VKApp> vkApp;
};



}



#endif