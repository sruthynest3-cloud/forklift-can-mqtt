#include "can_codec.h"
#include <string.h>

static void write_u16_be(uint8_t *buf, uint16_t val) {
    buf[0] = (uint8_t)((val >> 8) & 0xFFU);
    buf[1] = (uint8_t)(val & 0xFFU);
}

static uint16_t read_u16_be(const uint8_t *buf) {
    return (uint16_t)(((uint16_t)buf[0] << 8) | (uint16_t)buf[1]);
}

void encode_motion_frame(const ForkliftTelemetry *t, uint8_t data[8]) {
    memset(data, 0, 8);
    uint16_t raw_speed = (uint16_t)(t->speed_kmh * 10.0f);
    data[0] = (uint8_t)t->state;
    data[1] = (uint8_t)t->direction;
    write_u16_be(&data[2], raw_speed);
    write_u16_be(&data[4], t->motor_rpm);
    data[6] = t->drive_mode;
    data[7] = t->motion_counter;
}

bool decode_motion_frame(const uint8_t data[8], size_t len, ForkliftTelemetry *out) {
    if (len != 8 || out == NULL || data[0] > STATE_FAULT || data[1] > DIR_REVERSE) {
        return false;
    }
    out->state = (VehicleState)data[0];
    out->direction = (Direction)data[1];
    out->speed_kmh = (float)read_u16_be(&data[2]) / 10.0f;
    out->motor_rpm = read_u16_be(&data[4]);
    out->drive_mode = data[6];
    out->motion_counter = data[7];
    return true;
}

void encode_battery_frame(const ForkliftTelemetry *t, uint8_t data[8]) {
    memset(data, 0, 8);
    uint16_t raw_v = (uint16_t)(t->battery_voltage * 10.0f);
    int16_t raw_i = (int16_t)(t->battery_current * 10.0f);
    write_u16_be(&data[0], raw_v);
    write_u16_be(&data[2], (uint16_t)raw_i);
    data[4] = t->battery_soc;
    data[5] = (uint8_t)t->battery_temp;
    data[6] = (uint8_t)t->bat_state;
    data[7] = t->battery_counter;
}

bool decode_battery_frame(const uint8_t data[8], size_t len, ForkliftTelemetry *out) {
    if (len != 8 || out == NULL || data[4] > 100U || data[6] > BAT_FAULT) {
        return false;
    }
    out->battery_voltage = (float)read_u16_be(&data[0]) / 10.0f;
    out->battery_current = (float)((int16_t)read_u16_be(&data[2])) / 10.0f;
    out->battery_soc = data[4];
    out->battery_temp = (int8_t)data[5];
    out->bat_state = (BatteryState)data[6];
    out->battery_counter = data[7];
    return true;
}

void encode_motor_frame(const ForkliftTelemetry *t, uint8_t data[8]) {
    memset(data, 0, 8);
    data[0] = (uint8_t)t->motor_temp;
    data[1] = (uint8_t)t->controller_temp;
    data[2] = t->motor_torque;
    data[3] = (uint8_t)t->fault_code;
}

bool decode_motor_frame(const uint8_t data[8], size_t len, ForkliftTelemetry *out) {
    if (len != 8 || out == NULL || data[2] > 100U || data[3] > FAULT_SENSOR) {
        return false;
    }
    out->motor_temp = (int8_t)data[0];
    out->controller_temp = (int8_t)data[1];
    out->motor_torque = data[2];
    out->fault_code = (MotorFaultCode)data[3];
    return true;
}

void encode_cmd_frame(uint8_t cmd, uint8_t data[8]) {
    memset(data, 0, 8);
    data[0] = cmd;
}

void encode_ack_frame(uint8_t cmd, bool accepted, uint8_t data[8]) {
    memset(data, 0, 8);
    data[0] = cmd;
    data[1] = accepted ? 1U : 0U;
}
