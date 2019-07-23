sources = $(wildcard src/*.c)
objects = $(sources:.c=.o)
flags = -Wall -g -lcoelum -lsqlite3 -lm -ldl -fPIC -I../coelum/GL/include -rdynamic


libathena.a: $(objects)
	ar rcs $@ $^

%.o: %.c include/%.h
	gcc -c $(flags) $< -o $@

install:
	make
	make libathena.a
	mkdir -p /usr/local/include/athena
	cp -r ./src/include/* /usr/local/include/athena/.
	cp libathena.a /usr/local/lib/.

clean:
	-rm *.out
	-rm *.o
	-rm *.a
	-rm src/*.o

lint:
	clang-tidy src/*.c src/include/*.h
