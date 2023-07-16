all: start end blk2file clear
	rm -f *.o && rm -rf bin && mkdir bin && mv start end blk2file clear bin

end: end.o sys.o
	g++ sys.o end.o -o end

start: start.o sys.o
	g++ sys.o start.o -o start

clear: clear.o sys.o
	g++ sys.o clear.o -o clear

blk2file: blk2file.o sys.o
	g++ sys.o blk2file.o -o blk2file
	
blk2file.o:
	g++ -c blk2file.cpp -o blk2file.o

clear.o:
	g++ -c clear.c -o clear.o

end.o:
	g++ -c end.c -o end.o

start.o:
	g++ -c start.c -o start.o

sys.o:
	g++ -c sys.c -o sys.o

PHONY: clean

clean:
	rm -f *.o && rm -rf bin