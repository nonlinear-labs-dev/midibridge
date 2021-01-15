#pragma once

#include <stdint.h>

typedef enum
{
  UNPOWERED,
  POWERED,
  OFFLINE,
  ONLINE,
  DROPPED_INCOMING,
  PACKET_START,
  PACKET_DELIVERED,
  PACKET_DROPPED,
} MonitorEvent_t;

void SMON_monitorEvent(uint8_t const port, MonitorEvent_t const event);
void SMON_Process(void);
