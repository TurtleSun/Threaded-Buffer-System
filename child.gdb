b main
focus cmd
set follow-fork-mode child
run -b sync -s 10 < testinput.txt
