#include <HookTypes.hpp>

#include "State.hpp"
#include "Output.hpp"
#include "Init.hpp"
#include "mmsg.hpp"

#include <Error.hpp>

#include <thread>
#include <cstdlib>
#include <cstdio>

#include <RetranslatorDefs.hpp>

#include "fixer.h"

#define IMPORT_DLL __declspec(dllimport)

extern "C" {
	IMPORT_DLL void initDll();
	IMPORT_DLL void setEventHook(EventType event, SetHook* onSet);
	IMPORT_DLL void fireEvent(EventType event);
	IMPORT_DLL void __stdcall setWeight(long weight);
	IMPORT_DLL void __stdcall setWeightFixed(long weight);
	IMPORT_DLL void __stdcall setStatus(long status);
}

void onSetCorr(long corr) {
	if (-100 < corr && corr < 100)
	{
		const double corr_koef = corr / 100.0;
		state.corr =
				[&min_w = state.min_weight, corr_koef](comptype inp_w) -> comptype 
					{ return static_cast<comptype>( (inp_w - min_w) * corr_koef ) + inp_w; };
	}
	else {
		state.corr =
				[corr_w = corr](comptype inp_w) -> comptype { return (inp_w + corr_w); };
	}
}

void onSetMinWeight(long new_min_w) {
	state.min_weight = new_min_w;
	state.authorized = true;
}

inline
comptype phase1(const comptype p1, const bool is_stable)
{
	comptype corr_weight = state.corr(p1);
	if (is_stable)
	{
		state.p0s = p1;
	}
	return corr_weight;
}


inline
void phase0(const comptype p1)
{
	if (state.p0 > p1 
		&& state.p0 > reset_thr 
		&& reset_thr > p1)
	{
		printf("resetting\n");
		if (get_log_lvl() > 1) {
			wchar_t buffer_msg[100] = {};
			swprintf(buffer_msg, std::size(buffer_msg), L"\"Resetting state\n↓%d\n- - - -< %d >- - - -\n%d\n\"", state.p0, reset_thr, p1);
			mMsgBox(L"INFO", buffer_msg, get_msg_duration());
		}
		reset_state();
	}
}


comptype fix(const comptype p1, const bool is_stable)
{
	comptype ret_value = p1;
	if (state.authorized)
	{
		if (p1 < state.min_weight)
			phase0(p1);
		else
			ret_value = phase1(p1, is_stable);
	}
	else if (p1 > reset_thr)
	{
		// do none
	}
	state.p0 = p1;

	setWeight(p1);
	setWeightFixed(ret_value);

	#define AUTHORIZED_BYTES(a) (a ? 1000 : 2000)
	#define STABLE_BYTES(s) (s ? 10 : 20)

	setStatus(AUTHORIZED_BYTES(state.authorized) + STABLE_BYTES(is_stable));

	return ret_value;
}


bool init_fixer(const inipp::Ini<char>& ini)
{
	initDll();
	setStatus(STATE_FL_RETRANSLATOR_STARTED);
	setEventHook(SetMinimalWeight, onSetMinWeight);
	setEventHook(SetCorr, onSetCorr);
	fireEvent(SetCorr); // if corr was set before loading retranslator, set it
	try
	{
		init_settings(ini);
	}
	catch (const ctrl::error& e)
	{
		printf(e.what());
		if (get_log_lvl() > 1)
			std::system("pause");
		std::exit(1);
	}
	return true;
}
