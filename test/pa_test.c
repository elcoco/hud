#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include<unistd.h>

#include <pulse/pulseaudio.h>

// compile:
// gcc -o pcm-playback pa_test.c -o pa_test `pkg-config --cflags --libs libpulse` && ./pa_test

static pa_mainloop_api *mainloop_api = NULL;


static void get_sink_input_info_callback(pa_context *context, const pa_sink_input_info *i, int is_last, void *userdata)
{
    char t[32], k[32], s[PA_SAMPLE_SPEC_SNPRINT_MAX], cv[PA_CVOLUME_SNPRINT_VERBOSE_MAX], cm[PA_CHANNEL_MAP_SNPRINT_MAX], f[PA_FORMAT_INFO_SNPRINT_MAX];
    char *proplist;

    if (is_last < 0) {
        printf("Failed to get sink input information\n");
        //pa_log(_("Failed to get sink input information: %s"), pa_strerror(pa_context_errno(context)));
        return;
    }

    if (is_last) {
        mainloop_api->quit(mainloop_api, 1);
        return;
    }

    char *sample_spec = pa_sample_spec_snprint(s, sizeof(s), &i->sample_spec);
    char *channel_map = pa_channel_map_snprint(cm, sizeof(cm), &i->channel_map);
    char *format_info = pa_format_info_snprint(f, sizeof(f), i->format);
    float balance = pa_cvolume_get_balance(&i->volume, &i->channel_map);

    printf("Sink Input #%u\n"
        // "\tDriver: %s\n"
        // "\tOwner Module: %s\n"
        // "\tClient: %s\n"
         "\tSink: %u\n"
         "\tSample Specification: %s\n"
         "\tChannel Map: %s\n"
         "\tFormat: %s\n"
         "\tCorked: %s\n"
         "\tMute: %s\n"
         "\tVolume: %s\n"
         "\t        balance %0.2f\n"
         "\tBuffer Latency: %0.0f usec\n"
         "\tSink Latency: %0.0f usec\n"
        // "\tResample method: %s\n"
         "\tProperties:\n\t\t%s\n",
       i->index,
       //pa_strnull(i->driver),
       //i->owner_module != PA_INVALID_INDEX ? t : _("n/a"),
       //i->client != PA_INVALID_INDEX ? k : _("n/a"),
       i->sink,
       sample_spec,
       channel_map,
       format_info,
       (i->corked) ? "yes" : "no",
       (i->mute) ? "yes" : "no",
       pa_cvolume_snprint_verbose(cv, sizeof(cv), &i->volume, &i->channel_map, 1),
       balance,
       (double) i->buffer_usec,
       (double) i->sink_usec,
       //i->resample_method ? i->resample_method : _("n/a"),
       proplist = pa_proplist_to_string_sep(i->proplist, "\n\t\t"));

    pa_xfree(proplist);
}

void context_state_cb(pa_context* context, void* mainloop) {
    //pa_threaded_mainloop_signal(mainloop, 0);
    pa_operation *o = NULL;

    o = pa_context_get_sink_input_info_list(context, get_sink_input_info_callback, NULL);
}

static void exit_signal_callback(pa_mainloop_api *m, pa_signal_event *e, int sig, void *userdata) {
    printf("Got SIGINT, exiting.\n");
    m->quit(m, 0);
    //exit(0);
}

int main()
{
    printf("start\n");
    pa_mainloop *mainloop;
    //pa_mainloop_api *mainloop_api;
    pa_context *context;

    int ret;

    // Get a mainloop and its context
    mainloop = pa_mainloop_new();
    assert(mainloop);
    mainloop_api = pa_mainloop_get_api(mainloop);
    context = pa_context_new(mainloop_api, "pcm-playback");
    assert(context);


    pa_signal_new(SIGINT, exit_signal_callback, NULL);
    pa_signal_new(SIGTERM, exit_signal_callback, NULL);


     // Set a callback so we can wait for the context to be ready
    pa_context_set_state_callback(context, &context_state_cb, NULL);
    //pa_context_set_state_callback(context, &context_state_cb, mainloop);

    // Lock the mainloop so that it does not run and crash before the context is ready
    //pa_threaded_mainloop_lock(mainloop);

    // Start the mainloop
    assert(pa_context_connect(context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) == 0);

    if (pa_mainloop_run(mainloop, &ret) < 0) {
        printf("pa_mainloop_run() failed.");
        return 1;
    }

    if (context)
        pa_context_unref(context);

    if (mainloop) {
        pa_signal_done();
        pa_mainloop_free(mainloop);
    }

    return 1;
}
