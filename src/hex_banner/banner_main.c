#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ncurses.h>
#include <time.h>

#define TTY "/dev/tty"
static int open_terminal(char **result, int mode) {
    const char *device = TTY;
    *result = strdup(device);
    return open(device, mode);
}

int main(int argc, char *argv[])
{
    int ch;

    initscr();
    WINDOW *win = newwin(LINES / 2, COLS, 0, 0);
    // raw(); /* Line buffering disabled*/
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);

    if (has_colors()) {
        start_color();
        init_color(COLOR_MAGENTA, 299, 407, 976);
        init_color(COLOR_CYAN, 341, 886, 886);
        init_pair(1, COLOR_WHITE, COLOR_MAGENTA);
        init_pair(2, COLOR_CYAN, COLOR_BLACK);
    }

    int fd1, fd2;
    char *device = NULL;
    FILE *pipe_input = stdin;
    char buf[1024] = {0};

    if(!isatty(fileno(stdin))) {
        if((fd1 = open_terminal(&device, O_RDONLY)) >= 0) {
            if((fd2 = dup(fileno(stdin))) >= 0) {
                pipe_input = fdopen(fd2, "r");
                if(freopen(device, "r", stdin) == 0)
                    perror("cannot open tty-input");
                if(fileno(stdin) != 0)
                    (void)dup2(fileno(stdin), 0);
            }
        }
    }

    refresh();
    wbkgd(win, COLOR_PAIR(1));
    wrefresh(win);

    int node_starty = (LINES / 2 - 10) / 2;
    int node_startx = (COLS - 50) / 2;
    attrset(COLOR_PAIR(1));
    attron(A_BOLD);

    char separator[] = "----------";
    int y_inc = 0;
    while (fgets(buf, 1024, pipe_input) != NULL) {
        int i;
        if(strncmp(buf, separator, sizeof(separator) - 1) == 0) {
            break;
        } else {
            for(i=0; buf[i]!='\n'; i++)
                mvprintw(node_starty + y_inc, node_startx + i, "%c", buf[i]);
        }
        y_inc++;
    }

    int cluster_starty = (LINES / 2) + node_starty;
    int cluster_startx = (COLS - 50) / 2;
    attrset(COLOR_PAIR(2));
    attron(A_BOLD);
    y_inc = 0;
    while (fgets(buf, 1024, pipe_input) != NULL) {
        int i;
        for(i=0; buf[i]!='\n'; i++) {
            mvprintw(cluster_starty + y_inc, cluster_startx + i, "%c", buf[i]);
        }
        y_inc++;
    }

    mvprintw(LINES - 2, 1, "Press %s to continue", "F2");
    int x=1;
    while((ch = getch())) {
        if(ch == KEY_F(2)) {
            break;
        } else {
	    time_t it = time(NULL);
	    struct tm *ptm = gmtime(&it);
	    mvprintw(LINES - 2, (x++ % 10), "%21s | %s", "Press F2 to continue", asctime(ptm));
        }
    }

    endwin();
    return 0;
}
