# CS410 HW2: Myshell, Tapper, and Tappet

## Overview
The programs in this suite serve two main purposes:
1) To create a custom, basic implementation of a shell interface
2) To create a program capable of handling a pipelined process

See below for provided files.

### Executable Program

- `myshell`: A program that serves as the shell interface, accepting input from stdin (either terminal input or a redirected file input)

### Supporting Code

- (None so far)

## Setup
The following instructions assume a UNIX-based operating system and that your current working directory in the terminal contains all of the files in this codebase. A Makefile has been provided to aid in assembling, compiling, and linking together the source code with the `gcc` compiler. In order to create the required programs, you can use the `make` command in your terminal to set up everything for you.

    make

If you'd rather create a specific object file or executable file, you can use the `make` combination with an argument of the filename you wish to create as shown below:

    make myshell

From there, the myshell program can be executed as desired. An example to use myshell with the current terminal interace is as follows:

    ./myshell

The myshell program can be executed as an unattended program, instead accepting commands from a file passed into stdin as follows:

    ./myshell < myinput.txt


If you'd like to remove the object files and executables at any given time, you can use the following command:

    make clean

---

Jake Gustin | Paula Lopez Burgos | Michelle Luo Xin Sun , 2024
