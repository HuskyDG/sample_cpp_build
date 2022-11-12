#include <limits.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>


static int inotifyFd;


static void displayInotifyEvent(struct inotify_event *i)
{
    printf("    wd =%2d; ", i->wd);
    if (i->cookie > 0)
        printf("cookie =%4d; ", i->cookie);

    printf("mask = ");
    if (i->mask & IN_ACCESS)        printf("IN_ACCESS ");
    if (i->mask & IN_ATTRIB)        printf("IN_ATTRIB ");
    if (i->mask & IN_CLOSE_NOWRITE) printf("IN_CLOSE_NOWRITE ");
    if (i->mask & IN_CLOSE_WRITE)   printf("IN_CLOSE_WRITE ");
    if (i->mask & IN_CREATE)        printf("IN_CREATE ");
    if (i->mask & IN_DELETE)        printf("IN_DELETE ");
    if (i->mask & IN_DELETE_SELF)   printf("IN_DELETE_SELF ");
    if (i->mask & IN_IGNORED)      printf("IN_IGNORED ");
    if (i->mask & IN_ISDIR)      printf("IN_ISDIR ");
    if (i->mask & IN_MODIFY)        printf("IN_MODIFY ");
    if (i->mask & IN_MOVE_SELF)  printf("IN_MOVE_SELF ");
    if (i->mask & IN_MOVED_FROM)    printf("IN_MOVED_FROM ");
    if (i->mask & IN_MOVED_TO)    printf("IN_MOVED_TO ");
    if (i->mask & IN_OPEN)        printf("IN_OPEN ");
    if (i->mask & IN_Q_OVERFLOW)    printf("IN_Q_OVERFLOW ");
    if (i->mask & IN_UNMOUNT)      printf("IN_UNMOUNT ");
    printf("\n");

    if (i->len > 0)
        printf("        name = %s\n", i->name);
}

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

static void sigio_handler(int sig)
{
    char buf[BUF_LEN] __attribute__ ((aligned(8)));

    ssize_t numRead = read(inotifyFd, buf, BUF_LEN);
    if (numRead == 0) {
        fprintf(stderr, "read() from inotify fd returned 0!");
    }

    if (numRead == -1) {
        fprintf(stderr, "read");
    }

    printf("Read %ld bytes from inotify fd\n", (long) numRead);

    /* Process all of the events in buffer returned by read() */

    for (char *p = buf; p < buf + numRead; ) {
        struct inotify_event *event = (struct inotify_event *) p;
        displayInotifyEvent(event);
        p += sizeof(struct inotify_event) + event->len;
    }

}

int main(int argc, char *argv[])
{
    if (argc < 2 || strcmp(argv[1], "--help") == 0)
        printf("%s pathname...\n", argv[0]);

    inotifyFd = inotify_init();              /* Create inotify instance */
    if (inotifyFd == -1) {
        fprintf(stderr, "inotify_init");
    }

    /* Establish handler for "I/O possible" signal */

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = sigio_handler;
    if (sigaction(SIGIO, &sa, NULL) == -1) {
        fprintf(stderr, "sigaction");
    }

    /* Set owner process that is to receive "I/O possible" signal */

    if (fcntl(inotifyFd, F_SETOWN, getpid()) == -1) {
        fprintf(stderr, "fcntl(F_SETOWN)");
    }

    /* Enable "I/O possible" signaling and make I/O nonblocking
       for file descriptor */

    int flags = fcntl(inotifyFd, F_GETFL);
    if (fcntl(inotifyFd, F_SETFL, flags | O_ASYNC | O_NONBLOCK) == -1) {
        fprintf(stderr, "fcntl(F_SETFL)");
    }

    for (int j = 1; j < argc; j++) {
        int wd = inotify_add_watch(inotifyFd, argv[j], IN_ALL_EVENTS);
        if (wd == -1) {
            fprintf(stderr, "inotify_add_watch");
        }

        printf("Watching %s using wd %d\n", argv[j], wd);
    }

    while (1) {
        pause();
    }

    return EXIT_SUCCESS;
}
