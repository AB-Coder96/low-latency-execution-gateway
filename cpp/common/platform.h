#pragma once
#include <cstdint>

#if defined(__linux__)
  #define ZL_OS_LINUX 1
#else
  #define ZL_OS_LINUX 0
#endif

#if defined(__x86_64__) || defined(_M_X64)
  #define ZL_ARCH_X86_64 1
#else
  #define ZL_ARCH_X86_64 0
#endif
