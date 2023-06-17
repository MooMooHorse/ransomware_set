all: start end
	rm -f *.o && rm -rf bin && mkdir bin && mv start end bin

end: end.o sys.o
	gcc sys.o end.o -o end

start: start.o sys.o
	gcc sys.o start.o -o start
	
end.o:
	gcc -c end.c -o end.o
start.o:
	gcc -c start.c -o start.o
sys.o:
	gcc -c sys.c -o sys.o

PHONY: clean

clean:
	rm -f *.o && rm -rf bin