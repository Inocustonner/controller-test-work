#pragma once

#define InterlockedReadStatus(p_dest_var, p_src_status_var) \
  (p_dest_var)->f_long = InterlockedExchangeAdd(&(p_src_status_var)->f_long, 0)

#define InterlockedSetStatus(p_dest_status_var, src_var) \
  InterlockedExchange(&(p_dest_status_var)->f_long, (src_var).f_long)

enum ErrorCode: long
{
 NoErrors = 0,
 ErrorNotStarted = 1,
 ErrorNotReady = 2,
 ErrorRetranslatorEndPort = 3,
 ErrorRetranslatorWeightsPort = 4,
 ErrorInvalidData = 5
};

struct Status {
  union {
    struct {
      unsigned int auth : 1;
      unsigned int stability : 1;
      unsigned int err : 30;
    };
    long f_long;
    // unsigned long f_ulong;
  };
};
static_assert(sizeof(Status) == sizeof(long));
