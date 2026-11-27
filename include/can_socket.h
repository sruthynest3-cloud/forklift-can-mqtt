#ifndef CAN_SOCKET_H
#define CAN_SOCKET_H

#include <linux/can.h>
#include <stdbool.h>
#include <stdint.h>

/* Open and bind a raw SocketCAN interface (e.g., "vcan0"). Returns fd or -1. */
int can_socket_open(const char *ifname);

/* Transmit an 8-byte CAN frame. Returns true on success. */
bool can_socket_send(int sock, canid_t id, const uint8_t data[8]);

#endif