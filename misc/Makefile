TARGET=kvl_sample

CC?=clang
#CFLAGS=--std=gnu99
LDFLAGS=-lbxiutil -lbxibase

all: $(TARGET)

run: all
	./$(TARGET) -v

clean:
	rm -f $(TARGET)
