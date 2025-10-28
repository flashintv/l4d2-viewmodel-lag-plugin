//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include <stdio.h>

#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "game/server/iplayerinfo.h"
#include "icliententity.h"
#include "eiface.h"
#include "igameevents.h"
#include "convar.h"
#include "Color.h"
#include "vstdlib/random.h"
#include "engine/IEngineTrace.h"
#include "tier2/tier2.h"
#include "utils.h"
#include "clientplugin/clientplugin_viewmodel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Interfaces from the engine
IVEngineServer	*engine = NULL; // helper functions (messaging clients, loading content, making entities, running commands, etc)
IGameEventManager *gameeventmanager = NULL; // game events interface
IPlayerInfoManager *playerinfomanager = NULL; // game dll interface to interact with players
IBotManager *botmanager = NULL; // game dll interface to interact with bots
IServerPluginHelpers *helpers = NULL; // special 3rd party plugin helpers from the engine
IUniformRandomStream *randomStr = NULL;
IEngineTrace *enginetrace = NULL;

//---------------------------------------------------------------------------------
// Purpose: a sample 3rd party plugin class
//---------------------------------------------------------------------------------
class CViewmodelPlugin : public IServerPluginCallbacks, public IGameEventListener
{
public:
	CViewmodelPlugin() {};
	~CViewmodelPlugin() {};

	// IServerPluginCallbacks methods
	virtual bool			Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory );
	virtual void			Unload( void );
	virtual void			Pause( void ) {};
	virtual void			UnPause( void ) {};
	virtual const char     *GetPluginDescription( void );      
	virtual void			LevelInit( char const *pMapName );
	virtual void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax ) {};
	virtual void			GameFrame( bool simulating ) {};
	virtual void			LevelShutdown( void );
	virtual void			ClientActive( edict_t *pEntity ) {};
	virtual void			ClientDisconnect( edict_t *pEntity ) {};
	virtual void			ClientPutInServer( edict_t *pEntity, char const *playername ) {};
	virtual void			SetCommandClient(int index) {};
	virtual void			ClientSettingsChanged( edict_t *pEdict ) {};
	virtual PLUGIN_RESULT	ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen ) { return PLUGIN_CONTINUE; };
	virtual PLUGIN_RESULT	ClientCommand( edict_t *pEntity, const CCommand &args ) { return PLUGIN_CONTINUE; };
	virtual PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName, const char *pszNetworkID ) { return PLUGIN_CONTINUE; };
	virtual void			OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue ) {};
	virtual void			FireGameEvent(KeyValues* event) {};
};
CViewmodelPlugin g_ViewmodelPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CViewmodelPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS_VERSION_2, g_ViewmodelPlugin); // use version 2 to support l4d1

// ------------------------------------------
// Actual function code vvvvvvvvvvvvvvvvvvvv
// ------------------------------------------

bool CViewmodelPlugin::Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
	ConnectTier1Libraries( &interfaceFactory, 1 );
	ConnectTier2Libraries( &interfaceFactory, 1 );

	playerinfomanager = (IPlayerInfoManager *)gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER, NULL);
	if ( !playerinfomanager )
	{
		Warning( "Unable to load playerinfomanager, ignoring\n" ); // this isn't fatal, we just won't be able to access specific player data
	}

	botmanager = (IBotManager *)gameServerFactory(INTERFACEVERSION_PLAYERBOTMANAGER, NULL);
	if ( !botmanager )
	{
		Warning( "Unable to load botcontroller, ignoring\n" ); // this isn't fatal, we just won't be able to access specific bot functions
	}

	engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
	gameeventmanager = (IGameEventManager *)interfaceFactory(INTERFACEVERSION_GAMEEVENTSMANAGER,NULL);
	helpers = (IServerPluginHelpers*)interfaceFactory(INTERFACEVERSION_ISERVERPLUGINHELPERS, NULL);
	enginetrace = (IEngineTrace *)interfaceFactory(INTERFACEVERSION_ENGINETRACE_SERVER,NULL);
	randomStr = (IUniformRandomStream *)interfaceFactory(VENGINE_SERVER_RANDOM_INTERFACE_VERSION, NULL);

	// get the interfaces we want to use (don't check g_pFullFileSystem because this is a partial project)
	if(	! ( engine && gameeventmanager && helpers && enginetrace && randomStr ) )
	{
		return false; // we require all these interface to function
	}

	if ( playerinfomanager )
	{
		gpGlobals = playerinfomanager->GetGlobalVars();
	}

	MathLib_Init(2.2f, 2.2f, 0.0f, 2);
	ConVar_Register(0);
	if (!ClientPlugin_Viewmodel::Viewmodel_Run(interfaceFactory)) {
		return false;
	}

	return true;
}

void CViewmodelPlugin::Unload( void )
{
	ClientPlugin_Viewmodel::Viewmodel_Stop();
	gameeventmanager->RemoveListener( this ); // make sure we are unloaded from the event system

	ConVar_Unregister();
	DisconnectTier2Libraries( );
	DisconnectTier1Libraries( );
}

const char *CViewmodelPlugin::GetPluginDescription( void )
{
	return "HL2 viewmodel restoration for L4D 1 and 2, Grizzle";
}

void CViewmodelPlugin::LevelInit( char const *pMapName )
{
	gameeventmanager->AddListener( this, true );
}

void CViewmodelPlugin::LevelShutdown( void ) // !!!!this can get called multiple times per map change
{
	gameeventmanager->RemoveListener( this );
}