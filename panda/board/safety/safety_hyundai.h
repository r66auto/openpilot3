const int HYUNDAI_MAX_STEER = 409;             // like stock
const int HYUNDAI_MAX_RT_DELTA = 112;          // max delta torque allowed for real time checks
const uint32_t HYUNDAI_RT_INTERVAL = 250000;   // 250ms between real time checks
const int HYUNDAI_MAX_RATE_UP = 3;
const int HYUNDAI_MAX_RATE_DOWN = 7;
const int HYUNDAI_DRIVER_TORQUE_ALLOWANCE = 500;
const int HYUNDAI_DRIVER_TORQUE_FACTOR = 2;
const int HYUNDAI_STANDSTILL_THRSLD = 30;  // ~1kph

const int HYUNDAI_MAX_ACCEL = 200;  // 1/100 m/s2
const int HYUNDAI_MIN_ACCEL = -350; // 1/100 m/s2

const CanMsg HYUNDAI_TX_MSGS[] = {
  {832, 0, 8},  // LKAS11 Bus 0
  {1265, 0, 4}, // CLU11 Bus 0
  {1157, 0, 4}, // LFAHDA_MFC Bus 0
 };

const CanMsg HYUNDAI_LONG_TX_MSGS[] = {
  {832, 0, 8},  // LKAS11 Bus 0
  {1265, 0, 4}, // CLU11 Bus 0
  {1157, 0, 4}, // LFAHDA_MFC Bus 0
  {1056, 0, 8}, // SCC11 Bus 0
  {1057, 0, 8}, // SCC12 Bus 0
  {1290, 0, 8}, // SCC13 Bus 0
  {905, 0, 8},  // SCC14 Bus 0
  {1186, 0, 2}, // FRT_RADAR11 Bus 0
  {909, 0, 8},  // FCA11 Bus 0
  {1155, 0, 8}, // FCA12 Bus 0
  {2000, 0, 8}, // radar UDS TX addr Bus 0 (for radar disable)
 };

AddrCheckStruct hyundai_addr_checks[] = {
  {.msg = {{608, 0, 8, .check_checksum = true, .max_counter = 3U, .expected_timestep = 10000U},
           {881, 0, 8, .expected_timestep = 10000U}, { 0 }}},
  {.msg = {{902, 0, 8, .check_checksum = true, .max_counter = 15U, .expected_timestep = 10000U}, { 0 }, { 0 }}},
  {.msg = {{916, 0, 8, .check_checksum = true, .max_counter = 7U, .expected_timestep = 10000U}, { 0 }, { 0 }}},
  {.msg = {{1057, 0, 8, .check_checksum = true, .max_counter = 15U, .expected_timestep = 20000U}, { 0 }, { 0 }}},
};
#define HYUNDAI_ADDR_CHECK_LEN (sizeof(hyundai_addr_checks) / sizeof(hyundai_addr_checks[0]))

AddrCheckStruct hyundai_long_addr_checks[] = {
  {.msg = {{608, 0, 8, .check_checksum = true, .max_counter = 3U, .expected_timestep = 10000U},
           {881, 0, 8, .expected_timestep = 10000U}, { 0 }}},
  {.msg = {{902, 0, 8, .check_checksum = true, .max_counter = 15U, .expected_timestep = 10000U}, { 0 }, { 0 }}},
  {.msg = {{916, 0, 8, .check_checksum = true, .max_counter = 7U, .expected_timestep = 10000U}, { 0 }, { 0 }}},
  {.msg = {{1265, 0, 4, .check_checksum = false, .max_counter = 15U, .expected_timestep = 20000U}, { 0 }, { 0 }}},
};
#define HYUNDAI_LONG_ADDR_CHECK_LEN (sizeof(hyundai_long_addr_checks) / sizeof(hyundai_long_addr_checks[0]))

// older hyundai models have less checks due to missing counters and checksums
AddrCheckStruct hyundai_legacy_addr_checks[] = {
  {.msg = {{608, 0, 8, .check_checksum = true, .max_counter = 3U, .expected_timestep = 10000U},
           {881, 0, 8, .expected_timestep = 10000U}, { 0 }}},
  {.msg = {{902, 0, 8, .expected_timestep = 10000U}, { 0 }, { 0 }}},
  {.msg = {{916, 0, 8, .expected_timestep = 10000U}, { 0 }, { 0 }}},
  {.msg = {{1057, 0, 8, .check_checksum = true, .max_counter = 15U, .expected_timestep = 20000U}, { 0 }, { 0 }}},
};
#define HYUNDAI_LEGACY_ADDR_CHECK_LEN (sizeof(hyundai_legacy_addr_checks) / sizeof(hyundai_legacy_addr_checks[0]))

const int HYUNDAI_PARAM_EV_GAS = 1;
const int HYUNDAI_PARAM_HYBRID_GAS = 2;
const int HYUNDAI_PARAM_LONGITUDINAL = 4;

bool hyundai_legacy = false;
bool hyundai_ev_gas_signal = false;
bool hyundai_hybrid_gas_signal = false;
bool hyundai_longitudinal = false;

addr_checks hyundai_rx_checks = {hyundai_addr_checks, HYUNDAI_ADDR_CHECK_LEN};

static uint8_t hyundai_get_counter(CAN_FIFOMailBox_TypeDef *to_push) {
  int addr = GET_ADDR(to_push);

  uint8_t cnt;
  if (addr == 608) {
    cnt = (GET_BYTE(to_push, 7) >> 4) & 0x3;
  } else if (addr == 902) {
    cnt = ((GET_BYTE(to_push, 3) >> 6) << 2) | (GET_BYTE(to_push, 1) >> 6);
  } else if (addr == 916) {
    cnt = (GET_BYTE(to_push, 1) >> 5) & 0x7;
  } else if (addr == 1057) {
    cnt = GET_BYTE(to_push, 7) & 0xF;
  } else if (addr == 1265) {
    cnt = (GET_BYTE(to_push, 3) >> 4) & 0xF;
  } else {
    cnt = 0;
  }
  return cnt;
}

static uint8_t hyundai_get_checksum(CAN_FIFOMailBox_TypeDef *to_push) {
  int addr = GET_ADDR(to_push);

  uint8_t chksum;
  if (addr == 608) {
    chksum = GET_BYTE(to_push, 7) & 0xF;
  } else if (addr == 902) {
    chksum = ((GET_BYTE(to_push, 7) >> 6) << 2) | (GET_BYTE(to_push, 5) >> 6);
  } else if (addr == 916) {
    chksum = GET_BYTE(to_push, 6) & 0xF;
  } else if (addr == 1057) {
    chksum = GET_BYTE(to_push, 7) >> 4;
  } else {
    chksum = 0;
  }
  return chksum;
}

static uint8_t hyundai_compute_checksum(CAN_FIFOMailBox_TypeDef *to_push) {
  int addr = GET_ADDR(to_push);

  uint8_t chksum = 0;
  if (addr == 902) {
    // count the bits
    for (int i = 0; i < 8; i++) {
      uint8_t b = GET_BYTE(to_push, i);
      for (int j = 0; j < 8; j++) {
        uint8_t bit = 0;
        // exclude checksum and counter
        if (((i != 1) || (j < 6)) && ((i != 3) || (j < 6)) && ((i != 5) || (j < 6)) && ((i != 7) || (j < 6))) {
          bit = (b >> (uint8_t)j) & 1U;
        }
        chksum += bit;
      }
    }
    chksum = (chksum ^ 9U) & 15U;
  } else {
    // sum of nibbles
    for (int i = 0; i < 8; i++) {
      if ((addr == 916) && (i == 7)) {
        continue; // exclude
      }
      uint8_t b = GET_BYTE(to_push, i);
      if (((addr == 608) && (i == 7)) || ((addr == 916) && (i == 6)) || ((addr == 1057) && (i == 7))) {
        b &= (addr == 1057) ? 0x0FU : 0xF0U; // remove checksum
      }
      chksum += (b % 16U) + (b / 16U);
    }
    chksum = (16U - (chksum %  16U)) % 16U;
  }

  return chksum;
}

static int hyundai_rx_hook(CAN_FIFOMailBox_TypeDef *to_push) {

  bool valid = addr_safety_check(to_push, &hyundai_rx_checks,
                                 hyundai_get_checksum, hyundai_compute_checksum,
                                 hyundai_get_counter);

  if (valid && (GET_BUS(to_push) == 0)) {
    int addr = GET_ADDR(to_push);

    if (addr == 593) {
      int torque_driver_new = ((GET_BYTES_04(to_push) & 0x7ff) * 0.79) - 808; // scale down new driver torque signal to match previous one
      // update array of samples
      update_sample(&torque_driver, torque_driver_new);
    }

    if (addr == 913) {
      bool lfa_pressed = (GET_BYTES_04(to_push) >> 4) & 0x1; // LFA on signal
      if (lfa_pressed && !lfa_pressed_prev)
      {
        controls_allowed = 1;
      }
      lfa_pressed_prev = lfa_pressed;
    }

    if (hyundai_longitudinal) {
      // ACC steering wheel buttons
      if (addr == 1265) {
        int button = GET_BYTE(to_push, 0) & 0x7;
        switch (button) {
          case 1:  // resume
          case 2:  // set
            controls_allowed = 1;
            break;
          case 4:  // cancel
            break;
          default:
            break;  // any other button is irrelevant
        }
      }
    } else {
      // enter controls on rising edge of ACC, exit controls on ACC off
      if (addr == 1057) {
        // 2 bits: 13-14
        int cruise_engaged = (GET_BYTES_04(to_push) >> 13) & 0x3;
        if (cruise_engaged && !cruise_engaged_prev) {
          controls_allowed = 1;
        }
        cruise_engaged_prev = cruise_engaged;
      }
    }

    if (addr == 1056) {
      bool acc_main_on = GET_BYTES_04(to_push) & 0x1; // ACC main_on signal
      if (acc_main_on && !acc_main_on_prev)
      {
        controls_allowed = 1;
      }
      acc_main_on_prev = acc_main_on;
    }

    if (addr == 1056) {
      bool acc_main_on = GET_BYTES_04(to_push) & 0x1; // ACC main_on signal
      if (acc_main_on_prev != acc_main_on)
      {
        disengageFromBrakes = false;
        controls_allowed = 0;
      }
      acc_main_on_prev = acc_main_on;
    }

    // read gas pressed signal
    if ((addr == 881) && hyundai_ev_gas_signal) {
      gas_pressed = (((GET_BYTE(to_push, 4) & 0x7F) << 1) | GET_BYTE(to_push, 3) >> 7) != 0;
    } else if ((addr == 881) && hyundai_hybrid_gas_signal) {
      gas_pressed = GET_BYTE(to_push, 7) != 0;
    } else if (addr == 608) {  // ICE
      gas_pressed = (GET_BYTE(to_push, 7) >> 6) != 0;
    } else {
    }

    // sample wheel speed, averaging opposite corners
    if (addr == 902) {
      int hyundai_speed = GET_BYTES_04(to_push) & 0x3FFF;  // FL
      hyundai_speed += (GET_BYTES_48(to_push) >> 16) & 0x3FFF;  // RL
      hyundai_speed /= 2;
      vehicle_moving = hyundai_speed > HYUNDAI_STANDSTILL_THRSLD;
    }

    if (addr == 916) {
      brake_pressed = false; //(GET_BYTE(to_push, 6) >> 7) != 0;
    }

    bool stock_ecu_detected = (addr == 832);

    // If openpilot is controlling longitudinal we need to ensure the radar is turned off
    // Enforce by checking we don't see SCC12
    if (hyundai_longitudinal && (addr == 1057)) {
      stock_ecu_detected = true;
    }
    generic_rx_checks(stock_ecu_detected);
  }
  return valid;
}

static int hyundai_tx_hook(CAN_FIFOMailBox_TypeDef *to_send) {

  int tx = 1;
  int addr = GET_ADDR(to_send);

  if (hyundai_longitudinal) {
    tx = msg_allowed(to_send, HYUNDAI_LONG_TX_MSGS, sizeof(HYUNDAI_LONG_TX_MSGS)/sizeof(HYUNDAI_LONG_TX_MSGS[0]));
  } else {
    tx = msg_allowed(to_send, HYUNDAI_TX_MSGS, sizeof(HYUNDAI_TX_MSGS)/sizeof(HYUNDAI_TX_MSGS[0]));
  }

  if (relay_malfunction) {
    tx = 0;
  }

  // FCA11: Block any potential actuation
  if (addr == 909) {
    int CR_VSM_DecCmd = GET_BYTE(to_send, 1);
    int FCA_CmdAct = (GET_BYTE(to_send, 2) >> 5) & 1;
    int CF_VSM_DecCmdAct = (GET_BYTE(to_send, 3) >> 7) & 1;

    if ((CR_VSM_DecCmd != 0) || (FCA_CmdAct != 0) || (CF_VSM_DecCmdAct != 0)) {
      tx = 0;
    }
  }

  // ACCEL: safety check
  if (addr == 1057) {
    int desired_accel_raw = (((GET_BYTE(to_send, 4) & 0x7) << 8) | GET_BYTE(to_send, 3)) - 1023;
    int desired_accel_val = ((GET_BYTE(to_send, 5) << 3) | (GET_BYTE(to_send, 4) >> 5)) - 1023;

    int aeb_decel_cmd = GET_BYTE(to_send, 2);
    int aeb_req = (GET_BYTE(to_send, 6) >> 6) & 1;

    bool violation = 0;

    if (!controls_allowed) {
      if ((desired_accel_raw != 0) || (desired_accel_val != 0)) {
        violation = 1;
      }
    }
    violation |= max_limit_check(desired_accel_raw, HYUNDAI_MAX_ACCEL, HYUNDAI_MIN_ACCEL);
    violation |= max_limit_check(desired_accel_val, HYUNDAI_MAX_ACCEL, HYUNDAI_MIN_ACCEL);

    violation |= (aeb_decel_cmd != 0);
    violation |= (aeb_req != 0);

    if (violation) {
      tx = 0;
    }
  }

  // LKA STEER: safety check
  if (addr == 832) {
    int desired_torque = ((GET_BYTES_04(to_send) >> 16) & 0x7ff) - 1024;
    uint32_t ts = microsecond_timer_get();
    bool violation = 0;

    if (controls_allowed) {

      // *** global torque limit check ***
      violation |= max_limit_check(desired_torque, HYUNDAI_MAX_STEER, -HYUNDAI_MAX_STEER);

      // *** torque rate limit check ***
      violation |= driver_limit_check(desired_torque, desired_torque_last, &torque_driver,
        HYUNDAI_MAX_STEER, HYUNDAI_MAX_RATE_UP, HYUNDAI_MAX_RATE_DOWN,
        HYUNDAI_DRIVER_TORQUE_ALLOWANCE, HYUNDAI_DRIVER_TORQUE_FACTOR);

      // used next time
      desired_torque_last = desired_torque;

      // *** torque real time rate limit check ***
      violation |= rt_rate_limit_check(desired_torque, rt_torque_last, HYUNDAI_MAX_RT_DELTA);

      // every RT_INTERVAL set the new limits
      uint32_t ts_elapsed = get_ts_elapsed(ts, ts_last);
      if (ts_elapsed > HYUNDAI_RT_INTERVAL) {
        rt_torque_last = desired_torque;
        ts_last = ts;
      }
    }

    // no torque if controls is not allowed
    if (!controls_allowed && (desired_torque != 0)) {
      violation = 1;
    }

    // reset to 0 if either controls is not allowed or there's a violation
    if (violation || !controls_allowed) {
      desired_torque_last = 0;
      rt_torque_last = 0;
      ts_last = ts;
    }

    if (violation) {
      tx = 0;
    }
  }

  // UDS: Only tester present ("\x02\x3E\x80\x00\x00\x00\x00\x00") allowed on diagnostics address
  if (addr == 2000) {
    if ((GET_BYTES_04(to_send) != 0x00803E02) || (GET_BYTES_48(to_send) != 0x0)) {
      tx = 0;
    }
  }

  // FORCE CANCEL: safety check only relevant when spamming the cancel button.
  // ensuring that only the cancel button press is sent (VAL 4) when controls are off.
  // This avoids unintended engagements while still allowing resume spam
  if ((addr == 1265) && !controls_allowed) {
    if ((GET_BYTES_04(to_send) & 0x7) != 4) {
      tx = 0;
    }
  }

  // 1 allows the message through
  return tx;
}

static int hyundai_fwd_hook(int bus_num, CAN_FIFOMailBox_TypeDef *to_fwd) {

  int bus_fwd = -1;
  int addr = GET_ADDR(to_fwd);
  // forward cam to ccan and viceversa, except lkas cmd
  if (!relay_malfunction) {
    if (bus_num == 0) {
      bus_fwd = 2;
    }
    if ((bus_num == 2) && (addr != 832) && (addr != 1157)) {
      bus_fwd = 0;
    }
  }
  return bus_fwd;
}

static const addr_checks* hyundai_init(int16_t param) {
  disengageFromBrakes = false;
  controls_allowed = false;
  relay_malfunction_reset();

  hyundai_legacy = false;
  hyundai_ev_gas_signal = GET_FLAG(param, HYUNDAI_PARAM_EV_GAS);
  hyundai_hybrid_gas_signal = !hyundai_ev_gas_signal && GET_FLAG(param, HYUNDAI_PARAM_HYBRID_GAS);

#ifdef ALLOW_DEBUG
  hyundai_longitudinal = GET_FLAG(param, HYUNDAI_PARAM_LONGITUDINAL);
#endif

  if (hyundai_longitudinal) {
    hyundai_rx_checks = (addr_checks){hyundai_long_addr_checks, HYUNDAI_LONG_ADDR_CHECK_LEN};
  } else {
    hyundai_rx_checks = (addr_checks){hyundai_addr_checks, HYUNDAI_ADDR_CHECK_LEN};
  }
  return &hyundai_rx_checks;
}

static const addr_checks* hyundai_legacy_init(int16_t param) {
  disengageFromBrakes = false;
  controls_allowed = false;
  relay_malfunction_reset();

  hyundai_legacy = true;
  hyundai_longitudinal = false;
  hyundai_ev_gas_signal = GET_FLAG(param, HYUNDAI_PARAM_EV_GAS);
  hyundai_hybrid_gas_signal = !hyundai_ev_gas_signal && GET_FLAG(param, HYUNDAI_PARAM_HYBRID_GAS);
  hyundai_rx_checks = (addr_checks){hyundai_legacy_addr_checks, HYUNDAI_LEGACY_ADDR_CHECK_LEN};
  return &hyundai_rx_checks;
}

const safety_hooks hyundai_hooks = {
  .init = hyundai_init,
  .rx = hyundai_rx_hook,
  .tx = hyundai_tx_hook,
  .tx_lin = nooutput_tx_lin_hook,
  .fwd = hyundai_fwd_hook,
};

const safety_hooks hyundai_legacy_hooks = {
  .init = hyundai_legacy_init,
  .rx = hyundai_rx_hook,
  .tx = hyundai_tx_hook,
  .tx_lin = nooutput_tx_lin_hook,
  .fwd = hyundai_fwd_hook,
};
