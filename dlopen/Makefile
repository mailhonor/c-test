all: do_all



do_all:
	gcc main.c -o main -Wall -ldl
	gcc lib1.c -o lib1.so -shared -fPIC -Wall
	gcc lib2.c -o lib2.so -shared -fPIC -Wall

clean:
	rm -f *~ main lib1.so lib2.so lib.so
