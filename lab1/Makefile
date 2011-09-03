CC = mpicc
CFLAGS  = -I.  #-Wall -I./ #-DDEBUG
LDFLAGS = 
SOURCE  = lab1.c 
HEADERS = lab1.h
OBJECTS = lab1.o
PROJECT = lab1
NUMPROC = 4
MPI_EXEC = mpiexec

run: clean $(PROJECT)
	$(MPI_EXEC) -n $(NUMPROC) ./$(PROJECT)

$(PROJECT): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(PROJECT)

$(OBJECTS): $(SOURCE) $(HEADERS)
	$(CC) -c $(CFLAGS) $(SOURCE)

clean:
	-rm -f $(PROJECT) $(OBJECTS) *out *ret

debug: clean
	$(CC) -DDEBUG -c $(CFLAGS) $(SOURCE)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(PROJECT)
	$(MPI_EXEC) -n $(NUMPROC) ./$(PROJECT)
