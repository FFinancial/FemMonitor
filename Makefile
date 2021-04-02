CFLAGS = -std=c99 `curl-config --cflags` `xml2-config --cflags`
LDFLAGS = `curl-config --libs` `xml2-config --libs` -lncurses -lpthread
OUT = fem

all:
	gcc -o $(OUT) $(CFLAGS) FemMonitor/*.c $(LDFLAGS)
clean:
	rm -f $(OUT)
