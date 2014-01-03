Distributed String Mining Framework
====

This software package implements the algorithm published in [1]. It also includes
the extensions  proposed in [2].

- [1] Niko Välimäki and Simon J. Puglisi: **Distributed String Mining for
High-Throughput Sequencing Data**. In _Proc. 12th Workshop on Algorithms
in Bioinformatics (WABI'12)_, Springer-Verlag, LNCS 7534, pages
441-452, Ljubljana, Slovenia, September 9-14, 2012.
- [2] Sohan Seth, Niko Välimäki, Samuel Kaski and Antti Honkela: **Exploration 
and retrieval of whole-metagenome sequencing samples**. 
Submitted. Available online [arXiv:1308.6074](http://arxiv.org/abs/1308.6074) 
[q-bio.GN], 2013. 


REQUIREMENTS AND COMPILING
----

Run `make clean` and `make` to compile. The requirements are

- GNU GCC (g++ 4.3) environment including the OpenMP library.
- TCP connectivity between the cluster nodes (some range of vacant TCP port numbers).
- some synchronization when initializing the server-side and 
client-side programs (i.e. wrapper-scripts).

The first requirement should be fullfilled in typical linux installations. 
The second requirement depends on the cluster environment (see technical 
details on this below). The third requirement is crucial and it will 
require some scripting to be able to run the framework on specific cluster 
environments; to help you get started, we provide example wrapper-scripts 
for the _SLURM batch job system_.


INSTALLATION AND GETTING STARTED
----

The current installation is, in short, to write suitable wrapper-scripts 
around the main C/C++ program. We provide example wrapper-scripts for the 
_SLURM batch job system_. The main computation is divided over

1. preprocessing of each dataset (`builder`),
2. 64-256 _CPU intensive_ processes (`metaserver`), and
3. _memory intensive_ processes which use very little CPU (`metaenumerate`).

The preprocessing step (1) is ran separately from (2) and (3). 
Steps (2) and (3) are run in parallel so that (2) is started first. The 
processes in Step (1) and (3) use memory that depends on the dataset sizes.
Processes from Step (2) require only a small amount of memory.

To get started, please download the following example data (~25 MB). The rest of
our example commands use these data files.

```
mkdir example
cd example
wget http://www.cs.helsinki.fi/u/nvalimak/dsm-framework/toydata-1.fasta.gz
wget http://www.cs.helsinki.fi/u/nvalimak/dsm-framework/toydata-2.fasta.gz
wget http://www.cs.helsinki.fi/u/nvalimak/dsm-framework/toydata-3.fasta.gz
wget http://www.cs.helsinki.fi/u/nvalimak/dsm-framework/toydata-4.fasta.gz
wget http://www.cs.helsinki.fi/u/nvalimak/dsm-framework/toydata-5.fasta.gz
```


PREPROCESSING THE DATA
----

The input data for the dsm-framework must be in FASTA format. It is
recommended that you first trim the FASTQ short-read data according
to sequencing quality and output the trimmed short-reads as a FASTA file.

Then the FASTA input files must first be preprocessed with builder:

```
    ./builder -v input.fasta
```

It will output the resulting index under the filename `input.fasta.fmi`.
You should run `builder` independendly over each of your input datasets.
The preprocessing can be parallelized by building multiple indexes
simultaneously, however, these processes might require
considerable amount of main-memory (depending on the input size).




Use e.g. the script `distributebuild.sh` to build multiple indexes at
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
