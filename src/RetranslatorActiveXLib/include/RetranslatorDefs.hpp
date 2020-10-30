#pragma once
// #include <Retranslator_i.h>

// #include <atomic>

using uint = unsigned int;

template<uint higher_flag, uint lower_flag>
constexpr uint compose_state() {
	static_assert(higher_flag >= 1000 && higher_flag < 10000);
	static_assert(lower_flag < 100);
	
	return higher_flag + lower_flag;
}

#define COMPOSE_STATE(higher_flag, lower_flag) compose_state<higher_flag, lower_flag>()

#define STATE_H_AUTHORIZED 1000
#define STATE_H_UNAUTHORIZED 2000

#define STATE_L_STABLE_WEIGHT 0010
#define STATE_L_UNSTABLE_WEIGHT 0020

#define STATE_H_ERROR 9000

#define STATE_L_ERROR_SRC_PORT 0031
#define STATE_L_ERROR_DST_PORT 0032

#define STATE_L_RETRANSLATOR_ERROR 01

#define STATE_ERROR(err_l_flag) COMPOSE_STATE(STATE_H_ERROR, err_l_flag)

#define STATE_FL_RETRANSLATOR_STARTED 8000
