/**
 * \file demo.c
 *
 * \brief Code for operating the Raspberry Pi demo at NetSys 2021
 **/

#include "feature/split/demo.h"

#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include "app/config/config.h"
#include "lib/evloop/timers.h"
#include "lib/log/log.h"
#include "lib/log/util_bug.h"

#if defined(ENABLE_DEMO_RPI)
#include <gpiod.h>
#endif /* defined(ENABLE_DEMO_RPI) */

/**** Defines ****/
#define DEMO_CONSUMER       "tor_demo"
#define DEMO_GPIO_CHIPNAME  "gpiochip0"
#define DEMO_NUM_SUBCIRCS   3
#define DEMO_BLINK_DURATION 100L //usec
#define DEMO_CELL_INTERVAL  100
#define DEMO_LINE_LED0_FWD  14
#define DEMO_LINE_LED0_BWD  4
#define DEMO_LINE_LED1_FWD  18
#define DEMO_LINE_LED1_BWD  17
#define DEMO_LINE_LED2_FWD  23
#define DEMO_LINE_LED2_BWD  22

/**** Private definitions ****/
struct demo_led_control {
#if defined(ENABLE_DEMO_RPI)
  struct gpiod_line_bulk lines;
#endif /* defined(ENABLE_DEMO_RPI) */

#if defined(ENABLE_DEMO_RPI) || defined(ENABLE_DEMO_TEST)
  tor_timer_t *timers[DEMO_NUM_SUBCIRCS];
#endif /* defined(ENABLE_DEMO_...) */

  unsigned int count[DEMO_NUM_SUBCIRCS];
};

/**** Global Variables ****/
static bool demo_initialized = false;
static struct demo_led_control fwd_leds = {0};
static struct demo_led_control bwd_leds = {0};

#if defined(ENABLE_DEMO_RPI)
static struct gpiod_chip *chip = NULL;
#endif /* defined(ENABLE_DEMO_RPI) */

static const struct timeval blink_duration = {
    .tv_sec = 0,
    .tv_usec = DEMO_BLINK_DURATION,
};


/**** Function definitions ****/

static void led_timer_cb(tor_timer_t *timer, void *args, const struct monotime_t *t)
{
  (void) timer; (void)t;
  tor_assert(args);

#if defined(ENABLE_DEMO_RPI)

  struct gpiod_line *line = (struct gpiod_line *)args;

  if (gpiod_line_set_value(line, 0) < 0) {
    log_err(LD_GENERAL, "Error setting GPIO %u to 0: %s",
            gpiod_line_offset(line), strerror(errno));
  }

#elif defined(ENABLE_DEMO_TEST)

  char *line = (char *)args;

  log_warn(LD_GENERAL, "Timeout: %s off", line);

#endif /* defined(ENABLE_DEMO_...) */
}

int demo_init(void)
{
  const or_options_t* options = get_options();

  if (options->DisableDemo) {
    log_warn(LD_GENERAL, "Demo code has been disabled via the 'DisableDemo' option");
    return 0;
  }

  log_notice(LD_GENERAL, "Initializing demo code...");

#if defined(ENABLE_DEMO_RPI)

  // get GPIO chip handle
  chip = gpiod_chip_open_by_name(DEMO_GPIO_CHIPNAME);
  if (!chip) {
    log_err(LD_GENERAL, "Error while opening the GPIO chip %s: %s",
        DEMO_GPIO_CHIPNAME, strerror(errno));
    return -1;
  }

  // get forward line handles
  unsigned int fwd_line_nums[] = {DEMO_LINE_LED0_FWD, DEMO_LINE_LED1_FWD, DEMO_LINE_LED2_FWD};
  if (gpiod_chip_get_lines(chip, fwd_line_nums, DEMO_NUM_SUBCIRCS, &fwd_leds.lines) < 0) {
    log_err(LD_GENERAL, "Error while getting forward lines: %s",
        strerror(errno));
    return -1;
  }

  //get backward line handles
  unsigned int bwd_line_nums[] = {DEMO_LINE_LED0_BWD, DEMO_LINE_LED1_BWD, DEMO_LINE_LED2_BWD};
  if (gpiod_chip_get_lines(chip, bwd_line_nums, DEMO_NUM_SUBCIRCS, &bwd_leds.lines) < 0) {
    log_err(LD_GENERAL, "Error while getting backward lines: %s",
        strerror(errno));
    return -1;
  }

  // request lines
  int default_vals[DEMO_NUM_SUBCIRCS] = {0};
  if (gpiod_line_request_bulk_output(&fwd_leds.lines, DEMO_CONSUMER, default_vals) < 0 ||
      gpiod_line_request_bulk_output(&bwd_leds.lines, DEMO_CONSUMER, default_vals) < 0) {
    log_err(LD_GENERAL, "Error while requesting lines as output: %s",
        strerror(errno));
    return -1;
  }

  // create timers for each LED
  for (unsigned int i = 0; i < DEMO_NUM_SUBCIRCS; i++) {
    fwd_leds.timers[i] = timer_new(led_timer_cb, gpiod_line_bulk_get_line(&fwd_leds.lines, i));
    bwd_leds.timers[i] = timer_new(led_timer_cb, gpiod_line_bulk_get_line(&bwd_leds.lines, i));
  }

#elif defined(ENABLE_DEMO_SERVER)

  (void)&led_timer_cb;

#elif defined(ENABLE_DEMO_TEST)

  fwd_leds.timers[0] = timer_new(led_timer_cb, "LED0, forward");
  fwd_leds.timers[1] = timer_new(led_timer_cb, "LED1, forward");
  fwd_leds.timers[2] = timer_new(led_timer_cb, "LED2, forward");
  bwd_leds.timers[0] = timer_new(led_timer_cb, "LED0, backward");
  bwd_leds.timers[1] = timer_new(led_timer_cb, "LED1, backward");
  bwd_leds.timers[2] = timer_new(led_timer_cb, "LED2, backward");

#endif /* defined(ENABLE_DEMO_...) */

  demo_initialized = true;
  log_notice(LD_GENERAL, "Initializing demo code... Success!");
  return 0;
}

void demo_exit(void)
{
#if defined(ENABLE_DEMO_RPI) || defined(ENABLE_DEMO_TEST)
  // free timers
  for (int i = 0; i < DEMO_NUM_SUBCIRCS; i++) {
    timer_free(fwd_leds.timers[i]);
    timer_free(bwd_leds.timers[i]);
  }
#endif /* defined(ENABLE_DEMO_...) */

#if defined(ENABLE_DEMO_RPI)
  // release reserved lines
  gpiod_line_release_bulk(&fwd_leds.lines);
  gpiod_line_release_bulk(&bwd_leds.lines);

  // close GPIO chip
  if (chip)
    gpiod_chip_close(chip);
#endif /* defined(ENABLE_DEMO_RPI) */
}

void demo_register_cell(subcirc_id_t subcirc, cell_direction_t direction, bool is_split_circuit)
{
  struct demo_led_control *leds;
  const char *direction_str;

  if (!demo_initialized)
    return;

  if (subcirc >= DEMO_NUM_SUBCIRCS)
    return;

  switch (direction) {
    case CELL_DIRECTION_OUT:
      leds = &fwd_leds;
      direction_str = "forward";
      break;
    case CELL_DIRECTION_IN:
      leds = &bwd_leds;
      direction_str = "backward";
      break;
    default:
      tor_assert_unreached();
  }

  // only handle every DEMO_CELL_INTERVAL-th cell per sub-circuit
  if (leds->count[subcirc] != 0) {
#if defined(ENABLE_DEMO_TEST)
    log_warn(LD_GENERAL, "LED%u, %s (not handled)", subcirc, direction_str);
#endif /* defined(ENABLE_DEMO_TEST) */
    return;
  }
  leds->count[subcirc] = (leds->count[subcirc] + 1) % DEMO_CELL_INTERVAL;

#if defined(ENABLE_DEMO_RPI)

  struct gpiod_line *line = gpiod_line_bulk_get_line(&leds->lines, subcirc);
  (void)direction_str;
  (void)is_split_circuit;

  // turn on LED
  if (gpiod_line_set_value(line, 1) < 0) {
    log_err(LD_GENERAL, "Error setting GPIO %u to 1: %s",
        gpiod_line_offset(line), strerror(errno));
  }

#elif defined(ENABLE_DEMO_SERVER)

  if (is_split_circuit) {
    log_notice(LD_DEMO, "%s %s relay cell on sub-circuit %u",
               direction == CELL_DIRECTION_OUT ? "Merge" : "Split",
               direction_str, subcirc);
  }

#elif defined(ENABLE_DEMO_TEST)

  log_warn(LD_GENERAL, "LED%u, %s %s", subcirc, direction_str,
           is_split_circuit ? "(split circuit)": "");

#endif /* defined(ENABLE_DEMO_...) */


#if defined(ENABLE_DEMO_RPI) || defined(ENABLE_DEMO_TEST)
  // schedule timer for turning LED off again
  timer_schedule(leds->timers[subcirc], &blink_duration);
#else
  (void)blink_duration;
#endif /* defined(ENABLE_DEMO_...) */
}

