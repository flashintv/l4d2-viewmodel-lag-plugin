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
#include "serverplugin_empty.h"
#include "clientplugin/clientplugin_viewmodel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Interfaces from the engine
IVEngineServer	*engine = NULL;
IGameEventManager *gameeventmanager = NULL;
IPlayerInfoManager *playerinfomanager = NULL;
IUniformRandomStream *randomStr = NULL;

//---------------------------------------------------------------------------------
// Purpose: a sample 3rd party plugin class
//---------------------------------------------------------------------------------

CViewmodelPlugin g_ViewmodelPlugin;
CViewmodelPlugin* g_pViewmodelPlugin = &g_ViewmodelPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CViewmodelPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS_VERSION_2, g_ViewmodelPlugin); // use version 2 to support l4d1

// ------------------------------------------
// Actual function code vvvvvvvvvvvvvvvvvvvv
// ------------------------------------------

bool CViewmodelPlugin::Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
	ConnectTier1Libraries(&interfaceFactory, 1);
	ConnectTier2Libraries(&interfaceFactory, 1);

	playerinfomanager = (IPlayerInfoManager *)gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER, NULL);
	if ( !playerinfomanager )
	{
		Warning( "Unable to load playerinfomanager!\n" ); // this is fatal, because we need global vars!
		return false;
	}

	engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
	gameeventmanager = (IGameEventManager *)interfaceFactory(INTERFACEVERSION_GAMEEVENTSMANAGER,NULL);
	randomStr = (IUniformRandomStream *)interfaceFactory(VENGINE_SERVER_RANDOM_INTERFACE_VERSION, NULL);

	// get the interfaces we want to use
	if(	! ( engine && gameeventmanager && randomStr ) )
	{
		Warning("Interface list: {%p, %p, %p} - one of them failed!\n", engine , gameeventmanager, randomStr);
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
	gameeventmanager->RemoveListener( this ); // make sure we are unloaded from the event system

	ConVar_Unregister();
	DisconnectTier2Libraries();
	DisconnectTier1Libraries();
	ClientPlugin_Viewmodel::Viewmodel_Stop();
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