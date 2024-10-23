CFLAGS = -O0 -fopenmp
DTYPE = double
#EXTRA_FLAGS=-DSKIP_RAW_DATA
EXTRA_FLAGS ?=
.PHONY: all clean

all:
	$(CC) $(CFLAGS) -DDTYPE=$(DTYPE) $(EXTRA_FLAGS) -c -o kernels.o kernels.c
	$(CC) $(CFLAGS) -DDTYPE=$(DTYPE) $(EXTRA_FLAGS) main.c kernels.o -o ncar
	$(CC) $(CFLAGS) -DDTYPE=$(DTYPE) $(EXTRA_FLAGS) -DOUTER_LOOP_THREAD_SYNC  main.c kernels.o -o ncar-barriers

clean:
	rm ncar
	rm ncar-barriers
	rm kernels.o



