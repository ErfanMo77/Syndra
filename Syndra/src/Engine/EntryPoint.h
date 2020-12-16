#pragma once
#include "Application.h"


#ifdef FB_PLATFORM_WINDOWS

extern Syndra::Application* Syndra::CreateApplication();

int main(int argc, char** argv) 
{
	Syndra::Log::init();
	EG_CORE_WARN("This is from Engine!");
	EG_INFO("HELLO!");

	auto app = Syndra::CreateApplication();
	app->Run();
	delete app;
}

#endif // FB_PLATFORM_WINDOWS
