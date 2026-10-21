CC := aarch64-linux-gnu-gcc
CFLAGS := -Wall -fPIC

all: libled.so libbutton.so

libled.so: libled/libled.c
	$(CC) $(CFLAGS) -shared -o $@ $<

libbutton.so: libbutton/libbutton.c
	$(CC) $(CFLAGS) -shared -o $@ $<

clean:
	rm -f *.so