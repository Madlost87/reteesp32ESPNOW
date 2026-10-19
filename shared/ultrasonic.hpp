#pragma once
// Included only for an explicitly enabled TX. No default GPIO assignments.
#include <driver/gpio.h>
#include "sensor_logic.hpp"
#ifndef SENSOR_TRIGGER_PIN
#error Define SENSOR_TRIGGER_PIN only after verifying the wiring
#endif
#ifndef SENSOR_ECHO_PIN
#error Define SENSOR_ECHO_PIN only after verifying the wiring
#endif
static_assert(GPIO_IS_VALID_OUTPUT_GPIO(SENSOR_TRIGGER_PIN), "Invalid trigger GPIO");
static_assert(GPIO_IS_VALID_GPIO(SENSOR_ECHO_PIN), "Invalid echo GPIO");
static_assert(SENSOR_TRIGGER_PIN!=SENSOR_ECHO_PIN, "Distinct GPIO required");
namespace ultrasonic {
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
volatile bool armed=false, rising=false, complete=false;
volatile uint32_t riseUs=0, widthUs=0;
bool waiting=false;
uint32_t startedUs=0,lastSampleMs=0;
cantiere::SensorAlarm alarm;
struct Diagnostics {
  uint32_t triggers=0,rises=0,falls=0,completed=0,noRise=0,noFall=0,lastWidth=0;
};
Diagnostics diagnostics;
Diagnostics snapshot() {
  portENTER_CRITICAL(&mux);Diagnostics copy=diagnostics;portEXIT_CRITICAL(&mux);
  return copy;
}
void IRAM_ATTR edge() {
  const uint32_t now=micros();
  const bool high=digitalRead(SENSOR_ECHO_PIN);
  portENTER_CRITICAL_ISR(&mux);
  if(high)++diagnostics.rises;else ++diagnostics.falls;
  if(armed) {
    if(high && !rising) {riseUs=now;rising=true;}
    else if(!high && rising) {widthUs=now-riseUs;complete=true;armed=false;}
  }
  portEXIT_CRITICAL_ISR(&mux);
}
void setup() {
  pinMode(SENSOR_TRIGGER_PIN,OUTPUT); digitalWrite(SENSOR_TRIGGER_PIN,LOW);
  pinMode(SENSOR_ECHO_PIN,INPUT);
  attachInterrupt(digitalPinToInterrupt(SENSOR_ECHO_PIN),edge,CHANGE);
}
void poll(uint32_t now) {
  if(waiting) {
    uint32_t width=0; bool done=false;
    portENTER_CRITICAL(&mux);
    if(complete) {width=widthUs;complete=false;done=true;++diagnostics.completed;diagnostics.lastWidth=width;}
    else if(uint32_t(micros()-startedUs)>=30000) {armed=false;done=true;if(rising)++diagnostics.noFall;else ++diagnostics.noRise;diagnostics.lastWidth=0;}
    portEXIT_CRITICAL(&mux);
    if(done) {waiting=false; alarm.observe(width,now);}
  }
  if(!waiting && cantiere::elapsed(now,lastSampleMs)>=100) {
    lastSampleMs=now;startedUs=micros();waiting=true;
    portENTER_CRITICAL(&mux);
    rising=false;complete=false;armed=true;++diagnostics.triggers;
    portEXIT_CRITICAL(&mux);
    // Only the 10us trigger pulse is synchronous; no waiting for the echo.
    digitalWrite(SENSOR_TRIGGER_PIN,HIGH);delayMicroseconds(10);digitalWrite(SENSOR_TRIGGER_PIN,LOW);
  }
}
}
