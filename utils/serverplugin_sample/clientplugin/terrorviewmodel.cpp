#include "terrorviewmodel.h"
#include "clientplugin_viewmodel.h"
#include "edict.h"
#include "../sigscan.h"
#include "../minhook/MinHook.h"

#define USE_REALTIME
#define USE_LASTTIMESTAMP
#include "interpolatedvar.h"
#include <shareddefs.h>

CalcViewModelView_t C_BaseViewModel_CalcViewModelView_Original;
CalcViewModelView_t C_TerrorViewModel_CalcViewModelView_Original;

CInterpolatedVar<QAngle> m_LagAnglesHistory;
QAngle m_vLagAngles;
void CalcViewModelLag(Vector& origin, QAngle& angles, QAngle& original_angles)
{
	// Calculate our drift
	Vector	forward, right, up;
	AngleVectors(angles, &forward, &right, &up);

	// Add an entry to the history.
	m_vLagAngles = angles;
	m_LagAnglesHistory.NoteChanged(gpGlobals->realtime, gpTerrorViewModel->SwayInterp()->GetFloat(), false);

	// Interpolate back 100ms.
	m_LagAnglesHistory.Interpolate(gpGlobals->realtime, gpTerrorViewModel->SwayInterp()->GetFloat());

	// Now take the 100ms angle difference and figure out how far the forward vector moved in local space.
	Vector vLaggedForward;
	QAngle angleDiff = m_vLagAngles - angles;
	AngleVectors(-angleDiff, &vLaggedForward, 0, 0);
	Vector vForwardDiff = Vector(1, 0, 0) - vLaggedForward;

	// Now offset the origin using that.
	vForwardDiff *= gpTerrorViewModel->SwayScale()->GetFloat();
	origin += forward * vForwardDiff.x + right * -vForwardDiff.y + up * vForwardDiff.z;
}

void __fastcall C_TerrorViewModel_CalcViewModelView(void* thisptr, void* edx, void* owner, const Vector& eyePosition, const QAngle& eyeAngles)
{
	if (!gpTerrorViewModel->SwayEnabled()->GetBool()) {
		C_TerrorViewModel_CalcViewModelView_Original(thisptr, edx, owner, eyePosition, eyeAngles);
		return;
	}

	QAngle vmangoriginal = eyeAngles;
	QAngle vmangles = eyeAngles;
	Vector vmorigin = eyePosition;

	CalcViewModelLag(vmorigin, vmangles, vmangoriginal);
	if (eEngine == k_eL4D1) {
		Vector vecRight, vecUp, vecForward;
		AngleVectors(vmangoriginal, &vecForward, &vecRight, &vecUp);

		vmorigin += (vecForward * gpTerrorViewModel->VMOffsetY()->GetFloat()) + 
					(vecUp * gpTerrorViewModel->VMOffsetZ()->GetFloat()) + 
					(vecRight * gpTerrorViewModel->VMOffsetX()->GetFloat());
	}
	C_TerrorViewModel_CalcViewModelView_Original(thisptr, edx, owner, vmorigin, vmangles);
}

bool TerrorViewModel::Setup_TerrorViewModel() 
{
	CSigScan C_TerrorViewModel_CalcViewModelView_Sig;
	if (eEngine == k_eL4D2) {
		C_TerrorViewModel_CalcViewModelView_Sig.Init((unsigned char*)
			"\x55\x8B\xEC\x83\xEC\x48\xA1\x00\x00\x00\x00\x33\xC5\x89\x45\xFC\x8B\x45\x10\x8B", "xxxxxxx????xxxxxxxxx", 20);
	}
	else {
		C_TerrorViewModel_CalcViewModelView_Sig.Init((unsigned char*)
			"\x83\xEC\x44\x8B\x44\x24\x50", "xxxxxxx", 7);
	}

	if (!C_TerrorViewModel_CalcViewModelView_Sig.is_set) {
		Warning("Signature scan for 'C_TerrorViewModel_CalcViewModelView' failed!\n");
		return false;
	}
	m_LagAnglesHistory.Setup(&m_vLagAngles, LATCH_SIMULATION_VAR);

	MH_Initialize();
	MH_CreateHook(C_TerrorViewModel_CalcViewModelView_Sig.sig_addr, &C_TerrorViewModel_CalcViewModelView, (LPVOID*)&C_TerrorViewModel_CalcViewModelView_Original);
	MH_EnableHook(MH_ALL_HOOKS);
}

void TerrorViewModel::Shutdown_TerrorViewModel()
{
	delete plugin_wpn_sway_cvar;
	delete plugin_wpn_sway_scale;
	delete plugin_wpn_sway_interp;

	if (eEngine == k_eL4D1) {
		delete plugin_viewmodel_offset_x;
		delete plugin_viewmodel_offset_y;
		delete plugin_viewmodel_offset_z;
	}

	MH_DisableHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}