/**
 * \file demo.h
 *
 * \brief Headers for demo.c
 **/

#ifndef TOR_DEMO_H
#define TOR_DEMO_H

#include "orconfig.h"
#include "core/or/or.h"
#include "feature/split/splitdefines.h"

/* ENABLE_DEMO is automatically defined by ./configure
 * when --enable-demo option is passed */
#ifdef ENABLE_DEMO
int demo_init(void);
void demo_exit(void);
void demo_register_cell(subcirc_id_t subcirc, cell_direction_t direction);
#else /* ENABLE_DEMO */
static inline int demo_init(void)
{
  return 0;
}

static inline void demo_exit(void)
{
  return;
}

static inline void demo_register_cell(subcirc_id_t subcirc, cell_direction_t direction)
{
  (void)subcirc; (void)direction; return;
}
#endif

#endif /* TOR_DEMO_H */
