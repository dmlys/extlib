#ifndef UNICODE
#define UNICODE
#endif // !UNICODE

#ifndef _UNICODE
#define _UNICODE
#endif // !_UNICODE

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600          // Windows Vista
#endif // !_WIN32_WINNT

#include <sdkddkver.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
