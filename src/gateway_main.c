#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include <MQTTClient.h>
#include "can_codec.h"
#include "can_socket.h"
#include "mqtt_gateway.h"

static volatile sig_atomic_t g_running = 1;
static int g_can_sock = -1;

static void handle_signal(int sig) {
    (void)sig;
    g_running = 0;
}

static void publish_json(MQTTClient client, const char *topic, char *payload, int retained) {
    if (payload == NULL) {
        return;
    }
    MQTTClient_message msg = MQTTClient_message_initializer;
    msg.payload = payload;
    msg.payloadlen = (int)strlen(payload);
    msg.qos = 0;
    msg.retained = retained;

    MQTTClient_deliveryToken token;
    int rc = MQTTClient_publishMessage(client, topic, &msg, &token);
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "[WARN] [GW] MQTT publish failed (%d) on %s\n", rc, topic);
    }
    free(payload);
}

static int on_mqtt_message(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    (void)context;
    (void)topicLen;

    char buf[256];
    int copy_len = message->payloadlen < (int)(sizeof(buf) - 1)
                       ? message->payloadlen
                       : (int)(sizeof(buf) - 1);
    if (copy_len > 0 && message->payload != NULL) {
        memcpy(buf, message->payload, (size_t)copy_len);
    }
    buf[copy_len] = '\0';

    uint8_t cmd = 0;
    if (gateway_parse_cmd_json(buf, &cmd)) {
        uint8_t raw[8];
        encode_cmd_frame(cmd, raw);
        if (can_socket_send(g_can_sock, CAN_ID_CMD, raw)) {
            printf("[INFO] [GW] MQTT cmd '%s' -> CAN 0x%03X (cmd=%u)\n",
                   buf, CAN_ID_CMD, (unsigned)cmd);
        } else {
            perror("[ERROR] [GW] Failed writing CAN command");
        }
    } else {
        fprintf(stderr, "[WARN] [GW] Ignored invalid JSON command on %s: %s\n", topicName, buf);
    }

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

static void on_connection_lost(void *context, char *cause) {
    (void)context;
    (void)cause;
    fprintf(stderr, "[WARN] [GW] MQTT connection lost: %s\n", cause ? cause : "unknown");
}

static void forward_can_frame(MQTTClient client, const char *id, const struct can_frame *frame) {
    char topic[128];
    ForkliftTelemetry telem;
    memset(&telem, 0, sizeof(telem));

    switch (frame->can_id & CAN_SFF_MASK) {
        case CAN_ID_MOTION:
            if (decode_motion_frame(frame->data, frame->can_dlc, &telem)) {
                snprintf(topic, sizeof(topic), "warehouse/forklift/%s/motion", id);
                publish_json(client, topic, gateway_motion_to_json(&telem), 0);
            }
            break;
        case CAN_ID_BATTERY:
            if (decode_battery_frame(frame->data, frame->can_dlc, &telem)) {
                snprintf(topic, sizeof(topic), "warehouse/forklift/%s/battery", id);
                publish_json(client, topic, gateway_battery_to_json(&telem), 0);
            }
            break;
        case CAN_ID_MOTOR:
            if (decode_motor_frame(frame->data, frame->can_dlc, &telem)) {
                snprintf(topic, sizeof(topic), "warehouse/forklift/%s/motor", id);
                publish_json(client, topic, gateway_motor_to_json(&telem), 0);
            }
            break;
        case CAN_ID_CMD_ACK:
            if (frame->can_dlc >= 2) {
                snprintf(topic, sizeof(topic), "warehouse/forklift/%s/ack", id);
                publish_json(client, topic,
                             gateway_ack_to_json(frame->data[0], frame->data[1] != 0), 0);
            }
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[]) {
    const char *can_if = (argc > 1) ? argv[1] : "vcan0";
    const char *broker_uri = (argc > 2) ? argv[2] : "tcp://192.168.50.2:1883";
    const char *forklift_id = (argc > 3) ? argv[3] : "FK001";

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    g_can_sock = can_socket_open(can_if);
    if (g_can_sock < 0) {
        return EXIT_FAILURE;
    }

    char client_id[64], topic_cmd[128], topic_status[128];
    snprintf(client_id, sizeof(client_id), "forklift_gw_%s", forklift_id);
    snprintf(topic_cmd, sizeof(topic_cmd), "warehouse/forklift/%s/cmd", forklift_id);
    snprintf(topic_status, sizeof(topic_status), "warehouse/forklift/%s/status", forklift_id);

    MQTTClient client;
    if (MQTTClient_create(&client, broker_uri, client_id,
                          MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTCLIENT_SUCCESS) {
        close(g_can_sock);
        return EXIT_FAILURE;
    }

    MQTTClient_setCallbacks(client, NULL, on_connection_lost, on_mqtt_message, NULL);

    MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;
    will_opts.topicName = topic_status;
    will_opts.message = "{\"online\":false}";
    will_opts.retained = 1;
    will_opts.qos = 1;

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.will = &will_opts;

    if (MQTTClient_connect(client, &conn_opts) != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "[ERROR] [GW] Failed to connect to broker %s\n", broker_uri);
        MQTTClient_destroy(&client);
        close(g_can_sock);
        return EXIT_FAILURE;
    }

    MQTTClient_subscribe(client, topic_cmd, 1);
    publish_json(client, topic_status, strdup("{\"online\":true}"), 1);
    printf("[INFO] [GW] Gateway active (%s <-> %s, ID=%s)\n", can_if, broker_uri, forklift_id);

    while (g_running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(g_can_sock, &read_fds);
        struct timeval tv = {0, 200000};

        int sel = select(g_can_sock + 1, &read_fds, NULL, NULL, &tv);
        if (sel < 0 && errno != EINTR) {
            break;
        }
        if (sel > 0) {
            struct can_frame frame;
            if (read(g_can_sock, &frame, sizeof(frame)) == (ssize_t)sizeof(frame)) {
                forward_can_frame(client, forklift_id, &frame);
            }
        }
    }

    publish_json(client, topic_status, strdup("{\"online\":false}"), 1);
    MQTTClient_disconnect(client, 1000);
    MQTTClient_destroy(&client);
    close(g_can_sock);
    return EXIT_SUCCESS;
}