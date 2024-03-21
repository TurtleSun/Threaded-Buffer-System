# CS410 HW1: A String Searching Program (+ custom Printf)

## Overview
The programs in this suite serve two main purposes:
1) To enable file traversal from a specified directory to search for a particular string pattern or regex pattern in the contents of various files. 
2) To demonstrate the capabilities of the custom printf() methodology.

See below for provided files.

### Executable Program

- `finds`: A program that recursively searches through the contents of directories 

### Supporting Code

- `myRegex.c`: Used in combination with `finds`, handles the majority of the Regex comparison logic, particularly implementing the functionality of the period, asterisk, question mark, and parentheses operators.

- `itoa.c`: Used in combination with the custom myPrintf(), handles the conversion of decimal numbers to an ASCII equivalent.

- `myPrintf.c`: Used in combination with `finds`, handles the need to print out strings with variable arguments to stdout.

## Setup
The following instructions assume a UNIX-based operating system and that your current working directory in the terminal contains all of the files in this codebase. A Makefile has been provided to aid in assembling, compiling, and linking together the source code with the `gcc` compiler. In order to create the required programs, you can use the `make` command in your terminal to set up everything for you.

    make

If you'd rather create a specific object file or executable file, you can use the `make` combination with an argument of the filename you wish to create as shown below:

    make getsections

From there, the program can be executed as desired. See the overview for required arguments, but an example is as follows:

    ./finds -p <pathname>  [-f c|h|S] [-l] -s <string-expression>

The command line arguments made available with the finds program include:
- Required:
  - `-p <pathname>`: This specifies the starting point for where the finds program should begin traversing from.
  - `-s <string-expression>`: This specifies the pattern that the finds program will try to find within the files in the specified path.
- Optional:
  - `-f c|h|s`: This specifies which types of files the finds program should search through, namely C source code files (.c), C header files (.h), or assembly source code files (.S). Note that this is case sensitive, and that if the -f flag is used, only one filetype can be searched at a time.
  - `-l`: This enables the finds program to search symbolic links on top of the existing search for regular files that is enabled by default. If the -f flag is used as well, the program will only search files where the target of the symbolic link has the matching file suffix.


If you'd like to remove the object files and executables at any given time, you can use the following command:

    make clean

## Finds: Current Feature Set and Limitations
- This version of the finds program enables both string literal comparison and regex comparison in the files being searched for.
- While support is enabled for the period, asterisk, and question mark operators, only one of each wildcard character is supported in any given search pattern.
- Up to two sets of non-nested parentheses are allowed for any given search pattern, ideally to be used in combination with the asterisk and question mark operators.
- No wildcard characters can be placed inside of a set of parentheses (i.e. `ab(c.d)ef`).

## myPrintf: An Overview
This program moreso serves as a demonstration of the custom myPrintf functionality, and has not yet been implemented into other files such as finds. This implementation takes values from registers and the stack corresponding to the x86-64 C calling conventions and prints out the format string and any applicable arguments formatted accordingly. Currently, this version of myPrintf has limited support for checking whether or not the number of arguments in the format string and the number of arguments provided matches. Currently, if four arguments or less are in the format string, the number of arguments can be checked. If it does not match, the program will exit with an error.

You can check out the source code in myPrintf.c and run the demonstration as follows:

    ./myPrintf

---

Jake Gustin, 2024