#ifndef PAGER_H
#define PAGER_H

#include "hardware.h"

typedef void (*PagerHook)(u8 page_i);

void pager_commence_page(u8 page_i);
void pager_next_page();
void pager_prev_page();

#endif