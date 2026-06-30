#include "pager.h"
#include "hardware.h"
#include "display_hw.h"

#define PAGER_TOTAL_PAGES 2

typedef struct
{
    PagerHook hook_init;
    PagerHook hook_update;
    PagerHook hook_exit;
} Page;


static const Page PAGES[PAGER_TOTAL_PAGES] = {
    { 0, 0, 0 },
    { 0, 0, 0 }
};

static u8 PAGER_CURRENT_INDEX = 0;

void pager_commence_page(u8 page_i) {
    if (page_i < 0 || page_i >= PAGER_TOTAL_PAGES || page_i == PAGER_CURRENT_INDEX) { return; }

    PAGES[PAGER_CURRENT_INDEX].hook_exit(page_i);
    PAGER_CURRENT_INDEX = page_i;
    PAGES[PAGER_CURRENT_INDEX].hook_init(page_i);
}

void pager_next_page() {
    u8 next_idx = PAGER_CURRENT_INDEX + 1;
    if (next_idx >= PAGER_TOTAL_PAGES) { next_idx = 0; }
    pager_commence_page(next_idx);
}

void pager_prev_page() {
    u8 next_idx = PAGER_CURRENT_INDEX - 1;
    if (next_idx >= PAGER_TOTAL_PAGES) { next_idx = 0; }
    pager_commence_page(next_idx);
}
