#pragma once
#include "Application.h"
#include "Instrument.h"

#ifdef SN_PLATFORM_WINDOWS

extern Syndra::Application* Syndra::CreateApplication();
extern "C" {
	__declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001;
}

int main(int argc, char** argv) 
{
	Syndra::Log::init();
	SN_WARN("HELLO! Welcome to Syndra!");

	auto app = Syndra::CreateApplication();
	SN_PROFILE_BEGIN_SESSION("Runtime", "Runtime.json");
	app->Run();
	SN_PROFILE_END_SESSION();
	delete app;
}

#endif // SN_PLATFORM_WINDOWS
