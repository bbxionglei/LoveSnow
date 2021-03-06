﻿#ifndef FRAMEWORK_H
#define FRAMEWORK_H
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
#define _CRT_SECURE_NO_WARNINGS			// strncpy This function or variable may be unsafe
// Windows 头文件
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <stdint.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#endif//FRAMEWORK_H