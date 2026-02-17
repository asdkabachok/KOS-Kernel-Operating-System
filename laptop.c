// laptop.c — Laptop Embedded Controller stub
#include "kernel.h"

void laptop_ec_init(void) {
    kprintf("EC: Initializing...\n");
    // Would communicate with EC via ACPI or I/O ports
}

uint8_t laptop_get_cpu_temp(void) {
    // Would read from EC or CPU temperature sensors
    return 45; // Fake temperature
}

uint16_t laptop_get_fan_rpm(void) {
    // Would read from EC
    return 2400; // Fake RPM
}

void laptop_set_fan_speed(uint8_t percent) {
    // Would write to EC to control fan
    (void)percent;
}

void laptop_thermal_init(void) {
    kprintf("Thermal: Initialized (stub)\n");
}

void laptop_thermal_poll(void) {
    // Would poll temperature and adjust fan speed
}
