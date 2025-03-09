#pragma once

#ifndef LFP_NOINLINE
#define LFP_NOINLINE __declspec(noinline)
#endif

#ifndef LFP_SHARED_EXPORT
#define LFP_SHARED_EXPORT __declspec(dllexport)
#endif
#ifndef LFP_SHARED_IMPORT
#define LFP_SHARED_IMPORT __declspec(dllimport)
#endif

#ifndef LFPAPI
#ifdef LFP_EXPORT
#define LFPAPI [[maybe_unused]] LFP_SHARED_EXPORT
#else
#define LFPAPI [[maybe_unused]] LFP_SHARED_IMPORT
#endif
#endif

#ifndef LFPNDAPI
#define LFPNDAPI [[nodiscard]] LFPAPI
#endif