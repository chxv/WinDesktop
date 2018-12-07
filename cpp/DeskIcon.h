#pragma once
#include "stdafx.h"



typedef struct tagLVITEM64
{
	UINT mask;
	int iItem;
	int iSubItem;
	UINT state;
	UINT stateMask;
	_int64 pszText;
	int cchTextMax;
	int iImage;
	_int64 lParam;
#if (_WIN32_IE >= 0x0300)
	int iIndent;
#endif
#if (_WIN32_WINNT >= 0x0501)
	int iGroupId;
	UINT cColumns; // tile view columns
	_int64 puColumns;
#endif
#if _WIN32_WINNT >= 0x0600
	_int64 piColFmt;
	int iGroup; // readonly. only valid for owner data.
#endif
} LVITEM64;


class DeskIcon {
private:
	HWND hDeskTop = 0;
	LVITEM64 lvi, *_lvi;  // sendMessage 的一个结构体及其指针
	RECT rc, *_rc;   // 存放获取的icon的位置的缓冲区
	wchar_t item[512];  // 目标进程中传递参数的一个缓冲区
	wchar_t *_item;  // 指向目标缓冲区的指针
	unsigned long pid;  // 目标进程的pid
	HANDLE process;  // 目标进程句柄

	// POINT pt, *_pt; // 获取目标位置的缓冲区

	void _setDesktopHwnd();  // 获取桌面listview的句柄
	void _setParamMem();  // 给可能用到的参数，在桌面窗口进程中申请内存

	void _enTitleNormal(wchar_t *wch);  //转义串中的敏感字符，空格 
	void _enTitleRaw(wchar_t *wch);  // 转义为原始字符

public:
	DeskIcon();  // 构造函数
	~DeskIcon();
	HWND getDesktopHwnd();  // 获取桌面句柄  // 不推荐
	int getItemCount();  // 获取桌面图标的数量
	wchar_t* getItemText(int index);  // 获取一个指定索引的图标的文件名
	RECT getItemRect(int index);  // 获取一个图标的RECT(位置)
	// POINT getItemPos(int index);
	void setItemPos(int index, int x, int y);  // 设置桌面图标的位置
	void flushWindow();  // 刷新窗口

	POINT getScreenSize();  // 获取屏幕"可用"尺寸
	void saveDeskTop(const wchar_t *file = L".saveDesk");  // 保存当前桌面布局
	void loadDeskTop(const wchar_t *file = L".saveDesk");  // 加载一个桌面布局
};
