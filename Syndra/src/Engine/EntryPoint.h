#pragma once
#include "Application.h"


#ifdef SN_PLATFORM_WINDOWS

extern Syndra::Application* Syndra::CreateApplication();

int main(int argc, char** argv) 
{
	Syndra::Log::init();
	SN_CORE_WARN("This is from Engine!");
	SN_INFO("HELLO!");

	auto app = Syndra::CreateApplication();
	app->Run();
	delete app;
}

#endif // FB_PLATFORM_WINDOWS
