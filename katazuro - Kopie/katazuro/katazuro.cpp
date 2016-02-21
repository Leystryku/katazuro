
#include "includes.h"

using namespace std;


//ILuaInterface::RunStringEx

//int __stdcall sub_1000A2E0(int, char, void *, char, char, char)

bool bInjectedLua = false;

void *lua_State_menu = NULL;
void *lua_State_client = NULL;

JMPHook *RunStringEx_jmp = NULL;



bool DataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask)
{
	for(;*szMask;++szMask,++pData,++bMask)
		if(*szMask=='x' && *pData!=*bMask )
			return false;

	return (*szMask) == NULL;
}

DWORD FindPattern( DWORD dwAddress, const char *c_bMask, char * szMask )
{
	BYTE *bMask = (PBYTE)c_bMask;
	DWORD dwMask = (DWORD)bMask;
	DWORD dwRet = 0;

	if ( !dwMask )
		return 0;

	if ( !bMask )
		return 0;

	DWORD dwSize = (DWORD)(dwAddress - dwMask);

	if ( !dwSize )
		return 0;

	__try {

		for(DWORD i=0; i < dwSize; i++)
		{
			if( DataCompare( (BYTE*)( dwAddress+i ),bMask,szMask) )
			{
				dwRet = (DWORD)(dwAddress+i);
				break;
			}
		}
	
	}

    __except(EXCEPTION_EXECUTE_HANDLER)
    {
		return 0;
    }

	return dwRet;
}

FILE* g_file = NULL;

const char* get_file_contents( const char *filename )
{
	g_file = fopen(filename, "r");

	if ( g_file == 0 )
		return 0;

   // Determine file size
   fseek(g_file, 0, SEEK_END);
   size_t size = ftell(g_file);

   char* cRet = new char[size];

   rewind(g_file);
   size_t len = fread(cRet, sizeof(char), size, g_file);

   cRet[len] = '\0';


   return cRet;
}


bool __fastcall RunStringEx_hooker( void* __this, int edx, const char *filename, const char *path, const char *stringToRun, bool run, bool showErrors, bool bunknown )
{

	

	if ( __this == NULL )
		MessageBox( NULL, "lua_State is NULL for some reason, game is probably going to crash!", "RunStringEx_hooker::Warning", MB_OK );

	if ( __this && !lua_State_menu )
		lua_State_menu = __this;

	if ( lua_State_menu != __this )
		lua_State_client = __this;

	if ( __this == lua_State_client )
	{

		if ( !bInjectedLua && strstr(filename, "lua/includes/init.lua") )
		{
			const char *luaToInject = get_file_contents("C://bananacocks.lua"); // intellisense is a gay thing
			
			if ( g_file )
			{
				fclose( g_file );
				g_file = NULL;
			}
			

			if ( luaToInject )
			{
				RunStringEx_jmp->SetECX( __this );
				RunStringEx_jmp->Call( 6, filename, path, luaToInject, run, showErrors, bunknown );
				//MessageBox( NULL, luaToInject, "injected lua!", MB_OK );
			}else{
				MessageBox( NULL, filename, "couldn't read bananacocks.lua :(", MB_OK );
			}

		}else{

			if ( bInjectedLua )
				bInjectedLua = false;
		}

		
	}

	RunStringEx_jmp->SetECX( __this );

	return (bool)RunStringEx_jmp->Call( 6, filename, path, stringToRun, run, showErrors, bunknown );

}


bool bSuppressDLLMain = false;

int InitThread( )
{

	//MessageBox( NULL, "initial thread started!", "k", MB_OK );

	HMODULE hLuaShared = GetModuleHandle("lua_shared.dll");

	while( !hLuaShared )
	{
		hLuaShared = GetModuleHandle("lua_shared.dll");

		if ( hLuaShared )
			break;

		Sleep(100);
	}

	DWORD dwLuaShared = (DWORD)hLuaShared;

	DWORD address = FindPattern( dwLuaShared, "\x55\x8B\xEC\x8B\x55\x10\x81\xEC", "xxxxxxxx" );

	if ( !address )
		MessageBox( NULL, "wrong address :(", "k", MB_OK );

	RunStringEx_jmp = new JMPHook( (void*)address, RunStringEx_hooker );

	if ( !RunStringEx_jmp )
		MessageBox( NULL, "hook failed", "k", MB_OK );

	const char *luaToInject = get_file_contents("C://bananacocks.lua");

	if ( g_file )
	{
		fclose( g_file );
		g_file = NULL;
	}

	if ( !luaToInject )
		MessageBox( NULL, "didn't find lua to inject:(", "k", MB_OK );


	return 0;

}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{


	if ( !bSuppressDLLMain && ul_reason_for_call == DLL_PROCESS_ATTACH )
	{
		bSuppressDLLMain = true;

		CreateThread( NULL, NULL, (LPTHREAD_START_ROUTINE)InitThread, NULL, NULL, NULL );
	}

	return TRUE;
}

