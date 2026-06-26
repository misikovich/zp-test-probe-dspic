#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "src/project_clock.h"

/* Scheduler */
#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      PROJECT_FCY_HZ
#define configTICK_RATE_HZ                      1000U       /* 1 ms tick */
#define configMAX_PRIORITIES                    5
#define configMINIMAL_STACK_SIZE                200U        /* 16-bit words per task */
#define configMAX_TASK_NAME_LEN                 12
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   1
#define configUSE_MUTEXES                       0
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           0
#define configQUEUE_REGISTRY_SIZE               0
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIME_SLICING                  1
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0

/* 1 = TICK_TYPE_WIDTH_32_BITS (portmacro.h uses this numeric value) */
#define configTICK_TYPE_WIDTH_IN_BITS           1

/* Memory */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 6 * 1024 ) )
#define configAPPLICATION_ALLOCATED_HEAP        0

/* Hook functions */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* Stats */
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/* Co-routines */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         1

/* Software timers — disabled; saves ~1 KB */
#define configUSE_TIMERS                        0
#define configTIMER_TASK_PRIORITY               2
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            configMINIMAL_STACK_SIZE

/* Interrupt priority — port.c hardcodes T1IP = this value */
#define configKERNEL_INTERRUPT_PRIORITY         1

/* Assert: halt on failure */
#define configASSERT( x )  if( ( x ) == 0 ) { __builtin_disable_interrupts(); for(;;); }

/* INCLUDE_* */
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelete                     0
#define INCLUDE_vTaskSuspend                    0
#define INCLUDE_vTaskPrioritySet                0
#define INCLUDE_uxTaskPriorityGet               0
#define INCLUDE_xTaskGetSchedulerState          0
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     0
#define INCLUDE_xTaskAbortDelay                 0

#endif /* FREERTOS_CONFIG_H */
