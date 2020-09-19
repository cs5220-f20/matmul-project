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

I have built the reference code with two compilers:

1.  GCC 8.3.0 on Debian Buster on GCP (gcc)
2.  CLang 7.0.1-8 on Debian Buster on GCP (clang)

You can switch between these options by adding `PLATFORM=clang` (for
example) to your `make` command, or by changing the `PLATFORM=gcc`
line at the top of the Makefile.  For example, to build all the
drivers on my laptop, I run

    make PLATFORM=mac

from the terminal.  If someone feels like adding an autoconf or CMake
script for these configurations, I would welcome it!

If you want to play around with the Intel compiler, it typically does
much better than GCC on this type of code.  However, we can't install
the Intel compiler on non-university machines under the terms of the
educational license, so we will stick with GCC and CLang on GCP.

For those who aren't familiar with the Makefile system and would like an overview, please consult these two links: [tutorial](http://mrbook.org/blog/tutorials/make/) [more in-depth tutorial](http://www.cs.swarthmore.edu/~newhall/unixhelp/howto_makefiles.html) 

### Setting up your virtual machine

We are going to install several packages to support this code.  You
should use the `e2-micro` instance type that we considered in the GCP
walkthrough on 9/17 (with Debian Buster as the recommended operating
system).  With this setup, you will *need* the following packages:

- `build-essential`: GCC, Make, and various other basic tools for
  working with compiled codes.
- `pkg-config`: Used to semi-automatically figure out where to look
  for various packages installed in the system (like the BLAS)
- `gfortran`: You are going to want a Fortran compiler to be able to
  test out the Fortran version of the DGEMM.  You may write your
  optimized code in Fortran using this type of interface, if you want,
  but it is not the default.
- `clang`: The CLang compiler
- `libomp-dev`: Support library for OpenMP with CLang
- `git`: So you can fetch this repository
- `libopenblas-base` and `libopenblas-dev`: The OpenBLAS library is a
  fast BLAS implementation for modern Intel processors.
- The Python stack: `python-numpy`, `python-scipy`, `python-pandas`, and
  `python-matplotlib`

I *recommend* also installing the following packages (which you may
have already done):

- `llvm`: To get access to the `llvm-mca` tool
- `google-perftools`: The Google performance tools for profiling

Once you have installed the compiler and the OpenBlas libraries,
building with GCC is as simple as typing `make` (or `make
PLATFORM=gcc`).  But it is worth poking through the Makefile.
You may notice the `-std=gnu99` flag in `Makefile.in.gcc`.  This tells
the compiler that we want to use the C99 language variant with
extensions, though the only extension we are using is actually the
call to the `drand48` random number generation routine.  If you don't
tell the compiler that you want at least C99, it will assume you are
using C89, and complain about things like variable declarations inside
of the initializer clause in a `for` loop.

Why do we assume that a 25-year-old standard takes precedence over the
"new" 15-year-old standard?  Beats me, but this is what it is.

### Building on MacOS

Some of you use a Mac for development.  I have included `mac-gcc`
and `mac-clang` for you to use if you want, but you have to have
things installed right first.  Note that you are in no way obliged to
use these things; I provide it solely for your own edification.

By default, the `gcc` program in MacOS is not GCC at all; rather, it
is an alias for Clang.  The driver code (`matmul.c`) uses the OpenMP
`omp_get_wtime` routine for timing; but, the Apple version of Clang
looks like it does not support OpenMP.  At least, I thought there was
no OpenMP support until recently!  Then I read [this
article](https://iscinumpy.gitlab.io/post/omp-on-high-sierra/) and
learned better.  So assuming you have Homebrew installed, you can
build with `make PLATFORM=mac-clang` provided that you first run the
line

    brew install libomp

If you want to use the "real" GCC, make sure you do

    brew install gcc gfortran
    
and then you can build with `make PLATFORM=mac-gcc`.

### Notes on system BLAS

For GCP, the Makefile is configured to link against
OpenBLAS, a high-performance open-source BLAS library based on the Goto BLAS (as an aside, 
there is an excellent NYTimes [article](http://www.nytimes.com/2005/11/28/technology/writing-the-fastest-code-by-hand-for-fun-a-human-computer-keeps.html?mcubz=1) about the history behind Goto BLAS)

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

To run all the timers, you will probably want to use

    make run

To clear up some of your workspace, use 

    make clean

which will remove executables and compute node logs (but not your code or benchmarks). To remove everything except for code, use

    make realclean

## Plotting results

You can produce timing plots by running

    make plot

The plotter assumes that all the relevant CSV files are already
in place.

You can also directly use the `plotter.py` script.  The `plotter.py`
script loads a batch of timings and turns them into a plot which is
saved to `timing.pdf`.  For example, running

    ./plotter.py basic blocked blas

or

    python plotter.py basic blocked blas

will compare the contents of `timing-basic.csv`, `timing-blocked.csv`,
and `timing-blas.csv`.


## Optimization Tips and Tricks

Please refer to these [notes](http://www.cs.cornell.edu/~bindel/class/cs5220-f11/notes/serial-tuning.pdf) to get started. The notes discuss blocking, buffering, SSE instructions, and auto-tuning, among other optimizations.
The [Roofline Paper](http://www.eecs.berkeley.edu/Pubs/TechRpts/2008/EECS-2008-134.pdf) is also worth looking at.

