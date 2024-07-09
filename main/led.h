#include <cstdint>

#define LED_IND_WARNING (1<<0)
#define LED_IND_WIFI    (1<<1)
#define LED_IND_LOCK    (1<<2)
#define LED_IND_AUTO    (1<<3)
#define LED_IND_NIGHT   (1<<4)
#define LED_IND_HEART   (1<<5)




void led_init();

void led_rgb_set(uint32_t red, uint32_t green, uint32_t blue);

void led_rgb_fade(uint32_t red, uint32_t green, uint32_t blue, int fade_time_ms);
