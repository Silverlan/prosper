// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#pragma once

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
