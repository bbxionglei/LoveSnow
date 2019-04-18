#ifndef FRAMEWORK_H
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

#ifdef _WIN32
#ifdef LSBASE_EXPORTS
#define LSBASE_API __declspec(dllexport)
#else
#define LSBASE_API __declspec(dllimport)
#endif
#else
#define LSBASE_API 
#endif

#endif//FRAMEWORK_H