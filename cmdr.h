#ifndef CMDR_H
#define CMDR_H

#include <stdbool.h>

struct cmdr_state
{
    int argc;
    char **argv;
    char *error;
};

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
extern const char *cmdr_error(struct cmdr_state *state);
extern int cmdr_init(struct cmdr_state *state, int argc, char **argv);
extern int cmdr_end(struct cmdr_state *state);
extern int cmdr_find(struct cmdr_state *state, const char *str, int *out_idx);
extern int cmdr_command(struct cmdr_state *state, const char *cmd);
extern int cmdr_flag(struct cmdr_state *state, const char *flag);
extern int cmdr_flag_str(struct cmdr_state *state, const char *flag, const char **out);
extern int cmdr_flag_long(struct cmdr_state *state, const char *flag, long *out);
extern int cmdr_flag_int(struct cmdr_state *state, const char *flag, int *out);
extern int cmdr_flag_double(struct cmdr_state *state, const char *flag, double *out);
extern int cmdr_flag_float(struct cmdr_state *state, const char *flag, float *out);
extern int cmdr_flag_bool(struct cmdr_state *state, const char *flag, bool *out);
extern int cmdr_confirm(struct cmdr_state *state, const char *msg);
extern int cmdr_confirm_default(struct cmdr_state *state, const char *msg, int def);
#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef CMDR_IMPLEMENTATION
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <float.h>
#include <math.h>

#define CMDR_ASSERT_MSG(cond, msg, ...) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "cmdr: " msg "\n", __VA_ARGS__); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

#define CMDR_ASSERT(cond) \
    CMDR_ASSERT_MSG(cond, "assert failed: %s", #cond)

static int cmdr_internal_valid_command(const char *command);
static int cmdr_internal_valid_flag(const char *flag);
static void cmdr_internal_strlower(char *str);
static void cmdr_internal_trim(char *str);
static void cmdr_internal_trim_left(char *str);
static void cmdr_internal_trim_right(char *str);

const char *cmdr_error(struct cmdr_state *state)
{
    CMDR_ASSERT(state != NULL);
    return state->error;
}

int cmdr_init(struct cmdr_state *state, int argc, char **argv)
{
    CMDR_ASSERT(state != NULL);
    CMDR_ASSERT(argc > 0);
    CMDR_ASSERT(argv != NULL);

    state->argc = argc;
    state->argv = argv;
    state->error = NULL;
    return 0;
}

int cmdr_end(struct cmdr_state *state)
{
    CMDR_ASSERT(state != NULL);

    state->argc = 0;
    state->argv = NULL;
    state->error = NULL;
    return 0;
}

int cmdr_find(struct cmdr_state *state, const char *str, int *out_idx)
{
    CMDR_ASSERT(state != NULL);
    CMDR_ASSERT(str != NULL);

    for (int i = 1; i < state->argc; i++) {
        if (strcmp(state->argv[i], str) == 0) {
            if (out_idx != NULL)
                *out_idx = i;
            return 1;
        }
    }

    return 0;
}

int cmdr_command(struct cmdr_state *state, const char *cmd)
{
    CMDR_ASSERT(state != NULL);
    CMDR_ASSERT(cmd != NULL);
    CMDR_ASSERT_MSG(cmdr_internal_valid_command(cmd),
                    "invalid command name: %s",
                    cmd);

    return cmdr_find(state, cmd, NULL);
}

int cmdr_flag(struct cmdr_state *state, const char *flag)
{
    CMDR_ASSERT(state != NULL);
    CMDR_ASSERT(flag != NULL);
    CMDR_ASSERT_MSG(cmdr_internal_valid_flag(flag),
                    "invalid flag name: %s",
                    flag);

    return cmdr_find(state, flag, NULL);
}

static int cmdr_internal_flag_raw(struct cmdr_state *state, const char *flag, const char **out)
{
    CMDR_ASSERT(state != NULL);
    CMDR_ASSERT(flag != NULL);
    CMDR_ASSERT(out != NULL);
    CMDR_ASSERT_MSG(cmdr_internal_valid_flag(flag),
                    "invalid flag name: %s",
                    flag);

    size_t len = strlen(flag);
    for (int i = 1; i < state->argc; i++) {
        const char *arg = state->argv[i];
        if (strncmp(arg, flag, len) != 0)
            continue;

        if (arg[len] == '=') {
            *out = arg + len + 1;
            return 1;
        }

        if (arg[len] == '\0') {
            if (i + 1 >= state->argc) {
                state->error = "missing flag value";
                return -1;
            }

            *out = state->argv[i + 1];
            return 1;
        }
    }

    return 0;
}

int cmdr_flag_str(struct cmdr_state *state, const char *flag, const char **out)
{
    CMDR_ASSERT(state != NULL);
    CMDR_ASSERT(flag != NULL);
    CMDR_ASSERT(out != NULL);
    CMDR_ASSERT_MSG(cmdr_internal_valid_flag(flag),
                    "invalid flag name: %s",
                    flag);

    return cmdr_internal_flag_raw(state, flag, out);
}

int cmdr_flag_long(struct cmdr_state *state, const char *flag, long *out)
{
    CMDR_ASSERT(state != NULL);
    CMDR_ASSERT(flag != NULL);
    CMDR_ASSERT(out != NULL);
    CMDR_ASSERT_MSG(cmdr_internal_valid_flag(flag),
                    "invalid flag name: %s",
                    flag);

    const char *raw;
    if (!cmdr_internal_flag_raw(state, flag, &raw))
        return 0;

    char *end;
    errno = 0;
    long v = strtol(raw, &end, 10);
    cmdr_internal_trim_right(end);
    if (end == raw || *end != '\0') {
        state->error = "invalid long";
        return -1;
    } else if (errno == ERANGE) {
        state->error = "out of range";
        return -1;
    }


    *out = v;
    return 1;
}

int cmdr_flag_int(struct cmdr_state *state, const char *flag, int *out)
{
    CMDR_ASSERT(state != NULL);
    CMDR_ASSERT(flag != NULL);
    CMDR_ASSERT(out != NULL);
    CMDR_ASSERT_MSG(cmdr_internal_valid_flag(flag),
                    "invalid flag name: %s",
                    flag);

    long v;
    int rc = cmdr_flag_long(state, flag, &v);
    if (rc != 1)
        return rc;
    if (v < INT_MIN || v > INT_MAX) {
        state->error = "out of range";
        return -1;
    }

    *out = (int)v;
    return 1;
}

int cmdr_flag_double(struct cmdr_state *state, const char *flag, double *out)
{
    CMDR_ASSERT(state != NULL);
    CMDR_ASSERT(flag != NULL);
    CMDR_ASSERT(out != NULL);
    CMDR_ASSERT_MSG(cmdr_internal_valid_flag(flag),
                    "invalid flag name: %s",
                    flag);

    const char *raw;
    if (!cmdr_internal_flag_raw(state, flag, &raw))
        return 0;

    errno = 0;
    char *end = NULL;
    double v = strtod(raw, &end);
    if (end == raw) {
        state->error = "invalid double";
        return -1;
    } else if (errno == ERANGE) {
        state->error = "out of range";
        return -1;
    } else if (!isfinite(v)) {
        state->error = "infinite double";
        return -1;
    }

    cmdr_internal_trim_right(end);
    if (*end != '\0') {
        state->error = "invalid double";
        return -1;
    }

    *out = v;
    return 1;
}

int cmdr_flag_float(struct cmdr_state *state, const char *flag, float *out)
{
    CMDR_ASSERT(state != NULL);
    CMDR_ASSERT(flag != NULL);
    CMDR_ASSERT(out != NULL);
    CMDR_ASSERT_MSG(cmdr_internal_valid_flag(flag),
                    "invalid flag name: %s",
                    flag);

    double v;
    int rc = cmdr_flag_double(state, flag, &v);
    if (rc != 1)
        return rc;
    if (v > FLT_MAX || v < -FLT_MAX) {
        state->error = "invalid float";
        return -1;
    }

    float result = (float)v;
    if (!isfinite(result)) {
        state->error = "infinite float";
        return -1;
    }

    *out = result;
    return 1;
}

int cmdr_flag_bool(struct cmdr_state *state, const char *flag, bool *out)
{
    CMDR_ASSERT(state != NULL);
    CMDR_ASSERT(flag != NULL);
    CMDR_ASSERT(out != NULL);
    CMDR_ASSERT_MSG(cmdr_internal_valid_flag(flag),
                    "invalid flag name: %s",
                    flag);

    const char *raw;
    if (!cmdr_internal_flag_raw(state, flag, &raw))
        return 0;

    char buf[32];
    snprintf(buf, sizeof(buf), "%s", raw);
    cmdr_internal_strlower(buf);
    cmdr_internal_trim(buf);

    if (strcmp(buf, "true") == 0 || strcmp(buf, "yes") == 0) {
        *out = true;
    } else if (strcmp(buf, "false") == 0 || strcmp(buf, "no") == 0) {
        *out = false;
    } else {
        state->error = "invalid bool";
        return -1;
    }

    return 1;
}

int cmdr_confirm(struct cmdr_state *state, const char *msg)
{
    CMDR_ASSERT(state != NULL);
    CMDR_ASSERT(msg != NULL);

    return cmdr_confirm_default(state, msg, 1);
}

int cmdr_confirm_default(struct cmdr_state *state, const char *msg, int def)
{
    CMDR_ASSERT(state != NULL);
    CMDR_ASSERT(msg != NULL);

    char yes_ch = 'Y';
    char no_ch = 'n';
    if (def == 0) {
        yes_ch = tolower(yes_ch);
        no_ch = toupper(no_ch);
    }

    printf("%s [%c/%c] ", msg, yes_ch, no_ch);
    fflush(stdout);

    char buf[32];
    if (fgets(buf, sizeof(buf), stdin) == NULL)
        return !!def;
    buf[strcspn(buf, "\n")] = '\0';

    cmdr_internal_trim(buf);
    cmdr_internal_strlower(buf);
    if (*buf == '\0')
        return !!def;

    if (strcmp(buf, "y") == 0 || strcmp(buf , "yes") == 0)
        return 1;
    else if (strcmp(buf, "n") == 0 || strcmp(buf , "no") == 0)
        return 0;

    return 0;
}

static void cmdr_internal_strlower(char *str)
{
    CMDR_ASSERT(str != NULL);

    for (char *ch = str; *ch; ch++)
        *ch = (char)tolower(*ch);
}

static void cmdr_internal_trim(char *str)
{
    CMDR_ASSERT(str != NULL);

    cmdr_internal_trim_left(str);
    cmdr_internal_trim_right(str);
}

static void cmdr_internal_trim_left(char *str)
{
    CMDR_ASSERT(str != NULL);

    size_t len = strlen(str);
    size_t start_idx = 0;
    while (start_idx < len && isspace(str[start_idx]))
        start_idx++;
    for (size_t i = 0; i <= len - start_idx; i++)
        str[i] = str[i + start_idx];
}

static void cmdr_internal_trim_right(char *str)
{
    CMDR_ASSERT(str != NULL);

    char *end = str + strlen(str);
    while (end > str && isspace((unsigned char)end[-1]))
        end--;
    *end = '\0';
}

static int cmdr_internal_valid_command(const char *command)
{
    CMDR_ASSERT(command != NULL);

    if (command[0] == '\0' || command[0] == '-')
        return 0;

    const char *ch = command;
    while (*ch != '\0') {
        if (isspace(*ch) || *ch == '=')
            return 0;

        ch++;
    }

    return 1;
}

static int cmdr_internal_valid_flag(const char *flag)
{
    CMDR_ASSERT(flag != NULL);

    if (flag[0] != '-')
        return 0;
    if (flag[1] == '\0' ||
        (flag[1] == '-' && flag[2] == '\0'))
        return 0;
    if (flag[1] == '-' && flag[2] == '-')
        return 0;


    const char *ch = flag;
    while (*ch != '\0') {
        if (isspace(*ch) || *ch == '=')
            return 0;

        ch++;
    }

    return 1;
}
#endif // CMDR_IMPLEMENTATION
#endif // CMDR_H
