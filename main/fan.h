#pragma once

#include <cstdint>

void fan_init();

uint8_t fan_get_percentage();

void fan_set_power(bool val);

void fan_set_percentage(uint8_t val);