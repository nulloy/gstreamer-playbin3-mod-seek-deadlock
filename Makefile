test: test.c
	cc $^ -o $@ `pkg-config --cflags --libs gstreamer-1.0`
