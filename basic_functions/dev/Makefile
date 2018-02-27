TARGETS=proxy client server

all: $(TARGETS)
clean:
	rm -f $(TARGETS) *~

client: client.cpp
	g++ -Wall -g -o $@ $<

server: server.cpp
	g++ -Wall -g -o $@ $<

proxy: proxy.cpp
	g++ -Wall -g -o $@ $<
