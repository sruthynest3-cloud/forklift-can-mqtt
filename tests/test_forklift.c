#include "forklift_fsm.h"
#include "can_codec.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>

static void test_01_initial_state(void) {
    ForkliftTelemetry t;
    forklift_init(&t);
    assert(t.state == STATE_OFF && t.motor_rpm == 0 && t.battery_soc == 85);
}

static void test_02_start_command_positive(void) {
    ForkliftTelemetry t;
    forklift_init(&t);
    assert(forklift_handle_command(&t, CMD_START) == true);
    assert(t.state == STATE_MOVING && t.direction == DIR_FORWARD);
}

static void test_03_stop_command_positive(void) {
    ForkliftTelemetry t;
    forklift_init(&t);
    forklift_handle_command(&t, CMD_START);
    forklift_step_100ms(&t);
    assert(forklift_handle_command(&t, CMD_STOP) == true);
    assert(t.state == STATE_OFF && t.motor_rpm == 0 && t.speed_kmh == 0.0f);
}

static void test_04_invalid_command_negative(void) {
    ForkliftTelemetry t;
    forklift_init(&t);
    assert(forklift_handle_command(&t, 99) == false);
    assert(t.state == STATE_OFF);
}

static void test_05_speed_max_boundary(void) {
    ForkliftTelemetry t;
    forklift_init(&t);
    forklift_handle_command(&t, CMD_START);
    for (int i = 0; i < 100; i++) forklift_step_100ms(&t);
    assert(fabsf(t.speed_kmh - 15.0f) < 0.01f);
    assert(t.motor_rpm == 3000);
}

static void test_06_charging_negative_current(void) {
    ForkliftTelemetry t;
    forklift_init(&t);
    t.state = STATE_CHARGING;
    forklift_step_100ms(&t);
    assert(t.battery_current < 0.0f && t.battery_soc == 86);
}

static void test_07_fault_blocks_start_and_reset(void) {
    ForkliftTelemetry t;
    forklift_init(&t);
    forklift_inject_fault(&t, FAULT_OVERTEMP);
    assert(t.state == STATE_FAULT);
    assert(forklift_handle_command(&t, CMD_START) == false);
    assert(forklift_handle_command(&t, CMD_RESET_FAULT) == false);
}

static void test_08_fault_clear_and_reset_positive(void) {
    ForkliftTelemetry t;
    forklift_init(&t);
    forklift_inject_fault(&t, FAULT_OVERTEMP);
    forklift_clear_fault_condition(&t);
    assert(forklift_handle_command(&t, CMD_RESET_FAULT) == true);
    assert(t.state == STATE_OFF);
}

static void test_09_motion_codec_roundtrip(void) {
    ForkliftTelemetry src, dst;
    forklift_init(&src);
    src.state = STATE_MOVING;
    src.direction = DIR_REVERSE;
    src.speed_kmh = 8.5f;
    src.motor_rpm = 1700;
    uint8_t frame[8];
    encode_motion_frame(&src, frame);
    assert(decode_motion_frame(frame, 8, &dst) == true);
    assert(fabsf(dst.speed_kmh - 8.5f) < 0.01f && dst.motor_rpm == 1700);
}

static void test_10_battery_codec_signed_current(void) {
    ForkliftTelemetry src, dst;
    forklift_init(&src);
    src.battery_voltage = 48.2f;
    src.battery_current = -12.5f;
    uint8_t frame[8];
    encode_battery_frame(&src, frame);
    assert(decode_battery_frame(frame, 8, &dst) == true);
    assert(fabsf(dst.battery_voltage - 48.2f) < 0.01f);
    assert(fabsf(dst.battery_current - (-12.5f)) < 0.01f);
}

static void test_11_motor_codec_roundtrip(void) {
    ForkliftTelemetry src, dst;
    forklift_init(&src);
    src.motor_temp = 75;
    src.controller_temp = 55;
    src.motor_torque = 80;
    src.fault_code = FAULT_OVERCURRENT;
    uint8_t frame[8];
    encode_motor_frame(&src, frame);
    assert(decode_motor_frame(frame, 8, &dst) == true);
    assert(dst.motor_temp == 75 && dst.fault_code == FAULT_OVERCURRENT);
}

static void test_12_codec_invalid_dlc_negative(void) {
    ForkliftTelemetry dst;
    uint8_t bad[8] = {0};
    assert(decode_motion_frame(bad, 7, &dst) == false);
    assert(decode_battery_frame(bad, 4, &dst) == false);
    assert(decode_motor_frame(bad, 0, &dst) == false);
}

int main(void) {
    test_01_initial_state();
    test_02_start_command_positive();
    test_03_stop_command_positive();
    test_04_invalid_command_negative();
    test_05_speed_max_boundary();
    test_06_charging_negative_current();
    test_07_fault_blocks_start_and_reset();
    test_08_fault_clear_and_reset_positive();
    test_09_motion_codec_roundtrip();
    test_10_battery_codec_signed_current();
    test_11_motor_codec_roundtrip();
    test_12_codec_invalid_dlc_negative();
    printf("SUCCESS: All 12 automated unit tests passed!\n");
    return 0;
}
