TARGET = sprdproto
OBJS   = sprdproto.o sprd_io.o sprd_usbdev.o

CFLAGS  = -Werror
LDFLAGS = -lusb-1.0

all: $(TARGET)

clean:
	rm -f $(OBJS)
	rm -f $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
