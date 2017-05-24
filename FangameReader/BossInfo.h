#pragma once
#include <ProgressReporter.h>
#include <GlSizedTexture.h>

namespace Fangame {

struct CBossInfo;
//////////////////////////////////////////////////////////////////////////

enum TEntryChildOrder {
	ECD_Sequential,
	ECD_Random,
	ECD_EnumCount
};

struct CEntryStats {
	int DeathCount = 0;
	int PassCount = 0;
	int CurrentStreak = 0;
	int MaxStreak = 0;
	float Time = 0.0f;

	CEntryStats() = default;
};

struct CEntryInfo {
	CXmlElement& SrcElement;
	int EntryId = 0;
	CUnicodeString VisualName;
	CEntryStats SessionStats;
	CEntryStats TotalStats;
	CStaticArray<CBossAttackInfo> Children;
	CDynamicBitSet<> ChildrenStartAddressMask;
	TEntryChildOrder ChildOrder = ECD_Sequential;
	CBossInfo& Root;
	bool IsConsistentWithChildren = true;

	explicit CEntryInfo( CXmlElement& srcElem, CBossInfo& root ) : SrcElement( srcElem ), Root( root ) {}
	CEntryInfo( CXmlElement& srcElem, CBossInfo& root, int entryId, CUnicodeString visualName ) : SrcElement( srcElem ), Root( root ), EntryId( entryId ), VisualName( move( visualName ) ) {}
	~CEntryInfo();
};

struct CBossAttackInfo : public CEntryInfo {
	int ChildId = NotFound;
	CEntryInfo& Parent;
	CUnicodeString KeyName;
	const IImageRenderData* Icon = nullptr;
	CDynamicBitSet<> EndTriggerAddressMask;
	CPtrOwner<IProgressReporter> Progress;
	CColor CurrentRectTopColor;
	CColor CurrentRectBottomColor;
	double TotalPB = 2.0;
	double SessionPB = 2.0;
	bool IsRepeatable = false;
	bool NotifyAddressChangeOnEnd = false;

	CBossAttackInfo( CXmlElement& srcElem, CBossInfo& root, CEntryInfo& parent, int attackId, CUnicodeString keyName, CUnicodeString visualName, 
			const IImageRenderData& icon, double totalPB, double sessionPB ) : 
		CEntryInfo{ srcElem, root, attackId, move( visualName ) }, Parent( parent ), KeyName( move( keyName ) ), Icon( &icon ), TotalPB( totalPB ), SessionPB( sessionPB ) {}
};

struct CBossEventData {
	int EventClassId;
	CTypelessActionOwner EventAction;

	CBossEventData() = default;
	CBossEventData( int classId, CTypelessActionOwner eventAction ) : EventClassId( classId ), EventAction( move( eventAction ) ) {}
};

struct CBossInfo : public CEntryInfo {
	int AttackCount = 0;
	double ResetFreezeTime = 0.0;
	CArray<CBossEventData> BossEvents;
	CDynamicBitSet<> AddressMask;
	const IFontRenderData& BossFont;
	bool SessionClearFlag = false;

	explicit CBossInfo( CXmlElement& srcElem, const IFontRenderData& font ) : CEntryInfo( srcElem, *this ), BossFont( font ) {}
};

void MakeParentsConsistentWithChildren( CBossAttackInfo& entry );
void MakeConsistentWithChildData( CBossInfo& entry );
const CBossAttackInfo& FindAttackById( const CBossInfo& boss, int id );
CBossAttackInfo& FindAttackById( CBossInfo& boss, int id );

//////////////////////////////////////////////////////////////////////////

}	// namespace Fangame.

