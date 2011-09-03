#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <mpi.h>
#include <lab1.h>

unsigned char seats[MAX_SEATS];
unsigned int  free_seats = MAX_SEATS;

// nice print 
void print_seats(void)
{
  unsigned int i;
  printf (" ------\n   ");
  for(i=1; i<MAX_SEATS / NUM_ROWS + 1; i++)
    printf ("%u ",i);
  for(i=0; i<MAX_SEATS; i++) {
    if (i % (MAX_SEATS / NUM_ROWS) == 0)
      printf ("\n%2u ",(i / NUM_ROWS) + 1);
    printf ("%c ",(seats[i] == 0) ? 'X' : seats[i] + 48 );
  }
  printf ("\n ------\n");
  fflush(NULL);
}

// rand stuff
inline int get_rand_num(int min, int max) {
  return min + (rand() % (max - min + 1));
}

// checks if seat array is free or not
inline int check_free(int seat_id, int size)
{
  if (seat_id < 0 || seat_id + size > MAX_SEATS)
    return 0;

  unsigned int i=0;
  while( seats[seat_id++] == SEAT_FREE && i < size)
    i++;
  return i;
}

// returns percentage of free seats 
inline double free_percentage(void) {
  return (double)free_seats/(double)MAX_SEATS  * 100;
}


/* routine to form response based on status of 
 * availability of requested seat array */
unsigned int arbiter_find_free_seats(const struct request * rq, struct response * rs)
{
  int ret_size = check_free(rq->start_seat_id,rq->seat_num);
  if (ret_size == rq->seat_num) {
    free_seats -= rq->seat_num;
    memset (&seats[rq->start_seat_id],rq->id,ret_size);
    rs->status = STATUS_AVAILABLE;
    rs->start_seat_id = rq->start_seat_id;
    rs->seat_num = rq->seat_num;
  }
  else {
    rs->status = STATUS_NON_FREE;
    rs->start_seat_id = rq->start_seat_id;
    rs->seat_num = rq->seat_num;
  }

  return rs->status;
}

/* main routine for seat arbiter */
void seat_arbiter(int numproc)
{
#ifdef DEBUG
  printf ("[%d]:started arbiter\n",SEAT_ARBITER_ID); fflush(NULL);
#endif

  MPI_Status status;
  struct request  rq;
  struct response rs;

  // init seats (set free)
  memset(seats,SEAT_FREE,MAX_SEATS);

  // reserve most of seats
  do
  {
    MPI_Recv(&rq, sizeof(rq), MPI_BYTE, MPI_ANY_SOURCE, TAG_LEET, MPI_COMM_WORLD, &status);
#ifdef DEBUG
    printf ("[%d]:received request [%hhu %hu %hu]\n", SEAT_ARBITER_ID,rq.id, rq.start_seat_id, rq.seat_num); fflush(NULL);
#endif  
  
    // process request
    arbiter_find_free_seats(&rq,&rs);

#ifdef DEBUG
    printf ("[%d]:sending response [%hhu %hu %hu]\n", SEAT_ARBITER_ID,rs.status, rs.start_seat_id, rs.seat_num); fflush(NULL);
#endif
    MPI_Send(&rs, sizeof(rq), MPI_BYTE, rq.id, TAG_LEET, MPI_COMM_WORLD);
  
  } while (free_percentage() >= EXIT_PERCENTAGE);

  printf ("%lf %% used\n",free_percentage());

  // send STATUS_CLOSE signal to cash boxes via response
  unsigned int i;
  rs.status = STATUS_CLOSE;
  rs.start_seat_id = 0;
  rs.seat_num = 0;
  for ( i=1; i<numproc; i++) {
    MPI_Send(&rs, sizeof(rs),MPI_BYTE, i, TAG_LEET, MPI_COMM_WORLD);
  }

#ifdef DEBUG
  printf ("[%d]:exiting arbiter\n",SEAT_ARBITER_ID); fflush(NULL);
#endif
}

/* iterates through left and right shifted arrays
 * of seats, if requested isn't availabe */
unsigned int cash_box_find_free_seats(struct request *rq, struct response *rs)
{
  MPI_Status status;
  int offset = 0;
  while (( rq->start_seat_id >= 0) && (offset <= rq->seat_num))
  {
    rq->start_seat_id += offset; // set positive offset
    MPI_Send(rq,sizeof(*rq),MPI_BYTE, SEAT_ARBITER_ID, TAG_LEET, MPI_COMM_WORLD);
    MPI_Recv(rs,sizeof(*rs),MPI_BYTE, SEAT_ARBITER_ID, TAG_LEET, MPI_COMM_WORLD, &status);
    if (rs->status == STATUS_AVAILABLE || rs->status == STATUS_CLOSE)
      break;
    else
      rs->start_seat_id -= offset; // set 0 offset
  
    if (offset == 0) {
      offset++;
      continue;
    }

    rq->start_seat_id -= offset; // set negative offset
    MPI_Send(rq,sizeof(*rq),MPI_BYTE, SEAT_ARBITER_ID, TAG_LEET, MPI_COMM_WORLD);
    MPI_Recv(rs,sizeof(*rs),MPI_BYTE, SEAT_ARBITER_ID, TAG_LEET, MPI_COMM_WORLD, &status);
    if (rs->status == STATUS_AVAILABLE || rs->status == STATUS_CLOSE)
      break;
    else
      rs->start_seat_id += offset; // set 0 offset

    offset++;
  }

  return rs->status;
}

/* main routine for cash box */
void cash_box(int id)
{
#ifdef DEBUG
  printf ("[%d]:started cash box\n", id); fflush(NULL);
#endif

  struct request  rq;
  struct response rs;

  do
  {
    rq.id = id;
    rq.start_seat_id = get_rand_num(0,MAX_SEATS - 1);
    rq.seat_num      = get_rand_num(1,4);

    sleep(get_rand_num(0,2)); 

#ifdef DEBUG
    printf ("[%d]:compiled request [%hhu %hu %hu]\n",id,rq.id,rq.start_seat_id,rq.seat_num); fflush(NULL);
#endif
    
    cash_box_find_free_seats(&rq,&rs);

#ifdef DEBUG
    printf ("[%d]:final response [%hhu %hu %hu]\n",id,rs.status,rs.start_seat_id,rs.seat_num); fflush(NULL);
#endif

    if (rs.status == STATUS_AVAILABLE) {
      printf ("[%d]:sold [%hu places, starting at %hu(%hu %hu)]\n", id, rs.seat_num, rs.start_seat_id, rs.start_seat_id / NUM_ROWS + 1, rs.start_seat_id % NUM_ROWS + 1); 
      fflush(NULL);
    }
    
  } while (rs.status != STATUS_CLOSE);

#ifdef DEBUG
  printf ("[%d]:exiting cash box\n",id); fflush(NULL);
#endif

  return;
}

int main (int argc, char *argv[])
{
  int numproc, myid, namelen;
  char processor_name[MPI_MAX_PROCESSOR_NAME];

  // init MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&numproc);
  MPI_Comm_rank(MPI_COMM_WORLD,&myid);
  MPI_Get_processor_name(processor_name,&namelen);

  // init random number generator
  srand(time(NULL) + myid);

  // check if there is enough processing units
  if (numproc < MIN_PROCS) {
    fprintf (stderr,"only %u of us, commiting suicide ...\n",numproc);
    exit(EXIT_FAILURE);
  }

  // output
  printf("[%d]: starting MPI process @%s\n", myid, processor_name); fflush(stdout);

  MPI_Barrier(MPI_COMM_WORLD);
  if (myid == SEAT_ARBITER_ID)
    seat_arbiter(numproc);  // role 1 - seat_arbiter
  else
    cash_box(myid);         // role 2 - cash_box
  
  // after everyone exited, print output
  MPI_Barrier(MPI_COMM_WORLD);
  if (myid == SEAT_ARBITER_ID)
    print_seats();

  MPI_Finalize();

  return 0;
}
