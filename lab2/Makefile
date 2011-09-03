CC = mpicc
#CFLAGS  = -DDEBUG -ggdb -Wall -Werror -I./
CFLAGS  = -O3 -Wall -I./ # -Werror 
LDFLAGS = -lm
SOURCE  = lab2.c 
HEADERS = lab2.h
OBJECTS = lab2.o
PROJECT = lab2
NUMPROC = 2
MPI_EXEC = mpiexec


run: clean $(PROJECT)
	$(MPI_EXEC) -n $(NUMPROC) ./$(PROJECT)

$(PROJECT): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(PROJECT)

$(OBJECTS): $(SOURCE) $(HEADERS)
	$(CC) -c $(CFLAGS) $(SOURCE)

clean:
	-rm -f $(PROJECT) $(OBJECTS) 

debug: clean
	$(CC) -DDEBUG -c $(CFLAGS) $(SOURCE)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(PROJECT)
	$(MPI_EXEC) -n $(NUMPROC) ./$(PROJECT)
