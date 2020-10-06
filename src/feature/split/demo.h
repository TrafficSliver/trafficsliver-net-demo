/**
 * \file demo.h
 *
 * \brief Headers for demo.c
 **/

#ifndef TOR_DEMO_H
#define TOR_DEMO_H

#include <stdbool.h>

#include "orconfig.h"
#include "core/or/or.h"
#include "feature/split/splitdefines.h"
#include "lib/log/log.h"

/* ENABLE_DEMO is automatically defined by ./configure
 * when --enable-demo option is passed */
#ifdef ENABLE_DEMO
int demo_init(void);
void demo_exit(void);
void demo_register_cell(subcirc_id_t subcirc, cell_direction_t direction, bool is_split_circuit);
#define demo_register_setup(...)          log_notice(LD_DEMO, __VA_ARGS__)
#define demo_register_instruction(...)    log_notice(LD_DEMO, __VA_ARGS__)
#else /* ENABLE_DEMO */
static inline int demo_init(void)
{
  return 0;
}

static inline void demo_exit(void)
{
  return;
}

static inline void demo_register_cell(subcirc_id_t subcirc, cell_direction_t direction, bool is_split_circuit))
{
  (void)subcirc; (void)direction; (void)is_split_circuit; return;
}

#define demo_register_setup(...)        //no-op
#define demo_register_instruction(...)  //no-op
#endif

#endif /* TOR_DEMO_H */
