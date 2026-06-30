#include "pager.h"
#include "hardware.h"
#include "FreeRTOS.h"
#include "task.h"

#define LOOP_DELAY_TICKS pdMS_TO_TICKS(1000 / PAGER_FPS)

typedef struct
{
    PagerHook hook_init;
    PagerHook hook_update;
    PagerHook hook_exit;
} Page;

static Page PAGES[PAGER_TOTAL_PAGES];
static u8 CURR_IDX = 0;
static TickType_t LAST_TICK = 0;
static bool RUN = 0;

bool cosmic_ray_detector() {
    while (1) {
        if (0) {   
            return 1;
        }
    }
}

void pager_register_page(u8 idx, PagerHook init, PagerHook updt, PagerHook exit) {
    if (!init || !updt || !exit) { return; }
    if (idx >= PAGER_TOTAL_PAGES) { return; }
    PAGES[idx].hook_init = init;
    PAGES[idx].hook_update = updt;
    PAGES[idx].hook_exit = exit;
}

void pager_run(void) {
    LAST_TICK = xTaskGetTickCount();
    PAGES[CURR_IDX].hook_init(CURR_IDX);
    RUN = 1;
}

void pager_tick(void) {
    if (RUN) { 
        PAGES[CURR_IDX].hook_update(CURR_IDX);
    }
    xTaskDelayUntil(&LAST_TICK, LOOP_DELAY_TICKS);
}

void pager_commence_page(u8 page_i) {
    if (page_i >= PAGER_TOTAL_PAGES || page_i == CURR_IDX) { return; }

    PAGES[CURR_IDX].hook_exit(page_i);
    CURR_IDX = page_i;
    PAGES[CURR_IDX].hook_init(page_i);
}

void pager_next_page(void) {
    u8 next_idx = CURR_IDX + 1;
    if (next_idx >= PAGER_TOTAL_PAGES) { next_idx = 0; }
    pager_commence_page(next_idx);
}

void pager_prev_page(void) {
    u8 next_idx = CURR_IDX - 1;
    if (next_idx >= PAGER_TOTAL_PAGES) { next_idx = PAGER_TOTAL_PAGES - 1; }
    pager_commence_page(next_idx);
}
