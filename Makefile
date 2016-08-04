CFLAGS = -g
CFLAGS += `pkg-config --cflags libavutil libavformat libavcodec`
LDFLAGS = `pkg-config --libs libavutil libavformat libavcodec`

all: m2mtest_h264 m2mtest_vp9

clean:
	rm -f *.o m2mtest*

.c.o:
	$(CC) $(CFLAGS) -c $<

m2mtest_h264: main_h264.o h264parser.o
	$(CC) -pthread -o $@ $^

m2mtest_vp9: main_vp9.o vp9parser.o
	$(CC) -pthread -o $@ $^ $(LDFLAGS)
