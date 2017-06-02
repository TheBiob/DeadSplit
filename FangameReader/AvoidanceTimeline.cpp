#include <common.h>
#pragma hdrstop

#include <AvoidanceTimeline.h>
#include <BossDeathTable.h>
#include <WindowUtils.h>
#include <FangameDetector.h>
#include <RecordStatusIcon.h>
#include <FangameEvents.h>
#include <ValueGetter.h>
#include <FangameChangeDetector.h>
#include <AddressSearchExpansion.h>
#include <WindowSettings.h>

#include <FangameVisualizerState.h>

namespace Fangame {

//////////////////////////////////////////////////////////////////////////

CAvoidanceTimeline::CAvoidanceTimeline( const CFangameProcessInfo& processInfo, const IValueGetter& _deathDetector, CEventSystem& _events, CFangameVisualizerState& _visualizer ) :
	events( _events ),
	visualizer( _visualizer ),
	windowHandle( processInfo.WndHandle ),
	deathDetector( _deathDetector )
{
	recordIcon = CreateOwner<CRecordStatusIcon>();
	bossChildrenExpansion = CreateOwner<CAddressSearchExpansion>();
}

void CAvoidanceTimeline::EmptyBossData()
{
	deathTable = nullptr;
}

void CAvoidanceTimeline::SetBossData( CBossInfo& newInfo, CBossDeathTable& newTable, bool isCleared )
{
	assert( status != BTS_Recording );
	setRecordStatus( isCleared ? BTS_Paused : BTS_Waiting );
	recordIcon->SetDeathTable( newTable );
	recordIcon->SetStatus( isCleared ? RS_Clear : RS_Pause );
	bossInfo = &newInfo;
	deathTable = &newTable;
	attacksTimeline.Empty();
}

bool CAvoidanceTimeline::IsAttackCurrent( int attackId ) const
{
	assert( attacksTimeline.Size() > attackId );
	return attacksTimeline[attackId].HasStarted && !attacksTimeline[attackId].HasFinished;
}

DWORD CAvoidanceTimeline::GetAttackStartTime( int attackId ) const
{
	assert( attacksTimeline.Size() > attackId );
	return attacksTimeline[attackId].StartTime;
}

void CAvoidanceTimeline::StartBoss()
{
	if( status == BTS_Paused || status == BTS_Recording ) {
		return;
	}
	startBoss( visualizer.GetCurrentFrameTime() );
}

void CAvoidanceTimeline::startBoss( DWORD currentTime )
{
	deathTable->ClearTableColors();
	deathTable->ClearAttackProgress();
	const auto& bossTable = deathTable->GetBossInfo();
	resetIgnoreTime = bossTable.ResetFreezeTime;
	const auto attackCount = bossTable.AttackCount;
	attacksTimeline.Empty();
	attacksTimeline.IncreaseSize( attackCount );
	attackEndExpansions.Empty();
	attackEndExpansions.IncreaseSize( attackCount );
	attackChildrenExpansions.Empty();
	attackChildrenExpansions.IncreaseSize( attackCount );
	bossStartTime = currentTime;
	deathStartCount = getCurrentDeathCount();
	recordIcon->SetStatus( RS_Play );
	setRecordStatus( BTS_Recording );
	auto& changeDetector = visualizer.GetChangeDetector();
	events.Notify( CFangameEvent<Events::CBossStart>( visualizer ) );
	*bossChildrenExpansion = changeDetector.ExpandAddressSearch( bossInfo->ChildrenStartAddressMask, false );
	changeDetector.ResendCurrentAddressChanges();
}

void CAvoidanceTimeline::StartBossAttack( int attackId )
{
	if( status != BTS_Recording ) {
		return;
	}
	auto& attack = FindAttackById( *bossInfo, attackId );
	if( &attack.Parent != &attack.Root && !IsAttackCurrent( attack.Parent.EntryId ) ) {
		return;
	}

	const auto currentTime = visualizer.GetCurrentFrameTime();
	assert( attacksTimeline.Size() > attackId );
	auto& attackTimeInfo = attacksTimeline[attackId];
	const bool shouldRestart = attack.IsRepeatable;
	if( IsAttackCurrent( attackId ) || ( attackTimeInfo.HasStarted && !shouldRestart ) ) {
		return;
	}

	attackTimeInfo.HasStarted = true;
	attackTimeInfo.HasFinished = false;
	attackTimeInfo.StartTime = currentTime;
	deathTable->StartAttack( attackId, currentTime );

	auto& changeDetector = visualizer.GetChangeDetector();
	attackEndExpansions[attackId] = changeDetector.ExpandAddressSearch( attack.EndTriggerAddressMask, false );
	attackChildrenExpansions[attackId] = changeDetector.ExpandAddressSearch( attack.ChildrenStartAddressMask, false );
	events.Notify( CBossAttackStartEvent( visualizer, attack ) );
	changeDetector.ResendCurrentAddressChanges();
}

void CAvoidanceTimeline::setRecordStatus( TBossTimelineStatus newValue )
{
	status = newValue;
	events.Notify( CRecordStatusEvent( visualizer, newValue ) );
}

void CAvoidanceTimeline::FinishBossAttack( int attackId )
{
	if( status != BTS_Recording ) {
		return;
	}

	const auto& attack = FindAttackById( *bossInfo, attackId );
	if( tryFinilizeAttack( attack ) ) {
		events.Notify( CBossAttackEndEvent( visualizer, attack ) );
		if( attack.NotifyAddressChangeOnEnd ) {
			visualizer.GetChangeDetector().ResendCurrentAddressChanges();
		}
	}
}

bool CAvoidanceTimeline::tryFinilizeAttack( const CBossAttackInfo& attack )
{
	const auto attackId = attack.EntryId;
	if( !IsAttackCurrent( attackId ) ) {
		return false;
	}

	assert( attacksTimeline.Size() > attackId );
	auto& targetAttack = attacksTimeline[attackId];
	assert( targetAttack.HasStarted );
	targetAttack.HasFinished = true;
	targetAttack.EndTime = visualizer.GetCurrentFrameTime();
	deathTable->EndAttack( attackId );
	attackEndExpansions[attackId] = CAddressSearchExpansion{};
	attackChildrenExpansions[attackId] = CAddressSearchExpansion{};

	for( const auto& child : attack.Children ) {
		tryFinilizeAttack( child );
	}
	return true;
}

void CAvoidanceTimeline::AbortBossAttack( int attackId )
{
	if( status != BTS_Recording ) {
		return;
	}
	const auto& attack = FindAttackById( *bossInfo, attackId );
	tryAbortAttack( attack );
}

void CAvoidanceTimeline::tryAbortAttack( const CBossAttackInfo& attack )
{
	const auto attackId = attack.EntryId;
	if( !IsAttackCurrent( attackId ) ) {
		return;
	}
	assert( attacksTimeline.Size() > attackId );
	auto& targetAttack = attacksTimeline[attackId];
	targetAttack.HasStarted = false;
	targetAttack.StartTime = 0;
	attackEndExpansions[attackId] = CAddressSearchExpansion{};
	attackChildrenExpansions[attackId] = CAddressSearchExpansion{};
	for( const auto& child : attack.Children ) {
		tryAbortAttack( child );
	}
}

void CAvoidanceTimeline::ClearBoss()
{
	if( status != BTS_Recording ) {
		return;
	}
	clearCurrentBoss();
}

void CAvoidanceTimeline::CheckBossAttacksFinish()
{
	if( status != BTS_Recording ) {
		return;
	}
	for( const auto& attack : attacksTimeline ) {
		if( !attack.HasFinished ) {
			return;
		}
	}

	events.Notify( CFangameEvent<Events::CAllAttacksClear>( visualizer ) );
}

void CAvoidanceTimeline::clearCurrentBoss()
{
	shrinkCurrentAttacks();
	setRecordStatus( BTS_Waiting );
	recordIcon->SetStatus( RS_Clear );
	deathTable->AddTotalPass();
	deathTable->ClearTableColors();
	deathTable->ClearAttackProgress();
	deathTable->ZeroPbMarks();
	attacksTimeline.Empty();
	events.Notify( CBossEndEvent( visualizer, *bossInfo ) );
}

void CAvoidanceTimeline::shrinkCurrentAttacks()
{
	assert( status == BTS_Recording );
	attackEndExpansions.Empty();
	attackChildrenExpansions.Empty();
}

CAvoidanceTimeline::~CAvoidanceTimeline()
{

}

const float babyRagePeriod = 1.0f;
void CAvoidanceTimeline::OnGameRestart()
{
	if( status == BTS_Paused || recordIcon->GetStatus() == RS_Clear ) {
		return;
	}

	const auto currentTime = visualizer.GetCurrentFrameTime();
	if( status == BTS_Recording ) {
		const auto timePassed = getTimeDelta( currentTime, bossStartTime );
		if( timePassed <= resetIgnoreTime ) {
			return;
		} else if( visualizer.GetWindowSettings().TreatRestartAsDeath() ) {
			signalHeroDeath( timePassed );
		} else {
			shrinkCurrentAttacks();
			deathTable->ClearTableColors();
		}
		recordIcon->SetStatus( RS_Restarted );
	}

	setRecordStatus( BTS_Waiting );
	if( deathTable != nullptr ) {
		deathTable->FadeFrozenAttacks( currentTime );
	}
	const auto timeSinceRestart = getTimeDelta( currentTime, lastRestartTime );
	lastRestartTime = currentTime;
	if( timeSinceRestart <= babyRagePeriod ) {
		recordIcon->AddBabyRage();
	}

	events.Notify( CFangameEvent<Events::CGameRestart>( visualizer ) );
}

void CAvoidanceTimeline::UndoRecording()
{
	if( status != BTS_Recording ) {
		return;
	}
	
	deathTable->ClearAttackProgress();
	deathTable->ClearTableColors();
	shrinkCurrentAttacks();
	setRecordStatus( BTS_Waiting );
	recordIcon->SetStatus( RS_Pause );
}

void CAvoidanceTimeline::PauseRecording( bool isSet )
{
	if( deathTable != nullptr ) {
		deathTable->ClearAttackProgress();
		deathTable->ClearTableColors();
	}
	if( status == BTS_Recording ) {
		shrinkCurrentAttacks();
	}
	setRecordStatus( isSet ? BTS_Paused : BTS_Waiting );
	recordIcon->SetStatus( isSet ? RS_Stop : RS_Pause );
}

void CAvoidanceTimeline::TogglePauseRecording()
{
	PauseRecording( status != BTS_Paused );
}

void CAvoidanceTimeline::UpdateStatus( DWORD currentTicks )
{
	events.Notify( CUpdateEvent( visualizer, currentTicks ) );

	if( status != BTS_Recording ) {
		return;
	}

	const auto timePasseed = getTimeDelta( currentTicks, bossStartTime );
	if( timePasseed > babyRagePeriod ) {
		recordIcon->EmptyBabyRage();
	}

	const int currentDeaths = getCurrentDeathCount();
	if( currentDeaths == NotFound ) {
		// Sometimes the title doesn't contain deaths, it's probably being updated.
		return;
	}

	if( deathStartCount == NotFound ) {
		deathStartCount = currentDeaths;
		return;
	}

	if( currentDeaths == deathStartCount ) {
		return;
	}

	// Super early deaths are most likely a result of the death count update lag, ignore them.
	const float deathLagPeriod = 0.25f;
	if( timePasseed > deathLagPeriod ) {
		signalHeroDeath( timePasseed );
		setRecordStatus( BTS_Dead );
		recordIcon->SetStatus( RS_Death );
	} else {
		deathStartCount = currentDeaths;
	}
}

void CAvoidanceTimeline::Draw( const IRenderParameters& renderParams ) const
{
	assert( HasActiveData() );

	recordIcon->Draw( renderParams );
}

int CAvoidanceTimeline::getCurrentDeathCount() const
{
	return deathDetector.GetValueAs<int>();
}

void CAvoidanceTimeline::signalHeroDeath( float secondDelta )
{
	assert( status == BTS_Recording );
	assert( deathTable != nullptr );
	if( secondDelta < babyRagePeriod ) {
		return;
	}

	shrinkCurrentAttacks();

	bool activeAttackFound = false;
	for( int i = 0; i < attacksTimeline.Size(); i++ ) {
		const auto attack = attacksTimeline[i];
		if( attack.HasStarted && !attack.HasFinished ) {
			activeAttackFound = true;
			deathTable->AddAttackDeath( i );
		}
	}

	if( activeAttackFound ) {
		deathTable->AddTotalDeath();
	}
}

float CAvoidanceTimeline::getTimeDelta( DWORD current, DWORD prev ) const
{
	return ( current - prev ) / 1000.0f;
}

//////////////////////////////////////////////////////////////////////////

}	// namespace Fangame.