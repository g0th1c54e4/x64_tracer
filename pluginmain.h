#ifndef _PLUGINMAIN_H
#define _PLUGINMAIN_H

#include <windows.h>
#include "pluginsdk\_plugins.h"

#ifndef DLL_EXPORT
#define DLL_EXPORT __declspec(dllexport)
#endif //DLL_EXPORT

//全局变量
extern int pluginHandle;    // 插件句柄
extern HWND hwndDlg;        // GUI窗口句柄
extern int hMenu;           // 插件菜单句柄

// 插件基本信息定义
#define plugin_name "KTracer"           // 插件名称
#define plugin_command "ktracer"        // 插件命令名
#define plugin_version 1                // 插件版本号

#ifdef __cplusplus
extern "C"
{
#endif

// 插件生命周期函数声明
DLL_EXPORT bool pluginit(PLUG_INITSTRUCT* initStruct);    // 插件初始化
DLL_EXPORT bool plugstop();                               // 插件停止
DLL_EXPORT void plugsetup(PLUG_SETUPSTRUCT* setupStruct); // 插件设置

#ifdef __cplusplus
}
#endif

#endif //_PLUGINMAIN_H
