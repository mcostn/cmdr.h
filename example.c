#define CMDR_IMPLEMENTATION
#include "cmdr.h"

#include <stdio.h>

static void usage()
{
    printf("usage: example <command> [flags]\n\n"
           "commands:\n"
           "  hello       print a hello message\n"
           "  goodbye     print a goodbye message\n"
           "  ask         prompt a yes/no question (default: yes)\n"
           "  ask-no      prompt a yes/no question (default: no)\n\n"
           "flags:\n"
           "  -a          example short flag\n"
           "  --long-name example long flag\n"
           "  --argv-pos  print its argv position\n"
           "  --str       string value\n"
           "  --long      long value\n"
           "  --int       int value\n"
           "  --double    double value\n"
           "  --float     float value\n"
           "  --bool      bool value\n"
           "  -h, --help  show this message\n\n");
}

int main(int argc, char **argv)
{
    struct cmdr_state cmdr;
    cmdr_init(&cmdr, argc, argv);

    if (argc == 1 || cmdr_flag(&cmdr, "-h") || cmdr_flag(&cmdr, "--help")) {
        usage();
        cmdr_end(&cmdr);
        return 0;
    }

    if (cmdr_command(&cmdr, "hello")) {
        printf("command: hello\n");
    } else if (cmdr_command(&cmdr, "goodbye")) {
        printf("command: goodbye\n");
    } else if (cmdr_command(&cmdr, "ask")) {
        if (cmdr_confirm(&cmdr, "Continue?"))
            printf("answered: yes\n");
        else
            printf("answered: no\n");
    } else if (cmdr_command(&cmdr, "ask-no")) {
        if (cmdr_confirm_default(&cmdr, "Continue?", 0))
            printf("answered: yes\n");
        else
            printf("answered: no\n");
    }


    if (cmdr_flag(&cmdr, "-a"))
        printf("flag: -a\n");
    if (cmdr_flag(&cmdr, "--long-name"))
        printf("flag: --long-name\n");

    const char *str;
    if (cmdr_flag_str(&cmdr, "--str", &str))
        printf("flag: --str, value is %s\n", str);

    long long_value;
    if (cmdr_flag_long(&cmdr, "--long", &long_value))
        printf("flag: --long, value is %ld\n", long_value);

    int int_value;
    if (cmdr_flag_int(&cmdr, "--int", &int_value))
        printf("flag: --int, value is %d\n", int_value);

    double double_value;
    if (cmdr_flag_double(&cmdr, "--double", &double_value))
        printf("flag: --double, value is %f\n", double_value);

    float float_value;
    if (cmdr_flag_float(&cmdr, "--float", &float_value))
        printf("flag: --float, value is %f\n", float_value);

    bool bool_value;
    if (cmdr_flag_bool(&cmdr, "--bool", &bool_value))
        printf("flag: --bool, value is %s\n", bool_value ? "true" : "false");

    int idx;
    if (cmdr_find(&cmdr, "--argv-pos", &idx))
        printf("--argv-pos is at argv[%d]\n", idx);

    cmdr_end(&cmdr);
    return 0;
}
