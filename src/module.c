#include "module.h"
#include "gobject/gmarshal.h"


static struct Module* module_get_head(struct Module *m);


struct Module* module_init(struct Module *m_prev, const char *name, struct Config *c, GObject*(*init_cb)(struct Module*), void* args)
{
    struct Module *m = malloc(sizeof(struct Module));
    m->name = strdup(name);
    m->widget = NULL;
    m->lock = 0;
    m->next = NULL;
    m->prev = NULL;

    m->config = c;

    m->init_cb = init_cb;
    m->args = args;

    if (m_prev != NULL) {
        m_prev->next = m;
        m->prev = m_prev;
    }

    m->head = module_get_head(m);

    //g_signal_new("module-lock", G_TYPE_NONE, G_SIGNAL_RUN_FIRST, 0, NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

    return m;
}

static struct Module* module_get_head(struct Module *m)
{
    while (m->prev != NULL)
        m = m->prev;
    return m;
}

void module_destroy(struct Module *m)
{
    // if this is the current head, update linked list
    // with new head
    if (m->prev == NULL && m->next != NULL) {
        struct Module *head = m->next;
        struct Module *tmp = head;

        // update head in linked list
        while (tmp != NULL) {
            tmp->head = head;
            tmp = tmp->next;
        }
    }

    // disconnect node from linked list so we can safely free it
    if (m->prev) {
        if (m->next)
            m->prev->next = m->next;
        else
            m->prev->next = NULL;
    }
    else {
        if (m->next)
            m->next->prev = NULL;
    }

    free(m->name);
    free(m);
}

void module_debug(struct Module *m)
{
    struct Module *tmp = m->head;
    int n = 0;

    while (tmp != NULL) {
        if (tmp == m) 
            printf("* %d: %s\n", n++, tmp->name);
        else
            printf("  %d: %s\n", n++, tmp->name);

        tmp = tmp->next;
    }

}

void module_destroy_all(struct Module *mod)
{
    struct Module *m = module_get_head(mod);

    while (m != NULL) {
        struct Module *tmp = m;
        m = m->next;
        module_destroy(tmp);
    }
}

void module_activate(struct Module *m)
{
    printf("Activate module: %s\n", m->name);
    m->widget = m->init_cb(m);
}

void module_deactivate(struct Module *m)
{
    printf("Deactivate module: %s\n", m->name);
}

void module_lock(struct Module *m)
{
    printf("%s: LOCK\n", m->name);
    m->lock = 1;
}

void module_unlock(struct Module *m)
{
    printf("%s: UNLOCK\n", m->name);
    m->lock = 0;
}

int module_is_locked(struct Module *m)
{
    return m->lock;
}
