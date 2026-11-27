#define _DEFAULT_SOURCE

#include "can_socket.h"
#include <linux/can/raw.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

int can_socket_open(const char *ifname) {
    if (ifname == NULL) {
        return -1;
    }

    int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        perror("socket(PF_CAN)");
        return -1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl(SIOCGIFINDEX)");
        close(sock);
        return -1;
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind(AF_CAN)");
        close(sock);
        return -1;
    }
    return sock;
}

bool can_socket_send(int sock, canid_t id, const uint8_t data[8]) {
    if (sock < 0 || data == NULL) {
        return false;
    }
    struct can_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.can_id = id;
    frame.can_dlc = 8;
    memcpy(frame.data, data, 8);

    return write(sock, &frame, sizeof(frame)) == (ssize_t)sizeof(frame);
}