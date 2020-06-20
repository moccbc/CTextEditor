#include <stdlib.h> // atexit()
#include <termios.h> // struct termios, tcgetattr(), tcsetattr(), ECHO, TCSAFLUSH
#include <unistd.h> // read() and STDIN_FILENO

struct termios orig_termios;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    // Adding "| ICANON" makes the program read byte by byte instead of line by line
    raw.c_lflag &= ~(ECHO | ICANON);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
    enableRawMode();
    char c;
    // Asking read() to read 1 byte from the std input into the var c
    // and will keep doing it until there aren't anymore bytes to read.
    // It returns the number of bytes that it read, and will return 0
    // when it reaches the end of the file.
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
    return 0;
}