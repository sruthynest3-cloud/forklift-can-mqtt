#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "forklift_fsm.h"
#include "can_codec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

static volatile sig_atomic_t running = 1;
static void on_sigint(int sig) { (void)sig; running = 0; }

static void log_event(const char *level, const char *msg) {
    time_t now = time(NULL);
    struct tm tm_info;
    char ts[24];
    localtime_r(&now, &tm_info);
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm_info);
    printf("[%s] [%s] [SIM] %s\n", ts, level, msg);
    fflush(stdout);
}

static int open_can_socket(const char *ifname) {
    int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) return -1;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IF_NAMESIZE- 1);
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) { close(s); return -1; }
    struct sockaddr_can addr = { .can_family = AF_CAN, .can_ifindex = ifr.ifr_ifindex };
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) { close(s); return -1; }
    fcntl(s, F_SETFL, O_NONBLOCK);
    return s;
}

static void send_can(int sock, canid_t id, const uint8_t data[8]) {
    struct can_frame frame = { .can_id = id, .can_dlc = 8 };
    memcpy(frame.data, data, 8);
    (void)write(sock, &frame, sizeof(frame));
}

int main(int argc, char **argv) {
    const char *ifname = (argc > 1) ? argv[1] : "vcan0";
    signal(SIGINT, on_sigint);
    signal(SIGTERM, on_sigint);

    int sock = open_can_socket(ifname);
    if (sock < 0) {
        log_event("ERROR", "Failed to open CAN interface");
        return 1;
    }

    ForkliftTelemetry telem;
    forklift_init(&telem);
    log_event("INFO", "Electric Forklift Simulator started on vcan0");

    uint32_t tick = 0;
    while (running) {
        struct can_frame rx;
        while (read(sock, &rx, sizeof(rx)) > 0) {
            if (rx.can_id == CAN_ID_CMD && rx.can_dlc == 8) {
                uint8_t cmd = rx.data[0];
                bool ok = forklift_handle_command(&telem, cmd);
                uint8_t ack[8];
                encode_ack_frame(cmd, ok, ack);
                send_can(sock, CAN_ID_CMD_ACK, ack);
                log_event(ok ? "INFO" : "WARN", ok ? "Command accepted" : "Command rejected");
            }
        }

        forklift_step_100ms(&telem);
        uint8_t tx[8];
        encode_motion_frame(&telem, tx);
        send_can(sock, CAN_ID_MOTION, tx); /* 100 ms */

        if (tick % 5 == 0) {
            telem.battery_counter++;
            encode_battery_frame(&telem, tx);
            send_can(sock, CAN_ID_BATTERY, tx); /* 500 ms */
        }
        if (tick % 10 == 0) {
            encode_motor_frame(&telem, tx);
            send_can(sock, CAN_ID_MOTOR, tx); /* 1000 ms[cite: 1] */
        }
        tick++;
        sleep(100000);
    }

    close(sock);
    log_event("INFO", "Graceful shutdown complete");
    return 0;
}
