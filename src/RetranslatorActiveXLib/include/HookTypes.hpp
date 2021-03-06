#pragma once 

enum EventType
{
  SetMaximalWeight = 0,
  SetMinimalWeight,
  SetCorr,
  SetNull,
  SetResetThr,
  ClearAuth,
	
  //EventsCnt
};

template <typename... InputArgs>
using HookF = void(InputArgs...);

using HookSet = void(*)(long);
using HookNotify = void(*)();
