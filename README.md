# marsl
## (TODO, better name)

marsl is a memory array redcode simulator, aiming to be like [this popular one](https://github.com/mbarbon/pMARS). Currently, it can parse redcode and run a single-warrior simulation.

## compilation

You must have a C compiler and [bison](https://www.gnu.org/software/bison) to build this project.

`make test` creates an executable named `test`, which runs all tests in the directory ./tests/ given no arguments, or specific tests given by name.

`make` creates the main executable for this whole project. However, this executable does nothing currently, as I'm focusing on testing. See `make test`.
