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
	$(LD) -o $@ $^ $(LDFLAGS) $(LIBS)

matmul-f2c: $(OBJS) dgemm_f2c.o dgemm_f2c_desc.o fdgemm.o
	$(LD) -o $@ $^ $(LDFLAGS) $(LIBS) 

matmul-blas: $(OBJS) dgemm_blas.o
	$(LD) -o $@ $^ $(LDFLAGS) $(LIBS) $(LIBBLAS)

matmul-mkl: $(OBJS) dgemm_mkl.o
	$(LD) -o $@ $^ $(LDFLAGS) $(LIBS) $(LIBMKL)

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

