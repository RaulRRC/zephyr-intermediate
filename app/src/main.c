//Based from L2 Demo 2: Mutex Protection https://github.com/iomico-public/zephyr-intermediate/blob/l2-demo2/app/src/main.c
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(l2task1, LOG_LEVEL_DBG);

#define STACK_SIZE      1024
#define PRIOA            5
#define PRIOB            10
#define INCREMENTS      1000000   /* each thread increments this many times */

/* Shared state - intentionally unprotected */
static volatile uint32_t counter, counter2;


static struct k_sem done_sem;

static K_MUTEX_DEFINE(counter_mutex);

void worker_fn_WithoutMutex(void *p1, void *p2, void *p3)
{
    const char *name = k_thread_name_get(k_current_get());

    for (int i = 0; i < INCREMENTS; i++) {
        uint32_t tmp = counter;
        k_yield();
        counter = tmp + 1;
    }
    LOG_INF("[%s] finished", name);
    k_sem_give(&done_sem);
}

void worker_fn_WithMutex(void *p1, void *p2, void *p3)
{
    const char *name = k_thread_name_get(k_current_get());

    for (int i = 0; i < INCREMENTS; i++) {
        k_mutex_lock(&counter_mutex, K_FOREVER);
        uint32_t tmp = counter2;
        k_yield();
        counter2 = tmp + 1;
        k_mutex_unlock(&counter_mutex);
    }

    LOG_INF("[%s] finished", name);
    k_sem_give(&done_sem);
}

K_THREAD_DEFINE(worker_a, STACK_SIZE, worker_fn_WithoutMutex, NULL, NULL, NULL,
                PRIOA, 0, 0);
K_THREAD_DEFINE(worker_b, STACK_SIZE, worker_fn_WithoutMutex, NULL, NULL, NULL,
                PRIOA, 0, 0);
                
K_THREAD_DEFINE(worker_c, STACK_SIZE, worker_fn_WithMutex, NULL, NULL, NULL,
                PRIOB, 0, 5000);
K_THREAD_DEFINE(worker_d, STACK_SIZE, worker_fn_WithMutex, NULL, NULL, NULL,
                PRIOB, 0, 5000);

int main(void)
{
    k_sem_init(&done_sem, 0, 2);
    int64_t time = k_uptime_get();

    LOG_INF("Assigment without Mutex Protection: Expected race condition ===");
    LOG_INF("Expected final value: %d", INCREMENTS * 2);

    /* Wait for both workers to complete */
    k_sem_take(&done_sem, K_FOREVER);
    k_sem_take(&done_sem, K_FOREVER);

    LOG_INF("Actual  final value: %u", counter);

    if (counter == INCREMENTS * 2) {
        LOG_WRN("No race this run");
    } else {
        LOG_ERR("Race condition confirmed: lost %d updates",
                (INCREMENTS * 2) - counter);
    }
    LOG_INF("Execution time: %lld ms", k_uptime_delta(&time));

    /* Reset counter and Enable Mutex for the next test */

    int64_t time2 = k_uptime_get();

    LOG_INF("Assigment with Mutex Protection: Expected no race condition ===");
    LOG_INF("Expected final value: %d", INCREMENTS * 2);

    // /* Wait for both workers to complete */
    k_sem_take(&done_sem, K_FOREVER);
    k_sem_take(&done_sem, K_FOREVER);

    LOG_INF("Actual  final value: %u", counter2);

    if (counter2 == INCREMENTS * 2) {
        LOG_WRN("No race this run");
    } else {
        LOG_ERR("Race condition confirmed: lost %d updates",
                (INCREMENTS * 2) - counter2);
    }
    LOG_INF("Execution time: %lld ms", k_uptime_delta(&time2));


    return 0;
}
