#pragma once 

enum EventType
{
  SetMinimalWeight = 0,
  SetCorr,
  SetNull,
  ClearAuth,
	
  EventsCnt
};

template <typename... InputArgs>
using HookF = void(InputArgs...);
using SetHook = HookF<long>;
