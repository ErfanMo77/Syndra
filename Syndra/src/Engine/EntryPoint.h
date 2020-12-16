#pragma once
#include "Application.h"


#ifdef FB_PLATFORM_WINDOWS

extern Engine::Application* Engine::CreateApplication();

int main(int argc, char** argv) 
{
	Engine::Log::init();
	EG_CORE_WARN("This is from Engine!");
	EG_INFO("HELLO!");

	auto app = Engine::CreateApplication();
	app->Run();
	delete app;
}

#endif // FB_PLATFORM_WINDOWS
