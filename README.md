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

You will probably mostly be looking at `Makefile.in` and `dgemm_*.c`.

## Makefile system

I have built the reference code with three compilers:

1.  GCC 4.8.2 on the C4 Linux cluster (`gcc`)
2.  The Intel compilers on the C4 Linux cluster (`icc`)
3.  GCC 4.8.2 on my OS X 10.9 laptop (`mac`)

You can switch between these options by adding `PLATFORM=icc` (for
example) to your `make` command, or by changing the `PLATFORM=gcc`
line at the top of the Makefile.  For example, to build all the
drivers on my laptop, I run

    make PLATFORM=mac

from the terminal.  If someone feels like adding an autoconf or CMake
script for these configurations, I would welcome it!

I recommend using the Intel compiler on the cluster.  The optimizer
generally does much better than the GCC optimizer on this type of code.

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
Mavericks.  I have used a build of GCC 4.8.2 using MacPorts.  If you
are trying things out on an OS X box, I recommend you do the same.

If you are running on C4 and want to try out the Clang compiler for
building your matrix multiply kernel, you certainly may.  The driver
uses OpenMP for timing; the kernel can be compiled with different flags.

In October 2013, Intel contributed their OpenMP implementation to the
Clang compiler, so I expect this caveat will no longer hold the next
time this class is offered!

### Notes on the Intel compilers

You must load the Intel module (`module load icsxe`) before building
with the Intel compilers.  Once you have loaded this module, you cannot
build the driver with GCC until after you unload it; there are conflicts
between Intel's version of standard header files and the GCC version.

There are two things in the `Makefile.in.icc` file that are worth
noting if you want to use the Intel compilers and mix C and Fortran
for this assignment.  First, we require `libirng` (the `-lirng` flag
in the `LIBS` variable) in order to use `drand48`.  This library is
included by default when we link using `icc`, but not when we link
using `ifort`.  Second, we require the flag `-nofor-main` to tell the
Fortran compiler that we are using C rather than Fortran to define the
main routine.

It is also worth noting that the Intel Fortran compiler with
optimization does a fantastic job.  But if you choose to build from
the Fortran routine as your starting point, you still have to improve
the performance!

### Notes on system BLAS

On the C4 cluster, the Makefile is configured to link against OpenBLAS,
a high-performance open-source BLAS library based on the Goto BLAS.
One could also link against the MKL BLAS (with the Intel compilers),
but I have not configured this.

On OS X, the Makefile is configured to link against the Accelerate framework.
If you are using a different version of OS X, you may have to fiddle a little
with the compiler macros in order to get this to work.  In particular, the
`cblas.h` file at one point was not a part of the framework.

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

The Makefile has targets for running the timer on the instructional nodes
on C4 using the `runner.sh` script.  For example,

    make info-blocked.out

on C4 will submit an HTCondor job to produce the timing-blocked.csv
file (and info-blocked.out).  To run all the timers, you can use

    make run-c4

Note that the `runner.sh` script does a little more than just running
the job; it also reports what host the job ran on, saves the standard
output to `info-XXX.out`, and sets the `OMP_NUM_THREADS` variable so
that runs using OpenBLAS don't get an unfair advantage by exploiting
multiple cores.

## Plotting results

The `plotter.py` script loads a batch of timings and turns them into
a plot which is saved to `timing.pdf`.  For example, running

    ./plotter.py basic blocked blas

or

    python plotter.py basic blocked blas

will compare the contents of `timing-basic.csv`, `timing-blocked.csv`,
and `timing-blas.csv`.  Note that you need to be using the Anaconda
module if you are going to explicitly run the Python version, since the
script uses Pandas (which the system Python lacks).
