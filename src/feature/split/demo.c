/**
 * \file demo.c
 *
 * \brief Code for operating the Raspberry Pi demo at NetSys 2021
 **/

#include "feature/split/demo.h"

#include <gpiod.h>
#include <errno.h>
#include <string.h>

#include "lib/evloop/timers.h"
#include "lib/log/log.h"
#include "lib/log/util_bug.h"

/**** Defines ****/
#define DEMO_CONSUMER       "tor_demo"
#define DEMO_GPIO_CHIPNAME  "gpiochip0"
#define DEMO_NUM_SUBCIRCS   3
#define DEMO_BLINK_DURATION 1000L //usec
#define DEMO_LINE_LED0_FWD  14
#define DEMO_LINE_LED0_BWD  4
#define DEMO_LINE_LED1_FWD  18
#define DEMO_LINE_LED1_BWD  17
#define DEMO_LINE_LED2_FWD  23
#define DEMO_LINE_LED2_BWD  22

/**** Private definitions ****/
struct demo_led_control {
  struct gpiod_line_bulk lines;
  tor_timer_t *timers[DEMO_NUM_SUBCIRCS];
};

/**** Global Variables ****/
struct gpiod_chip *chip = NULL;
struct demo_led_control fwd_leds = {0};
struct demo_led_control bwd_leds = {0};

const struct timeval blink_duration = {
    .tv_sec = 0,
    .tv_usec = DEMO_BLINK_DURATION,
};


/**** Function definitions ****/

static void led_timer_cb(tor_timer_t *timer, void *args, const struct monotime_t *t)
{
  struct gpiod_line *line;
  (void) timer; (void)t;

  tor_assert(args);
  line = (struct gpiod_line *)args;

  if (gpiod_line_set_value(line, 0) < 0) {
    log_err(LD_GENERAL, "Error setting GPIO %u to 0: %s",
            gpiod_line_offset(line), strerror(errno));
  }
}

int demo_init(void)
{
  log_notice(LD_GENERAL, "Initializing demo code...");

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

  log_notice(LD_GENERAL, "Initializing demo code... Success!");
  return 0;
}

void demo_exit(void)
{
  // free timers
  for (int i = 0; i < DEMO_NUM_SUBCIRCS; i++) {
    timer_free(fwd_leds.timers[i]);
    timer_free(bwd_leds.timers[i]);
  }

  // release reserved lines
  gpiod_line_release_bulk(&fwd_leds.lines);
  gpiod_line_release_bulk(&bwd_leds.lines);

  // close GPIO chip
  if (chip)
    gpiod_chip_close(chip);
}

void demo_register_cell(subcirc_id_t subcirc, cell_direction_t direction)
{
  struct demo_led_control *leds;
  struct gpiod_line *line;

  if (subcirc >= DEMO_NUM_SUBCIRCS)
    return;

  switch (direction) {
    case CELL_DIRECTION_OUT:
      leds = &fwd_leds;
      break;
    case CELL_DIRECTION_IN:
      leds = &bwd_leds;
      break;
    default:
      tor_assert_unreached();
  }

  line = gpiod_line_bulk_get_line(&leds->lines, subcirc);

  // turn on LED
  if (gpiod_line_set_value(line, 1) < 0) {
    log_err(LD_GENERAL, "Error setting GPIO %u to 1: %s",
        gpiod_line_offset(line), strerror(errno));
  }

  // schedule timer for turning LED off again
  timer_schedule(leds->timers[subcirc], &blink_duration);
}

