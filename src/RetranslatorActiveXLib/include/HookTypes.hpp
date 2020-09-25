#pragma once 

enum EventType
{
  SetMinimalWeight = 0,
  SetCorr,

  EventsCnt
};

template <typename... InputArgs>
using HookF = void(InputArgs...);
using SetHook = HookF<long>;