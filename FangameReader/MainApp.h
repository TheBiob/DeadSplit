#pragma once

namespace Fangame {

class IRenderer;
class IBroadcaster;
class CWindowSettings;
//////////////////////////////////////////////////////////////////////////

// Start information passed in command line.
struct CStartupInfo : public IStartupInfo {
	CUnicodeString InitialFangameName;
	CUnicodeString FangameUpdateSource;
	bool OpenAppAfterUpdate;
	bool AllowDuplicateProcess;
};

// Command argument name type.
enum TCommandArgumentName {
	CAN_Fangame,
	CAN_UpdateFrom,
	CAN_UpdateOpen,
	CAN_AllowDuplicate,
	CAN_EnumCount
};

//////////////////////////////////////////////////////////////////////////

class CMainApp : public CApplication {
public:
	CMainApp();
	~CMainApp();

	const CWindowSettings& GetWindowSettings() const
		{ return *windowSettings; }

	virtual void OnWindowResize( CGlWindow&, CVector2<int>, bool ) override final;

protected:
	virtual CPtrOwner<IState> onInitialize( CPtrOwner<IStartupInfo> ) override final;
	virtual CPtrOwner<IStartupInfo> createStrartupInfo( CUnicodeView commandLine ) override final;

	virtual void onExit() override final;

private:
	CEventSystem eventSystem;
	CPtrOwner<CWindowSettings> windowSettings;
	CPtrOwner<IRenderer> renderer;
	CPtrOwner<IBroadcaster> broadcaster;
	CPtrOwner<CMutex> uniqueMutex;
	CPtrOwner<CMutexLock> uniqueMutexLock;

	bool initializeUniqueApplication( bool allowDuplicateProcess );
	void initializeRenderer();

	CPtrOwner<CStartupInfo> parseCommandLine( CUnicodeView commandLine );
	int parseSingleArgument( int pos, CUnicodeView commandLine, CStaticArray<CUnicodeString>& argValues );
	int skipWhitespace( int pos, CUnicodeView str );

	void finalizeUpdateInstall( CUnicodeView updateSource, bool openAfter );
	void cleanupUpdater();

	virtual void onInitializeGlContext() override;
	virtual void cleanupGlContext() override;
};

//////////////////////////////////////////////////////////////////////////

}	// namespace Fangame.
