/*---------- Header Files -----------*/
#include <ctype.h> // iscntrl()
#include <errno.h> // errno, EAGAIN
#include <stdio.h> // printf() perror()
#include <stdlib.h> // atexit() realloc() free()
#include <string.h> // memcpy()
#include <sys/ioctl.h> // ioctl() TIOCGWINSZ struct winsize
#include <termios.h> // struct termios, tcgetattr(), tcsetattr(), ECHO, TCSAFLUSH, OPOST, IXON, ICANON, ISIG, IEXTEN
// VMIN, VTIME
#include <unistd.h> // read() STDIN_FILENO write() STDOUT_FILENO

/*----------- Defines ----------*/
#define CTRL_KEY(k) ((k) & 0x1F)
#define KILO_VERSION "0.0.1"

/*---------- Data -----------*/
// This struct contains the editor state
struct editorConfig {
    int cx, cy;
    int screenrows;
    int screencols;
    struct termios orig_termios;
};

struct editorConfig E;

/*---------- Terminal Functions -----------*/
void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
        die("tcsetattr");
    }
}

void enableRawMode() {
    if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) {
        die("tcsetattr");
    }
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;
    // Adding BRKINT, INPCK, ISTRIP, and CS8 disables other miscellaneous
    // flags, but they are there more for the legacy of enabling 
    // "raw mode".

    // This disables ctrl-s and ctrl-q.
    // These are inputs for software flow control.
    // Adding ICNRL fixes ctrl-m to be correctly read in bytes.
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    // This turns off output proessing. This is when each newline "\n"
    // is translated into "\r\n".
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    // Adding "| ICANON" makes the program read byte by byte instead of line by line.
    // This disables the canonical mode ie get input by pressing enter mode.
    // Adding ISIG local flag disables ctrl-c and ctrl-z
    // By default: ctrl-c -> sends SIGINT to stop program
    //             ctrl-z -> sends SIGTSTP to suspend current process
    // Adding IEXTEN disables ctrl-v.
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    //tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        die("tcsetattr");
    }
}

// The job of this function is to wait for one keypress and return it.
char editorReadKey() {
    int nread;
    char c;
    // Asking read() to read 1 byte from the std input into the var c
    // and will keep doing it until there aren't anymore bytes to read.
    // It returns the number of bytes that it read, and will return 0
    // when it reaches the end of the file.
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}

int getCursorPosition(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    while (i < sizeof(buf) - 1) {
        if(read(STDIN_FILENO, &buf[i], 1) != 1) break;

        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        // This is a fallback method for when ioctl doesn't work.
        // c moves the cursor to the right and B moves the cursor down.
        // We use 999 (a large number) to ensure that the cursor will move to the bottom
        // right corner.
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return getCursorPosition(rows, cols);
    }
    else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/*---------- Append Buffer ----------*/
// The append buffer gets rid of the need to call a bunch of small write()s.
// This is important because this allows the program to update the whole screen at once.
// Otherwise, there could be several small unpredicatable pauses between write()s, causing
// an annoying flicker effect.
struct abuf {
    char* b;
    int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
    // Creating a block of memory that is the size of the current string
    // plus the size of the string that we are appending.
    char *new = realloc(ab->b, ab->len + len);

    if (new == NULL) return;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab) {
    free(ab->b);
}

/*---------- Input Functions ----------*/
void editorMoveCursor(char key) {
    switch (key) {
        case 'a':
            E.cx--;
            break;
        case 'd':
            E.cx++;
            break;
        case 'w':
            E.cy--;
            break;
        case 's':
            E.cy++;
            break;
    }
}

// This function waits for a keypress and then handles it.
void editorProcessKeypress() {
    char c = editorReadKey();

    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case 'w':
        case 's':
        case 'a':
        case 'd':
            editorMoveCursor(c);
            break;
    }
}

/*---------- Output Functions -----------*/
// This is the function to draw "~" like defualt vim does.
void editorDrawRows(struct abuf *ab) {
    int y;
    for (y = 0; y < E.screenrows; y++) {
        if (y == E.screenrows / 3) {
            // Displaying welcome message
            char welcome[80];
            int welcomelen = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION);
            if (welcomelen > E.screencols) welcomelen = E.screencols;
            // Centering the welcome message
            int padding = (E.screencols - welcomelen) / 2;
            if (padding) {
                abAppend(ab, "~", 1);
                padding--;
            }
            while (padding--) abAppend(ab, " ", 1);
            abAppend(ab, welcome, welcomelen);
        }
        else {
            abAppend(ab, "~", 1);
        }

        abAppend(ab, "\x1b[K", 3);
        if (y < E.screenrows - 1) {
            abAppend(ab, "\r\n", 2);
        }
    }
}

void editorRefreshScreen() {
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    // The 4 means we are writing 4 bytes to the terminal.
    // "\x1b" is equal to 27 and it is the escape character.
    // We are using VT100 escape sequences, and J is the erase
    // in display. 2 tells it to clear the entire screen.
    //abAppend(&ab, "\x1b[2J", 4);
    // H is the Cursor position command
    // This repositions the cursor back to the top-left corner.
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    // Positions the cursor
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    // Freeing the memory used by abuf
    abFree(&ab);
}

/*---------- Init Functions -----------*/
void initEditor() {
    E.cx = 0;
    E.cy = 0;

    if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main() {
    enableRawMode();
    initEditor();

    while(1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}
