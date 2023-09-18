# marsl
## (TODO, better name)

marsl is a memory array redcode simulator, aiming to be like [this popular one](https://github.com/mbarbon/pMARS). Currently, it can parse redcode and run a single-warrior simulation.

## compilation

`make depend` updates dependencies.

`make parser` uses [bison](https://www.gnu.org/software/bison) to generate a parser given parser.y. The parser needs to be generated in order to use any other make command, except `make clean`, which invokes `make parser` automatically.

`make test` creates an executable named `test`, which runs all tests in the directory ./tests/ given no arguments, or specific tests given by name.

`make corewars` creates the main executable for this whole project. However, this executable does nothing currently, as I'm focusing on testing. See `make test`.
