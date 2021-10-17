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
    yed_plugin_set_command(self, "pastebin", thr_wrap);
    return 0;
}

void
thr_wrap(int n_Args, char** args)
{
    yed_frame*  frame;
    yed_buffer* buffer;
    pthread_t   pbtr;
    int         tret;
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

    tret = pthread_create(&pbtr, NULL, pastebin, NULL);
    if (tret != 0)
    {
        yed_cerr("Failed to create thread");
        return;
    }
}

pthread_mutex_t pbmtx = PTHREAD_MUTEX_INITIALIZER;
void*
pastebin()
{
    yed_frame*  frame;
    yed_buffer* buff;

    int pbmtx_state;

    int  output_len, status;
    char cmd_buff[4096];

    if (!ys->active_frame)
    {
        yed_cerr("no active frame");
        pthread_exit(NULL);
    }
    frame = ys->active_frame;
    if (!frame->buffer)
    {
        yed_cerr("active frame has no buffer");
        pthread_exit(NULL);
    }
    buff = frame->buffer;
    if (!buff->has_selection)
    {
        yed_cerr("nothing is selected");
        pthread_exit(NULL);
    }

    pbmtx_state = pthread_mutex_trylock(&pbmtx);
    if (pbmtx_state != 0)
    {
        yed_cerr("Pastebin currently working.");
        pthread_exit(NULL);
    }

    char* str2paste = get_sel_text(frame->buffer);
    char* service   = yed_get_var("pastebin-fav");
    FILE* fp        = fopen("/tmp/pastebintmp", "w+");
    int   i         = 0;
    while (str2paste[i] != '\0')
    {
        fputc(str2paste[i], fp);
        i++;
    }
    fclose(fp);
    if (strcmp(service, "ixio") == 0)
    {
        snprintf(cmd_buff, sizeof(cmd_buff),
                 "cat /tmp/pastebintmp | curl -F 'f:1=<-' ix.io");
    }
    else if (strcmp(service, "paste-rs") == 0)
    {
        snprintf(
                    cmd_buff, sizeof(cmd_buff),
                    "cat /tmp/pastebintmp | curl --data-binary @- https://paste.rs/");
    }
    else if (strcmp(service, "dpaste") == 0)
    {
        snprintf(cmd_buff, sizeof(cmd_buff),
                 "cat /tmp/pastebintmp | curl -F 'format=url' -F 'content=<-' "
                 "https://dpaste.org/api/");
    }
    else if (strcmp(service, "mozilla") == 0)
    {
        snprintf(cmd_buff, sizeof(cmd_buff),
                 "cat /tmp/pastebintmp | curl -F 'format=url' -F 'content=<-' "
                 "https://paste.mozilla.org/api/");
    }
    yed_cprint("Uploading to pastebin service...");
    yed_cerr(yed_run_subproc(cmd_buff, &output_len, &status));
    remove("/tmp/pastebintmp");
    free(str2paste);
    pthread_mutex_unlock(&pbmtx);
    pthread_exit(NULL);
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
