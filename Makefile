# This first line says "kilo" is what we want to name the program, and "kilo.c" is what is needed to create it.
kilo: kilo.c
        # This is the actual command to compile the program.
        # Make sure to use two actual tab inputs and not spaces.
        $(CC) kilo.c -o kilo -Wall -Wextra -pedantic -std=c99
