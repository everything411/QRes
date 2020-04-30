CC := tcc.exe
M32ARG := -m32
INCLUDEDIR := D:/bin/tcc/include/winapi

exe : qres.c qres32.c shared
	$(CC) $(M32ARG) -I$(INCLUDEDIR) -L. -lqres32 -o qres.exe qres.c 

shared : qres.c qres32.c qreslib.o
	$(CC) $(M32ARG) -I$(INCLUDEDIR) -luser32 -lgdi32 -ladvapi32 -shared -rdynamic -o qres32.dll qres32.c qreslib.o 

qreslib.o : qreslib.c
	$(CC) $(M32ARG) -c qreslib.c -o qreslib.o

clean :
	rm *.o *.exe *.dll *.def
