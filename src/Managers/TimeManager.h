#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <GLFW/glfw3.h>


namespace Neki
{



class TimeManager
{
public:
	static void NewFrame();
	static double dt;

private:
	static double lastTime;
};



}



#endif