//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Basic BOT handling.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#include "interface.h"
#include "filesystem.h"
#undef VECTOR_NO_SLOW_OPERATIONS
#include "mathlib/vector.h"

#include "eiface.h"
#include "edict.h"
#include "game/server/iplayerinfo.h"
#include "igameevents.h"
#include "vstdlib/random.h"
#include "../../game/shared/in_buttons.h"
#include "../../game/shared/shareddefs.h"
#include "../utils.h"
#include "../sigscan.h"
#include "icliententity.h"
#include "../minhook/minhook.h"
#include "cdll_int.h"
#include "utlvector.h"
#include "terrorviewmodel.h"
#include "clientplugin_viewmodel.h"
#include "../serverplugin_empty.h"

#define USE_REALTIME
#define USE_LASTTIMESTAMP
#include "interpolatedvar.h"
// -------------------------------------------------------------------------
// Variables so that CInterpolatedVar can work
bool CInterpolationContext::s_bAllowExtrapolation = false;
float CInterpolationContext::s_flLastTimeStamp = 0.f;
ConVar cl_extrapolate_amount = ConVar("", "", FCVAR_HIDDEN);
// -------------------------------------------------------------------------

ESourceEngine eEngine = k_eOther;

CServerPlugin_t* serverplugins = NULL;
CGlobalVars* gpGlobals = NULL;
IVEngineClient* engineclient = NULL;
ICvar* pcvar = NULL;

TerrorViewModel tvm;
TerrorViewModel* gpTerrorViewModel = &tvm;

bool ClientPlugin_Viewmodel::Viewmodel_Run(CreateInterfaceFn interfaceFactory)
{
	if (!CSigScan::SetDllMemInfo("client.dll")) {
		Warning("Failed to set client.dll memory info for signature scan!\n");
		return false;
	}

	static char modDir[MAX_PATH];
	if (Q_strlen(modDir) == 0)
	{
		const char* gamedir = CommandLine()->ParmValue("-game", CommandLine()->ParmValue("-defaultgamedir", "hl2"));
		Q_strncpy(modDir, gamedir, sizeof(modDir));
		if (strchr(modDir, '/') || strchr(modDir, '\\'))
		{
			Q_StripLastDir(modDir, sizeof(modDir));
			int dirlen = Q_strlen(modDir);
			Q_strncpy(modDir, gamedir + dirlen, sizeof(modDir) - dirlen);
		}
	}

	if (V_strcmp(modDir, "left4dead2") == 0) {
		eEngine = k_eL4D2;
		DEBUG_Msg("This is the L4D2 engine!\n");
	} else if (V_strcmp(modDir, "left4dead") == 0) {
		eEngine = k_eL4D1;
		DEBUG_Msg("This is the L4D1 engine!\n");
	} else {
		Warning("Plugin running on an engine different from Left 4 Dead series.\n");
		return false;
	}

	serverplugins = (CServerPlugin_t*)interfaceFactory(INTERFACEVERSION_ISERVERPLUGINHELPERS, NULL);
	engineclient = (IVEngineClient*)interfaceFactory(VENGINE_CLIENT_INTERFACE_VERSION, NULL);
	pcvar = (ICvar*)interfaceFactory(CVAR_INTERFACE_VERSION, NULL);

	CSigScan gpGlobals_Sig;
	if (eEngine == k_eL4D2) {
		tvm.plugin_wpn_sway_cvar = new ConVar("pl_wpn_sway_enabled", "1", FCVAR_CLIENTDLL, "Restores HL2 sway.");
		tvm.plugin_wpn_sway_scale = new ConVar("pl_wpn_sway_scale", "1.5", FCVAR_CLIENTDLL);
		tvm.plugin_wpn_sway_interp = new ConVar("pl_wpn_sway_interp", "0.1", FCVAR_CLIENTDLL);

		tvm.plugin_viewmodel_offset_x = cvar->FindVar("viewmodel_offset_x");
		tvm.plugin_viewmodel_offset_y = cvar->FindVar("viewmodel_offset_y");
		tvm.plugin_viewmodel_offset_z = cvar->FindVar("viewmodel_offset_z");
	} 
	else {
		tvm.plugin_wpn_sway_cvar = (ConVar*)new ConVar_L4D("pl_wpn_sway_enabled", "1", FCVAR_CLIENTDLL, "Restores HL2 sway.");
		tvm.plugin_wpn_sway_scale = (ConVar*)new ConVar_L4D("pl_wpn_sway_scale", "1.5", FCVAR_CLIENTDLL);
		tvm.plugin_wpn_sway_interp = (ConVar*)new ConVar_L4D("pl_wpn_sway_interp", "0.1", FCVAR_CLIENTDLL);

		tvm.plugin_viewmodel_offset_x = (ConVar*)new ConVar_L4D("viewmodel_offset_x", "0.0", FCVAR_CLIENTDLL);
		tvm.plugin_viewmodel_offset_y = (ConVar*)new ConVar_L4D("viewmodel_offset_y", "0.0", FCVAR_CLIENTDLL);
		tvm.plugin_viewmodel_offset_z = (ConVar*)new ConVar_L4D("viewmodel_offset_z", "0.0", FCVAR_CLIENTDLL);
	}

	return tvm.Setup_TerrorViewModel();
}

void ClientPlugin_Viewmodel::Viewmodel_Stop()
{
	delete tvm.plugin_wpn_sway_cvar;
	delete tvm.plugin_wpn_sway_scale;
	delete tvm.plugin_wpn_sway_interp;

	// Only delete these in L4D1 because game creates them in L4D2
	if (eEngine == k_eL4D1) { 
		delete tvm.plugin_viewmodel_offset_x;
		delete tvm.plugin_viewmodel_offset_y;
		delete tvm.plugin_viewmodel_offset_z;
	}

	tvm.Shutdown_TerrorViewModel();
}

void ClientPlugin_Viewmodel::Viewmodel_ForceUnload()
{
	if (!serverplugins) {
		Warning("We cannot force unload ourselves! 'serverplugins' is NULL!\n");
		return;
	}
	Warning("Unloading viewmodel lag plugin due to error!\n");

	for (int i = serverplugins->m_Plugins.Count() - 1; i >= 0; --i)
	{
		// L4D1 and L4D2 have slightly different CPlugin class structures
		// found out when L4D2 kept trying to unload a string like a module
		if (eEngine == k_eL4D1) {
			auto plugin = &serverplugins->m_Plugins[i]->L4D1;
			if (!plugin->m_pPlugin) continue;
			if (plugin->m_pPlugin != g_pViewmodelPlugin) continue;

			plugin->m_pPlugin->Unload();
			plugin->m_pPlugin = NULL;

			serverplugins->m_Plugins.Remove(i);
			Sys_UnloadModule(plugin->m_pPluginModule);
		}
		else {
			auto plugin = &serverplugins->m_Plugins[i]->L4D2;
			if (!plugin->m_pPlugin) continue;
			if (plugin->m_pPlugin != g_pViewmodelPlugin) continue;

			plugin->m_pPlugin->Unload();
			plugin->m_pPlugin = NULL;

			serverplugins->m_Plugins.Remove(i);
			Sys_UnloadModule(plugin->m_pPluginModule);
		}
	}
}