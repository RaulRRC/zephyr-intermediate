#include <zephyr/kernel.h>
#include <zephyr/task_wdt/task_wdt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE 1024

/* ================================================================== */
/*  Shared message type                                               */
/* ================================================================== */

struct sensor_msg {
    uint32_t timestamp_ms;
    int32_t value;
    uint8_t seq;
};


/* Watchdog callback*/
void my_callback(int channel_id, void *user_data)
{
    k_tid_t thread = (k_tid_t)user_data;

    LOG_ERR("Thread %s missed watchdog feed!", k_thread_name_get(thread));
}

/* ================================================================== */
/*  Thread-to-thread pipeline                                */
/* ================================================================== */
int chan;

#define P1_QUEUE_DEPTH 15
#define P1_COUNT       30

K_MSGQ_DEFINE(p1_q, sizeof(struct sensor_msg), P1_QUEUE_DEPTH, 4);

static K_SEM_DEFINE(p1_prod_done, 0, 1);
static K_SEM_DEFINE(p1_cons_done, 0, 1);


static volatile bool p1_prod_finished;

static void p1_producer(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    k_thread_name_set(k_current_get(), "p1_prod");

    for (int i = 0; i < P1_COUNT; i++) {
        struct sensor_msg msg = {
            .timestamp_ms = k_uptime_get_32(),
            .value = 100 + i,
            .seq = (uint8_t)i,
        };

        int ret = k_msgq_put(&p1_q, &msg, K_MSEC(500));
        if (ret == 0) {
            LOG_INF("[P1-PROD] sent seq=%u val=%d q=%u/%d",
                    msg.seq,
                    msg.value,
                    k_msgq_num_used_get(&p1_q),
                    P1_QUEUE_DEPTH);

        } else {
            LOG_WRN("[P1-PROD] put failed ret=%d", ret);
        }
        
        k_msleep(75);
    }

    p1_prod_finished = true;
    LOG_INF("[P1-PROD] done");
    k_sem_give(&p1_prod_done);
}

static void p1_consumer(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    k_thread_name_set(k_current_get(), "p1_cons");

    while (true) {
        struct sensor_msg msg = {0};

        int ret = k_msgq_get(&p1_q, &msg, K_MSEC(300));
        if (ret != 0) {
            if (p1_prod_finished && k_msgq_num_used_get(&p1_q) == 0) {
                break;
            }

            LOG_WRN("[P1-CONS] timeout waiting for message");
            continue;
        }

        uint32_t latency = k_uptime_get_32() - msg.timestamp_ms;

        LOG_INF("[P1-CONS] got seq=%u val=%d q=%u/%d latency=%ums",
                msg.seq,
                msg.value,
                k_msgq_num_used_get(&p1_q),
                P1_QUEUE_DEPTH,
                latency);

        task_wdt_feed(chan);     
        k_msleep(750);
    }

    LOG_INF("[P1-CONS] done");
    k_sem_give(&p1_cons_done);
}

static void HealthMonitor(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    k_thread_name_set(k_current_get(), "health_monitor");

    uint32_t used;

    while (true) {
        used = k_msgq_num_used_get(&p1_q);

        if (used > (P1_QUEUE_DEPTH * 3 / 4)) {
        LOG_WRN("Queue at %d/%d", used, P1_QUEUE_DEPTH);
        }

        k_msleep(100);
    }
}

/* ================================================================== */
/*  Runtime threads                                                   */
/* ================================================================== */

K_THREAD_STACK_DEFINE(p1_prod_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(p1_cons_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(health_monitor_stack, STACK_SIZE);

static struct k_thread p1_prod_thread;
static struct k_thread p1_cons_thread;
static struct k_thread health_monitor_thread;


/* ================================================================== */
/*  Main                                                              */
/* ================================================================== */

int main(void)
{
    LOG_INF("=== L5 Homework 1: Message Queue Pipeline with watchdog and health status ===");
    task_wdt_init(NULL);

    /* Register a channel per monitored thread */
    chan = task_wdt_add(600, /* timeout ms */
                        my_callback, /* called if thread misses feed */
                        (void *)k_current_get());


    LOG_INF("sizeof(sensor_msg)=%u", sizeof(struct sensor_msg));

    LOG_INF("\n--- thread-to-thread pipeline ---");

    k_msgq_purge(&p1_q);
    p1_prod_finished = false;

    k_thread_create(&p1_prod_thread, p1_prod_stack,
                    K_THREAD_STACK_SIZEOF(p1_prod_stack), p1_producer, 
                    NULL, NULL, NULL, 5, 0, K_NO_WAIT);

    /*
    * Let producer get ahead.
    * This makes the queue buffer real messages instead of direct handoff.
    */
    k_thread_create(&health_monitor_thread,
                health_monitor_stack,      
                K_THREAD_STACK_SIZEOF(health_monitor_stack), HealthMonitor,
                NULL, NULL, NULL, 5, 0, K_NO_WAIT);

    k_msleep(180);

    k_thread_create(&p1_cons_thread,
                    p1_cons_stack,
                    K_THREAD_STACK_SIZEOF(p1_cons_stack), p1_consumer,
                    NULL, NULL, NULL, 5, 0, K_NO_WAIT);



    k_sem_take(&p1_prod_done, K_FOREVER);
    k_sem_take(&p1_cons_done, K_FOREVER);

    return 0;
}

