# k-truss
Compute k-cycle-trusses and k-flow-trusses

## Usage

From CUI Interface

    $ ./cmake .
    $ ./make
    $ ./k-truss -G=sample-graph.txt
    1 2 1
    2 3 1
    2 5 0
    3 1 1
    4 2 0
    4 3 0
    4 5 2
    4 8 2
    5 3 0
    5 6 2
    5 7 2
    6 4 2
    6 9 2
    7 4 2
    7 9 2
    8 6 2
    8 7 2
    8 10 0
    9 5 2
    9 8 2

By default, the program outputs the cycle-truss numbers of edges.
Each row is of the form "$u$ $v$ $k$", and it denotes that there is an edge from $u$ to $v$ with a cycle-truss number of $k$.

You can compute the flow-truss numbers of edges as follows:

    $ ./k-truss -G=sample-graph.txt --truss_type=flow



