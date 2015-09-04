# ---
# Platform-dependent configuration
#
# If you have multiple platform-dependent configuration options that you want
# to play with, you can put them in an appropriately-named Makefile.in.
# For example, the default setup has a Makefile.in.icc and Makefile.in.gcc.

PLATFORM=icc
include Makefile.in.$(PLATFORM)

.PHONY:	all
all:	matmul-mine matmul-basic matmul-blocked matmul-blas matmul-f2c

# ---
# Rules to build the drivers

matmul-%: $(OBJS) dgemm_%.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

matmul-f2c: $(OBJS) dgemm_f2c.o dgemm_f2c_desc.o fdgemm.o
	$(FC) -o $@ $^ $(LDFLAGS) $(LIBS) 

matmul-blas: $(OBJS) dgemm_blas.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS) $(LIBBLAS)

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

# ---
# Rules for building timing CSV outputs

.PHONY: run run-c4
run-c4: info-mine.out info-basic.out info-blocked.out \
	info-f2c.out info-blas.out
run:    timing-mine.csv timing-basic.csv timing-blocked.csv \
	timing-f2c.csv timing-blas.csv

info-%.out: matmul-%
	csub ./runner.sh ./$< $*

timing-%.csv: matmul-%
	./$<

# ---

.PHONY:	clean realclean 
clean:
	rm -f matmul-* *.o
	rm -f csub-*

realclean:	clean
	rm -f *~ timing-*.csv info-*.out timing.pdf

