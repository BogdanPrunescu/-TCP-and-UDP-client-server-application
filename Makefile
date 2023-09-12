
all: server subscriber

common.o: common.cpp

server: server.cpp common.o

subscriber: subscriber.cpp common.o

clean:
	rm -rf server subscriber common.o
