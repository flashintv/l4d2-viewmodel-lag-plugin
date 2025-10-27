#pragma once
#include "edict.h"
#include "cdll_int.h"
#include "interface.h"
#include "terrorviewmodel.h"

enum ESourceEngine {
	k_eL4D1,
	k_eL4D2,
	k_eOther
};

class ClientPlugin_Viewmodel {
public:
	static bool Viewmodel_Run(CreateInterfaceFn);
	static void Viewmodel_Stop();
};

extern ESourceEngine eEngine;

extern CGlobalVars* gpGlobals;
extern IVEngineClient* engineclient;
extern ICvar* pcvar;

extern TerrorViewModel* gpTerrorViewModel;