#include "mqtt_gateway.h"
#include <cjson/cJSON.h>
#include <string.h>
#include <strings.h>

static const char *vehicle_state_str(VehicleState s) {
    switch (s) {
        case STATE_OFF:      return "OFF";
        case STATE_IDLE:     return "IDLE";
        case STATE_MOVING:   return "MOVING";
        case STATE_CHARGING: return "CHARGING";
        case STATE_FAULT:    return "FAULT";
        default:             return "UNKNOWN";
    }
}

static const char *direction_str(Direction d) {
    switch (d) {
        case DIR_NEUTRAL: return "NEUTRAL";
        case DIR_FORWARD: return "FORWARD";
        case DIR_REVERSE: return "REVERSE";
        default:          return "UNKNOWN";
    }
}

static const char *battery_state_str(BatteryState b) {
    switch (b) {
        case BAT_NORMAL:   return "NORMAL";
        case BAT_LOW:      return "LOW";
        case BAT_CHARGING: return "CHARGING";
        case BAT_FAULT:    return "FAULT";
        default:           return "UNKNOWN";
    }
}

static const char *fault_code_str(MotorFaultCode f) {
    switch (f) {
        case FAULT_NONE:        return "NONE";
        case FAULT_OVERTEMP:    return "OVERTEMP";
        case FAULT_OVERCURRENT: return "OVERCURRENT";
        case FAULT_SENSOR:      return "SENSOR";
        default:                return "UNKNOWN";
    }
}

char *gateway_motion_to_json(const ForkliftTelemetry *t) {
    if (t == NULL) {
        return NULL;
    }
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return NULL;
    }
    cJSON_AddStringToObject(root, "state", vehicle_state_str(t->state));
    cJSON_AddNumberToObject(root, "state_code", (int)t->state);
    cJSON_AddStringToObject(root, "direction", direction_str(t->direction));
    cJSON_AddNumberToObject(root, "speed_kmh", (double)t->speed_kmh);
    cJSON_AddNumberToObject(root, "motor_rpm", (double)t->motor_rpm);
    cJSON_AddNumberToObject(root, "drive_mode", (double)t->drive_mode);
    cJSON_AddNumberToObject(root, "counter", (double)t->motion_counter);

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

char *gateway_battery_to_json(const ForkliftTelemetry *t) {
    if (t == NULL) {
        return NULL;
    }
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return NULL;
    }
    cJSON_AddNumberToObject(root, "voltage_v", (double)t->battery_voltage);
    cJSON_AddNumberToObject(root, "current_a", (double)t->battery_current);
    cJSON_AddNumberToObject(root, "soc_pct", (double)t->battery_soc);
    cJSON_AddNumberToObject(root, "temp_c", (double)t->battery_temp);
    cJSON_AddStringToObject(root, "bat_state", battery_state_str(t->bat_state));
    cJSON_AddNumberToObject(root, "counter", (double)t->battery_counter);

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

char *gateway_motor_to_json(const ForkliftTelemetry *t) {
    if (t == NULL) {
        return NULL;
    }
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return NULL;
    }
    cJSON_AddNumberToObject(root, "motor_temp_c", (double)t->motor_temp);
    cJSON_AddNumberToObject(root, "controller_temp_c", (double)t->controller_temp);
    cJSON_AddNumberToObject(root, "torque_pct", (double)t->motor_torque);
    cJSON_AddStringToObject(root, "fault", fault_code_str(t->fault_code));
    cJSON_AddNumberToObject(root, "fault_code", (double)t->fault_code);

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

char *gateway_ack_to_json(uint8_t cmd, bool accepted) {
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return NULL;
    }
    cJSON_AddNumberToObject(root, "cmd", (double)cmd);
    cJSON_AddBoolToObject(root, "accepted", accepted);

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

bool gateway_parse_cmd_json(const char *json_str, uint8_t *out_cmd) {
    if (json_str == NULL || out_cmd == NULL) {
        return false;
    }
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        return false;
    }

    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, "cmd");
    if (item == NULL) {
        item = cJSON_GetObjectItemCaseSensitive(root, "command");
    }

    bool valid = false;
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        if (strcasecmp(item->valuestring, "START") == 0) {
            *out_cmd = (uint8_t)CMD_START;
            valid = true;
        } else if (strcasecmp(item->valuestring, "STOP") == 0) {
            *out_cmd = (uint8_t)CMD_STOP;
            valid = true;
        } else if (strcasecmp(item->valuestring, "RESET_FAULT") == 0 ||
                   strcasecmp(item->valuestring, "RESET") == 0) {
            *out_cmd = 5U;
            valid = true;
        } else if (strcasecmp(item->valuestring, "INJECT_FAULT") == 0) {
            *out_cmd = 4U;
            valid = true;
        }
    } else if (cJSON_IsNumber(item)) {
        int val = item->valueint;
        if (val >= 1 && val <= 5) {
            *out_cmd = (uint8_t)val;
            valid = true;
        }
    }

    cJSON_Delete(root);
    return valid;
}