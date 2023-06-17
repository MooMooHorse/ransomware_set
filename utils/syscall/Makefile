all: start end blk2file clear
	rm -f *.o && rm -rf bin && mkdir bin && mv start end blk2file clear bin

end: end.o sys.o
	gcc sys.o end.o -o end

start: start.o sys.o
	gcc sys.o start.o -o start

clear: clear.o sys.o
	gcc sys.o clear.o -o clear

blk2file: blk2file.o sys.o
	gcc sys.o blk2file.o -o blk2file
	
blk2file.o:
	gcc -c blk2file.c -o blk2file.o

clear.o:
	gcc -c clear.c -o clear.o

end.o:
	gcc -c end.c -o end.o

start.o:
	gcc -c start.c -o start.o

sys.o:
	gcc -c sys.c -o sys.o

PHONY: clean

clean:
	rm -f *.o && rm -rf bin