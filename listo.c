#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <pwd.h>
#include <sys/stat.h>
#include <errno.h>

#include "listo.h"

static char *keep_recent_file(char *wfile)
{
    FILE *fp;
    struct passwd *pd;
    char *buf, recf[FSIZE];
    unsigned long eol;

    pd = getpwuid(getuid());

    memset(recf, '\0', sizeof(recf));

    strcat(recf, "/home/");
    strcat(recf, pd->pw_name);
    strcat(recf, "/.listo/");
    strcat(recf, RECENT_VIEW_FILE);

    if (wfile != NULL) {
        if (access(recf, F_OK) != -1)
            if (remove(recf) != 0)
                perrnor(EXIT_FAILURE, "remove()");
        /* Ignore errors */

        fp = fopen(recf, "w");

        /**
        * Used normal fopen() instead of wrapped xopen(),
        * since I've to return NULL. Same happened in below
        * as well.
        */
        if (fp == NULL)
            return NULL;

        fprintf(fp, "%s\n", wfile);
        fclose(fp);

        return NULL;
    } else {
        buf = xmalloc(FSIZE);
        fp = fopen(recf, "r");

        if (fp == NULL) {
            free(buf);

            return NULL;
        }

        memset(buf, '\0', FSIZE);
        fread(buf, 1, FSIZE, fp);
        fclose(fp);

        eol = (strlen(recf) - strlen(RECENT_VIEW_FILE)) + DATE_FMT_SIZE;
        buf[eol] = '\0';

        return buf;
    }
}

static void create_new_list(void)
{
    FILE *fp;
    time_t ti;
    struct passwd *pd;
    struct tm *tm;
    char *krf, tn[FSIZE];

    ti = time(NULL);
    tm = localtime(&ti);
    pd = getpwuid(getuid());
    errno = 0;

    if (pd == NULL)
        perrnor(EXIT_FAILURE, "getpwuid()");

    memset(tn, '\0', sizeof(tn));

    strcat(tn, "/home/");
    strcat(tn, pd->pw_name);
    strcat(tn, "/.listo/");

    if (mkdir(tn, 0755) != 0)
        if (errno != EEXIST)
            perrnor(EXIT_FAILURE, "mkdir");

    xsprintf(strlen(tn) + tn,
        "%d-%02d-%02d_%02d:%02d:%02d",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec
    );

    krf = keep_recent_file(tn);
    fp = xopen(tn, "a");

    fprintf(stdout, "OK: Created %s\n", tn);
    fclose(fp);
    free(krf);
}

static void create_append_list(char *msg)
{
    FILE *fp;
    char *krf;
    long size;

    krf = keep_recent_file(NULL);

    if (krf == NULL)
        errnor(EXIT_FAILURE, "Error: No recently created list was found");

    fp = xopen(krf, "a");

    fseek(fp, 1L, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    if (size > 1)
        fprintf(fp, "\n%s", msg);
    else
        fprintf(fp, "%s", msg);

    fclose(fp);
    free(krf);
}

static void print_have_list(int line_nums)
{
    FILE *fp;
    char *buf, *krf;
    long size;
    int c, i;

    krf = keep_recent_file(NULL);

    if (krf == NULL)
        errnor(EXIT_FAILURE, "Error: No recently created list was found");

    fp = xopen(krf, "r");
    i = 1;

    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    buf = xmalloc((unsigned long)(size + 1));

    memset(buf, '\0', (unsigned long)(size + 1));

    if (line_nums) {
        while ((c = fgetc(fp)) != EOF) {
            if (c != '\n' && i == 1) {
                fprintf(stdout, "%d. ", i);

                i++;
            }

            fprintf(stdout, "%c", c);

            if (c == '\n') {
                fprintf(stdout, "%d. ", i);

                i++;
            }
        }
    } else {
        while ((c = fgetc(fp)) != EOF)
            fprintf(stdout, "%c", c);
    }

    /* Add a newline, as the text file won't have a newline before EOF */
    fprintf(stdout, "\n");

    fclose(fp);
    free(buf);
    free(krf);
}

static void open_with_editor(char *filename)
{
    char *krf, *args[4];
    int size, i;

    if (filename != NULL) {
        krf = filename;
    } else {
        krf = keep_recent_file(NULL);

        if (krf == NULL)
            errnor(EXIT_FAILURE, "Error: No recently created list was found");
    }

    size = ARRAY_SIZE(edt) + 1;
    args[1] = krf;

    for (i = 0; i < size; i++) {
        if (access(edt[i].path, F_OK) != -1) {
            args[0] = edt[i].path;

            fprintf(stdout,
                "Info: Trying to open %s with %s\n", krf, edt[i].editor
            );

            if (execv(edt[i].path, args) == -1) {
                perror("execv()");
                free(krf);
                exit(EXIT_FAILURE);
            }
        }

        if (i == (size - 1))
            fprintf(stderr,
                "Error: None of the editors are installed on your system.\n"
                "(Supported editors: ed, micro, nano, nvim, vi and vim)\n"
            );
    }

    free(krf);
}

static void rm_line(long line)
{
    FILE *fp, *fw;
    struct passwd *pd;
    char *krf, *buf, fpath[FSIZE];
    int matched;
    long cur, size;

    if (line < 1) {
        fprintf(stderr, "Error: Line number can't be zero or negative\n");
        exit(EXIT_FAILURE);
    }

    pd = getpwuid(getuid());

    memset(fpath, '\0', sizeof(fpath));

    strcat(fpath, "/home/");
    strcat(fpath, pd->pw_name);
    strcat(fpath, "/.listo/");
    strcat(fpath, TEMP_FILE);

    krf = keep_recent_file(NULL);

    if (krf == NULL)
        errnor(EXIT_FAILURE, "Error: No recently created list was found");

    fp = xopen(krf, "r");
    fw = xopen(fpath, "w");

    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    if (size == 0) {
        fprintf(stdout, "Error: List entries are empty\n");
        exit(EXIT_FAILURE);
    }

    buf = xmalloc((unsigned long)(size + 1));
    cur = 1;
    matched = 0; /* Temporary boolean alike to check if cur and line matched together */

    memset(buf, '\0', (size + 1));

    while (fgets(buf, size, fp) != NULL) {
        if (cur != line)
            fprintf(fw, "%s", buf);
        else
            matched = 1;

        /* Add a null-terminator at the end of the string */
        if (feof(fp)) {
            if (matched != 1) {
                fprintf(stderr, "Error: Couldn't find line number %ld\n", line);

                goto cleanup;
            }

            fseek(fw, -1, SEEK_END);
            fputc('\0', fw);
            fprintf(stdout, "OK: Removed\n");
        }

        if (ferror(fp))
            goto cleanup;

        cur++;
    }

    if (rename(fpath, krf) != 0)
        perrnor(EXIT_FAILURE, "rename()");

cleanup:
    fclose(fp);
    fclose(fw);
    free(krf);
    free(buf);
}

static void print_all_lists(void)
{
    DIR *dir;
    struct passwd *pd;
    struct dirent *den;
    char *krf, buf[FSIZE];
    int i;

    pd = getpwuid(getuid());

    if (pd == NULL)
        perrnor(EXIT_FAILURE, "getpwuid()");

    memset(buf, '\0', sizeof(buf));

    strcat(buf, "/home/");
    strcat(buf, pd->pw_name);
    strcat(buf, "/.listo");

    i = 1;
    dir = opendir(buf);

    if (dir == NULL)
        errnor(EXIT_FAILURE, "Error: It seems quite empty here");

    krf = keep_recent_file(NULL);

    if (krf == NULL)
        errnor(EXIT_FAILURE, "Error: No recently created list was found");

    while ((den = readdir(dir)) != NULL) {
        if (strcmp(den->d_name, ".") == 0 ||
            strcmp(den->d_name, "..") == 0 ||
            strcmp(den->d_name, RECENT_VIEW_FILE) == 0 ||
            strcmp(den->d_name, TEMP_FILE) == 0)
            continue;

        /* Make sure needle isn't an empty string */
        if (strlen(den->d_name) != 0 &&
            krf != NULL &&
            strstr(krf, den->d_name) != NULL)
            fprintf(stdout, "%d. %s [current]\n", i, den->d_name);
        else
            fprintf(stdout, "%d. %s\n", i, den->d_name);

        i++;
    }

    free(krf);

    if (closedir(dir) != 0)
        perrnor(EXIT_FAILURE, "closedir()");
}

static void edit_recent_one(void)
{
    struct passwd *pd;
    char buf[FSIZE];

    pd = getpwuid(getuid());

    memset(buf, '\0', sizeof(buf));

    strcat(buf, "/home/");
    strcat(buf, pd->pw_name);
    strcat(buf, "/.listo/");
    strcat(buf, RECENT_VIEW_FILE);

    if (access(buf, F_OK) != 0)
        /* We'd expect file to be in .recent_view */
        perrnor(EXIT_FAILURE, "access()");

    open_with_editor(buf);
}

static void purge_all(void)
{
    char buf[FSIZE];
    struct passwd *pd;

    memset(buf, '\0', sizeof(buf));
    fprintf(stdout,
        "Purge all existing list(s)? "
        "[Y/y or any]: "
    );

    if (fgets(buf, 2, stdin) == NULL)
        perrnor(EXIT_FAILURE, "fgets()");

    if (strcmp(buf, "Y") == 0 || strcmp(buf, "y") == 0) {
        pd = getpwuid(getuid());

        if (pd == NULL)
            perrnor(EXIT_FAILURE, "getpwuid()");

        memset(buf, '\0', sizeof(buf));

        strcat(buf, "/home/");
        strcat(buf, pd->pw_name);
        strcat(buf, "/.listo");

        if (rmdir_all(buf) == -1) {
            fprintf(stderr, "Error: No list was found\n");
            exit(EXIT_FAILURE);
        }

        fprintf(stdout, "OK: Purged\n");
    } else {
        fprintf(stdout, "Answer was \"NO\", so now leaving...\n");
        exit(EXIT_SUCCESS);
    }
}

static void print_help(void)
{
    fprintf(stdout,
        "%s - A small program to manage your task list\n\n"
        "Usage:\n"
        " -a [i(0)..i(n)] -- Append item(s) in a already existing list file\n"
        " -d              -- Print all existing list(s)\n"
        " -e              -- Edit a list entry to make it current\n"
        " -n [i(0)..i(n)] -- Create a new list with pre-adjusted item(s)\n"
        " -o              -- Open the list file to a terminal-based editor\n"
        " -p [t]          -- Print all items from the most recent file\n"
        " -r              -- Purge all old list(s)\n"
        " -l [lnum]       -- Delete a specific line from the current list\n"
    , PROGRAM);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_help();
        exit(EXIT_SUCCESS);
    }

    int opt, i;

    if (argc > 1 && argv[1][0] != '-') {
        for (i = 1; i < argc; i++)
            create_append_list(argv[i]);

        fprintf(stdout, "OK: Appended\n");
        exit(EXIT_SUCCESS);
    }

    const char *short_opts;
    int a_flag, d_flag, e_flag, n_flag, p_flag,
    o_flag, r_flag, l_flag;

    a_flag = 0, d_flag = 0, e_flag = 0, n_flag = 0,
    p_flag = 0, o_flag = 0, r_flag = 0, l_flag = 0;
    short_opts = "a:l:denoprh";
    opterr = 0;

    while ((opt = getopt(argc, argv, short_opts)) != -1) {
        switch (opt) {
        case 'a':
            a_flag = 1;
            break;

        case 'd':
            d_flag = 1;
            break;

        case 'e':
            e_flag = 1;
            break;

        case 'n':
            n_flag = 1;
            break;

        case 'o':
            o_flag = 1;
            break;

        case 'p':
            p_flag = 1;
            break;

        case 'r':
            r_flag = 1;
            break;

        case 'l':
            l_flag = 1;
            break;

        case 'h':
        default:
        /* No flags */
            print_help();
            break;
        }
    }

    if (a_flag) {
        for (i = 2; i < argc; i++)
            create_append_list(argv[i]);

        fprintf(stdout, "OK: Appended\n");
    }

    if (d_flag)
        print_all_lists();

    if (e_flag)
        edit_recent_one();

    if (n_flag) {
        if (argc > 2) {
            create_new_list();

            for (i = 2; i < argc; i++)
                create_append_list(argv[i]);

            fprintf(stdout, "OK: Appended\n");
        } else {
            create_new_list();
        }
    }

    if (o_flag)
        open_with_editor(NULL);

    if (p_flag)
        print_have_list((argc > 2 && argv[2][0] == 't') ? 1 : 0);

    if (r_flag)
        purge_all();

    if (l_flag) {
        long ret; 

        if ((ret = to_int(argv[2])) != -1)
            rm_line(ret);
    }
}
