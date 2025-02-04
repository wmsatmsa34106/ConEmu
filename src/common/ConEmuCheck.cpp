﻿
/*
Copyright (c) 2009-present Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#define HIDE_USE_EXCEPTION_INFO
#include "Common.h"
#include "ConEmuCheck.h"
#include "ConEmuPipeMode.h"
#include "MFileMapping.h"
#include "HkFunc.h"
#include "MModule.h"

#ifdef _DEBUG
#include "CmdLine.h"
#include "MStrDup.h"
#endif

#ifdef _DEBUG
#define DEBUGSTRCMD(s) //OutputDebugString(s)
//#define DEBUG_RETRY_PIPE_OPEN_DLG
#else
#define DEBUGSTRCMD(s)
#undef DEBUG_RETRY_PIPE_OPEN_DLG
#endif

// !!! Использовать только через функцию: LocalSecurity() !!!
SECURITY_ATTRIBUTES* gpLocalSecurity = nullptr;

// Informational. Will be set in pHdr->hModule on calls
HANDLE2 ghWorkingModule = {};

#ifdef _DEBUG
bool gbPipeDebugBoxes = false;
#endif

//#ifdef _DEBUG
//	#include <crtdbg.h>
//#else
//	#ifndef _ASSERTE
//	#define _ASSERTE(f)
//	#endif
//#endif

//extern LPVOID _calloc(size_t,size_t);
//extern LPVOID _malloc(size_t);
//extern void   _free(LPVOID);

//typedef DWORD (APIENTRY *FGetConsoleProcessList)(LPDWORD,DWORD);


//#if defined(__GNUC__)
//extern "C" {
//#endif
//	WINBASEAPI DWORD GetEnvironmentVariableW(LPCWSTR lpName,LPWSTR lpBuffer,DWORD nSize);
//#if defined(__GNUC__)
//};
//#endif

//// Это для CONEMU_MINIMAL, чтобы wsprintf не был нужен
//LPCWSTR CreatePipeName(wchar_t (&szGuiPipeName)[128], LPCWSTR asFormat, DWORD anValue)
//{
//	return msprintf(szGuiPipeName, countof(szGuiPipeName), asFormat, L".", anValue);
//	//szGuiPipeName[0] = 0;
//
//	//const wchar_t* pszSrc = asFormat;
//	//wchar_t* pszDst = szGuiPipeName;
//
//	//while (*pszSrc)
//	//{
//	//	if (*pszSrc == L'%')
//	//	{
//	//		pszSrc++;
//	//		switch (*pszSrc)
//	//		{
//	//		case L's':
//	//			pszSrc++;
//	//			*(pszDst++) = L'.';
//	//			continue;
//	//		case L'u':
//	//			{
//	//				pszSrc++;
//	//				wchar_t szValueDec[16] = {};
//	//				DWORD nValue = anValue, i = 0;
//	//				wchar_t* pszValue = szValueDec;
//	//				while (nValue)
//	//				{
//	//					WORD n = (WORD)(nValue % 10);
//	//					*(pszValue++) = (wchar_t)(L'0' + n);
//	//					nValue = (nValue - n) / 10;
//	//				}
//	//				if (pszValue == szValueDec)
//	//					*(pszValue++) = L'0';
//	//				// Теперь перекинуть в szGuiPipeName
//	//				while (pszValue > szValueDec)
//	//				{
//	//					*(pszDst++) = *(pszValue--);
//	//				}
//	//				continue;
//	//			}
//	//		case L'0':
//	//			if (pszSrc[1] == L'8' && pszSrc[2] == L'X')
//	//			{
//	//				pszSrc += 3;
//	//				wchar_t szValueHex[16] = L"00000000";
//	//				DWORD nValue = anValue, i = 0;
//	//				wchar_t* pszValue = szValueHex;
//	//				while (nValue)
//	//				{
//	//					WORD n = (WORD)(nValue & 0xF);
//	//					if (n <= 9)
//	//						*(pszValue++) = (wchar_t)(L'0' + n);
//	//					else
//	//						*(pszValue++) = (wchar_t)(L'A' + n - 10);
//	//					nValue = nValue >> 4;
//	//				}
//	//				// Теперь перекинуть в szGuiPipeName
//	//				for (pszValue = (szValueHex+7); pszValue >= szValueHex; pszValue--)
//	//				{
//	//					*(pszDst++) = *pszValue;
//	//				}
//	//				continue;
//	//			}
//	//		default:
//	//			_ASSERTE(*pszSrc == L'u' || *pszSrc == L's');
//	//		}
//	//	}
//	//	else
//	//	{
//	//		*(pszDst++) = *(pszSrc++);
//	//	}
//	//}
//	//*pszDst = 0;
//
//	//_ASSERTE(lstrlen(szGuiPipeName) < countof(szGuiPipeName));
//
//	//return szGuiPipeName;
//}

LPCWSTR ModuleName(LPCWSTR asDefault)
{
	if (asDefault && *asDefault)
		return asDefault;

	static wchar_t szFile[32];

	if (szFile[0])
		return szFile;

	wchar_t szPath[MAX_PATH*2];

	if (GetModuleFileNameW(nullptr, szPath, countof(szPath)))
	{
		wchar_t *pszSlash = wcsrchr(szPath, L'\\');

		if (pszSlash)
			pszSlash++;
		else
			pszSlash = szPath;

		lstrcpynW(szFile, pszSlash, countof(szFile));
	}

	if (szFile[0] == 0)
	{
		wcscpy_c(szFile, L"Unknown");
	}

	return szFile;
}

#ifdef _DEBUG
BOOL IsProcessDebugged(DWORD nPID)
{
	BOOL lbServerIsDebugged = FALSE;

	// WinXP SP1 and above
	typedef BOOL (WINAPI* CheckRemoteDebuggerPresent_t)(HANDLE hProcess, PBOOL pbDebuggerPresent);
	static CheckRemoteDebuggerPresent_t _CheckRemoteDebuggerPresent = nullptr;

	if (!_CheckRemoteDebuggerPresent)
	{
		const MModule hKernel(GetModuleHandle(L"kernel32.dll"));
		hKernel.GetProcAddress("CheckRemoteDebuggerPresent", _CheckRemoteDebuggerPresent);
	}
	
	if (_CheckRemoteDebuggerPresent)
	{
		auto* const hProcess = OpenProcess(MY_PROCESS_ALL_ACCESS, FALSE, nPID);
		if (hProcess)
		{
			BOOL lb = FALSE;

			if (_CheckRemoteDebuggerPresent(hProcess, &lb) && lb)
				lbServerIsDebugged = TRUE;

			CloseHandle(hProcess);
		}
	}

	return lbServerIsDebugged;
}
#endif

// nTimeout - таймаут подключения
HANDLE ExecuteOpenPipe(const wchar_t* szPipeName, wchar_t (&szErr)[MAX_PATH*2], const wchar_t* szModule, DWORD nServerPID, DWORD nTimeout, BOOL Overlapped /*= FALSE*/, HANDLE hStop /*= nullptr*/, BOOL bIgnoreAbsence /*= FALSE*/)
{
	HANDLE hPipe = nullptr;
	DWORD dwErr = 0, dwMode = 0;
	BOOL fSuccess = FALSE;
	const DWORD dwStartTick = GetTickCount();
	const DWORD nSleepError = 10;
	// допустимое количество обломов, отличных от ERROR_PIPE_BUSY. после каждого - Sleep(nSleepError);
	// Увеличим допустимое количество попыток, иначе облом наступает раньше секунды ожидания
	const int nDefaultTries = 100;
	int nTries = nDefaultTries;
	// nTimeout должен ограничивать ВЕРХНЮЮ границу времени ожидания
	_ASSERTE(EXECUTE_CMD_OPENPIPE_TIMEOUT >= nTimeout);
	const DWORD nOpenPipeTimeout = nTimeout ? std::min<DWORD>(nTimeout, EXECUTE_CMD_OPENPIPE_TIMEOUT) : EXECUTE_CMD_OPENPIPE_TIMEOUT;
	_ASSERTE(nOpenPipeTimeout > 0);
	const DWORD nWaitPipeTimeout = std::min<DWORD>(250, nOpenPipeTimeout);

	BOOL bWaitPipeRc = FALSE, bWaitCalled = FALSE;
	DWORD nWaitPipeErr = 0;
	DWORD nDuration = 0;
	DWORD nStopWaitRc = static_cast<DWORD>(-1);

	#ifdef DEBUG_RETRY_PIPE_OPEN_DLG
	wchar_t szDbgMsg[512], szTitle[128];
	#endif

	// WinXP SP1 and higher
	// ReSharper disable once CppDeclaratorNeverUsed
	DEBUGTEST(BOOL lbServerIsDebugged = nServerPID ? IsProcessDebugged(nServerPID) : FALSE);

	
	_ASSERTE(LocalSecurity()!=nullptr);


	// Try to open a named pipe; wait for it, if necessary.
	while (true)
	{
		hPipe = CreateFile(
		            szPipeName,     // pipe name
		            GENERIC_READ|GENERIC_WRITE,
		            0,              // no sharing
		            LocalSecurity(), // default security attributes
		            OPEN_EXISTING,  // opens existing pipe
		            (Overlapped ? FILE_FLAG_OVERLAPPED : 0), // default attributes
		            nullptr);          // no template file
		dwErr = GetLastError();

		// Break if the pipe handle is valid.
		if (hPipe && (hPipe != INVALID_HANDLE_VALUE))
		{
			break; // OK, opened
		}
		_ASSERTE(hPipe);

		#ifdef DEBUG_RETRY_PIPE_OPEN_DLG
		if (gbPipeDebugBoxes)
		{
			szDbgMsg[0] = 0;
			GetModuleFileName(nullptr, szDbgMsg, countof(szDbgMsg));
			msprintf(szTitle, countof(szTitle), L"%s: PID=%u", PointToName(szDbgMsg), GetCurrentProcessId());
			msprintf(szDbgMsg, countof(szDbgMsg), L"Can't open pipe, ErrCode=%u\n%s\nWait: %u,%u,%u", dwErr, szPipeName, bWaitCalled, bWaitPipeRc, nWaitPipeErr);
			const int nBtn = MessageBoxW(nullptr, szDbgMsg, szTitle, MB_SYSTEMMODAL|MB_RETRYCANCEL);
			if (nBtn == IDCANCEL)
				return nullptr;
		}
		#endif

		nDuration = GetTickCount() - dwStartTick;

		if (hStop)
		{
			// Was requested application stop or so
			nStopWaitRc = WaitForSingleObject(hStop, 0);
			if (nStopWaitRc == WAIT_OBJECT_0)
			{
				return nullptr;
			}
		}

		if (dwErr == ERROR_PIPE_BUSY)
		{
			if ((nTries > 0) && (nDuration < nOpenPipeTimeout))
			{
				// ReSharper disable once CppAssignedValueIsNeverUsed
				bWaitCalled = TRUE;

				// All pipe instances are busy, so wait for a while (not more 500 ms).
				// ReSharper disable once CppAssignedValueIsNeverUsed
				bWaitPipeRc = WaitNamedPipe(szPipeName, nWaitPipeTimeout);
				// ReSharper disable once CppAssignedValueIsNeverUsed
				nWaitPipeErr = GetLastError();
				UNREFERENCED_PARAMETER(bWaitPipeRc); UNREFERENCED_PARAMETER(nWaitPipeErr);
				// -- 120602 раз они заняты (но живы), то будем ждать, пока не освободятся
				//nTries--;
				continue;
			}
			else
			{
				_ASSERTEX(dwErr != ERROR_PIPE_BUSY);
			}
		}

		// Сделаем так, чтобы хотя бы пару раз он попробовал повторить
		if ((nTries <= 0) || (nDuration > nOpenPipeTimeout))
		{
			#ifdef DEBUG_RETRY_PIPE_OPEN_DLG
			msprintf(szErr, countof(szErr), L"%s.%u\nCreateFile(%s) failed\ncode=%u%s%s",
			          ModuleName(szModule), GetCurrentProcessId(), szPipeName, dwErr,
			          (nTries <= 0) ? L", Tries" : L"", (nDuration > nOpenPipeTimeout) ? L", Duration" : L"");
			//_ASSERTEX(FALSE && "Pipe open failed with timeout!");
			const int iBtn = bIgnoreAbsence ? IDCANCEL
				: MessageBoxW(nullptr, szErr, L"Pipe open failed with timeout!", MB_ICONSTOP|MB_SYSTEMMODAL|MB_RETRYCANCEL);
			if (iBtn == IDRETRY)
			{
				nTries = nDefaultTries;
				continue;
			}
			#endif
			SetLastError(dwErr);
			return nullptr;
		}
		else
		{
			nTries--;
		}

		// Может быть пайп еще не создан (в процессе срабатывания семафора)
		if (dwErr == ERROR_FILE_NOT_FOUND)
		{
			// Wait for a while (10 ms)
			Sleep(nSleepError);
			continue;
		}

		// Exit if an error other than ERROR_PIPE_BUSY occurs.
		// -- if (dwErr != ERROR_PIPE_BUSY) // уже проверено выше
		msprintf(szErr, countof(szErr), L"%s.%u: CreateFile(%s) failed, code=%u",
		          ModuleName(szModule), GetCurrentProcessId(), szPipeName, dwErr);
		SetLastError(dwErr);

		// Failed!
		return nullptr;

		// Уже сделано выше
		//// All pipe instances are busy, so wait for 500 ms.
		//WaitNamedPipe(szPipeName, 500);
		//if (!WaitNamedPipe(szPipeName, 1000) )
		//{
		//	dwErr = GetLastError();
		//	if (pszErr)
		//	{
		//		StringCchPrintf(pszErr, countof(pszErr), L"%s: WaitNamedPipe(%s) failed, code=0x%08X, WaitNamedPipe",
		//			szModule ? szModule : L"Unknown", szPipeName, dwErr);
		//		// Видимо это возникает в момент запуска (обычно для ShiftEnter - новая консоль)
		//		// не сразу срабатывает GUI и RCon еще не создал Pipe для HWND консоли
		//		_ASSERTE(dwErr == 0);
		//	}
		//    return nullptr;
		//}
	}

#ifdef _DEBUG
	DWORD nCurState = 0, nCurInstances = 0;
	// ReSharper disable once CppDeclaratorNeverUsed
	BOOL bCurState = GetNamedPipeHandleState(hPipe, &nCurState, &nCurInstances, nullptr, nullptr, nullptr, 0);
#endif

	// The pipe connected; change to message-read mode.
	dwMode = CE_PIPE_READMODE;
	// ReSharper disable once CppAssignedValueIsNeverUsed
	fSuccess = SetNamedPipeHandleState(
	               hPipe,    // pipe handle
	               &dwMode,  // new pipe mode
	               nullptr,     // don't set maximum bytes
	               nullptr);    // don't set maximum time

	UNREFERENCED_PARAMETER(bWaitCalled);
	UNREFERENCED_PARAMETER(fSuccess);

	return hPipe;
}


void ExecutePrepareCmd(CESERVER_REQ* pIn, DWORD nCmd, size_t cbSize)
{
	if (!pIn)
		return;

	ExecutePrepareCmd(&(pIn->hdr), nCmd, cbSize);
	
	// Reset data tail
	if (cbSize > sizeof(pIn->hdr))
	{
		memset(reinterpret_cast<LPBYTE>(&(pIn->hdr)) + sizeof(pIn->hdr), 0, cbSize - sizeof(pIn->hdr));
	}
}

void ExecutePrepareCmd(CESERVER_REQ_HDR* pHdr, DWORD nCmd, size_t cbSize)
{
	if (!pHdr)
		return;

	pHdr->nCmd = nCmd;
	pHdr->bAsync = FALSE; // reset
	pHdr->nSrcThreadId = GetCurrentThreadId();
	pHdr->nSrcPID = GetCurrentProcessId();
	// We exchange data between 32bit and 64bit processes, 64-bit sizes are not allowed
	_ASSERTE(cbSize == static_cast<DWORD>(cbSize));
	pHdr->cbSize = static_cast<DWORD>(cbSize);
	pHdr->nVersion = CESERVER_REQ_VER;
	pHdr->nCreateTick = GetTickCount();
	_ASSERTE(ghWorkingModule!=0);
	pHdr->hModule = ghWorkingModule;
	pHdr->nBits = WIN3264TEST(32,64);
	pHdr->nLastError = GetLastError();
	pHdr->IsDebugging = IsDebuggerPresent();
}

CESERVER_REQ* ExecuteNewCmd(DWORD nCmd, size_t nSize)
{
	_ASSERTE(nSize>=sizeof(CESERVER_REQ_HDR));
	CESERVER_REQ* pIn = nullptr;

	if (nSize)
	{
		const DWORD nErr = GetLastError();
		
		// Обязательно с обнулением выделяемой памяти
		pIn = static_cast<CESERVER_REQ*>(calloc(nSize, 1));

		if (pIn)
		{
			ExecutePrepareCmd(pIn, nCmd, nSize);
			pIn->hdr.nLastError = nErr;
		}
	}

	return pIn;
}

bool ExecuteNewCmd(CESERVER_REQ* &ppCmd, DWORD &pcbCurMaxSize, DWORD nCmd, size_t nSize)
{
	if (!ppCmd || (pcbCurMaxSize < nSize))
	{
		const DWORD nErr = GetLastError();
		ExecuteFreeResult(ppCmd);
		ppCmd = ExecuteNewCmd(nCmd, nSize);
		if (ppCmd != nullptr)
		{
			// Обмен данными идет и между 32bit & 64bit процессами, размеры __int64 недопустимы
			_ASSERTE(nSize == static_cast<DWORD>(nSize));
			pcbCurMaxSize = static_cast<DWORD>(nSize);
			ppCmd->hdr.nLastError = nErr;
		}
	}
	else
	{
		ExecutePrepareCmd(ppCmd, nCmd, nSize);
	}
	
	return (ppCmd != nullptr);
}

// hConWnd - HWND _реальной_ консоли

/// <summary>
/// Loads CESERVER_CONSOLE_MAPPING_HDR data for specified hConWnd
/// </summary>
/// <param name="hConWnd">HWND of the <b>REAL</b> console</param>
/// <param name="SrvMapping">CESERVER_CONSOLE_MAPPING_HDR& [OUT]</param>
/// <returns>TRUE if mapping was successfully loaded</returns>
BOOL LoadSrvMapping(HWND hConWnd, CESERVER_CONSOLE_MAPPING_HDR& SrvMapping)
{
	if (!hConWnd)
		return false;

	MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> SrvInfoMapping;
	SrvInfoMapping.InitName(CECONMAPNAME, LODWORD(hConWnd));
	const CESERVER_CONSOLE_MAPPING_HDR* pInfo = SrvInfoMapping.Open();
	if (!pInfo || pInfo->nProtocolVersion != CESERVER_REQ_VER)
	{
		return false;
	}

	memmove(&SrvMapping, pInfo, std::min<size_t>(pInfo->cbSize, sizeof(SrvMapping)));
	SrvInfoMapping.CloseMap();

	return (SrvMapping.cbSize != 0);
}

BOOL LoadGuiMapping(DWORD nConEmuPID, ConEmuGuiMapping& GuiMapping)
{
	if (!nConEmuPID)
		return FALSE;

	MFileMapping<ConEmuGuiMapping> GuiInfoMapping;
	GuiInfoMapping.InitName(CEGUIINFOMAPNAME, nConEmuPID);
	const ConEmuGuiMapping* pInfo = GuiInfoMapping.Open();
	if (!pInfo || pInfo->nProtocolVersion != CESERVER_REQ_VER)
	{
		return false;
	}

	memmove(&GuiMapping, pInfo, std::min<size_t>(pInfo->cbSize, sizeof(GuiMapping)));
	GuiInfoMapping.CloseMap();

	return (GuiMapping.cbSize != 0);
}


CESERVER_REQ* ExecuteNewCmdOnCreate(CESERVER_CONSOLE_MAPPING_HDR* pSrvMap, HWND hConWnd, enum CmdOnCreateType aCmd,
				LPCWSTR asAction, LPCWSTR asFile, LPCWSTR asParam, LPCWSTR asDir,
				DWORD* anShellFlags, DWORD* anCreateFlags, DWORD* anStartFlags, DWORD* anShowCmd,
				int mn_ImageBits, int mn_ImageSubsystem,
				HANDLE hStdIn, HANDLE hStdOut, HANDLE hStdErr)
{
	static GuiLoggingType LastLogType = glt_None;

	if (!pSrvMap)
	{
		static DWORD nLastWasEnabledTick = 0;

		// Чтобы проверки слишком часто не делать
		if (!nLastWasEnabledTick || ((GetTickCount() - nLastWasEnabledTick) > 1000))
		{
			CESERVER_CONSOLE_MAPPING_HDR *Info = (CESERVER_CONSOLE_MAPPING_HDR*)calloc(1,sizeof(*Info));
			if (Info)
			{
				if (::LoadSrvMapping(hConWnd, *Info))
				{
					_ASSERTE(Info->ComSpec.ConEmuExeDir[0] && Info->ComSpec.ConEmuBaseDir[0]);
					LastLogType = (GuiLoggingType)Info->nLoggingType;
				}
				free(Info);
			}

			nLastWasEnabledTick = GetTickCount();
		}
	}
	else
	{
		LastLogType = (GuiLoggingType)pSrvMap->nLoggingType;
	}

	bool bEnabled;
	if ((aCmd == eShellExecute) || (aCmd == eCreateProcess))
		bEnabled = (LastLogType == glt_Processes) || (LastLogType == glt_Shell);
	else
		bEnabled = (LastLogType == glt_Shell);

	// Was logging requested?
	if (!bEnabled)
	{
		return nullptr;
	}

	
	CESERVER_REQ *pIn = nullptr;
	
	int nActionLen = (asAction ? lstrlen(asAction) : 0)+1;
	int nFileLen = (asFile ? lstrlen(asFile) : 0)+1;
	int nParamLen = (asParam ? lstrlen(asParam) : 0)+1;
	int nDirLen = (asDir ? lstrlen(asDir) : 0)+1;
	
	pIn = ExecuteNewCmd(CECMD_ONCREATEPROC, sizeof(CESERVER_REQ_HDR)
		+sizeof(CESERVER_REQ_ONCREATEPROCESS)
		+(nActionLen+nFileLen+nParamLen+nDirLen)*sizeof(wchar_t));
	
	pIn->OnCreateProc.cbStructSize = sizeof(pIn->OnCreateProc);
	pIn->OnCreateProc.nSourceBits = WIN3264TEST(32,64); //-V112
	//pIn->OnCreateProc.bUnicode = TRUE;
	pIn->OnCreateProc.nImageSubsystem = mn_ImageSubsystem;
	pIn->OnCreateProc.nImageBits = mn_ImageBits;
	pIn->OnCreateProc.hStdIn = (unsigned __int64)(hStdIn ? hStdIn : GetStdHandle(STD_INPUT_HANDLE));
	pIn->OnCreateProc.hStdOut = (unsigned __int64)(hStdOut ? hStdOut : GetStdHandle(STD_OUTPUT_HANDLE));
	pIn->OnCreateProc.hStdErr = (unsigned __int64)(hStdErr ? hStdErr : GetStdHandle(STD_ERROR_HANDLE));
	
	switch (aCmd)
	{
	case eShellExecute:
		wcscpy_c(pIn->OnCreateProc.sFunction, L"Shell"); break;
	case eCreateProcess:
		wcscpy_c(pIn->OnCreateProc.sFunction, L"Create"); break;
	case eInjectingHooks:
		wcscpy_c(pIn->OnCreateProc.sFunction, L"Inject"); break;
	case eHooksLoaded:
		wcscpy_c(pIn->OnCreateProc.sFunction, L"HkLoad"); break;
	case eSrvLoaded:
		wcscpy_c(pIn->OnCreateProc.sFunction, L"SrLoad"); break;
	case eParmsChanged:
		wcscpy_c(pIn->OnCreateProc.sFunction, L"Changed"); break;
	case eLoadLibrary:
		wcscpy_c(pIn->OnCreateProc.sFunction, L"LdLib"); break;
	case eFreeLibrary:
		wcscpy_c(pIn->OnCreateProc.sFunction, L"FrLib"); break;
	default:
		wcscpy_c(pIn->OnCreateProc.sFunction, L"Unknown");
	}
	
	pIn->OnCreateProc.nShellFlags = anShellFlags ? *anShellFlags : 0;
	pIn->OnCreateProc.nCreateFlags = anCreateFlags ? *anCreateFlags : 0;
	pIn->OnCreateProc.nStartFlags = anStartFlags ? *anStartFlags : 0;
	pIn->OnCreateProc.nShowCmd = anShowCmd ? *anShowCmd : 0;
	pIn->OnCreateProc.nActionLen = nActionLen;
	pIn->OnCreateProc.nFileLen = nFileLen;
	pIn->OnCreateProc.nParamLen = nParamLen;
	pIn->OnCreateProc.nDirLen = nDirLen;
	
	wchar_t* psz = pIn->OnCreateProc.wsValue;
	if (nActionLen > 1)
		_wcscpy_c(psz, nActionLen, asAction);
	psz += nActionLen;
	if (nFileLen > 1)
		_wcscpy_c(psz, nFileLen, asFile);
	psz += nFileLen;
	if (nParamLen > 1)
		_wcscpy_c(psz, nParamLen, asParam);
	psz += nParamLen;
	if (nDirLen > 1)
		_wcscpy_c(psz, nDirLen, asDir);
	psz += nDirLen;

	return pIn;
}

// Forward
//CESERVER_REQ* ExecuteCmd(const wchar_t* szGuiPipeName, CESERVER_REQ* pIn, DWORD nWaitPipe, HWND hOwner);

// Выполнить в GUI (в CRealConsole)
CESERVER_REQ* ExecuteGuiCmd(HWND hConWnd, CESERVER_REQ* pIn, HWND hOwner, BOOL bAsyncNoResult /*= FALSE*/)
{
	wchar_t szGuiPipeName[128];

	if (!hConWnd)
		return nullptr;

	DWORD nLastErr = GetLastError();
	//swprintf_c(szGuiPipeName, CEGUIPIPENAME, L".", (DWORD)hConWnd);
	msprintf(szGuiPipeName, countof(szGuiPipeName), CEGUIPIPENAME, L".", LODWORD(hConWnd));

	#ifdef _DEBUG
	DWORD nStartTick = GetTickCount();
	#endif

	CESERVER_REQ* lpRet = ExecuteCmd(szGuiPipeName, pIn, 1000, hOwner, bAsyncNoResult);

	#ifdef _DEBUG
	DWORD nEndTick = GetTickCount();
	DWORD nDelta = nEndTick - nStartTick;
	if (nDelta >= EXECUTE_CMD_WARN_TIMEOUT)
	{
		if (!IsDebuggerPresent())
		{
			if (lpRet)
			{
				_ASSERTE(nDelta <= EXECUTE_CMD_WARN_TIMEOUT || lpRet->hdr.IsDebugging || (pIn->hdr.nCmd == CECMD_CMDSTARTSTOP && nDelta <= EXECUTE_CMD_WARN_TIMEOUT2));
			}
			else
			{
				_ASSERTE(nDelta <= EXECUTE_CMD_TIMEOUT_SRV_ABSENT);
			}
		}
	}
	#endif

	SetLastError(nLastErr); // Чтобы не мешать процессу своими возможными ошибками общения с пайпом
	return lpRet;
}

CESERVER_REQ* ExecuteGuiCmd(HWND hConWnd, DWORD nCmd, size_t cbDataSize, const BYTE* data, HWND hOwner, BOOL bAsyncNoResult /*= FALSE*/)
{
	CESERVER_REQ* pOut = nullptr;
	CESERVER_REQ* pIn = ExecuteNewCmd(nCmd, sizeof(CESERVER_REQ_HDR)+cbDataSize);

	if (pIn)
	{
		if (cbDataSize)
		{
			_ASSERTEX(data != nullptr);
			memmove(pIn->Data, data, cbDataSize);
		}

		pOut = ExecuteGuiCmd(hConWnd, pIn, hOwner, bAsyncNoResult);

		ExecuteFreeResult(pIn);
	}

	return pOut;
}

// Выполнить в ConEmuC
CESERVER_REQ* ExecuteSrvCmd(DWORD dwSrvPID, CESERVER_REQ* pIn, HWND hOwner, BOOL bAsyncNoResult, DWORD nTimeout /*= 0*/, BOOL bIgnoreAbsence /*= FALSE*/)
{
	wchar_t szPipeName[128];

	if (!dwSrvPID)
		return nullptr;

	DWORD nLastErr = GetLastError();
	//swprintf_c(szPipeName, CESERVERPIPENAME, L".", (DWORD)dwSrvPID);
	msprintf(szPipeName, countof(szPipeName), CESERVERPIPENAME, L".", (DWORD)dwSrvPID);
	CESERVER_REQ* lpRet = ExecuteCmd(szPipeName, pIn, nTimeout, hOwner, bAsyncNoResult, dwSrvPID, bIgnoreAbsence);
	_ASSERTE(pIn->hdr.bAsync == bAsyncNoResult);
	SetLastError(nLastErr); // Чтобы не мешать процессу своими возможными ошибками общения с пайпом
	return lpRet;
}

// Выполнить в ConEmuC
CESERVER_REQ* ExecuteSrvCmd(DWORD dwSrvPID, DWORD nCmd, size_t cbDataSize, const BYTE* data, HWND hOwner, BOOL bAsyncNoResult /*= FALSE*/)
{
	CESERVER_REQ* pOut = nullptr;

	if (CESERVER_REQ* pIn = ExecuteNewCmd(nCmd, sizeof(CESERVER_REQ_HDR)+cbDataSize))
	{
		if (cbDataSize && data)
			memmove_s(pIn->Data, pIn->DataSize(), data, cbDataSize);

		pOut = ExecuteSrvCmd(dwSrvPID, pIn, hOwner, bAsyncNoResult);

		ExecuteFreeResult(pIn);
	}

	return pOut;
}

// Выполнить в ConEmuHk
CESERVER_REQ* ExecuteHkCmd(DWORD dwHkPID, CESERVER_REQ* pIn, HWND hOwner, BOOL bAsyncNoResult /*= FALSE*/, BOOL bIgnoreAbsence /*= FALSE*/)
{
	wchar_t szPipeName[128];

	if (!dwHkPID)
		return nullptr;

	DWORD nLastErr = GetLastError();
	//swprintf_c(szPipeName, CESERVERPIPENAME, L".", (DWORD)dwSrvPID);
	msprintf(szPipeName, countof(szPipeName), CEHOOKSPIPENAME, L".", (DWORD)dwHkPID);
	CESERVER_REQ* lpRet = ExecuteCmd(szPipeName, pIn, 1000, hOwner, bAsyncNoResult, dwHkPID, bIgnoreAbsence);
	SetLastError(nLastErr); // Чтобы не мешать процессу своими возможными ошибками общения с пайпом
	return lpRet;
}

//Arguments:
//   hConWnd - Хэндл КОНСОЛЬНОГО окна (по нему формируется имя пайпа для GUI)
//   pIn     - выполняемая команда
//   nTimeout- таймаут подключения
//Returns:
//   CESERVER_REQ. Его необходимо освободить через free(...);
//WARNING!!!
//   Эта процедура не может получить с сервера более 600 байт данных!
// В заголовке hOwner в дебаге может быть отображена ошибка
CESERVER_REQ* ExecuteCmd(const wchar_t* szGuiPipeName, CESERVER_REQ* pIn, DWORD nWaitPipe, HWND hOwner, BOOL bAsyncNoResult, DWORD nServerPID, BOOL bIgnoreAbsence /*= FALSE*/)
{
	CESERVER_REQ* pOut = nullptr;
	HANDLE hPipe = nullptr;
	BYTE cbReadBuf[600]; // чтобы CESERVER_REQ_OUTPUTFILE поместился
	wchar_t szErr[MAX_PATH*2]; szErr[0] = 0;
	BOOL fSuccess = FALSE;
	DWORD cbRead = 0, /*dwMode = 0,*/ dwErr = 0;
	int nAllSize;
	LPBYTE ptrData;
	#ifdef _DEBUG
	bool bIsAltSrvCmd;
	wchar_t szDbgPrefix[64], szDbgResult[64];
	CEStr pszDbgMsg;
	#endif

	if (!pIn || !szGuiPipeName)
	{
		_ASSERTE(pIn && szGuiPipeName);
		pOut = nullptr;
		goto wrap;
	}

	#ifdef _DEBUG
	swprintf_c(szDbgPrefix, L">> ExecCmd: PID=%5u  TID=%5u  Cmd=%3u  ", GetCurrentProcessId(), GetCurrentThreadId(), pIn->hdr.nCmd);
	pszDbgMsg = CEStr(szDbgPrefix, szGuiPipeName, L"\n");
	if (pszDbgMsg) { DEBUGSTRCMD(pszDbgMsg); pszDbgMsg.Release(); }
	#endif

	pIn->hdr.bAsync = bAsyncNoResult;

	_ASSERTE(pIn->hdr.nSrcPID && pIn->hdr.nSrcThreadId);
	_ASSERTE(pIn->hdr.cbSize >= sizeof(pIn->hdr));
	hPipe = ExecuteOpenPipe(szGuiPipeName, szErr, nullptr/*Сюда хорошо бы имя модуля подкрутить*/, nServerPID, nWaitPipe, FALSE, nullptr, bIgnoreAbsence);

	if (hPipe == nullptr || hPipe == INVALID_HANDLE_VALUE)
	{
		#ifdef _DEBUG
		dwErr = GetLastError();

		// в заголовке "чисто" запущенного фара появляются отладочные(?) сообщения
		// по идее - не должны, т.к. все должно быть через мэппинг
		// *** _ASSERTEX(hPipe != nullptr && hPipe != INVALID_HANDLE_VALUE); - no need in assert, it was already shown
		#ifdef CONEMU_MINIMAL
		SetConsoleTitle(szErr);
		#else
		if (hOwner)
		{
			DWORD_PTR dwResult = 0;
			//TODO: Get off changing window title completely - prefer writing to log
			if (hOwner == myGetConsoleWindow())
				SetConsoleTitle(szErr);
			else
				SendMessageTimeout(hOwner, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(szErr), SMTO_ABORTIFHUNG, 1000, &dwResult);
		}
		#endif

		#endif

		pOut = nullptr;
		goto wrap;
	}

	#ifdef _DEBUG
	// Some commands come from different process than specified in hdr
	bIsAltSrvCmd = (pIn->hdr.nCmd == CECMD_ALTBUFFER || pIn->hdr.nCmd == CECMD_ALTBUFFERSTATE
		|| pIn->hdr.nCmd == CECMD_SETCONSCRBUF
		|| pIn->hdr.nCmd == CECMD_STARTXTERM
		|| pIn->hdr.nCmd == CECMD_LOCKSTATION || pIn->hdr.nCmd == CECMD_UNLOCKSTATION);
	_ASSERTE(pIn->hdr.nSrcThreadId==GetCurrentThreadId() || (bIsAltSrvCmd && pIn->hdr.nSrcPID != GetCurrentProcessId()));
	#endif

	if (bAsyncNoResult)
	{
		// Если нас не интересует возврат и нужно сразу вернуться
		fSuccess = WriteFile(hPipe, pIn, pIn->hdr.cbSize, &cbRead, nullptr);
		#ifdef _DEBUG
		dwErr = GetLastError();
		_ASSERTE(fSuccess && (cbRead == pIn->hdr.cbSize));
		#endif

		// -- Do not close hPipe, otherwise the reader may fail with that packet
		// -- with error ‘pipe was closed before end’.
		// -- Handle leak, yeah, however this is rarely used op.
		// -- Must be refactored, but not so critical...
		// -- CloseHandle(hPipe);

		pOut = nullptr;
		goto wrap;
	}
	else
	{
		WARNING("При Overlapped часто виснет в этом месте.");

		// Send a message to the pipe server and read the response.
		fSuccess = TransactNamedPipe(
					   hPipe,                  // pipe handle
					   (LPVOID)pIn,            // message to server
					   pIn->hdr.cbSize,         // message length
					   cbReadBuf,              // buffer to receive reply
					   sizeof(cbReadBuf),      // size of read buffer
					   &cbRead,                // bytes read
					   nullptr);                  // not overlapped
		dwErr = GetLastError();
		//CloseHandle(hPipe);

		if (!fSuccess && (dwErr != ERROR_MORE_DATA))
		{
			//_ASSERTE(fSuccess || (dwErr == ERROR_MORE_DATA));
			CloseHandle(hPipe);

			pOut = nullptr;
			goto wrap;
		}
	}

	if (cbRead < sizeof(CESERVER_REQ_HDR))
	{
		CloseHandle(hPipe);

		pOut = nullptr;
		goto wrap;
	}

	pOut = (CESERVER_REQ*)cbReadBuf; // temporary

	if (pOut->hdr.cbSize < cbRead)
	{
		CloseHandle(hPipe);
		if (pOut->hdr.cbSize)
		{
			_ASSERTE(pOut->hdr.cbSize == 0 || pOut->hdr.cbSize >= cbRead);
			DEBUGSTR(L"!!! Wrong nSize received from GUI server !!!\n");
		}

		pOut = nullptr;
		goto wrap;
	}

	if (pOut->hdr.nVersion != CESERVER_REQ_VER)
	{
		CloseHandle(hPipe);
		DEBUGSTR(L"!!! Wrong nVersion received from GUI server !!!\n");

		pOut = nullptr;
		goto wrap;
	}

	nAllSize = pOut->hdr.cbSize;
	pOut = (CESERVER_REQ*)malloc(nAllSize);
	_ASSERTE(pOut);

	if (!pOut)
	{
		CloseHandle(hPipe);

		_ASSERTE(pOut == nullptr);
		goto wrap;
	}

	memmove(pOut, cbReadBuf, cbRead);
	ptrData = reinterpret_cast<LPBYTE>(pOut) + cbRead;
	nAllSize -= cbRead;

	while (nAllSize>0)
	{
		// Break if TransactNamedPipe or ReadFile is successful
		if (fSuccess)
			break;

		// Read from the pipe if there is more data in the message.
		fSuccess = ReadFile(
		               hPipe,      // pipe handle
		               ptrData,    // buffer to receive reply
		               nAllSize,   // size of buffer
		               &cbRead,    // number of bytes read
		               nullptr);      // not overlapped

		// Exit if an error other than ERROR_MORE_DATA occurs.
		if (!fSuccess && ((dwErr = GetLastError()) != ERROR_MORE_DATA))
			break;

		ptrData += cbRead;
		nAllSize -= cbRead;
	}

	CloseHandle(hPipe);
	if (pOut && (pOut->hdr.nCmd != pIn->hdr.nCmd))
	{
		_ASSERTE(pOut->hdr.nCmd == pIn->hdr.nCmd);
		if (pOut->hdr.nCmd == 0)
		{
			ExecuteFreeResult(pOut);
			pOut = nullptr;
		}
	}

wrap:
	#ifdef _DEBUG
	if (pOut)
		swprintf_c(szDbgResult, L"- Data=%5u  Err=%u\n", pOut->DataSize(), dwErr);
	else
		lstrcpyn(szDbgResult, L"[NULL]\n", countof(szDbgResult));
	pszDbgMsg = CEStr(szDbgPrefix, szDbgResult);
	if (pszDbgMsg) { DEBUGSTRCMD(pszDbgMsg); pszDbgMsg.Release(); }
	#endif

	return pOut;
}

void ExecuteFreeResult(CESERVER_REQ* &pOut)
{
	if (!pOut) return;

	CESERVER_REQ* p = pOut;
	pOut = nullptr;
	free(p);
}

ConEmuRpc::ConEmuRpc()
{
	nPreLastError = GetLastError();
	szPipeName[0] = 0;
	szError[0] = 0;
}

ConEmuRpc::~ConEmuRpc()
{
	// Don't harm active process with possible pipe errors
	SetLastError(nPreLastError);
}

ConEmuRpc& ConEmuRpc::SetStopHandle(HANDLE ahStop)
{
	this->hStop = ahStop;
	return *this;
}

ConEmuRpc& ConEmuRpc::SetOwner(HWND ahOwner)
{
	this->hOwner = ahOwner;
	return *this;
}

ConEmuRpc& ConEmuRpc::SetModuleName(const wchar_t* asModule)
{
	this->szModule = asModule;
	return *this;
}

ConEmuRpc& ConEmuRpc::SetAsyncNoResult(const bool abAsyncNoResult)
{
	this->bAsyncNoResult = abAsyncNoResult;
	return *this;
}

ConEmuRpc& ConEmuRpc::SetOverlapped(const bool abOverlapped)
{
	this->bOverlapped = abOverlapped;
	return *this;
}

ConEmuRpc& ConEmuRpc::SetIgnoreAbsence(const bool abIgnoreAbsence)
{
	this->bIgnoreAbsence = abIgnoreAbsence;
	return *this;
}

ConEmuRpc& ConEmuRpc::SetTimeout(const DWORD anTimeoutMs)
{
	this->nTimeoutMs = anTimeoutMs;
	return *this;
}

CESERVER_REQ* ConEmuRpc::Execute(CESERVER_REQ* pIn) const
{
	if (szPipeName[0] == L'\0')
	{
		_ASSERTE(szPipeName[0] != L'\0');
		return nullptr;
	}
	if (!pIn)
	{
		_ASSERTE(pIn != nullptr);
		return nullptr;
	}
	
	#ifdef _DEBUG
	const DWORD nStartTick = GetTickCount();
	#endif

	CESERVER_REQ* lpRet = ExecuteCmd(szPipeName, pIn, nTimeoutMs, hOwner, bAsyncNoResult, nServerPID, bIgnoreAbsence);

	#ifdef _DEBUG
	const DWORD nEndTick = GetTickCount();
	const DWORD nDelta = nEndTick - nStartTick;
	const DWORD nWarnExecutionTime = (pIn->hdr.nCmd == CECMD_CMDSTARTSTOP) ? EXECUTE_CMD_WARN_TIMEOUT2 : EXECUTE_CMD_WARN_TIMEOUT;
	if (nDelta >= EXECUTE_CMD_WARN_TIMEOUT)
	{
		if (!IsDebuggerPresent())
		{
			if (lpRet)
			{
				_ASSERTE(nDelta <= nWarnExecutionTime || lpRet->hdr.IsDebugging);
			}
			else
			{
				_ASSERTE(nDelta <= EXECUTE_CMD_TIMEOUT_SRV_ABSENT || bIgnoreAbsence);
			}
		}
	}
	#endif

	return lpRet;
}

CESERVER_REQ* ConEmuRpc::Execute(const CECMD nCmd, const void* const data, const size_t cbDataSize) const
{
	CESERVER_REQ* pOut = nullptr;
	CESERVER_REQ* pIn = ExecuteNewCmd(nCmd, sizeof(CESERVER_REQ_HDR) + cbDataSize);

	if (pIn)
	{
		if (cbDataSize)
		{
			_ASSERTEX(data != nullptr);
			memmove_s(pIn->Data, cbDataSize, data, cbDataSize);
		}

		pOut = this->Execute(pIn);

		ExecuteFreeResult(pIn);
	}

	return pOut;
}

LPCWSTR ConEmuRpc::GetErrorText() const
{
	return this->szError;
}

ConEmuGuiRpc::ConEmuGuiRpc(HWND ahConWnd)
	: ConEmuRpc(), hConWnd(ahConWnd)
{
	msprintf(szPipeName, countof(szPipeName), CEGUIPIPENAME, L".", LODWORD(hConWnd));
}

ConEmuGuiRpc::~ConEmuGuiRpc()
= default;

bool AllocateSendCurrentDirectory(CESERVER_REQ* &ppCmd, DWORD &pcbCurMaxSize, LPCWSTR asDirectory, LPCWSTR asPassiveDirectory /*= nullptr*/)
{
	int iALen = asDirectory ? (lstrlen(asDirectory)+1) : 0;
	int iPLen = asPassiveDirectory ? (lstrlen(asPassiveDirectory)+1) : 0;
	if ((iALen < 0) || (iPLen < 0) || (!iALen && !iPLen))
		return false;

	size_t cbMax = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_STORECURDIR) + (iALen + iPLen) * sizeof(*asDirectory);
	if (!ExecuteNewCmd(ppCmd, pcbCurMaxSize, CECMD_STORECURDIR, cbMax))
		return false;

	ppCmd->CurDir.iActiveCch = iALen;
	ppCmd->CurDir.iPassiveCch = iPLen;
	wchar_t* psz = ppCmd->CurDir.szDir;
	if (iALen)
	{
		lstrcpyn(psz, asDirectory, iALen);
		psz += iALen;
	}
	if (iPLen)
	{
		lstrcpyn(psz, asPassiveDirectory, iPLen);
	}

	return true;
}

void SendCurrentDirectory(HWND hConWnd, LPCWSTR asDirectory, LPCWSTR asPassiveDirectory /*= nullptr*/)
{
	CESERVER_REQ* pIn = nullptr; DWORD cbSize = 0;
	if (!AllocateSendCurrentDirectory(pIn, cbSize, asDirectory, asPassiveDirectory))
		return;

	CESERVER_REQ* pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd, TRUE);
	ExecuteFreeResult(pOut);
	ExecuteFreeResult(pIn);
}

bool IsConsoleClass(LPCWSTR asClass)
{
	if (IsStrNotEmpty(asClass)
		&& (
			(lstrcmp(asClass, RealConsoleClass) == 0)
			|| (lstrcmp(asClass, WineConsoleClass) == 0)
		))
		return true;

	return false;
}

bool IsConsoleWindow(HWND hWnd)
{
	wchar_t szClass[64] = L"";

	if (!hWnd)
		return false;
	if (!GetClassName(hWnd, szClass, countof(szClass)))
		return false;
	if (!IsConsoleClass(szClass))
		return false;

	// But when process is hooked, GetConsoleClass will return "proper" name
	// even if hWnd points to our VirtualConsole

	// RealConsole handle is stored in the Window DATA
	wchar_t szClassPtr[64] = L"";
	HWND h = (HWND)GetWindowLongPtr(hWnd, WindowLongDCWnd_ConWnd);
	if (h && (h != hWnd) && IsWindow(h))
	{
		if (GetClassName(h, szClassPtr, countof(szClassPtr)))
		{
			if (IsConsoleClass(szClassPtr))
			{
				_ASSERTE(FALSE && "RealConsole handle was not retrieved properly!");
				return false;
			}
		}
	}

	// Well, it's a RealConsole
	return true;
}

bool IsPseudoConsoleWindow(HWND hWnd)
{
	wchar_t szClass[64] = L"";

	if (!hWnd)
		return false;
	if (!GetClassName(hWnd, szClass, countof(szClass)))
		return false;
	const int cmp = lstrcmp(szClass, PseudoConsoleClass);
	return (cmp == 0);
}

GetConsoleWindow_T gfGetRealConsoleWindow = nullptr;

HWND myGetConsoleWindow()
{
	HWND hConWnd = nullptr;

	// If we are in ConEmuHk than gfGetRealConsoleWindow may be set
	if (gfGetRealConsoleWindow)
	{
		hConWnd = gfGetRealConsoleWindow();
		// If the function pointer was set - it must be proper function
		_ASSERTEX(hConWnd==nullptr || IsConsoleWindow(hConWnd));
		return hConWnd;
	}

	_ASSERTE(ghWorkingModule != 0);
	HMODULE hOurModule = (HMODULE)(DWORD_PTR)ghWorkingModule;

	if (!hConWnd)
	{
		if (!hkFunc.isConEmuHk())
		{
			// Must be already called, but JIC
			hkFunc.Init(L"Unknown", hOurModule);
		}

		hConWnd = GetConsoleWindow();
		// Current process may be GUI and have no console at all
		if (!hConWnd)
			return nullptr;

		// RealConsole handle is stored in the Window DATA
		if (!hkFunc.isConEmuHk())
		{
			#ifdef _DEBUG
			wchar_t sClass[64] = L""; GetClassName(hConWnd, sClass, countof(sClass));
			#endif

			// Regardless of GetClassName result, it may be VirtualConsoleClass
			HWND h = (HWND)GetWindowLongPtr(hConWnd, WindowLongDCWnd_ConWnd);
			if (h && IsWindow(h) && IsConsoleWindow(h))
			{
				hConWnd = h;
			}
			else if (IsPseudoConsoleWindow(hConWnd))
			{
				const auto wndFromEnv = GetConEmuWindowsFromEnv();
				if (wndFromEnv.RealConWnd && IsConsoleWindow(wndFromEnv.RealConWnd))
				{
					hConWnd = wndFromEnv.RealConWnd;
				}
			}
		}
	}

	return hConWnd;

#if 0
	// Смысла звать GetProcAddress для "GetConsoleWindow" мало, все равно хукается
	typedef HWND (APIENTRY *FGetConsoleWindow)();
	static FGetConsoleWindow fGetConsoleWindow = nullptr;

	if (!fGetConsoleWindow)
	{
		HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");

		if (hKernel32)
		{
			fGetConsoleWindow = (FGetConsoleWindow)GetProcAddress(hKernel32, "GetConsoleWindow");
		}
	}

	if (fGetConsoleWindow)
		hConWnd = fGetConsoleWindow();

	return hConWnd;
#endif
}

ConEmuWindows GetConEmuWindows(HWND realConsole)
{
	ConEmuWindows result{};
	CESERVER_CONSOLE_MAPPING_HDR* p = nullptr;
	const size_t cchMax = 128;
	wchar_t guiMappingName[cchMax] = L"";

	msprintf(guiMappingName, cchMax, CECONMAPNAME, LODWORD(realConsole));
#ifdef _DEBUG
	size_t nSize = sizeof(*p);
#endif
	auto* hMapping = OpenFileMapping(FILE_MAP_READ, FALSE, guiMappingName);
	if (hMapping)
	{
		const DWORD nFlags = FILE_MAP_READ;
		p = static_cast<CESERVER_CONSOLE_MAPPING_HDR*>(MapViewOfFile(hMapping, nFlags, 0, 0, 0));
	}

	if (p && p->hConEmuRoot && IsWindow(p->hConEmuRoot))
	{
		// Успешно
		result.ConEmuRoot = p->hConEmuRoot;
		result.ConEmuHwnd = p->hConEmuWndDc;
		result.ConEmuBack = p->hConEmuWndBack;
		result.RealConWnd = realConsole;
	}

	if (p)
		UnmapViewOfFile(p);
	if (hMapping)
		CloseHandle(hMapping);

	return result;
}

ConEmuWindows GetConEmuWindowsFromEnv()
{
	ConEmuWindows result{};
	const DWORD envCchMax = 32;
	wchar_t guiPidStr[envCchMax] = L"", srvPidStr[envCchMax] = L"";
	if (GetEnvironmentVariableW(ENV_CONEMUPID_VAR_W, guiPidStr, envCchMax)
		&& GetEnvironmentVariableW(ENV_CONEMUSERVERPID_VAR_W, srvPidStr, envCchMax)
		&& IsStrNotEmpty(guiPidStr) && IsStrNotEmpty(srvPidStr))
	{
		const DWORD guiPID = wcstoul(guiPidStr, nullptr, 10);
		const DWORD srvPID = wcstoul(srvPidStr, nullptr, 10);

		if (!result.ConEmuHwnd)
		{
			ConEmuGuiMapping guiMapping{};
			guiMapping.cbSize = sizeof(guiMapping);
			if (guiPID && LoadGuiMapping(guiPID, guiMapping))
			{
				for (const auto& console : guiMapping.Consoles)
				{
					if (console.ServerPID == srvPID)
					{
						result.RealConWnd = console.Console;
						result.ConEmuHwnd = console.DCWindow;
						result.ConEmuRoot = guiMapping.hGuiWnd;
						break;
					}
				}
			}
		}
	}
	return result;
}

HWND GetConEmuHWND(const ConEmuWndType aiType)
{
	const DWORD nLastErr = GetLastError();
	HWND realConsoleWnd = nullptr;
	ConEmuWindows result{};

	realConsoleWnd = myGetConsoleWindow();
	if (!realConsoleWnd || (aiType == ConEmuWndType::ConsoleWindow))
	{
		goto wrap;
	}

	result = GetConEmuWindows(realConsoleWnd);

wrap:
	SetLastError(nLastErr);

	switch (aiType)
	{
	case ConEmuWndType::GuiBackWindow:
		return result.ConEmuBack;
	case ConEmuWndType::ConsoleWindow:
		return realConsoleWnd;
	case ConEmuWndType::GuiDcWindow:
		return result.ConEmuHwnd;
	case ConEmuWndType::GuiMainWindow:
	default:
		return result.ConEmuRoot;
	}
}


// 0 -- All OK (ConEmu found, Version OK)
// 1 -- NO ConEmu (simple console mode)
// (obsolete) 2 -- ConEmu found, but old version
int ConEmuCheck(HWND* ahConEmuWnd)
{
	//int nChk = -1;
	HWND ConEmuWnd = nullptr;
	ConEmuWnd = GetConEmuHWND(ConEmuWndType::GuiDcWindow);

	// Если хотели узнать хэндл - возвращаем его
	if (ahConEmuWnd) *ahConEmuWnd = ConEmuWnd;

	if (ConEmuWnd == nullptr)
	{
		return 1; // NO ConEmu (simple console mode)
	}
	else
	{
		//if (nChk>=3)
		//    return 2; // ConEmu found, but old version
		return 0;
	}
}

/*HWND AtoH(char *Str, int Len)
{
  DWORD Ret=0;
  for (; Len && *Str; Len--, Str++)
  {
    if (*Str>='0' && *Str<='9')
      Ret = (Ret<<4)+(*Str-'0');
    else if (*Str>='a' && *Str<='f')
      Ret = (Ret<<4)+(*Str-'a'+10);
    else if (*Str>='A' && *Str<='F')
      Ret = (Ret<<4)+(*Str-'A'+10);
  }
  return (HWND)Ret;
}*/

int GuiMessageBox(HWND hConEmuWndRoot, LPCWSTR asText, LPCWSTR asTitle, int anBtns)
{
	int nResult = 0;
	
	if (hConEmuWndRoot)
	{
		HWND hConWnd = myGetConsoleWindow();
		CESERVER_REQ *pIn = (CESERVER_REQ*)malloc(sizeof(*pIn));
		ExecutePrepareCmd(pIn, CECMD_ASSERT, sizeof(CESERVER_REQ_HDR)+sizeof(MyAssertInfo));
		pIn->AssertInfo.nBtns = anBtns;
		_wcscpyn_c(pIn->AssertInfo.szTitle, countof(pIn->AssertInfo.szTitle), asTitle, countof(pIn->AssertInfo.szTitle)); //-V501
		_wcscpyn_c(pIn->AssertInfo.szDebugInfo, countof(pIn->AssertInfo.szDebugInfo), asText, countof(pIn->AssertInfo.szDebugInfo)); //-V501

		wchar_t szGuiPipeName[128];
		msprintf(szGuiPipeName, countof(szGuiPipeName), CEGUIPIPENAME, L".", LODWORD(hConEmuWndRoot));

		CESERVER_REQ* pOut = ExecuteCmd(szGuiPipeName, pIn, 1000, hConWnd);

		free(pIn);

		if (pOut)
		{
			if (pOut->hdr.cbSize > sizeof(CESERVER_REQ_HDR))
			{
				nResult = pOut->dwData[0];
			}
			ExecuteFreeResult(pOut);
		}
	}
	else
	{
		//_ASSERTE(hConEmuWndRoot!=nullptr);
		nResult = MessageBoxW(nullptr, asText, asTitle, MB_SYSTEMMODAL|anBtns);
	}

	return nResult;
}
