#ifndef _TEST_H
#define _TEST_H

#include "pluginmain.h"
#include <string>

// 插件子菜单标识符
#define MENU_Start 0        // 开始菜单
#define MENU_Stop 1         // 停止菜单
#define MENU_Save 2         // 保存菜单
#define MENU_Abandon 3      // 放弃菜单

// 插件模块名称，用于获取插件模块的实例句柄，
// 这将用于加载对话框资源
#if defined( _WIN64 )
#define plugin_DLLname "Ktracer.dp64"      // 64位DLL名称
#else
#define plugin_DLLname "Ktracer.dp32"      // 32位DLL名称
#endif

// 每步执行的时间间隔（毫秒）
#define TimerStepMs 20

// 32位和64位PE文件的NT头大小
#define NT32dataSize 0xF8       // 32位NT头大小
#define NT64dataSize 0x108      // 64位NT头大小
// PE文件中NT节的大小
#define NTSectionSize 0x28      // 节头大小

// 用于从目标模块读取NT头的缓冲区
#if defined( _WIN64 )
static unsigned char NTdata [NT64dataSize];        // 64位NT头数据缓冲区
#else
static unsigned char NTdata [NT32dataSize];        // 32位NT头数据缓冲区
#endif

// 指向节头数组的指针，用于计算文件偏移
static unsigned char* SectionHeadersData;

using namespace std;

// 核心功能函数声明
void testInit(PLUG_INITSTRUCT* initStruct);        // 测试初始化
void testStop();                                    // 测试停止
void testSetup();                                   // 测试设置

// 对话框回调过程
INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// 用于显示插件主对话框
void ShowForm(HINSTANCE hInstance, HINSTANCE hPrevInstance);
// 检查结束VA是否有效
bool CheckENDAddress(HWND hWnd);
// 显示保存文件对话框，如果按下OK返回true
bool SaveFileDialog(char Buffer[MAX_PATH]);

// 数字转换为十六进制字符串的函数
string Int32ToHexString ( int Number );            // 32位整数转十六进制字符串
string Int64ToHexString ( long long Number );      // 64位整数转十六进制字符串
string Int64ToHexString ( INT64* Number );         // 64位整数指针转十六进制字符串

// 来自主机的插件回调过程
void MyCallBack (CBTYPE cbType, void* callbackinfo);

// VA到RVA转换函数
DWORD VAtoRVA(duint VA, duint ImageBase);           // 64位VA转RVA
DWORD VAtoRVA(UINT32 VA, UINT32 ImageBase);        // 32位VA转RVA
// RVA到文件偏移转换函数
ULONG RVA2FOA(DWORD ulRVA, PIMAGE_NT_HEADERS32 pNTHeader, IMAGE_SECTION_HEADER* SectionHeaders );
// 初始化模块NT数据
void InitModuleNTData(INT64 TargetImageBase);
// VA到文件偏移转换函数
DWORD VAtoFileOffset(UINT32 VA, UINT32 ImageBase); // 32位VA转文件偏移
DWORD VAtoFileOffset(duint VA, duint ImageBase);    // 64位VA转文件偏移

// 定时器回调函数，向主机发送命令
VOID CALLBACK TimerProcSingleStep(HWND hWnd, UINT nMsg, UINT nIDEvent, DWORD dwTime);  // 单步执行定时器
VOID CALLBACK TimerProcStepOut(HWND hWnd, UINT nMsg, UINT nIDEvent, DWORD dwTime);     // 步过执行定时器

#endif // _TEST_H
