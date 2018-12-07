#include "stdafx.h"
#include "DeskIcon.h"


DeskIcon::DeskIcon() {
	this->_setDesktopHwnd();  // 初始化桌面句柄
	this->_setParamMem();
}

DeskIcon::~DeskIcon()
{
	VirtualFreeEx(this->process, this->_lvi, 0, MEM_RELEASE);
	VirtualFreeEx(this->process, this->_item, 0, MEM_RELEASE);
	VirtualFreeEx(this->process, this->_rc, 0, MEM_RELEASE);

	CloseHandle(this->process);
}

void DeskIcon::_setDesktopHwnd()
{
	HWND dwndparent;
	HWND dwndviem = NULL;
	HWND hDeskTop;
	dwndparent = FindWindowEx(0, 0, L"WorkerW", L"");//获得第一个WorkerW类的窗口，
	while ((!dwndviem) && dwndparent)//因为可能会有多个窗口类名为“WorkerW”的窗口存在，所以只能依次遍历
	{
		dwndviem = FindWindowEx(dwndparent, 0, L"SHELLDLL_DefView", 0);
		dwndparent = FindWindowEx(0, dwndparent, L"WorkerW", L"");
	}
	hDeskTop = FindWindowEx(dwndviem, 0, L"SysListView32", L"FolderView");
	
	this->hDeskTop = hDeskTop;
}

void DeskIcon::_setParamMem()
{
	if (this->hDeskTop == 0) {
		printf("[-] 桌面句柄未初始化！");
		exit(1);
	}

	GetWindowThreadProcessId(this->hDeskTop, &this->pid);
	process = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, this->pid);

	this->_lvi = (LVITEM64*)VirtualAllocEx(process, NULL, sizeof(LVITEM64), MEM_COMMIT, PAGE_READWRITE);
	this->_item = (wchar_t*)VirtualAllocEx(process, NULL, 512 * sizeof(wchar_t), MEM_COMMIT, PAGE_READWRITE);
	
	this->_rc = (RECT*)VirtualAllocEx(process, NULL, sizeof(RECT), MEM_COMMIT, PAGE_READWRITE);
	//this->_pt = (POINT*)VirtualAllocEx(process, NULL, sizeof(POINT), MEM_COMMIT, PAGE_READWRITE);
}

void DeskIcon::_enTitleNormal(wchar_t * wch)
{
	int length = lstrlenW(wch);
	if (length < 1) return;
	// 遍历wch
	for (int i = 0; i < length; ++i) {
		if (wch[i] == L' ') {
			wch[i] = 666; // 把所有空格替换为特殊字符
		}
	}
}

void DeskIcon::_enTitleRaw(wchar_t * wch)
{
	int length = lstrlenW(wch);
	if (length < 1) return;
	// 遍历wch
	for (int i = 0; i < length; ++i) {
		if (wch[i] == 666) {
			wch[i] = L' ';  // 将特殊字符替换回空格
		}
	}
}

int DeskIcon::getItemCount()
{
	return (int)::SendMessage(this->hDeskTop, LVM_GETITEMCOUNT, 0, 0);;
}

HWND DeskIcon::getDesktopHwnd()
{
	return this->hDeskTop;
}

wchar_t* DeskIcon::getItemText(int index)
{
	this->lvi.cchTextMax = 512;
	this->lvi.iSubItem = 0;
	this->lvi.pszText = (_int64)_item;
	WriteProcessMemory(this->process, this->_lvi, &this->lvi, sizeof(LVITEM64), NULL);
	::SendMessage(this->hDeskTop, LVM_GETITEMTEXT, (WPARAM)index, (LPARAM)this->_lvi);
	// 返回的参数在目标进程里所以需要读目标进程的内存
	ReadProcessMemory(this->process, this->_item, this->item, 512 * sizeof(wchar_t), NULL);  
	return (this->item);
}

RECT DeskIcon::getItemRect(int index)
{
	this->rc.left = LVIR_ICON;  //这个一定要设定 可以去看MSDN关于LVM_GETITEMRECT的说明

	::WriteProcessMemory(this->process, this->_rc, &this->rc, sizeof(this->rc), NULL);
	::SendMessage(this->hDeskTop, LVM_GETITEMRECT, (WPARAM)index, (LPARAM)this->_rc);

	ReadProcessMemory(process, this->_rc, &this->rc, sizeof(this->rc), NULL);

	return this->rc;
}

//POINT DeskIcon::getItemPos(int index)
//{
//	::WriteProcessMemory(this->process, this->_pt, &this->pt, sizeof(this->pt), NULL);
//	::SendMessage(this->hDeskTop, LVM_GETITEMRECT, (WPARAM)index, (LPARAM)this->_pt);
//
//	ReadProcessMemory(process, this->_pt, &this->pt, sizeof(this->pt), NULL);
//	return this->pt;
//}

void DeskIcon::setItemPos(int index, int x, int y)
{
	::SendMessage(this->hDeskTop, LVM_SETITEMPOSITION, index, MAKELPARAM(x, y));
}

void DeskIcon::flushWindow()
{
	ListView_RedrawItems(this->hDeskTop, 0, ListView_GetItemCount(this->hDeskTop) - 1);
	::UpdateWindow(this->hDeskTop);
}

POINT DeskIcon::getScreenSize()
{
	int  cx = GetSystemMetrics(SM_CXFULLSCREEN);
	int  cy = GetSystemMetrics(SM_CYFULLSCREEN);
	POINT r;
	r.x = cx;
	r.y = cy;
	return r;
}

void DeskIcon::saveDeskTop(const wchar_t * file)
{
	printf("[+] save desktop to %ws \n", file);
	
	FILE* f;
	_wfopen_s(&f, file, L"wb");
	int iconNum = this->getItemCount();
	fwprintf(f, L"xchens.cn %d \n", iconNum);  // 写入文件标志位
	// 开始保存内容
	for (int i = 0; i < iconNum; ++i) {
		RECT rc = this->getItemRect(i);
		wchar_t* title = this->getItemText(i);
		// 除去特殊字符:空格
		this->_enTitleNormal(title);
		fwprintf(f, L"%ws %d %d \n", title, rc.left, rc.top);
	}
	fclose(f);
}

void DeskIcon::loadDeskTop(const wchar_t * file)
{
	/*
		* warning: this func had new many memory and delete it. becareful when use it.
	*/
	printf("[+] load desktop from %ws \n", file);

	FILE* f;
	errno_t er = _wfopen_s(&f, file, L"rb");
	if (er) {
		// 无法找到文件
		printf("[-] can't read target file:  %ws", file);
		return;
	}

	//读取文件头
	int iconNum = 0;
	wchar_t tagx[10]; // buf of file header
	fgetws(tagx, 10, f);
	if (lstrcmpW(tagx, L"xchens.cn")) {
		//标志位错误
		printf("[-] find error in file header");
		return;
	}
	fwscanf_s(f, L"%d", &iconNum);

	// 读取现存桌面图标
	int iconNumNow = this->getItemCount();
	wchar_t **iconTitlesNow = new wchar_t*[iconNumNow]; // 保存现有的所有icon的text
	for (int i = 0; i < iconNumNow; ++i) {
		wchar_t *curIconTitle = this->getItemText(i); // 获取当前索引的图标的text
		int curIconSize = lstrlenW(curIconTitle); // 获取该text的长度
		iconTitlesNow[i] = new wchar_t[curIconSize + 1]; // 申请内存存放这个text
		lstrcpynW(iconTitlesNow[i], curIconTitle, curIconSize + 1);  // 保存这个text
	}
	
	//读取记录文件正文
	wchar_t iconTitle[512]; // 局部变量会自动释放
	int x = 0, y = 0;

	for (int i = 0; i < iconNum; ++i) {
		fwscanf_s(f, L"%ws", iconTitle, 512);
		fwscanf_s(f, L"%d", &x);
		fwscanf_s(f, L"%d", &y);
		this->_enTitleRaw(iconTitle);
		for (int j = 0; j < iconNumNow; ++j) { // 尝试找到对应的图标并修改其位置
			if (!lstrcmpW(iconTitlesNow[j], iconTitle)) {  // 如果相同
				this->setItemPos(i, x + 21, y - 3); // 这里加上本机偏差
				break;
			}
		}
	}

	// 回收内存
	fclose(f);
	for (int i = 0; i < iconNumNow; ++i) {
		delete iconTitlesNow[i];
	}
	delete iconTitlesNow;
}
