#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE 1024

#define PRIO_A 7
#define PRIO_C 5
#define PRIO_B 3
#define PRIO_Cooperative (-1)

void t_Cooperative_fn(void *p1, void *p2, void *p3)
{
    LOG_INF("[COOPERATIVE THREAD] Starting cooperative thread - will run 5 steps and then yield to other threads");
    for(int i=0; i<5; i++) {
        k_busy_wait(40000);
        LOG_INF("[COOP] step %d/5 - still holding CPU tick=%u", i+1, k_uptime_get_32());
    };
    LOG_INF("[COOPERATIVE THREAD] Finished cooperative thread - yielding to other threads");
    k_yield();
    LOG_INF("[COOPERATIVE THREAD] Done");
}

void t_low_fn(void *p1, void *p2, void *p3)
{
    LOG_INF("[LOW THREAD] started");
    for(int i=0; i<8; i++) {
        LOG_INF("[LOW] step %d/8 tick=%u", i+1, k_uptime_get_32());
        k_msleep(300);
    }
    LOG_INF("[LOW THREAD] Done");
}

void t_med_fn(void *p1, void *p2, void *p3)
{
    LOG_INF("[MEDIUM THREAD] started"); 
    for(int i=0; i<8; i++) {
        LOG_INF("[MEDIUM] step %d/8 tick=%u", i+1, k_uptime_get_32());  
        k_msleep(200);
    }
    LOG_INF("[MEDIUM THREAD] Done");
}

void t_high_fn(void *p1, void *p2, void *p3)
{
    LOG_INF("[HIGH THREAD] Starting high priority thread - will preempt LOW whenever Ready");
    for(int i=0; i<8; i++) {
        LOG_INF("[HIGH] step %d/8 tick=%u", i+1, k_uptime_get_32());
        k_msleep(100);
    }
    LOG_INF("[HIGH THREAD] Done");
}

K_THREAD_DEFINE(thread_a, STACK_SIZE, t_low_fn,
                NULL, NULL, NULL, PRIO_A, 0, 0);
K_THREAD_DEFINE(thread_b, STACK_SIZE, t_high_fn,
                NULL, NULL, NULL, PRIO_B, 0, 0);
K_THREAD_DEFINE(thread_c, STACK_SIZE, t_med_fn,
                NULL, NULL, NULL, PRIO_C, 0, 0);
K_THREAD_DEFINE(thread_d, STACK_SIZE, t_Cooperative_fn,
                NULL, NULL, NULL, PRIO_Cooperative, 0, 0);                

int main(void)
{
    LOG_INF("Zephyr Intermediate Demo - Cooperative Threading");
    LOG_INF("Thread A: LOW priority %d", PRIO_A);
    LOG_INF("Thread B: HIGH priority %d", PRIO_B);
    LOG_INF("Thread C: MEDIUM priority %d", PRIO_C);
    LOG_INF("Thread D: COOPERATIVE priority %d", PRIO_Cooperative);

    return 0;
}

