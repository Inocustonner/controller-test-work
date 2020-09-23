#pragma once
#include <Retranslator_i.h>

#include <atomic>

class IRetranslatorAX: public IRetranslator {
  public:
   // these 2 methods are not declared in .idl, because they are intended to be used by retranslator only
   virtual void __stdcall setWeight(long weight) = 0;
   virtual void __stdcall setWeightFixed(long weight) = 0;
};