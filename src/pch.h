#ifndef PCH_H
#define PCH_H
#pragma once

/*
    Only put files that never change or are rarely changed in this file.
    An include graph will be provided for some headers that were not explicit
    added. VC++ is a requirement.

    We don't care about warnings for library files.
*/
#pragma warning(push)
#pragma warning(disable : 4091 4191 4365 4464 4571 4619 4623 4625 4626 4768 4774 5026 5027 5039 5045)
/* Windows Stuff */
#include "windows_nodefines.h"
#include <atlbase.h> // atldef.h (Windows.h), atlcore.h (tchar.h), Shlwapi.h
#include <direct.h>
#include <io.h>
#include <ShlObj.h> // CommCtrl
#include <wincodec.h>
#include <WindowsX.h>
#include <winhttp.h>

#if WIN32_LEAN_AND_MEAN
#include <CommDlg.h>
#include <MMSystem.h>
#include <ShellAPI.h>
#endif // WIN32_LEAN_AND_MEAN

/*
    Nuke WinAPI #defines - anything including this file should be using the
    ra versions.
*/
#undef CreateDirectory
#undef GetMessage

/* C Stuff */
#include <cctype>
#undef BUFSIZ // make it constexpr in RA_Defs

/* STL Stuff */
#include <array> // algorithm, iterator, tuple
#include <atomic>
#include <fstream>
#include <iomanip>
#include <map>
#include <memory>
#include <mutex> // chrono (time.h), functional, thread, utility
#include <queue> // deque, vector, algorithm
#include <set>
#include <sstream> // string
#include <stack>
#include <unordered_map>

#pragma warning(push)
// Look at this file if you want to know what they are and what they mean
#include "pch_cppcorecheck_suppressions.h"
#include "pch_microsoft_suppressions.h"

/* RapidJSON Stuff */
#define RAPIDJSON_HAS_STDSTRING 1
#define RAPIDJSON_NOMEMBERITERATORCLASS 1
#include <rapidjson\document.h> // has reader.h
#include <rapidjson\writer.h> // has stringbuffer.h
#include <rapidjson\istreamwrapper.h>
#include <rapidjson\ostreamwrapper.h>
#include <rapidjson\error\en.h>  
#pragma warning(pop)

#include <md5.h>

#if RA_UTEST
#include <CodeAnalysis\Warnings.h>
#pragma warning(push)
#pragma warning(disable : ALL_CPPCORECHECK_WARNINGS)
#include "CppUnitTest.h"
#pragma warning(pop)
#endif /* RA_UTEST */


#pragma warning(pop)

#endif /* !PCH_H */