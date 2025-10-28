#pragma once
#include "edict.h"
#include "cdll_int.h"
#include "interface.h"
#include "terrorviewmodel.h"
#include "engine/iserverplugin.h"

typedef struct {
	union {
		struct L4D1 {
			char m_szDescription[128];
			bool m_bDisable;
			IServerPluginCallbacks* m_pPlugin;
			int m_iPluginInterfaceVersion;
			CSysModule* m_pPluginModule;
		} L4D1;
		struct L4D2 {
			char m_szDescription[128];
			char m_szFileName[128];
			bool m_bDisable;
			IServerPluginCallbacks* m_pPlugin;
			int m_iPluginInterfaceVersion;
			CSysModule* m_pPluginModule;
		} L4D2;
	};
} CPlugin_t;
typedef struct {
	void** __vfptr;
	CUtlVector<CPlugin_t*> m_Plugins;
} CServerPlugin_t;

enum ESourceEngine {
	k_eL4D1,
	k_eL4D2,
	k_eOther
};

class ClientPlugin_Viewmodel {
public:
	static bool Viewmodel_Run(CreateInterfaceFn);
	static void Viewmodel_Stop();

	static void Viewmodel_ForceUnload();
};

extern ESourceEngine eEngine;

extern CGlobalVars* gpGlobals;
extern IVEngineClient* engineclient;
extern ICvar* pcvar;

extern TerrorViewModel* gpTerrorViewModel;