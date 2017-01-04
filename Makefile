CC=arm-linux-gnueabihf-gcc
CFLAGS=-I ./alsa_lib/include
CFLAGS+=-I ./libsndfile/include
LDFLAGS+=-L ./alsa_lib/lib -lasound
LDFLAGS+=-L ./libsndfile/lib -lsndfile
src=$(wildcard *.c)

objs=$(patsubst %.c, %.o, $(src))

.PHONY:alsa_test clean

alsa_test:$(objs)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)
	
$(objs):%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@
	
clean:
	rm alsa_test *.o
