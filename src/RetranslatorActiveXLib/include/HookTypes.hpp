#pragma once 

enum EventType
{
  SetMinimalWeight = 0,
  SetCorr,
  SetNull,
  SetResetThr,
  ClearAuth,
	
  EventsCnt
};

template <typename... InputArgs>
using HookF = void(InputArgs...);
using SetHook = HookF<long>;
