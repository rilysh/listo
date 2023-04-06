#ifndef LISTO_H
#define LISTO_H

#ifndef ARRAY_SIZE
    #define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

#ifdef PROGRAM
    #undef PROGRAM
#endif
#ifdef FSIZE
    #undef FSIZE
#endif
#define PROGRAM             "listo"
#define RECENT_VIEW_FILE    ".recent_view"
#define TEMP_FILE           ".tmp.f"
#define FSIZE               100 /* Including a user name */
#define DATE_FMT_SIZE       19

struct Editors {
    char editor[8];
    char path[20];
};

struct Editors edt[] = {
    { "ed", "/usr/bin/ed" },
    { "micro", "/usr/bin/micro" },
    { "nano", "/usr/bin/nano" },
    { "nvim", "/usr/bin/nvim" },
    { "vi", "/usr/bin/vi" },
    { "vim", "/usr/bin/vim" }
};

static FILE *xopen(const char *filename, const char *modes)
{
    FILE *fp;
    char buf[FSIZE * 2];

    memset(buf, '\0', sizeof(buf));

    fp = fopen(filename, modes);

    if (fp == NULL) {
        sprintf(buf, "fopen() -> %s", filename);
        perror(buf);
        exit(EXIT_FAILURE);
    }

    return fp;
}

static void *xmalloc(unsigned long size)
{
    void *ptr = malloc(size);

    if (ptr == NULL) {
        perror("malloc()");
        return NULL;
    }

    return ptr;
}

static int xsprintf(char *dst, const char *fmt, ...)
{
    int n;
    va_list args;

    va_start(args, fmt);
    n = vsprintf(dst, fmt, args);
    va_end(args);

    return n;
}

static void errnor(int status, const char *err)
{
    fprintf(stderr, "%s\n", err);
    exit(status);
}

static void perrnor(int stat, const char *err)
{
    perror(err);
    exit(stat);
}

/* Shamelessly copied from: https://stackoverflow.com/a/2256974 */
static int rmdir_all(const char *path)
{
    DIR *dir;
    struct dirent *den;
    struct stat statbuf;
    char *buf;
    size_t len;
    int i, j;

    dir = opendir(path);
    i = -1;

    if (dir == NULL)
        return -1;

    while ((den = readdir(dir)) != NULL) {
        /* Skip the names "." and ".." as we don't want to recurse on them. */
        if (strcmp(den->d_name, ".") == 0 ||
            strcmp(den->d_name, "..") == 0)
            continue;

        len = strlen(path) + strlen(den->d_name) + 2; 
        buf = xmalloc(len);

        snprintf(buf, len, "%s/%s", path, den->d_name);

        if (stat(buf, &statbuf) != -1)
            if (S_ISDIR(statbuf.st_mode))
                j = rmdir_all(buf);
            else
                j = unlink(buf);
        else
            perrnor(EXIT_FAILURE, "stat()");

        i = j;

        free(buf);
    }

    if (closedir(dir) != 0)
        perrnor(EXIT_FAILURE, "closedir()");

    if (!i)
        i = rmdir(path);

    return i;
}

static long to_int(char *str)
{
    char *endptr;
    long val;

    val = strtol(str, &endptr, 0);

    if (endptr == str) {
        fprintf(stderr, "Error: Passed argument isn't an integer\n");
        return -1L;
    }

    return val;
}

#endif
