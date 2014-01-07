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


CHANGE LOG
----

* **2014-01-14** Added example scripts for the SLURM batch job system (under `wrapper-SLURM/`).
                 Improved the server-side parallelization. Scaling was tested up to 256 (high-CPU) server-processes 
                 and 50,944 simultaneous (low-CPU) client-threads divided over 29 cluster nodes.


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
gunzip toydata-?.fasta.gz
```


PREPROCESSING THE DATA
----

The input data for the dsm-framework must be in FASTA format. It is
recommended that you first trim the FASTQ short-read data according
to sequencing quality and output the trimmed short-reads as a FASTA file.

Then the FASTA input files must be preprocessed with:
```
    ./builder -v input.fasta
```
It will output the resulting _index_ under the filename `input.fasta.fmi`.
You should run `builder` independendly over each of your input datasets.
The preprocessing can be parallelized by building multiple indexes
simultaneously, however, these processes might require
considerable amount of main-memory (depending on the input size).

If you have access to a cluster environment with SLURM batch job system,
you can use the following commands to build indexes for the example data.
Notice that you might need to give the input filename with full directory 
path according to your own cluster environment.
```
export DSM_FRAMEWORK_PATH=/path/to/dsm-framework/
sbatch $DSM_FRAMEWORK_PATH/wrapper-SLURM/example-builder.sh toydata-1.fasta
sbatch $DSM_FRAMEWORK_PATH/wrapper-SLURM/example-builder.sh toydata-2.fasta
sbatch $DSM_FRAMEWORK_PATH/wrapper-SLURM/example-builder.sh toydata-3.fasta
sbatch $DSM_FRAMEWORK_PATH/wrapper-SLURM/example-builder.sh toydata-4.fasta
sbatch $DSM_FRAMEWORK_PATH/wrapper-SLURM/example-builder.sh toydata-5.fasta
```
See the specific scripts above for details and SLURM settings. 

Remark: If your cluster does not have SLURM, modify e.g. the script 
`wrapper-simple/distributebuild.sh` to build multiple indexes at once. 
The script expects a list of cluster node names as standard
input. Please see the actual script to setup the correct paths to your
fasta files etc. You will need to modify it to suite your own cluster environment.


INITIALIZING THE SERVER
----

The server must be set up before running the clients. It expects a
list of dataset names as input. This list must match the dataset names
that the clients use when they connect to the server.

```
usage: ./metaserver [options] < names.txt

  names.txt         A list of expected library names (read from stdin).

Mandatory option:
 -E,--emax <double> Maximum entropy to output.

Other options:
 -p,--port <p>      Listen to port number p.
 -P,--pmin <int>    p_min value.
 -e,--emin <double> Minimum entropy to output (default 0.0)
 -F,--topfreq <p>   Print the top-p output frequencies.
 -T,--toptimes <p>  Print the top-p latencies.
 -v,--verbose       Print progress information.
 --debug            More verbose but still safe.
 -A,--outputall     Even more verbose (not safe).
```
Here follows an example using the SLURM scripts. First, you need to construct
a text file that contains a list of dataset names. In this example, you can use:
```
echo -e "toydata-1\ntoydata-2\ntoydata-3\ntoydata-4\ntoydata-5" > sample-names.txt
```
Thus, the resulting file `sample-names.txt` should look like
```
toydata-1
toydata-2
toydata-3
toydata-4
toydata-5
```
Then you can initialize the server-side processes using the attached script 
```
export DSM_FRAMEWORK_PATH=/path/to/dsm-framework/
rm -fr tmp_dsmframework_config
mkdir tmp_dsmframework_config
$DSM_FRAMEWORK_PATH/wrapper-SLURM/example-server.sh sample-names.txt tmp_dsmframework_config
```
The script will store temporary configuration files under
the directory `tmp_dsmframework_config/`. These files are used
track the hostname of each server process so that the client-side
processes know which hosts to connect to. The files also store the
TCP port number and the (unique) hash associated with each server.


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
usage: ./metaenumerate [options] <index>  < hostinfo.txt

 <index>        Index file.
 hostinfo.txt   A list of server details, say 
                   <hostname> <TCP port n:o> <hash>
                to connect to (read from stdin).

Options:
 --check        Check integrity of index and quit.
 --port <p>     Connect to port <p>.
 --verbose      Print progress information.
Debug options:
 --debug        Print more progress information.
```
Here follows an example on how to initialize the client side processes.
First, you need to make sure that all the server-side processes are 
up and running - you might want to set up SLURM job dependencies
to ensure that the server-side processes are started first. Once you 
initialize the client processes, they will
connect to the server processes, load the index and start the computation.
Here, each client is associated with one index (i.e. one dataset).
```
export DSM_FRAMEWORK_PATH=/path/to/dsm-framework/
$DSM_FRAMEWORK_PATH/wrapper-SLURM/example-client.sh sample-names.txt tmp_dsmframework_config
```
The resulting output is found under the files server-output*.txt.gz.
See the above example scripts and papers [1] and [2] for details. 
More information and related Matlab packages are available at
https://github.com/HIITMetagenomics

Remark: If your cluster does not have SLURM, the script 
`wrapper-simple/distribute.sh` gives an example how to distribute the client
processes. You will need to modify it to suite your own
cluster environment.
