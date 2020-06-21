# This first line says "kilo" is what we want to name the program, and "kilo.c" is what is needed to create it.
yim: yim.c
        # This is the actual command to compile the program.
        # Make sure to use two actual tab inputs and not spaces.
        $(CC) yim.c -o yim -Wall -Wextra -pedantic -std=c99
