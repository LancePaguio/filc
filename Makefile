CXXFLAGS=-std=c++17 -g -O2 -Wall -Wextra -Isrc
PREFIX=$(HOME)/.local

SRCS=$(wildcard src/*.cpp)

# filc
filc: $(SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Installation
install: filc
	install -m 755 filc $(DESTDIR)$(PREFIX)/bin/filc

debug: CXXFLAGS = -std=c++17 -g -O0 -fno-omit-frame-pointer -fsanitize=address,undefined -Isrc
debug: $(SRCS)
	$(CXX) $(CXXFLAGS) $^ -o filc-debug

valgrind: CXXFLAGS = -std=c++17 -g -O1 -Isrc
valgrind: $(SRCS)
	$(CXX) $(CXXFLAGS) $^ -o filc-valgrind

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/filc

clean:
	rm -f filc filc-debug src/*.o src/*.d

.PHONY: install debug valgrind uninstall clean
