CXXFLAGS=-std=c++17 -g -Wall -Wextra -Isrc
PREFIX=/usr/local

SRCS=$(wildcard src/*.cpp)
OBJS=$(SRCS:.cpp=.o)
LIB_OBJS=$(filter-out src/main.o,$(OBJS))

# filc
filc: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

-include $(OBJS:.o=.d)

# Installation
install: filc
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 filc $(DESTDIR)$(PREFIX)/bin/filc

clean:
	rm -f filc src/*.o src/*.d

.PHONY: install clean
