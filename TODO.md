## Myshell
- Tentatively done. Should be tested with tapper/tappet once those are done.

## Tapper Main
- Probably need to refine exec code a bit
  - Initial attempt made with "./observe" for example, would need to account for this requirement in README/code
- Should define whether async/sync buffer here.

## Tappet Main
- Verify if system() works
- Should define whether async/sync buffer here.

## Observe
- Mostly working, but must put result into buffer: Cast shared memory to some structure, write to its attributes/slots.
- Existing base case should be modified to detect either EOF or no input (i.e. input = "\n")

## Reconstruct
- Need to cast shared memory to some structure, read/write from slots of respective buffers
- Need to define core restructuring logic.

## Tapplot
- Need to implement, Paula has info regarding gnuplot examples.