#ifndef FORKLIFT_TYPES_H
#define FORKLIFT_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* Classical CAN 11-bit Identifiers */
#define CAN_ID_MOTION   0x100U
#define CAN_ID_BATTERY  0x101U
#define CAN_ID_MOTOR    0x102U
#define CAN_ID_CMD      0x200U
#define CAN_ID_CMD_ACK  0x201U

typedef enum {
    STATE_OFF = 0,
    STATE_IDLE = 1,
    STATE_MOVING = 2,
    STATE_CHARGING = 3,
    STATE_FAULT = 4
} VehicleState;

typedef enum {
    DIR_NEUTRAL = 0,
    DIR_FORWARD = 1,
    DIR_REVERSE = 2
} Direction;

typedef enum {
    BAT_NORMAL = 0,
    BAT_LOW = 1,
    BAT_CHARGING = 2,
    BAT_FAULT = 3
} BatteryState;

typedef enum {
    FAULT_NONE = 0,
    FAULT_OVERTEMP = 1,
    FAULT_OVERCURRENT = 2,
    FAULT_SENSOR = 3
} MotorFaultCode;

typedef enum {
    CMD_START = 1,
    CMD_STOP = 2,
    CMD_RESET_FAULT = 3
} ForkliftCommand;

typedef struct {
    VehicleState state;
    Direction direction;
    float speed_kmh;        /* 0.0 to 15.0 km/h */
    uint16_t motor_rpm;     /* 0 to 3000 RPM */
    uint8_t drive_mode;     /* 0=ECO, 1=NORMAL, 2=POWER */
    uint8_t motion_counter;

    float battery_voltage;  /* 40.0 to 55.0 V */
    float battery_current;  /* + discharge, - charge (A) */
    uint8_t battery_soc;    /* 0 to 100 % */
    int8_t battery_temp;    /* 20 to 60 C */
    BatteryState bat_state;
    uint8_t battery_counter;

    int8_t motor_temp;      /* 20 to 120 C */
    int8_t controller_temp; /* 20 to 100 C */
    uint8_t motor_torque;   /* 0 to 100 % */
    MotorFaultCode fault_code;
} ForkliftTelemetry;

#endif
