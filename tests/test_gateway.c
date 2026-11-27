#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "forklift_fsm.h"
#include "mqtt_gateway.h"

static void test_telemetry_json_serialization(void) {
    ForkliftTelemetry t;
    forklift_init(&t);
    t.state = STATE_MOVING;
    t.direction = DIR_FORWARD;
    t.speed_kmh = 12.5f;
    t.motor_rpm = 2500;
    t.battery_soc = 80;
    t.motor_temp = 45;

    char *m_json = gateway_motion_to_json(&t);
    assert(m_json != NULL);
    assert(strstr(m_json, "\"state\":\"MOVING\"") != NULL);
    assert(strstr(m_json, "\"speed_kmh\":12.5") != NULL);
    free(m_json);

    char *b_json = gateway_battery_to_json(&t);
    assert(b_json != NULL);
    assert(strstr(b_json, "\"soc_pct\":80") != NULL);
    free(b_json);

    char *mot_json = gateway_motor_to_json(&t);
    assert(mot_json != NULL);
    assert(strstr(mot_json, "\"motor_temp_c\":45") != NULL);
    free(mot_json);

    char *ack_json = gateway_ack_to_json(1, true);
    assert(ack_json != NULL);
    assert(strstr(ack_json, "\"accepted\":true") != NULL);
    free(ack_json);
}

static void test_command_json_parsing(void) {
    uint8_t cmd = 0;

    /* Positive cases */
    assert(gateway_parse_cmd_json("{\"cmd\":\"START\"}", &cmd) && cmd == CMD_START);
    assert(gateway_parse_cmd_json("{\"cmd\":\"stop\"}", &cmd) && cmd == CMD_STOP);
    assert(gateway_parse_cmd_json("{\"cmd\":\"INJECT_FAULT\"}", &cmd) && cmd == 4U);
    assert(gateway_parse_cmd_json("{\"cmd\":\"RESET_FAULT\"}", &cmd) && cmd == 5U);
    assert(gateway_parse_cmd_json("{\"command\":2}", &cmd) && cmd == 2U);

    /* Negative & boundary cases */
    assert(!gateway_parse_cmd_json(NULL, &cmd));
    assert(!gateway_parse_cmd_json("{\"cmd\":\"START\"}", NULL));
    assert(!gateway_parse_cmd_json("{invalid_json", &cmd));
    assert(!gateway_parse_cmd_json("{\"cmd\":\"UNKNOWN\"}", &cmd));
    assert(!gateway_parse_cmd_json("{\"cmd\":0}", &cmd));
    assert(!gateway_parse_cmd_json("{\"cmd\":6}", &cmd));
}

static void test_gateway_null_inputs(void) {
    assert(gateway_motion_to_json(NULL) == NULL);
    assert(gateway_battery_to_json(NULL) == NULL);
    assert(gateway_motor_to_json(NULL) == NULL);
}

int main(void) {
    test_telemetry_json_serialization();
    test_command_json_parsing();
    test_gateway_null_inputs();
    printf("All Week 2 gateway unit tests passed!\n");
    return 0;
}