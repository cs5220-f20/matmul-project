# ---
# Platform-dependent configuration
#
# If you have multiple platform-dependent configuration options that you want
# to play with, you can put them in an appropriately-named Makefile.in.
# For example, the default setup has a Makefile.in.icc and Makefile.in.gcc.

PLATFORM=gcc

include Makefile.in.$(PLATFORM)
DRIVERS=$(addprefix matmul-,$(BUILDS))
TIMINGS=$(addsuffix .csv,$(addprefix timing-,$(BUILDS)))

.PHONY:	all
all:	$(DRIVERS)

# ---
# Rules to build the drivers

matmul-%: $(OBJS) dgemm_%.o
	$(LD) -o $@ $^ $(LDFLAGS) $(LIBS)

matmul-f2c: $(OBJS) dgemm_f2c.o dgemm_f2c_desc.o fdgemm.o
	$(LD) -o $@ $^ $(LDFLAGS) $(LIBS) 

matmul-blas: $(OBJS) dgemm_blas.o
	$(LD) -o $@ $^ $(LDFLAGS) $(LIBS) $(LIBBLAS)

matmul-mkl: $(OBJS) dgemm_mkl.o
	$(LD) -o $@ $^ $(LDFLAGS) $(LIBS) $(LIBMKL)

matmul-veclib: $(OBJS) dgemm_veclib.o
	$(LD) -o $@ $^ $(LDFLAGS) $(LIBS) -framework Accelerate

# --
# Rules to build object files

matmul.o: matmul.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

%.o: %.c
	$(CC) -c $(CFLAGS) $(OPTFLAGS) $(CPPFLAGS) $<

%.o: %.f
	$(FC) -c $(FFLAGS) $(OPTFLAGS) $<

dgemm_blas.o: dgemm_blas.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INCBLAS) $< 

dgemm_mkl.o: dgemm_blas.c
	$(CC) -o $@ -c $(CFLAGS) $(CPPFLAGS) $(INCMKL) $< 

dgemm_veclib.o: dgemm_blas.c
	clang -o $@ -c $(CFLAGS) $(CPPFLAGS) -DOSX_ACCELERATE $< 

# ---
# Rules for building timing CSV outputs

.PHONY: run
run:    $(TIMINGS)

timing-%.csv: matmul-%
	OPENMP_NUM_THREADS=1 ./matmul-$*

# ---
#  Rules for plotting

.PHONY: plot
plot:
	$(PYTHON) plotter.py $(BUILDS)

# ---

.PHONY:	clean realclean 
clean:
	rm -f matmul-* *.o

realclean: clean
	rm -f *~ timing-*.csv timing.pdf
