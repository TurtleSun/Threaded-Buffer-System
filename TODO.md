## Myshell
- Tidy up possible edge cases
  - `ls -la >` yields an open error, need to identify missing redirection operand.
  - need to check if we maxed out our file descriptors
- Enable piping in myshell
  - `ls | wc`
  - `ls -la | wc`
  - `ls | cat | grep .c`
- Enable background tasks
  - `sleep 10 &`
  - `make &`

## Tapper
- Need to create initial implementation

## Tappet
- Need to create initial implementation
