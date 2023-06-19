#ifndef JOB_H
#define JOB_H

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

struct Job {
    int id;
    int worker_running;

    // signals that job can be freed
    int job_done;

    // tell job to stop
    int do_stop;

    // does the non blocking work (don't do any non-threadsafe GTK stuff here)
    void*(*worker_cb)(void* args);

    // threadsafe blocking callback that handles the results that the worker produces
    int(*handle_data_cb)(void* args);
    void* arg;

    GThread* worker;
};

struct JobPool {
    // linked list to all jobs
    GList *jobs;
};

struct Job* job_init(int(*handle_data_cb)(void *arg), void*(*worker_cb)(void *arg), void *arg);
void job_destroy(struct Job *j);
struct JobPool* pool_init();
void pool_add_job(struct JobPool* pool, struct Job *j);
void job_run(struct Job *j);
void pool_kill_all(struct JobPool *pool);

int job_manager_thread(void *arg);

#endif // !JOB_H
