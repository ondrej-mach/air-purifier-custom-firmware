#include <cstdint>

void led_init();

void led_rgb_set(uint32_t red, uint32_t green, uint32_t blue);

void led_rgb_fade(uint32_t red, uint32_t green, uint32_t blue, int fade_time_ms);
