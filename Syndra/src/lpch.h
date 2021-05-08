#pragma once

#ifdef HZ_PLATFORM_WINDOWS
#ifndef NOMINMAX
// See github.com/skypjack/entt/wiki/Frequently-Asked-Questions#warning-c4003-the-min-the-max-and-the-macro
#define NOMINMAX
#endif
#endif
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <filesystem>

#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stdint.h>
#include "Engine/Core/core.h"

#include "Engine/Core/Log.h"

#ifdef SN_PLATFORM_WINDOWS
#include "windows.h"
#endif // SN_PLATFORM_WINDOWS