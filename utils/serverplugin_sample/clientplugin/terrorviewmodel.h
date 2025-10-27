#pragma once

#include "convar.h"
#include "convar_l4d.h"

#if defined(_REL_N_DEBUG) || defined(_DEBUG)
#define DEBUG_Msg(...) Msg(__VA_ARGS__)
#else
#define DEBUG_Msg(...)
#endif

#define CONVAR_GET_FUNCTION(name, functionname) \
	private: ConVar* name = NULL; \
	public: ConVar* functionname##() const { return name##; }

typedef void(__fastcall* CalcViewModelView_t)(void* thisptr, void*, void* owner, const Vector& eyePosition, const QAngle& eyeAngles);
class TerrorViewModel
{
	friend class ClientPlugin_Viewmodel;
public:
	bool Setup_TerrorViewModel();
	void Shutdown_TerrorViewModel();

private:
	CONVAR_GET_FUNCTION(plugin_wpn_sway_cvar, SwayEnabled);
	CONVAR_GET_FUNCTION(plugin_wpn_sway_scale, SwayScale);
	CONVAR_GET_FUNCTION(plugin_wpn_sway_interp, SwayInterp);

	CONVAR_GET_FUNCTION(plugin_viewmodel_offset_x, VMOffsetX);
	CONVAR_GET_FUNCTION(plugin_viewmodel_offset_y, VMOffsetY);
	CONVAR_GET_FUNCTION(plugin_viewmodel_offset_z, VMOffsetZ);
};
