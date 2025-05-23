#include "pluginmain.h"
#include "Tracing.h"

using namespace std;

// 插件全局变量
int pluginHandle;       // 插件句柄
HWND hwndDlg;          // GUI窗口句柄
int hMenu;             // 插件菜单句柄

// 插件初始化函数 - 在插件加载时被调用一次
DLL_EXPORT bool pluginit(PLUG_INITSTRUCT* initStruct)
{
	_plugin_logprintf("[Ktracer] 插件句柄: %d : 事件 %s \n", pluginHandle, "pluginit");

    // 设置插件基本信息
    initStruct->pluginVersion = plugin_version;         // 插件版本
    initStruct->sdkVersion = PLUG_SDKVERSION;          // SDK版本
    strcpy(initStruct->pluginName, plugin_name);       // 插件名称
    pluginHandle = initStruct->pluginHandle;           // 保存插件句柄
    
    // 调用测试初始化函数
    testInit(initStruct);
    return true;
}

// 插件停止函数 - 当插件停止时被调用
DLL_EXPORT bool plugstop()
{
	_plugin_logprintf("[Ktracer] 插件句柄: %d : 事件 %s \n", pluginHandle, "plugstop");

    // 调用测试停止函数
    testStop();
    return true;
}

// 插件设置函数 - 在初始化事件之后被调用
DLL_EXPORT void plugsetup(PLUG_SETUPSTRUCT* setupStruct)
{
	_plugin_logprintf("[Ktracer] 插件句柄: %d : 事件 %s \n", pluginHandle, "plugsetup");

    // 保存GUI相关句柄
    hwndDlg = setupStruct->hwndDlg;     // 主窗口句柄
    hMenu = setupStruct->hMenu;         // 菜单句柄
 
    // 调用测试设置函数
    testSetup();
}

// DLL入口点函数
extern "C" DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return TRUE;
}



