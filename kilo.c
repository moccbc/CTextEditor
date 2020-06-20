/*---------- Header Files -----------*/
#include <ctype.h> // iscntrl()
#include <errno.h> // errno, EAGAIN
#include <stdio.h> // printf() perror()
#include <stdlib.h> // atexit()
#include <termios.h> // struct termios, tcgetattr(), tcsetattr(), ECHO, TCSAFLUSH, OPOST, IXON, ICANON, ISIG, IEXTEN
// VMIN, VTIME
#include <unistd.h> // read() and STDIN_FILENO

/*----------- Defines ----------*/
#define CTRL_KEY(k) ((k) & 0x1F)

/*---------- Data -----------*/
struct termios orig_termios;

/*---------- Terminal Functions -----------*/
void die(const char *s) {
    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
        die("tcsetattr");
    }
}

void enableRawMode() {
    if(tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        die("tcsetattr");
    }
    atexit(disableRawMode);

    // Adding BRKINT, INPCK, ISTRIP, and CS8 disables other miscellaneous
    // flags, but they are there more for the legacy of enabling 
    // "raw mode".

    struct termios raw = orig_termios;
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

/*---------- Input Functions ----------*/
// This function waits for a keypress and then handles it.
void editorProcessKeypress() {
    char c = editorReadKey();

    switch (c) {
        case CTRL_KEY('q'):
            exit(0);
            break;
    }
}

/*---------- Header Files -----------*/
int main() {
    enableRawMode();
    while(1) {
        editorProcessKeypress();
    }
    return 0;
}
