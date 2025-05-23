#define WINVER 0x0501
#define _WIN32_WINNT 0x0501

#include "Tracing.h"
#include "pluginsdk\TitanEngine\TitanEngine.h"
#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#include "icons.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include "resource.h"
#include <Commdlg.h>
#include <exception>

using namespace std;

// 插件模块实例句柄，用于定位资源
HINSTANCE hInst;
// 用于设置主对话框图标
HICON main_icon;

// 全局标志
bool LoggingActive;         // 日志记录是否活跃
// 事件列表
vector<string> Events;      // 存储追踪事件的向量

// 停止VA（虚拟地址）
long long EndVA;

// 主对话框句柄
HWND TracerWHandle;
// 目标模块名称，在点击开始按钮时获取
string TaregtModule;
// 主对话框是否显示
bool FormDisplayed = false;
// 用于显示消息和警告
const WCHAR* MessageBoxTitle = L"x64_Tracer";

// 指示是否可以计算目标模块中的文件偏移
bool IsFileOffsetPossible;

// 将64位长整数转换为十六进制字符串
string Int64ToHexString ( long long Number )
{
	ostringstream ss;
	ss << std::hex << Number;
	return ss.str();
}

// 将指针指向的64位值转换为十六进制字符串
string Int64ToHexString ( void* Number )
{
	ostringstream ss;
	ss << std::hex <<  *(long long*)(Number);
	return ss.str();
}

// 将32位整数转换为十六进制字符串
string Int32ToHexString ( int Number )
{
	ostringstream ss;
	ss << std::hex << Number;
	return ss.str();
}

// 显示保存文件对话框
bool SaveFileDialog(char Buffer[MAX_PATH])
{
	OPENFILENAMEA sSaveFileName;

	ZeroMemory(&sSaveFileName,sizeof(sSaveFileName));
	sSaveFileName.hwndOwner = GuiGetWindowHandle();

	const char szFilterString[] = "Text files (*.txt, *.log)\0*.txt;*.log\0All Files (*.*)\0*.*\0\0";
	const char szDialogTitle[] = "Select dump file...";

	sSaveFileName.lStructSize = sizeof(sSaveFileName);
	sSaveFileName.lpstrFilter = szFilterString;

	sSaveFileName.lpstrFile = Buffer;
	sSaveFileName.nMaxFile = MAX_PATH;
	sSaveFileName.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	sSaveFileName.lpstrTitle = szDialogTitle;
	sSaveFileName.lpstrDefExt = "txt";

	return (FALSE != GetSaveFileNameA(&sSaveFileName));
}

// 检查结束地址的有效性
bool CheckENDAddress(HWND hWnd)
{
	CHAR buffer[1024];

	ZeroMemory(&buffer,sizeof(buffer));

	// 从文本框获取结束地址
	GetWindowTextA(GetDlgItem(hWnd, IDC_EDITEndAddress), buffer, sizeof(buffer));

	// 将字符串转换为数值
	EndVA = DbgValFromString(buffer);

	// 验证表达式有效性和数值大于0
	if(DbgIsValidExpression(buffer)  && EndVA > 0)
	{
		return true;
	}

	return false;
}

// 检查模块名称的有效性
bool CheckModuleName(HWND hWnd)
{
	CHAR buffer[1024];

	ZeroMemory(&buffer,sizeof(buffer));

	// 从文本框获取模块名称
	GetWindowTextA(GetDlgItem(hWnd, IDC_EDITModule), buffer, sizeof(buffer));

	TaregtModule = string(buffer);

	// 检查模块名称长度是否大于1
	if(TaregtModule.length() > 1)
	{
		return true;
	}

	return false;
}

// 返回复选框是否被选中（用于禁用GUI）
bool ShouldDisableGUI()
{
	HWND CheckBoxHandle = GetDlgItem(TracerWHandle, IDC_CHECKGUI);

	long IsChecked = (SendMessage(CheckBoxHandle, BM_GETCHECK, 0, 0));
 
	if(IsChecked == BST_CHECKED)
	{
		return true;
	}

	return false;
}

// 将VA转换为RVA（相对虚拟地址）
// 64位版本
DWORD VAtoRVA(duint VA, duint ImageBase)
{
	return (DWORD)(VA - ImageBase);
}

// 将RVA转换为文件偏移（FOA）
// 32位版本
ULONG RVA2FOA(DWORD ulRVA, PIMAGE_NT_HEADERS32 pNTHeader, IMAGE_SECTION_HEADER* SectionHeaders )
{
    int local = 0;
    
    // 遍历所有节头，找到包含该RVA的节
    for(int i = 0; i < pNTHeader->FileHeader.NumberOfSections; i++)
    {
        if ( (ulRVA >= SectionHeaders[i].VirtualAddress) && (ulRVA <= SectionHeaders[i].VirtualAddress + SectionHeaders[i].SizeOfRawData) )
        {
          return SectionHeaders[i].PointerToRawData + (ulRVA - SectionHeaders[i].VirtualAddress);
        } 
    }
    return 0;
}

// 将RVA转换为文件偏移（FOA）
// 64位版本
ULONG RVA2FOA(DWORD ulRVA, PIMAGE_NT_HEADERS64 pNTHeader, IMAGE_SECTION_HEADER* SectionHeaders )
{
    int local = 0;
    
    // 遍历所有节头，找到包含该RVA的节
    for(int i = 0; i < pNTHeader->FileHeader.NumberOfSections; i++)
    {
        if ( (ulRVA >= SectionHeaders[i].VirtualAddress) && (ulRVA <= SectionHeaders[i].VirtualAddress + SectionHeaders[i].SizeOfRawData) )
        {
          return SectionHeaders[i].PointerToRawData + (ulRVA - SectionHeaders[i].VirtualAddress);
        } 
    }
    return 0;
}

// 将VA转换为文件偏移
// 64位版本
DWORD VAtoFileOffset(duint VA, duint ImageBase)
{
	// 首先转换为RVA
	DWORD RVA = VAtoRVA(VA, ImageBase);

#if defined( _WIN64 )
	PIMAGE_NT_HEADERS64 pNTHeader = (PIMAGE_NT_HEADERS64)(NTdata);
#else
	PIMAGE_NT_HEADERS32 pNTHeader = (PIMAGE_NT_HEADERS32)(NTdata);
#endif

	IMAGE_SECTION_HEADER* SectionHeaders = (IMAGE_SECTION_HEADER*)(SectionHeadersData);

	DWORD result = RVA2FOA(RVA, pNTHeader, SectionHeaders);
 
    return result;
}

// 读取NT头和其后的节数据到2个缓冲区中
void InitModuleNTData()
{
	// 获取目标模块名称
	char module[MAX_MODULE_SIZE] = "";
	ZeroMemory(&module,sizeof(module));
	GetWindowTextA(GetDlgItem(TracerWHandle, IDC_EDITModule), module, sizeof(module));

	// 获取目标模块基址
	duint TargetImageBase = DbgModBaseFromName(module);
	// 检查是否能获取目标模块基址
	if(TargetImageBase > 0)
	{
		IsFileOffsetPossible = true;
	}
	else
	{
		IsFileOffsetPossible = false;
		return;
	}

	// 先释放内存
	free(SectionHeadersData);
	
	// 只在点击开始按钮时读取2个缓冲区一次

#if defined( _WIN64 )
	
	// 获取DOS头指针
	unsigned char DOSbuffer [0x40];
	DbgMemRead( TargetImageBase , DOSbuffer, 0x40);

	PIMAGE_DOS_HEADER DOS = (PIMAGE_DOS_HEADER)(DOSbuffer);

	// NT头偏移
	duint addr = (TargetImageBase + DOS->e_lfanew);
	DbgMemRead( addr , NTdata, NT64dataSize);
 
	PIMAGE_NT_HEADERS64 pNTHeader = (PIMAGE_NT_HEADERS64)(NTdata);
 
	// 节的数量
	int NumebrOfSections = pNTHeader->FileHeader.NumberOfSections;
	// 节数量 * 40字节（每个节头的大小）
	int sectionsDataSize = NumebrOfSections * NTSectionSize;

	// 创建动态缓冲区来保存节头数据
	SectionHeadersData = (unsigned char*)malloc(sectionsDataSize);
	DbgMemRead(addr + NT64dataSize, SectionHeadersData, sectionsDataSize);

#else

	// 获取DOS头指针
	unsigned char DOSbuffer [0x40];
	DbgMemRead( TargetImageBase , DOSbuffer, 0x40);

	PIMAGE_DOS_HEADER DOS = (PIMAGE_DOS_HEADER)(DOSbuffer);

	// NT头偏移
	duint addr = (TargetImageBase + DOS->e_lfanew);
	DbgMemRead(addr, NTdata, NT32dataSize);
 
	PIMAGE_NT_HEADERS32 pNTHeader = (PIMAGE_NT_HEADERS32)(NTdata);
 
	// 节的数量
	int NumebrOfSections = pNTHeader->FileHeader.NumberOfSections;
	// 节数量 * 40字节（每个节头的大小）
	int sectionsDataSize = NumebrOfSections * NTSectionSize;

	// 创建动态缓冲区来保存节头数据
	SectionHeadersData = (unsigned char*)malloc(sectionsDataSize);
	DbgMemRead(addr + NT32dataSize, SectionHeadersData, sectionsDataSize);

#endif
}

// 比较两个字符数组，忽略大小写
bool iequals(const string& a, const string& b)
{
	unsigned int sz = a.size();
	if (b.size() != sz)
		return false;
	for (unsigned int i = 0; i < sz; ++i)
		if (tolower(a[i]) != tolower(b[i]))
			return false;
	return true;
}

// 显示GUI界面
void ShowForm(HINSTANCE hInstance, HINSTANCE hPrevInstance)
{
	_plugin_logprintf("IsFileOffsetPossible : [%d] !\n", IsFileOffsetPossible);

	// 全局变量
	hInst = hInstance;

	// 准备主对话框图标
	main_icon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));

	// 显示对话框
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOGMain), 0, &DialogProc );
}

// 在文本框中设置当前目标模块名称
void TrySetTargetModule()
{
	try
	{
		CHAR buffer[MAX_MODULE_SIZE];

		// 获取当前RIP（指令指针）
		duint entry = GetContextData(UE_CIP);

		DbgGetModuleAt(entry, buffer);

		SetWindowTextA(GetDlgItem(TracerWHandle, IDC_EDITModule), buffer );
	}
	catch (exception& e)
	{
		_plugin_logprintf("[%s] : TrySetTargetModule 失败 !\n", plugin_name);
	}
}

// 更新显示事件数量的标签
void UpdateCountLabel()
{
	CHAR buffer[20];

	// 获取事件数量
	int count = Events.size();

	itoa(count,buffer, 10);

	SetWindowTextA(GetDlgItem(TracerWHandle, IDC_STATIC_Count), buffer );
}

// 单步执行定时器回调函数
VOID CALLBACK TimerProcSingleStep(HWND hWnd, UINT nMsg, UINT nIDEvent, DWORD dwTime) 
{
	KillTimer(hWnd, nIDEvent);

	if(!LoggingActive)
	{
		return;
	}

	DbgCmdExec("eSingleStep");
}

// 步过执行定时器回调函数
VOID CALLBACK TimerProcStepOut(HWND hWnd, UINT nMsg, UINT nIDEvent, DWORD dwTime) 
{
	KillTimer(hWnd, nIDEvent);

	if(!LoggingActive)
	{
		return;
	}

	DbgCmdExec("eStepOver");
}

// 主回调函数 - 处理调试器事件
void MyCallBack (CBTYPE cbType, void* callbackinfo)
{
	// 如果日志记录未激活则返回
	if( !LoggingActive )
	{
		return;
	}

	// 如果窗体已关闭则返回
	// 意味着插件未在使用中
	if( !FormDisplayed )
	{
		return;
	}

	// 获取当前RIP（指令指针）
	duint entry = GetContextData(UE_CIP);

	// 检查是否到达结束VA
	if(entry == EndVA) {
		GuiUpdateEnable(true);
		LoggingActive = false;
		MessageBoxW(hwndDlg, L"已到达追踪结束点" , MessageBoxTitle, MB_ICONINFORMATION);
		return;
	}

	// 当前模块名称
	char module[MAX_MODULE_SIZE] = "";

	switch(cbType)
	{
	case CB_STEPPED:  // 单步执行事件
		// 根据我们当前所在的模块，选择正确的命令

		if (DbgGetModuleAt(entry,module))
		{
			if(LoggingActive == true)
			{
				// 比较当前模块与目标模块
				if ( iequals(string(module), TaregtModule) )
				{
					// 反汇编当前行
					BASIC_INSTRUCTION_INFO basicinfo;
					DbgDisasmFastAt(entry, &basicinfo);

					// 我们将忽略调用，只记录跳转
					if ((basicinfo.branch))// && !(basicinfo.call)))
					{
						char disasm[GUI_MAX_DISASSEMBLY_SIZE] = "";
						GuiGetDisassembly(entry, disasm);

						bool IsTaken = DbgIsJumpGoingToExecute(entry);

						// 文件偏移
						duint ImageBase = DbgModBaseFromName(module);
						duint FileOffset = ConvertVAtoFileOffset(ImageBase, entry, false);

						char Data[2048] = "";
 
						string Row;
						if(basicinfo.call)
						{
							// 调用指令格式
							Row = Int64ToHexString(entry) + "\t\t" + Int32ToHexString( VAtoRVA(entry, ImageBase)) + "\t\t" + (IsFileOffsetPossible ? Int32ToHexString( VAtoFileOffset(entry, ImageBase)) : Int32ToHexString( 0)) + "\t\t" + "\t-->\t\t\t\t"  + string(basicinfo.instruction);
						}
						else
						{
							// 跳转指令格式（显示是否执行）
							Row = Int64ToHexString(entry) + "\t\t" + Int32ToHexString( VAtoRVA(entry, ImageBase)) + "\t\t" + (IsFileOffsetPossible ? Int32ToHexString( VAtoFileOffset(entry, ImageBase)) : Int32ToHexString( 0)) + "\t\t" + (IsTaken ? "\tYes\t\t\t\t" : "\tNot\t\t\t\t") + string(basicinfo.instruction);
						}

						Events.push_back(Row);
						UpdateCountLabel();
					}
				}

				// 分支目标地址
				duint destination = DbgGetBranchDestination(entry);

				// 分支目标模块
				char DestModule[MAX_MODULE_SIZE] = "";
				DbgGetModuleAt(destination,DestModule);

				// 比较当前模块与目标模块
				// 内部跳转？
				if ( iequals(string(module), TaregtModule) )
				{
					// 我们在目标模块内部，所以单步进入
					UINT_PTR TimerId = SetTimer(hwndDlg, 0, TimerStepMs, (TIMERPROC)TimerProcSingleStep);
				}
				else
				{
					// 我们在目标模块外部
					if(iequals(TaregtModule , DestModule))
					{
						// 如果分支回到目标模块，则单步进入
						UINT_PTR TimerId = SetTimer(hwndDlg, 0, TimerStepMs, (TIMERPROC)TimerProcSingleStep);
					}
					else
					{
						// 如果跳转到其他地方，则步过它
						UINT_PTR TimerId = SetTimer(hwndDlg, 0, TimerStepMs, (TIMERPROC)TimerProcStepOut);
					}

					// 此后单步事件不会被触发
					// 我们将面临PausedDebug事件
				}
			}
		}
		else
		{
			// 在这个未知模块中执行直到返回！
			//UINT_PTR TimerId = SetTimer(hwndDlg, 0, TimerStepMs, (TIMERPROC)TimerProcStepOut);
			//UINT_PTR TimerId = SetTimer(hwndDlg, 0, TimerStepMs, (TIMERPROC)TimerProcSingleStep);
			//UINT_PTR TimerId = SetTimer(hwndDlg, 0, 10, (TIMERPROC)TimerProcStepOut);
		}
		break;

	case CB_BREAKPOINT:  // 断点事件
		{

		}
		break;

	case CB_PAUSEDEBUG:  // 暂停调试事件
		{
			// 反汇编当前行
			//BASIC_INSTRUCTION_INFO basicinfo;
			//DbgDisasmFastAt(entry, &basicinfo);

			// 我们是否在"RET"指令处？
			//if( iequals( string(basicinfo.instruction) , "RET"))
			//{
				// 执行单步
				//UINT_PTR TimerId = SetTimer(hwndDlg, 0, 10, (TIMERPROC)TimerProcSingleStep);
			//}
		}
		break;

	case CB_RESUMEDEBUG:  // 恢复调试事件
		break;
	}
}

// 主对话框过程
INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:  // 对话框初始化
		{
			// 保存对话框窗口句柄
			TracerWHandle = hWnd;

			// 设置标志
			FormDisplayed = true;

			// 设置对话框图标
			SendMessage (hWnd, WM_SETICON, WPARAM (ICON_SMALL), LPARAM (main_icon));

			// 如果可能，设置初始目标模块名称
			TrySetTargetModule();
		}

		// +- 按钮例程 -+
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDC_START:  // 开始按钮
				{
					if(!DbgIsDebugging())
					{
						MessageBoxW(hWnd, (L"您需要在调试状态下使用此功能！"), MessageBoxTitle , MB_OK | MB_ICONHAND);
					}
					else if(!CheckENDAddress(hWnd))
					{
						MessageBoxW(hWnd, (L"无效的结束地址！"), MessageBoxTitle , MB_OK | MB_ICONHAND);
					}
					else if(!CheckModuleName(hWnd))
					{
						MessageBoxW(hWnd, (L"无效的目标模块名称！"), MessageBoxTitle, MB_OK | MB_ICONHAND);
					}
					else  // 可以开始
					{
						// 读取目标模块的NT头信息
						InitModuleNTData();

						LoggingActive = true;

						DbgCmdExec("sst");
					}
				}
				break;

			case IDC_CHECKGUI:  // GUI复选框
				{
					// 复选框被点击，决定要做什么
					if(ShouldDisableGUI())
					{
						GuiUpdateDisable();
					}
					else
					{
						GuiUpdateEnable(true);
					}
				}
				break;

			case IDC_FLUSH:  // 清空按钮
				{
					Events.clear();
					UpdateCountLabel();
				}
				break;

			case IDC_SAVE:  // 保存按钮
				{
					if(Events.size() < 1)
					{
						MessageBoxW(hwndDlg, L"日志缓冲区为空！", MessageBoxTitle, MB_ICONHAND);
					}
					else
					{
						// 暂停被调试进程
						DbgCmdExec("pause");

						char szFileName[MAX_PATH];
						ZeroMemory(szFileName, MAX_PATH);
						if(SaveFileDialog(szFileName))
						{
							// 打印表头
							string Row;
 
							Row = "VA\t\t\t\tRVA\t\t\tOffset\t\t\tStatus\t\t\tInsturction";

							ofstream Outfile(szFileName);

							Outfile << "-----------------------------------------------------------------------------------------" << endl;
							Outfile << Row << endl;
							Outfile << "-----------------------------------------------------------------------------------------" << endl;

							// 写入所有事件
							for(int t=0;t< Events.size();++t)
							{
								Outfile << Events[t] << endl;
							}

							MessageBoxW(hwndDlg, L"日志文件已保存！", MessageBoxTitle, MB_ICONINFORMATION);
						}
					}
				}
				break;
			}
		}
		break;

	case WM_CLOSE:  // 窗口关闭
		{
			// 如果GUI被锁定，则在退出时解锁
			if(ShouldDisableGUI)
			{
				GuiUpdateEnable(true);
			}

			// 清理和重置
			hInst = NULL;
			main_icon = NULL;

			FormDisplayed = false;
			LoggingActive = false;
			IsFileOffsetPossible = false;

			Events.clear();
			EndVA = 0;
			TracerWHandle = 0;

			DestroyIcon(main_icon);
			DestroyWindow(hWnd);
		}
		break;

		return (DefWindowProc(hWnd, uMsg, wParam, lParam));
	}
	// 防止关闭对话框
	return FALSE;
}

// 当插件菜单被点击时调用
extern "C" __declspec(dllexport) void CBMENUENTRY(CBTYPE cbType, PLUG_CB_MENUENTRY* info)
{
	switch(info->hEntry)
	{
	case MENU_Start:
		{
			if(!DbgIsDebugging())
			{
				MessageBoxW(hwndDlg, (L"您需要在调试状态下使用此功能！"), MessageBoxTitle , MB_OK | MB_ICONHAND);
				//_plugin_logputs("you need to be debugging to use this command");
				return;
			}
			// 如果窗体仍在显示，则将其置于前台
			if(FormDisplayed)
			{
				BringWindowToTop(TracerWHandle);
				return;
			}

			// 如果调试器开启，显示窗体
			ShowForm(GetModuleHandleA(plugin_DLLname),NULL);
		}
		break;

	case MENU_Stop:
		{

		}
		break;

	case MENU_Save:
		{

		}
		break;

	case MENU_Abandon:
		{

		}
		break;
	}
}

// 当新的调试会话开始时调用
extern "C" __declspec(dllexport) void CBINITDEBUG(CBTYPE cbType, PLUG_CB_INITDEBUG* info)
{
	_plugin_logprintf("[TEST] 开始调试文件 %s !\n", (const char*)info->szFileName);
}

// 当调试会话结束时调用
extern "C" __declspec(dllexport) void CBSTOPDEBUG(CBTYPE cbType, PLUG_CB_STOPDEBUG* info)
{
	_plugin_logputs("[TEST] 调试已停止!");
}

// 测试命令处理函数
bool cbTestCommand(int argc, char* argv[])
{
	// 显示输入框
	char line[GUI_MAX_LINE_SIZE] = "";
	if(!GuiGetLineWindow("Enter VA", line))
	{
		_plugin_logputs("[TEST] 已按下取消!");
	}
	else
	{
		duint VA = DbgValFromString(line);
		
		char module[MAX_MODULE_SIZE] = "";
		
		DbgGetModuleAt(VA, module);
		
		duint ImageBase = DbgModBaseFromName(module);

		DWORD RVA = VAtoRVA(VA, ImageBase);

		_plugin_logprintf("[TEST] VA 转 RVA : \"%d\"\n", RVA);
	}

	return true;
}

// 使用主机初始化插件
void testInit(PLUG_INITSTRUCT* initStruct)
{
	// 这将在日志屏幕中打印插件句柄
	//_plugin_logprintf("[TEST] pluginHandle: %d\n", pluginHandle);

	// 注册一个"ktracer"命令，可以在命令文本框中写入
	// 该命令将在方法"cbTestCommand"中处理
	if(!_plugin_registercommand(pluginHandle, plugin_command, cbTestCommand, false))
	{
		_plugin_logputs("[TEST] 注册\"ktracer\"命令时出错!");
	}
}

// 当关闭主机时调用
void testStop()
{
	_plugin_unregistercommand(pluginHandle, plugin_command);

	_plugin_menuclear(hMenu);

	EndDialog(TracerWHandle, NULL);
}

// 在"init"事件之后调用一次
void testSetup()
{
	// 设置菜单图标
	ICONDATA rocket;
	rocket.data = icon_rocket;
	rocket.size = sizeof(icon_rocket);

	_plugin_menuseticon(hMenu, &rocket);

	// 设置菜单
	_plugin_menuaddentry(hMenu, MENU_Start, "&开始记录...");
	//_plugin_menuaddentry(hMenu, MENU_Stop, "&停止记录");
	//_plugin_menuaddentry(hMenu, MENU_Save, "&保存到文本文件...");
	//_plugin_menuaddentry(hMenu, MENU_Abandon, "&清空记录缓冲区");

	// 注册调试事件回调
	_plugin_registercallback(pluginHandle, CB_STEPPED, MyCallBack);
	_plugin_registercallback(pluginHandle, CB_PAUSEDEBUG, MyCallBack);
}
