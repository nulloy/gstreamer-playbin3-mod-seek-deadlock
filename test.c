#include <assert.h>
#include <gst/gst.h>
#include <stdio.h>
#include <string.h>

static gboolean handle_keyboard(GIOChannel *source, GIOCondition cond,
                                void *data)
{
    gchar *str = NULL;
    if (g_io_channel_read_line(source, &str, NULL, NULL, NULL) !=
        G_IO_STATUS_NORMAL) {
        return TRUE;
    }

    int sign = 1;
    switch (g_ascii_tolower(str[0])) {
        case 'b':
            sign = -1;
            g_print("seeking backwards...\n");
            break;
        case 'n':
            g_print("seeking forward...\n");
            break;
        default:
            return TRUE;
    }

    GstElement *playbin = (GstElement *)data;
    gint64 pos;
    if (!gst_element_query_position(playbin, GST_FORMAT_TIME, &pos)) {
        g_print("gst_element_query_position failed!\n");
    }
    gint64 len;
    if (!gst_element_query_duration(playbin, GST_FORMAT_TIME, &len)) {
        g_print("gst_element_query_duration failed!\n");
    }

    if (!gst_element_seek(playbin, 1.0, GST_FORMAT_TIME,
                          GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT |
                              GST_SEEK_FLAG_SNAP_NEAREST,
                          GST_SEEK_TYPE_SET, pos + len * 0.05 * sign,
                          GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
        g_print("gst_element_seek failed!\n");
    }

    g_free(str);

    return TRUE;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        g_printerr("usage: %s FILE\n", argv[0]);
        return -1;
    }
    const char *file = argv[1];

    gst_init(&argc, &argv);

    GstElement *playbin = gst_element_factory_make("playbin3", NULL);
    assert(playbin);

    GIOChannel *io_stdin = g_io_channel_unix_new(fileno(stdin));
    g_io_add_watch(io_stdin, G_IO_IN, (GIOFunc)handle_keyboard, playbin);

    char abs_path[PATH_MAX + 1];
    assert(realpath(file, abs_path));

    GUri *uri = gst_filename_to_uri(abs_path, NULL);
    assert(uri);
    g_object_set(G_OBJECT(playbin), "uri", uri, NULL);
    g_free(uri);

    g_print("playing: %s\n", file);
    gst_element_set_state(playbin, GST_STATE_PLAYING);

    g_print("enter 'n' to seek forward, or 'b' to seek backwards:\n");

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    return 0;
}
