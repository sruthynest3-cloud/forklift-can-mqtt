#ifndef MQTT_GATEWAY_H
#define MQTT_GATEWAY_H

#include <stdbool.h>
#include <stdint.h>
#include "forklift_types.h"

/* Serialize decoded CAN telemetry into heap-allocated JSON strings (caller must free) */
char *gateway_motion_to_json(const ForkliftTelemetry *t);
char *gateway_battery_to_json(const ForkliftTelemetry *t);
char *gateway_motor_to_json(const ForkliftTelemetry *t);
char *gateway_ack_to_json(uint8_t cmd, bool accepted);

/* Parse incoming JSON command (e.g. {"cmd":"START"}) into ForkliftCommand byte */
bool gateway_parse_cmd_json(const char *json_str, uint8_t *out_cmd);

#endif