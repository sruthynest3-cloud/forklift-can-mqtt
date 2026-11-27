#include "forklift_fsm.h"
#include <string.h>

void forklift_init(ForkliftTelemetry *t) {
    memset(t, 0, sizeof(*t));
    t->state = STATE_OFF;          /* Initial parked/off state */
    t->direction = DIR_NEUTRAL;
    t->drive_mode = 1;             /* NORMAL */
    t->battery_voltage = 48.5f;
    t->battery_soc = 85;
    t->battery_temp = 28;
    t->bat_state = BAT_NORMAL;
    t->motor_temp = 30;
    t->controller_temp = 29;
    t->fault_code = FAULT_NONE;
}

bool forklift_handle_command(ForkliftTelemetry *t, uint8_t cmd) {
    if (cmd == CMD_START) 
    {
        if (t->state == STATE_FAULT || t->fault_code != FAULT_NONE) {
            return false;
        }
        t->state = STATE_MOVING;
        t->direction = DIR_FORWARD;
        return true;
    }
    if (cmd == CMD_STOP) 
    {
        if (t->state != STATE_FAULT) {
            t->state = STATE_OFF;
        }
        t->direction = DIR_NEUTRAL;
        t->speed_kmh = 0.0f;
        t->motor_rpm = 0;
        t->motor_torque = 0;
        return true;
    }
    if (cmd == CMD_RESET_FAULT) 
    {
        if (t->motor_temp > 100 || t->battery_voltage < 40.0f || t->fault_code != FAULT_NONE) {
            return false;
        }
        t->state = STATE_OFF;
        t->bat_state = BAT_NORMAL;
        return true;
    }
    if (cmd == 4U) {
        forklift_inject_fault(t, FAULT_OVERTEMP);
        return true;
    }
    if (cmd == 5U) {
        forklift_clear_fault_condition(t);
        t->state = STATE_OFF;
        t->bat_state = BAT_NORMAL;
        return true;
    }
    return false;
}

void forklift_inject_fault(ForkliftTelemetry *t, MotorFaultCode code) {
    t->fault_code = code;
    if (code == FAULT_OVERTEMP) {
        t->motor_temp = 115;
    }
    t->state = STATE_FAULT;
    t->speed_kmh = 0.0f;
    t->motor_rpm = 0;
}

void forklift_clear_fault_condition(ForkliftTelemetry *t) {
    t->fault_code = FAULT_NONE;
    if (t->motor_temp > 100) {
        t->motor_temp = 45;
    }
}

void forklift_step_100ms(ForkliftTelemetry *t) {
    t->motion_counter++;
    if (t->state == STATE_MOVING) {
        if (t->speed_kmh < 15.0f) t->speed_kmh += 0.2f;
        if (t->speed_kmh > 15.0f) t->speed_kmh = 15.0f;
        t->motor_rpm = (uint16_t)(t->speed_kmh * 200.0f);
        t->motor_torque = 45;
        t->battery_current = 24.5f;
        if (t->battery_voltage > 42.0f) t->battery_voltage -= 0.001f;
        if (t->motor_temp < 65) t->motor_temp += 1;
    } else if (t->state == STATE_CHARGING) {
        t->speed_kmh = 0.0f;
        t->motor_rpm = 0;
        t->battery_current = -15.0f; /* Negative current during charging */
        if (t->battery_soc < 100) t->battery_soc += 1;
    } else {
        t->speed_kmh = 0.0f;
        t->motor_rpm = 0;
        t->motor_torque = 0;
        t->battery_current = 0.5f;
        if (t->motor_temp > 30 && t->fault_code == FAULT_NONE) {
            t->motor_temp -= 1;
        }
    }
}
