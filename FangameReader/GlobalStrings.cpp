#include <common.h>
#pragma hdrstop

#include <GlobalStrings.h>

namespace Fangame {

//////////////////////////////////////////////////////////////////////////

namespace Paths {

extern const CUnicodeView IconDir = L"Icons";
extern const CUnicodeView UserSettingsFile = L"userSettings.ini";
extern const CUnicodeView FanagameSaveFile = L"saveData.sav";
extern const CUnicodeView FangameAliasesFile = L"attackAliases.xml";
extern const CUnicodeView FangameLayoutFile = L"layout.xml";
extern const CUnicodeView FangameInfoFolder = L"FangameInfo";
extern const CUnicodeView BaseConnectionFile = L"baseConnections.xml";
extern const CUnicodeView UserConnectionFile = L"userConnections.xml";
extern const CUnicodeView SessionFileName = L"CurrentSession";
extern const CUnicodeView UpdateTempFolder = L"Update";
extern const CUnicodeView UpdatedExeName = L"Update\\DeadSplit.exe";

}

namespace UI {

extern const CUnicodeView InitializeSessionQuestion = L"Initialize a new session for all fangames, are you sure?";
extern const CUnicodeView ClearSessionQuestion = L"Clear the session column for all tables, are you sure?";
extern const CUnicodeView ClearTableQuestion = L"Clear the entire death count for the current table, are you sure?";

}

//////////////////////////////////////////////////////////////////////////

}	// namespace Fangame.
