#include "line_discipline.h"
#include "../../../util/printf.h"
#include "../../../memory/heap.h"

//Required for visibility purposes
void line_discipline_delete_last(struct line_discipline *ld);


int ld_ignore(struct line_discipline* ld, char c) {
    (void)ld;
    (void)c;
    return 0;
}

int ld_bell(struct line_discipline* ld, char c) {
    (void)ld;
    (void)c;
    printf("A bell is ringing...\n");
    return 0;
}

int ld_eof(struct line_discipline* ld, char c) {
    (void)ld;
    (void)c;
    printf("EOF is called...\n");
    return 0;
}

int ld_erase(struct line_discipline* ld, char c) {
    (void)c;
    line_discipline_delete_last(ld);
    return 0;
}

int ld_htab(struct line_discipline* ld, char c) {
    (void)ld;
    (void)c;
    printf("Horizontal tab is called...\n");
    return 0;
}

int ld_lfeed(struct line_discipline* ld, char c) {
    (void)ld;
    (void)c;
    printf("Line feed is called...\n");
    return 0;
}

int ld_vtab(struct line_discipline* ld, char c) {
    (void)ld;
    (void)c;
    printf("Vertical tab is called...\n");
    return 0;
}

int ld_newpage(struct line_discipline* ld, char c) {
    (void)ld;
    (void)c;
    printf("New page is called...\n");
    return 0;
}

int ld_cret(struct line_discipline* ld, char c) {
    (void)c;
    line_discipline_force_flush(ld);
    return 0;
}

int ld_restart_out(struct line_discipline* ld, char c) {
    (void)ld;
    (void)c;
    printf("Restart output is called...\n");
    return 0;
}

int ld_stop_out(struct line_discipline* ld, char c) {
    (void)ld;
    (void)c;
    printf("Stop output is called...\n");
    return 0;
}

int ld_sigint(struct line_discipline* ld, char c) {
    (void)ld;
    (void)c;
    printf("SIGINT is called...\n");
    return 0;
}

int ld_sigstop(struct line_discipline* ld, char c) {
    (void)ld;
    (void)c;
    printf("SIGSTOP is called...\n");
    return 0;
}

int ld_del(struct line_discipline* ld, char c) {
    (void)ld;
    (void)c;
    printf("DEL is called...\n");
    return 0;
}

struct line_discipline_action_table_entry default_ld_table[] = {
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_ignore},   //0
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_ignore},   //1 A
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_ignore},   //2 B 
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_sigint},   //3 C 
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_eof},      //4 D 
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_ignore},   //5 E 
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_ignore},   //6 F
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_bell},     //7 G
    {.output = {'\b', ' ', '\b', LD_NULL},           .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_erase},   //8 H
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_htab},   //9 I
    {.output = {'\n', LD_NULL, LD_NULL, LD_NULL},    .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {'\r', '\n', LD_NULL, LD_NULL},    .action = ld_lfeed},   //10 J
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_vtab},   //11 K
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_newpage},   //12 L
    {.output = {'\r', '\n', LD_NULL, LD_NULL},    .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, '\n', LD_NULL, LD_NULL},    .action = ld_cret},   //13 M
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_ignore},   //14 N
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_ignore},   //15 O 
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_ignore},   //16 P
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_restart_out},   //17 Q 
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_ignore},   //18 R 
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_stop_out},   //19 S 
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_ignore},   //20 T
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_ignore},   //21 U
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_ignore},   //22 V
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_ignore},   //23 W 
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_ignore},   //24 X 
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_ignore},   //25 Y
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_sigstop},   //26 Z
    {.output = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserti = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .inserto = {LD_NULL, LD_NULL, LD_NULL, LD_NULL}, .action = ld_del},   //27
};

struct line_discipline * line_discipline_create(int mode, int echo, int table, int buffer_size, void* parent, void (*flush_cb)(void* parent, char *buffer, int size), void (*echo_cb)(void* parent, char c)) {
    struct line_discipline *ld = (struct line_discipline *) malloc(sizeof(struct line_discipline));
    if (ld == 0) return 0;

    if (mode == LINE_DISCIPLINE_MODE_RAW || mode == LINE_DISCIPLINE_MODE_CANONICAL) ld->mode = mode;
    else ld->mode = LINE_DISCIPLINE_MODE_CANONICAL;

    if (echo == LINE_DISCIPLINE_MODE_ECHO_ON || echo == LINE_DISCIPLINE_MODE_ECHO_OFF) ld->echo = echo;
    else ld->echo = LINE_DISCIPLINE_MODE_ECHO_ON;

    ld->buffer = (char*) malloc(sizeof(char) * buffer_size);
    if (ld->buffer == 0) {
        free(ld);
        return 0;
    }

    ld->size = buffer_size;
    ld->tail = 0;
    ld->head = 0;

    switch (table) {
        case LD_DEFAULT_TABLE:
            ld->action_table = default_ld_table;
            break;
        default:
            ld->action_table = default_ld_table;
            break;
    }

    ld->parent = parent;
    ld->flush_cb = flush_cb;
    ld->echo_cb = echo_cb;
    ld->valid = 1;

    return ld;
}

void line_discipline_delete_last(struct line_discipline *ld) {
    if (ld == 0 || ld->valid == 0) return;
    if (ld->tail == ld->head) return;
    //printf("Debug tail before = %d ", ld->tail);
    ld->tail = (ld->tail - 1) % ld->size;
    //printf("tail = %d head = %d text = ", ld->tail, ld->head);
    //for (int i = ld->head; i != ld->tail; i = (i + 1) % ld->size) {
    //    printf("%c", ld->buffer[i]);
    //}
    //printf("\n");
}

void line_discipline_insert(struct line_discipline* ld, char c) {
    if (ld == 0 || ld->valid == 0) return;
    if (c == LD_NULL) return;

    ld->buffer[ld->tail] = c;
    ld->tail = (ld->tail + 1) % ld->size;
    if (ld->tail == ld->head) ld->head = (ld->head + 1) % ld->size;
}

void line_discipline_destroy(struct line_discipline *ld) {
    if (ld == 0) return;
    if (ld->buffer) free(ld->buffer);
    free(ld);
}

void line_discipline_set_mode(struct line_discipline *ld, int mode) {
    if (ld != 0 && ld->valid == 1) {
        if (mode == LINE_DISCIPLINE_MODE_RAW || mode == LINE_DISCIPLINE_MODE_CANONICAL) ld->mode = mode;
    }
}
void line_discipline_set_echo(struct line_discipline *ld, int echo) {
    if (ld != 0 && ld->valid == 1) {
        if (echo == LINE_DISCIPLINE_MODE_ECHO_ON || echo == LINE_DISCIPLINE_MODE_ECHO_OFF) ld->echo = echo;
    }
}	

void line_discipline_apply(struct line_discipline *ld, char c) {
    if (ld == 0 || ld->valid != 1) {
        return;
    }
    if (c != LD_DEL && (c < 0 || c > 27)) {
        line_discipline_insert(ld, c);
        if (ld->echo == LINE_DISCIPLINE_MODE_ECHO_ON && ld->echo_cb != 0) {
            ld->echo_cb(ld->parent, c);
        }
        return;
    }

    if (c == LD_DEL) {//TODO: Fix this
        line_discipline_delete_last(ld);
        ld->echo_cb(ld->parent, '\b');
        ld->echo_cb(ld->parent, ' ');
        ld->echo_cb(ld->parent, '\b');
        return;
    }

    for (int i = 0; i < LD_INSERT_SIZE; i++) {
        line_discipline_insert(ld, ld->action_table[(uint8_t)c].inserti[i]);
    }

    if (ld->echo == LINE_DISCIPLINE_MODE_ECHO_ON && ld->echo_cb != 0) {
        for (int i = 0; i < LD_OUTPUT_SIZE; i++) {
            if (ld->action_table[(uint8_t)c].output[i] == LD_NULL) break;
            ld->echo_cb(ld->parent, ld->action_table[(uint8_t)c].output[i]);
        }
    }

    if (ld->action_table[(uint8_t)c].action != LD_NULL)
        ld->action_table[(uint8_t)c].action(ld, c);
}

void line_discipline_read(struct line_discipline *ld, char character) {
    if (ld == 0 || ld->valid != 1) {
        return;
    }

    if (ld->mode == LINE_DISCIPLINE_MODE_RAW) {
        if (ld->echo == LINE_DISCIPLINE_MODE_ECHO_ON && ld->echo_cb != 0)
            ld->echo_cb(ld->parent, character);
        ld->flush_cb(ld->parent, &character, 1);
    } else if (ld->mode == LINE_DISCIPLINE_MODE_CANONICAL) {
        line_discipline_apply(ld, character);
    }
}

//TODO: Implement this
int line_discipline_translate(struct line_discipline *ld, char original_character, char* translated_characters) {
    if (ld == 0 || ld->valid != 1) {
        goto same;
    }

    //printf("original_character = %x\n", original_character);

    if (ld->mode == LINE_DISCIPLINE_MODE_RAW) {
        goto same;
    } else if (ld->mode == LINE_DISCIPLINE_MODE_CANONICAL) {
        if (original_character != LD_DEL && (original_character < 0 || original_character > 27))
            goto same;

        int i = 0;
        for (i = 0; i < LD_OUTPUT_SIZE; i++) {
            if (ld->action_table[(uint8_t)original_character].inserto[i] == LD_NULL) break;
            translated_characters[i] = ld->action_table[(uint8_t)original_character].inserto[i];
        }
        return i;
    }

same:
    translated_characters[0] = original_character;
    return 1;
}

void line_discipline_debug() {
    printf("ld_ignore: %p\n", ld_ignore);
    printf("ld_bell: %p\n", ld_bell);
    printf("ld_eof: %p\n", ld_eof);
    printf("ld_erase: %p\n", ld_erase);
    printf("ld_htab: %p\n", ld_htab);
    printf("ld_lfeed: %p\n", ld_lfeed);
    printf("ld_vtab: %p\n", ld_vtab);
    printf("ld_newpage: %p\n", ld_newpage);
    printf("ld_cret: %p\n", ld_cret);
    printf("ld_restart_out: %p\n", ld_restart_out);
    printf("ld_sigint: %p\n", ld_sigint);
    printf("ld_sigstop: %p\n", ld_sigstop);
    printf("ld_del: %p\n", ld_del);
}

void line_discipline_force_flush(struct line_discipline *ld) {
    if (ld == 0 || ld->valid != 1) {
        return;
    }

    if (ld->mode == LINE_DISCIPLINE_MODE_CANONICAL) {
        int size = (ld->tail - ld->head + ld->size) % ld->size;
        ld->flush_cb(ld->parent, &ld->buffer[ld->head], size);
        ld->head = ld->tail;
    }
}