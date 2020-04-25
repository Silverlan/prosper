/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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

#include <cinttypes>

namespace prosper
{
	// These have to match Vulkan's types
	using DeviceSize = uint64_t;
};

// #define DEBUG_VERBOSE

#endif
