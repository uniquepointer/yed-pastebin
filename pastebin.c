#include <yed/plugin.h>
void pastebin(int n_args, char **args);
char* get_sel_text(yed_buffer *buffer);

int
yed_plugin_boot(yed_plugin *self)
{
    /*Check for plug version*/
    YED_PLUG_VERSION_CHECK();
    yed_plugin_set_command(self, "pastebin", pastebin);
    return 0;
}

void
pastebin(int n_args, char **args)
{
    yed_frame  *frame;
    yed_buffer *buff;

    int output_len, status;
    char cmd_buff[4096];

    if (!ys->active_frame) {
        yed_cerr("no active frame");
        return;
    }

    frame = ys->active_frame;

    if (!frame->buffer) {
        yed_cerr("active frame has no buffer");
        return;
    }

    buff = frame->buffer;

    if (!buff->has_selection) {
        yed_cerr("nothing is selected");
        return;
    }
    char* str2paste = get_sel_text(frame->buffer);
    snprintf(cmd_buff, sizeof(cmd_buff), "echo '%s' | curl -F 'f:1=<-' ix.io", str2paste);
    yed_cprint("Uploading to pastebin service...");
    yed_cerr(yed_run_subproc(cmd_buff, &output_len, &status));
    free(str2paste);
}

/* ty kammer */
char* get_sel_text(yed_buffer *buffer) {
    char      nl;
    array_t   chars;
    int       r1;
    int       c1;
    int       r2;
    int       c2;
    int       r;
    yed_line *line;
    int       cstart;
    int       cend;
    int       i;
    int       n;
    char     *data;

    nl    = '\n';
    chars = array_make(char);

    yed_range_sorted_points(&buffer->selection, &r1, &c1, &r2, &c2);

    if (buffer->selection.kind == RANGE_LINE) {
        for (r = r1; r <= r2; r += 1) {
            line = yed_buff_get_line(buffer, r);
            if (line == NULL) { break; } /* should not happen */

            data = (char*)array_data(line->chars);
            array_push_n(chars, data, array_len(line->chars));
            array_push(chars, nl);
        }
    } else {
        for (r = r1; r <= r2; r += 1) {
            line = yed_buff_get_line(buffer, r);
            if (line == NULL) { break; } /* should not happen */

            if (line->visual_width > 0) {
                cstart = r == r1 ? c1 : 1;
                cend   = r == r2 ? c2 : line->visual_width + 1;

                i    = yed_line_col_to_idx(line, cstart);
                n    = yed_line_col_to_idx(line, cend) - i;
                data = array_item(line->chars, i);

                array_push_n(chars, data, n);
            }

            if (r < r2) {
                array_push(chars, nl);
            }
        }
    }

    array_zero_term(chars);

    return (char*)array_data(chars);
}
