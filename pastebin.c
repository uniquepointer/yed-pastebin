#include <yed/plugin.h>
void*
pastebin();
char*
get_sel_text(yed_buffer* buffer);
void
thr_wrap(int n_Args, char** args);

int
yed_plugin_boot(yed_plugin* self)
{
    /*Check for plug version*/
    YED_PLUG_VERSION_CHECK();
    if (yed_get_var("pastebin-fav") == NULL)
    {
        yed_set_var("pastebin-fav", "paste-rs");
    }
    yed_set_var("pastebin-url", "none");
    yed_plugin_set_command(self, "pastebin", thr_wrap);
    return 0;
}

struct Params
{
    char* str2paste;
    char* pb_pref;
};
void
thr_wrap(int n_args, char** args)
{
    yed_frame*  frame;
    yed_buffer* buffer;
    pthread_t   pbtr;
    int         tret;
    struct Params* p;

    if (!ys->active_frame)
    {
        yed_cerr("no active frame");
        return;
    }

    frame = ys->active_frame;
    if (!frame->buffer)
    {
        yed_cerr("active frame has no buffer");
        return;
    }

    buffer = frame->buffer;
    if (!buffer->has_selection)
    {
        yed_cerr("nothing is selected");
        return;
    }

    p = (struct Params*)malloc(sizeof(struct Params));
    p->str2paste = get_sel_text(frame->buffer);
    p->pb_pref = yed_get_var("pastebin-fav");

    tret = pthread_create(&pbtr, NULL, pastebin, p);
    if (tret != 0)
    {
        yed_cerr("Failed to create thread");
        return;
    }
}

void*
pastebin(struct Params* p)
{
    char cmd_buff[4096];

    if (strcmp(p->pb_pref, "ixio") == 0)
    {
        snprintf(cmd_buff, sizeof(cmd_buff),
                 "curl -F 'f:1=<-' ix.io > /tmp/lXSdHQ");
    }
    else if (strcmp(p->pb_pref, "paste-rs") == 0)
    {
        snprintf(
                    cmd_buff, sizeof(cmd_buff),
                    "curl --data-binary @- https://paste.rs/ > /tmp/lXSdHQ");
    }
    else if (strcmp(p->pb_pref, "dpaste") == 0)
    {
        snprintf(cmd_buff, sizeof(cmd_buff),
                 "curl -F 'format=url' -F 'content=<-' "
                 "https://dpaste.org/api/ > /tmp/lXSdHQ");
    }
    else if (strcmp(p->pb_pref, "mozilla") == 0)
    {
        snprintf(cmd_buff, sizeof(cmd_buff),
                 "curl -F 'format=url' -F 'content=<-' "
                 "https://paste.mozilla.org/api/ > /tmp/lXSdHQ");
    }

    int  output_len, status;
    FILE* pb_pipe;

    if ((pb_pipe = popen(cmd_buff, "w")) == NULL)
    {
        pthread_exit(NULL);
    }
    fprintf(pb_pipe, "%s", p->str2paste);
    pclose(pb_pipe);

    char url[1024];
    snprintf(url, sizeof(url), "cat /tmp/lXSdHQ");

    char* pb_url;
    pb_url = yed_run_subproc(url, &output_len, &status);
    if (pb_url != NULL)
    {
        yed_set_var("pastebin-url", pb_url);
        yed_cerr(pb_url);
    }

    free(p->str2paste);
    free(p);
    remove("/tmp/lXSdHQ");
    return (NULL);
}

/* ty kammer */
char*
get_sel_text(yed_buffer* buffer)
{
    char      nl;
    array_t   chars;
    int       r1;
    int       c1;
    int       r2;
    int       c2;
    int       r;
    yed_line* line;
    int       cstart;
    int       cend;
    int       i;
    int       n;
    char*     data;
    nl    = '\n';
    chars = array_make(char);
    yed_range_sorted_points(&buffer->selection, &r1, &c1, &r2, &c2);
    if (buffer->selection.kind == RANGE_LINE)
    {
        for (r = r1; r <= r2; r += 1)
        {
            line = yed_buff_get_line(buffer, r);
            if (line == NULL)
            {
                break;
            } /* should not happen */
            data = (char*)array_data(line->chars);
            array_push_n(chars, data, array_len(line->chars));
            array_push(chars, nl);
        }
    }
    else
    {
        for (r = r1; r <= r2; r += 1)
        {
            line = yed_buff_get_line(buffer, r);
            if (line == NULL)
            {
                break;
            } /* should not happen */
            if (line->visual_width > 0)
            {
                cstart = r == r1 ? c1 : 1;
                cend   = r == r2 ? c2 : line->visual_width + 1;
                i      = yed_line_col_to_idx(line, cstart);
                n      = yed_line_col_to_idx(line, cend) - i;
                data   = array_item(line->chars, i);
                array_push_n(chars, data, n);
            }
            if (r < r2)
            {
                array_push(chars, nl);
            }
        }
    }
    array_zero_term(chars);
    return (char*)array_data(chars);
}
