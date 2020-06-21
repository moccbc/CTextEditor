# CTextEditor

## Introduction

### What is it?
This is a text editor in the command line. The program is written in C made by following [this](https://viewsourcecode.org/snaptoken/kilo/) tutorial. The Features section explains which features I implemented following the tutorial, and which features I implemented on my own.

### Purpose
The following are the main purposes of undertaking this project:

- Grasp an understanding of how Vim works under the hood
- Developing usable software
- Adding features to an existing codebase

## Prerequisites
The c compiler and the make programis needed in order to run the program. Below instructions are from Chapter 1: Setup of the tutorial.

### Windows
Some kind of Linux environment is needed. See the [installation guide](https://docs.microsoft.com/en-us/windows/wsl/install-win10?redirectedfrom=MSDN) in order to install Bash on Windows. Then run `sudo apt-get install gcc make` to install the C compiler and make.

### MacOS
Open terminal and run `xcode-select --install` and click install. This will install a C compiler with make.

### Linux
Run `sudo apt-get install gcc make` in the terminal to install the C compiler and the make program.

## Installation
Open up BASH in Windows or Linux, terminal on MacOS and run the following commands.

    $ git clone https://github.com/moccbc/CTextEditor.git
    $ make

## Running the program
Run the following commands to use the program. Replace "[filename]" with an actual file you want to edit. The "[filename]" can be a non-existing file. This will create a new file.

    $ ./yim [filename]

## Features
Below are the features that are implemented into the text editor. The "Tutorial" section covers what I implemented based on following the tutorial. The "On my own" section covers what I implemented, well, on my own.

### Tutorial
- Movement of the cursor
- Exiting the file
- Viewing the file
- Editing the file
- Creating a new file (./yim [newfileName])
- Saving the file

### On my own
- Searching (Not started)
  * This is covered in the tutorial, but I decided to try and implement this on my own.
- Vim style movement keys (Not Started)
- Modal editing (Not Started)
- Copy and paste (Not Started)
- Line numbers (Not Started)
