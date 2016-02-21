
#include "includes.h"

using namespace std;
void(*Msg)(const char *, ...);

//ILuaInterface::RunStringEx

//int __stdcall sub_1000A2E0(int, char, void *, char, char, char)


typedef void*(__cdecl *tRunStringEx)(void* state, const char* buff, size_t sz, const char* name, int shit);
//typedef bool(__fastcall* tRunStringEx)(void*__this, int __edx, const char *filename, const char *path, const char *stringToRun, bool run, bool showErrors, bool bunknown);
tRunStringEx RunStringEx;

typedef void*(__cdecl *tluaL_loadbufferx)(void* state, const char* buff, size_t sz, const char* name, int shit);
tluaL_loadbufferx luaL_loadbufferx;

char kamifolder[MAX_PATH];

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



FILE *fhandle;

void*fbuf = 0;
char* get_file_contents(const char *filename)
{

	char realfile[512];
	strcat(realfile, kamifolder);
	strcat(realfile, filename);

	fhandle = fopen(realfile, "r");

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

#define LUA_PATH "preautorun.lua"
#define SETTINGS_PATH "settings.txt"

bool rundef = true;
bool printfiles = false;
bool printcode = false;
bool printrs = false;

const char*defcode = "local h = pairs local z = RunString local e = table.concat local d = nil local function r(p,c,a,s) z(s) end local function rr(p,c,a,s) z(file.Read('lua/' .. s, 'GAME')) end function pairs(...) if(search and !d ) then d = true concommand.Add('lua_rrun', r) concommand.Add('lua_ropen', rr) pairs = h end return h(...) end";

/*
bool __fastcall RunStringEx_hook(void*__this, int __edx, const char *filename, const char *path, const char *stringToRun, bool run, bool showErrors, bool bunknown)
{
	closefile();

	if (!filename)
		return true;//RunStringEx(__this, __edx, filename, path, stringToRun, run, showErrors, bunknown);

	if (printfiles && strcmp(filename, "LuaCmd") && strcmp(filename, "RunString"))
		Msg("[kami_lua] Running: %s\n", filename);

	if (stringToRun)
	{
		if (printcode && strcmp(filename, "LuaCmd") && strcmp(filename, "RunString"))
			Msg("[kami_lua] Code: %s\n", stringToRun);

		if (printrs && (!strcmp(filename, "LuaCmd") || !strcmp(filename, "RunString")))
			Msg("[kami_lua] RS Code: %s\n", stringToRun);
	}

	if (strstr(filename, "lua/includes/init.lua"))
	{
		const char *luaToInject = get_file_contents(LUA_PATH);

		if (rundef)
		{
			Msg("[kami_lua] Ran default code!\n");
			RunStringEx(__this, __edx, filename, path, defcode, run, showErrors, bunknown);
		}


		Msg("[kami_lua] Looking for pre-autorun lua at %s\n", LUA_PATH);

		if (luaToInject)
		{
			Msg("[kami_lua] Ran big undetectable luas!\n");
			RunStringEx(__this, __edx, filename, path, luaToInject, run, showErrors, bunknown);
		}
		else{
			Msg("[kami_lua] Couldn't find any pre-autorun lua!\n");
		}
	}


	return RunStringEx(__this, __edx, filename, path, stringToRun, run, showErrors, bunknown);


}*/

void* __cdecl RunStringEx_hook(void* state, const char* buff, size_t sz, const char* name)
{
	Msg("LOL: %s\n", name);

	return luaL_loadbufferx(state, buff, sz, name, 0);
}

bool bSuppressDLLMain = false;

int InitThread()
{
	void* hLuaShared = 0;

	while (!hLuaShared)
	{
		hLuaShared = GetModuleHandleA("lua_shared.dll");

		if (hLuaShared)
			break;

		Sleep(300);
	}

	Msg = (void(*)(const char *, ...))GetProcAddress(GetModuleHandleA("tier0.dll"), "Msg");
	if (!Msg)
		return 0;

	void* address = GetProcAddress((HMODULE)hLuaShared, "luaL_loadbuffer");

	if (!address)
	{
		Msg("[kami_lua] no runstring found!\n");
		return 0;
	}


	closefile();

	RunStringEx = (tRunStringEx)DetourCreate(address, RunStringEx_hook, 6);
	luaL_loadbufferx = (tluaL_loadbufferx)GetProcAddress((HMODULE)hLuaShared, "luaL_loadbufferx");
	Msg("ADDR: %x\n", RunStringEx);

	if (!RunStringEx)
	{
		Msg("[kami_lua] couldnt hook runstring!\n");
		return 0;
	}

	
	BOOL success = SHGetSpecialFolderPathA(NULL, kamifolder, CSIDL_MYDOCUMENTS, false);
	if (!success)
	{
		Msg("[kami_lua] couldnt find your documents folder!\n");
		return 0;
	}
	strcat(kamifolder, "\\kami_lua");

	CreateDirectoryA(kamifolder, NULL);

	strcat(kamifolder, "\\");
	
	Msg("[kami_lua] Folder found: %s\n", kamifolder);

	const char *yourmomtits_settings = get_file_contents(SETTINGS_PATH);

	if (yourmomtits_settings&&strnlen_s(yourmomtits_settings, 300) < 150)
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
		Msg("[kami_lua] Normal mode\nYou can always run lua with 'lua_rrun' ( works like lua_run ) or 'lua_ropen' ( works like lua_openscript ) in console(ingame)!\n");
	}else{
		Msg("[kami_lua] Running in safe mode, to avoid detections from the concommand.\n");
	}

	if (printfiles)
		Msg("[kami_lua] Going to print lua files!\n");

	if (printcode)
		Msg("[kami_lua] Going to print code!\n");

	if (printrs)
		Msg("[kami_lua] Going to print runstrings!\n");

	Msg("[kami_lua] Made by Kami - copyright gmodcheats.net 2015\n");
	Msg("[kami_lua] If you need any help post in the thread.\n");
	Msg("[kami_lua] Pre-autorun lua can be put in %s%s\n", kamifolder, LUA_PATH);
	Msg("[kami_lua] The settings can be changed at %s%s\n", kamifolder, SETTINGS_PATH);
	Msg("[kami_lua] http://www.gmodcheats.net/showthread.php?tid=2&pid=2#pid2 \n");


	ShellExecuteA(NULL, "open", "http://www.gmodcheats.net/showthread.php?tid=2&pid=2#pid2", NULL, NULL, SW_SHOWNORMAL);

	return 0;

}

int __stdcall DllMain(void* hModule, int ul_reason_for_call, void* lpReserved)
{


	if (!bSuppressDLLMain && ul_reason_for_call == 1)
	{
		bSuppressDLLMain = true;

		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)InitThread, NULL, NULL, NULL);
	}

	return 1;
}

