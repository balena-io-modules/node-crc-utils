CC = emcc
CFLAGS = -O3
NAME = crc32

JSFLAGS = \
	-s EXPORTED_FUNCTIONS="['_crc32_combine']" \
	-s EXPORTED_RUNTIME_METHODS="['ccall']"

all: $(NAME).js

$(NAME).js: src/$(NAME).c
	$(CC) $(CFLAGS) $(JSFLAGS) $< -o $@

clean:
	rm -f $(NAME).js $(NAME).wasm
