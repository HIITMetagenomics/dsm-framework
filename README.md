Distributed String Mining Framework
====

This software implements the algorithm published in [1]. 

[1] Niko Välimäki and Simon J. Puglisi: Distributed String Mining for
High-Throughput Sequencing Data. In Proc. 12th Workshop on Algorithms
in Bioinformatics (WABI'12), Springer-Verlag, LNCS 7534, pages
441-452, Ljubljana, Slovenia, September 9-14, 2012.


COMPILING
----
        
Run `make clean` and `make` to compile.


BUILDING THE INDEXES
----

The FASTA input files must first be preprocessed with builder:

```
    ./builder -v input.fasta
```

The output index will be stored under the filename `input.fasta.fmi`.
Use e.g. the script distributebuild.sh to build multiple indexes at
once. The script expects a list of host names as standard
input. Please see the actual script to setup the correct paths to your
fasta files etc. - The user will need to modify it to suite their own
cluster environment.

The current index uses 32 bit representation, thus, large input files
need to be split into multiple indexes and client processes.


INITIALIZING THE SERVER
----

The server must be set up before running the clients. It expects a
list of dataset names as input. This list must match the dataset names
that the clients use when they connect to the server.

```
Usage: ./metaserver [options]

  <stdin>       A list of expected dataset names.

Options:
 --port <p>     Listen to port number p.
 --topfreq <p>  Print the top-p output frequencies.
 --toptimes <p> Print the top-p latencies.
 --verbose      Print progress information.
 --debug        More verbose but still safe.
 --outputall    Even more verbose (not safe).
```


RUNNING THE CLIENTS
----

The clients are run using `metaenumerate`. The dataset name assigned
to each client is determined from the index filename. Currently, the
name is truncated from the index filename by finding the last `/`
symbol and the first `.` symbol. Thus, starting the client for the
index file `/path/index01.fmi` will use `index01` as the client's
dataset name. The server will compare the dataset name against its own
list (see above).

```
usage: ./metaenumerate [options] <index> <hostname>

 <index>        Index file.
 <hostname>     Host to connect to.

Options: 
 --check        Check integrity of index and quit.
 --port <p>     Connect to port <p>.
 --verbose      Print progress information.
Debug options:
 --debug        Print more progress information.
 --path <p>     Enumerate only below path p.
```

The script `distribute.sh` gives an example how to distribute the client
processes. - The user will need to modify it to suite their own
cluster environment.
