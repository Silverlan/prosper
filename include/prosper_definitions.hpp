// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_DEFINITIONS_HPP__
#define __PROSPER_DEFINITIONS_HPP__

#ifdef SHPROSPER_STATIC
#define DLLPROSPER
#elif SHPROSPER_DLL
#ifdef __linux__
#define DLLPROSPER __attribute__((visibility("default")))
#else
#define DLLPROSPER __declspec(dllexport)
#endif
#else
#ifdef __linux__
#define DLLPROSPER
#else
#define DLLPROSPER __declspec(dllimport)
#endif
#endif
#ifndef UNUSED
#define UNUSED(x) x
#endif

#include "prosper_types.hpp"

namespace prosper {
	// These have to match Vulkan's types
	using DeviceSize = uint64_t;
};

	// #define DEBUG_VERBOSE

#endif
