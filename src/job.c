#include "job.h"

struct Job* job_init(int(*handle_data_cb)(void *arg), void*(*worker_cb)(void *arg), void *arg)
{
    static int job_id = 0;

    struct Job* j = malloc(sizeof(struct Job));
    j->worker_running = 0;
    j->do_stop = 0;

    // this is where the long running task is
    j->worker_cb = worker_cb;
    j->handle_data_cb = handle_data_cb;
    j->arg = arg;
    j->id = job_id++;
    return j;
}

void job_destroy(struct Job *j)
{
    free(j);
}

struct JobPool* pool_init()
{
    struct JobPool *pool = malloc(sizeof(struct JobPool));
    pool->jobs = NULL;
    return pool;
}

void job_run(struct Job *j)
{
    g_timeout_add(100, job_manager_thread, j);
    j->worker = g_thread_new("worker", j->worker_cb, j); 
    j->worker_running = 1;
}

void pool_add_job(struct JobPool* pool, struct Job *j)
{
    if (pool->jobs == NULL)
        pool->jobs = g_list_append(NULL, j);
    else
        pool->jobs = g_list_append(pool->jobs, j);
}

void pool_kill_all(struct JobPool *pool)
{
    /* Tell all jobs to die!!! */

    // TODO filter/free finished jobs

    GList *item = pool->jobs;
    while (item != NULL) {
        struct Job* j = item->data;
        if (j->do_stop == 0) {
            j->do_stop = 1;
            printf(">>>>>>>>>SETTING DO STOP %d\n", j->id);
        }
        item = item->next;
    }
}

int job_manager_thread(void *arg)
{
    /* Non blocking thread manager that checks for data from worker thread.
     * If worker thread is finished it will start the blocking handle_data function
     * that does stuff with the data.
     * It is important that this is executed in the same thread as GTK main loop is running in
     */
    struct Job *j = arg;
    printf("%d manager: check\n", j->id);

    if (j->do_stop) {
        // Here we trust the thread to exit on it's own without causing any fuss
        printf("%d manager: stop\n", j->id);
        return G_SOURCE_REMOVE;
    }
    if (!j->worker_running) {
        j->handle_data_cb(j);
        return G_SOURCE_REMOVE;
    }

    return G_SOURCE_CONTINUE;
}
