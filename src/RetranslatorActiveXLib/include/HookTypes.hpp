#pragma once 

enum EventType
{
  SetMinimalWeight,
  SetCorr,

  EventsCnt
};

template <typename... InputArgs>
using HookF = void(InputArgs...);
using SetHook = HookF<long>;