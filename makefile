

CXX_FLAGS     = -Wall -Wextra
CXX_CMD       = gcc $(CXX_FLAGS)

SRCS      = $(notdir $(filter-out src/pipeduino.c, $(wildcard src/*.c)) )
OBJECTS   = $(patsubst %.c, src/%.o, $(SRCS) )

pipeduino: src/pipeduino.o $(OBJECTS)
	$(CXX_CMD) -o $@ $< $(OBJECTS) -lpthread

src/pipeduino.o: src/pipeduino.c $(OBJECTS) src/configure.h
	$(CXX_CMD) -o $@ -c $<

src/%.o: src/%.c src/%.h src/configure.h
	$(CXX_CMD) -o $@ -c $<

.PHONY: clean
clean:
	rm -f pipeduino src/*.o 
