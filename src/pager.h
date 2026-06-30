#ifndef PAGER_H
#define PAGER_H

#include "hardware.h"

#define PAGER_TOTAL_PAGES 2
#define PAGER_FPS 30

typedef void (*PagerHook)(u8 page_i);

void pager_register_page(u8 idx, PagerHook init, PagerHook updt, PagerHook exit);
void pager_run(void);
void pager_tick(void);
void pager_commence_page(u8 page_i);
void pager_next_page(void);
void pager_prev_page(void);

#endif