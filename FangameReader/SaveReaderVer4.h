#pragma once

namespace Fangame {

struct CBossSaveData;
//////////////////////////////////////////////////////////////////////////

class CSaveReaderVer4 {
public:
	CMap<CUnicodeString, CBossSaveData> SerializeData( CArchiveReader& src );
};

//////////////////////////////////////////////////////////////////////////

}	// namespace Fangame.

