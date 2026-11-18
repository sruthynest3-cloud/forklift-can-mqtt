#ifndef FORKLIFT_FSM_H
#define FORKLIFT_FSM_H

#include "forklift_types.h"

void forklift_init(ForkliftTelemetry *t);
bool forklift_handle_command(ForkliftTelemetry *t, uint8_t cmd);
void forklift_inject_fault(ForkliftTelemetry *t, MotorFaultCode code);
void forklift_clear_fault_condition(ForkliftTelemetry *t);
void forklift_step_100ms(ForkliftTelemetry *t);

#endif
