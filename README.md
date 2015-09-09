# Matrix multiply reference code

Matrix muliplication is a good first example of code optimization
for three reasons:

1.  It is ubiquitous
2.  It looks trivial
3.  The naive approach is orders of magnitude slower than the tuned version

Part of the point of this exercise is that it often makes sense to
build on existing high-performance libraries when they are available.

The rest of this README describes the reference code setup.

## Basic layout

The main files are:

* `README.md`: This file
* `Makefile`: The build rules
* `Makefile.in.*`: Platform-specific flag and library specs used in the Makefile
* `dgemm_*`: Different modules implementing the `square_dgemm` routine
* `fdgemm.f`: Reference Fortran `dgemm` from Netlib (c.f. `dgemm_f2c`)
* `matmul.c`: Driver script for testing and timing `square_dgemm` versions
* `plotter.py`: Python script for drawing performance plots
* `runner.sh`: Helper script for running the timer on the instructional nodes

You will probably mostly be looking at `Makefile.in` and `dgemm_*.c`. Note that "dgemm" stands for "**D**ouble Precision **GE**neral **M**atrix **M**ultiply".   
## Makefile system

I have built the reference code with three compilers:

1.  GCC 4.9.2 on the C4 Linux cluster (`gcc`)
2.  The Intel compilers on the C4 Linux cluster (`icc`)
3.  Homebrew GCC 5.2.0 on my OS X 10.9 laptop (`mac`)

You can switch between these options by adding `PLATFORM=icc` (for
example) to your `make` command, or by changing the `PLATFORM=gcc`
line at the top of the Makefile.  For example, to build all the
drivers on my laptop, I run

    make PLATFORM=mac

from the terminal.  If someone feels like adding an autoconf or CMake
script for these configurations, I would welcome it!

I recommend using the Intel compiler on the cluster.  The optimizer
generally does much better than the GCC optimizer on this type of code.

For those who aren't familiar with the Makefile system and would like an overview, please consult these two links: [tutorial] (http://mrbook.org/blog/tutorials/make/) [more in-depth tutorial](http://www.cs.swarthmore.edu/~newhall/unixhelp/howto_makefiles.html) 
### Notes on GCC

You may notice the `-std=gnu99` flag in `Makefile.in.gcc`.  This tells
the compiler that we want to use the C99 language variant with
extensions, though the only extension we are using is actually the
call to the `drand48` random number generation routine.  If you don't
tell the compiler that you want at least C99, it will assume you are
using C89, and complain about things like variable declarations inside
of the initializer clause in a `for` loop.

Why do we assume that a 25-year-old standard takes precedence over the
"new" 15-year-old standard?  Beats me, but this is what it is.

### Notes on Clang and OS X

The driver code (`matmul.c`) uses the OpenMP `omp_get_wtime` routine
for timing; unfortunately, the Clang compiler does not yet include
OpenMP by default.  This means that if you want to use OpenMP -- even
the timing routines -- you cannot use the default compiler under OS X
Mavericks.  I have used a build of GCC 5.2.0 using HomeBrew.  If you
are trying things out on an OS X box, I recommend you do the same.

If you are running on totient and want to try out the Clang compiler for
building your matrix multiply kernel, you certainly may.  The driver
uses OpenMP for timing; the kernel can be compiled with different flags.

In October 2013, Intel contributed their OpenMP implementation to the
Clang compiler, so I expect this caveat will no longer hold the next
time this class is offered!

### Notes on the Intel compilers

You must load the Intel module (`module load psxe`) before building
with the Intel compilers.

There are two things in the `Makefile.in.icc` file that are worth
noting if you want to use the Intel compilers and mix C and Fortran
for this assignment.  First, we require `libirng` (the `-lirng` flag
in the `LIBS` variable) in order to use `drand48`.  This library is
included by default when we link using `icc`, but not when we link
using `ifort`.  Second, we require the flag `-nofor_main` to tell the
Fortran compiler that we are using C rather than Fortran to define the
main routine.

It is also worth noting that the Intel Fortran compiler with
optimization does a fantastic job.  But if you choose to build from
the Fortran routine as your starting point, you still have to improve
the performance!

### Notes on system BLAS

On the totient cluster, the Makefile is configured to link against
OpenBLAS, a high-performance open-source BLAS library based on the Goto BLAS.
This build also lets you link against the MKL BLAS (with the Intel compilers).

On OS X, the Makefile is configured to link against the Accelerate
framework with the `veclib` tag.

### Notes on mixed C-Fortran programming

The reference implementation includes three files for calling a
Fortran `dgemm` from the C driver:

* `dgemm_f2c.f`: An interface file for converting from C to Fortran conventions
* `dgemm_f2c_desc.c`: A stub file defining the global `dgemm_desc` variable
  used by the driver
* `fdgemm.f`: The Fortran `dgemm` routine taken from the Netlib reference BLAS

The "old-school" way of mixing Fortran and C involve figuring out how
types map between the two languages and how the compiler does name
mangling.  It's feasible, but a pain to maintain.  If you want, you
can find many examples of this approach to mixed language programming
online, including in the sources for old versions of this assignment.
But it has now been over a decade since the Fortran 2003 standard,
which includes explicit support for C-Fortran interoperability.  So
we're going this route, using the `iso_c_binding` module in
`dgemm_f2c.f` to wrap a Fortran implementation of `dgemm` from the
reference BLAS.

Apart from knowing how to create a "glue" layer with something like
`iso_c_binding`, one of the main things to understand if trying to mix
Fortran and C++ is that Fortran has its own set of required support
libraries and its own way of handling the `main` routine.  If you want
to link C with Fortran, the easiest way to do so is usually to compile
the individual modules with C or Fortran compilers as appropriate, then
use the Fortran compiler for linking everything together.

## Running the code

The code writes a sequence of timings to a CSV (comma-separated value)
text file that can be loaded into a spreadsheet or array for further
processing.  By default, the name of the CSV file is based on the executable
name; for example, running

    ./matmul-blocked

with no arguments produces the output file `timing-blocked.csv`.  You can
also provide the file name as an argument, i.e.

    ./matmul-blocked timing-blocked.csv

These commands run benchmarks on the *local machine*. If you're logged into Totient, this will usually be the *head node*. 
To run benchmarks on a *compute node*, which will give completely different results, you want to use make commands (listed below). 
Make sure to keep things consistent i.e. don't compare a head node benchmark with a computer node benchmark. 
While testing on the head node is perfectly fine, your submitted benchmarks should be from a compute node, as I want to keep benchmarks consistent among the class.  

The Makefile has targets for running the timer on the compute nodes
on totient using the `job-*.pbs` scripts.  For example,

    make timing-blocked.csv

on totient will submit a PBS job to produce the timing-blocked.csv
file.  To run all the timers on the compute nodes, you can use

    make run

To run all the timers on your local machine, you will probably want
to use

    make run-local

Note that the `.pbs` scripts do a little more than just running the
job; they also set environment variables so that OpenBLAS, VecLib,
and MKL don't get an unfair advantage by exploiting multiple cores.

You can also manually submit the `.pbs` scripts to the compute nodes by using 

    qsub job-*.pbs

i.e. if you want to benchmark only your own code, you would type `qsub job-mine.pbs`. Note that this is what happens "under the hood" when you run the timers on the compute nodes with make commands.  

To clear up some of your workspace, use 

    make clean

which will remove executables and compute node logs (but not your code or benchmarks). To remove everything except for code, use

    make realclean


## Plotting results

You can produce timing plots by running

    make plot

The plotter assumes that all the relevant CSV files are already
in place.  Note, though, that you can (for example) put the CSV
files from totient onto your laptop and run `make plot`.

You can also directly use the `plotter.py` script.  The `plotter.py`
script loads a batch of timings and turns them into a plot which is
saved to `timing.pdf`.  For example, running

    ./plotter.py basic blocked blas

or

    python plotter.py basic blocked blas

will compare the contents of `timing-basic.csv`, `timing-blocked.csv`,
and `timing-blas.csv`.  Note that you need to be using the Anaconda
module if you are going to explicitly run the Python version, since the
script uses Pandas (which the system Python lacks).


## Optimization Tips and Tricks

Please refer to these [notes](http://www.cs.cornell.edu/~bindel/class/cs5220-f11/notes/serial-tuning.pdf) to get started. The notes discuss blocking, buffering, SSE instructions, and auto-tuning, among other optimizations.
The [Roofline Paper] (http://www.eecs.berkeley.edu/Pubs/TechRpts/2008/EECS-2008-134.pdf) discussed in class on 9/08/2015 is also worth looking at, although you might have to do a bit of extra reading. 
The previous Project instructions for this assignment can be found [here] (https://bitbucket.org/dbindel/cs5220-s14/wiki/HW1). 
Assume for the time being that the final submission instructions for this assignment have not changed. 
