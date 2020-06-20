/*---------- Header Files -----------*/
#include <ctype.h> // iscntrl()
#include <errno.h> // errno, EAGAIN
#include <stdio.h> // printf() perror()
#include <stdlib.h> // atexit()
#include <sys/ioctl.h> // ioctl() TIOCGWINSZ struct winsize
#include <termios.h> // struct termios, tcgetattr(), tcsetattr(), ECHO, TCSAFLUSH, OPOST, IXON, ICANON, ISIG, IEXTEN
// VMIN, VTIME
#include <unistd.h> // read() STDIN_FILENO write() STDOUT_FILENO

/*----------- Defines ----------*/
#define CTRL_KEY(k) ((k) & 0x1F)

/*---------- Data -----------*/
// This struct contains the editor state
struct editorConfig {
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

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return -1;
    }
    else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/*---------- Input Functions ----------*/
// This function waits for a keypress and then handles it.
void editorProcessKeypress() {
    char c = editorReadKey();

    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
    }
}

/*---------- Output Functions -----------*/
// This is the function to draw "~" like defualt vim does.
void editorDrawRows() {
    int y;
    for (y = 0; y < E.screenrows; y++) {
        write(STDOUT_FILENO, "~\r\n",3);
    }
}

void editorRefreshScreen() {
    // The 4 means we are writing 4 bytes to the terminal.
    // "\x1b" is equal to 27 and it is the escape character.
    // We are using VT100 escape sequences, and J is the erase
    // in display. 2 tells it to clear the entire screen.
    write(STDOUT_FILENO, "\x1b[2J", 4);
    // H is the Cursor position command
    // This repositions the cursor back to the top-left corner.
    write(STDOUT_FILENO, "\x1b[H", 3);
    editorDrawRows();
    write(STDOUT_FILENO, "\x1b[H", 3);
}

/*---------- Init Functions -----------*/
void initEditor() {
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
