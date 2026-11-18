#ifndef CAN_CODEC_H
#define CAN_CODEC_H

#include "forklift_types.h"
#include <stddef.h>

void encode_motion_frame(const ForkliftTelemetry *t, uint8_t data[8]);
bool decode_motion_frame(const uint8_t data[8], size_t len, ForkliftTelemetry *out);

void encode_battery_frame(const ForkliftTelemetry *t, uint8_t data[8]);
bool decode_battery_frame(const uint8_t data[8], size_t len, ForkliftTelemetry *out);

void encode_motor_frame(const ForkliftTelemetry *t, uint8_t data[8]);
bool decode_motor_frame(const uint8_t data[8], size_t len, ForkliftTelemetry *out);

void encode_cmd_frame(uint8_t cmd, uint8_t data[8]);
void encode_ack_frame(uint8_t cmd, bool accepted, uint8_t data[8]);

#endif
