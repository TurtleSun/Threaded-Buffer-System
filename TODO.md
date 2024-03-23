## Myshell
- Enable output redirection in myshell
  - `ls > stdout.txt`
    - redirect stdout to file
  - `ls 1> stdout.txt`
    - redirect stdout to file (same as above)
  - `ls 2> stderr.txt`
    - redirect stderr to file
  - `ls &> stdout_stderr.txt` 
    - redirect stdout and stderr to same file
  - `ls < in.txt`
    - redirect file to stdin
- Enable piping in myshell
  - `ls | cat`
  - `ls -la | cat`
  - `ls | cat | grep .c`
- Enable background tasks
  - `sleep 10 &`
  - `make &`

## Tapper
- Need to create initial implementation

## Tappet
- Need to create initial implementation
