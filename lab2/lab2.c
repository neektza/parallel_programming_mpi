#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include <lab2.h>
#include <limits.h>

unsigned int num_rows;
unsigned int num_columns;
unsigned int max_depth = INIT_DEPTH + MAX_DEPTH;
unsigned char *field;

double *res = NULL;

int numproc,myid,namelen;
char processor_name[MPI_MAX_PROCESSOR_NAME];

int t_dest = 0;
int t_id  = -1;
int t_id2 = -1;

double worker_evaluate(int c_depth, unsigned char move);
void root_dispatch(int c_depth, unsigned char move);
void root_collect();
double root_evaluate(int c_depth, unsigned char move);


/*
 * Something really bad happened
 */
void 
panic(const char * msg)
{
  fprintf (stderr,"%s\n",msg);
  exit(EXIT_FAILURE);
}

/*
 * Returns random number in range
 * min - max, inclusive
 */
inline int 
get_rand_num(int min, int max) 
{
  return min + (rand() % (max - min + 1));
}

/* 
 * Checks if fist line is empty
 */
void 
check_overflow(void)
{
  unsigned int i;
  for (i=0; i<num_columns; i++)
  {
    if (field[i] != EMP)
    {
      expand_field();
      return;
    }
  }
  return;
}

/*
 * Expands game field putting
 * empty line above first one
 */
void 
expand_field(void)
{
  num_rows++;
  field = realloc(field,num_rows * num_columns);
  if (field == NULL)
  {
    perror("realloc");
    exit(EXIT_FAILURE);
  }
  memmove(field + num_columns,field,(num_rows-1)*num_columns);
  memset(field,'.',num_columns);
}


/*
 * Checks if move indexed by i,j
 * made a win for sentinel sen
 */
int 
eval_winner(
  int i, 
  int j, 
  unsigned char sen)
{

  int d1,d2;
  int d = 4;

  if (field[i*num_columns + j] != sen)
    return 0;

  ///////////////////////////
  /* analyze horizontal */
  d1 = d2 = 0;
  while ( j - (d1 + 1) >= 0  && field[i*num_columns + (j - (d1 + 1))] == sen )
    d1++;

  while ( j + (d2 + 1) < num_columns && field[i*num_columns + (j + (d2 + 1))] == sen )
    d2++;

  if ((d1 - (-d2)) >= d-1)
  {
   // printf ("horizontal win on i=%d, j=%d (d1=%d, d2=%d, diff=%d)\n",i,j,d1,d2,d1-(-d2));
    return 1;
  }

  /////////////////////////
  /* analyze vertical */
  d1 = d2 = 0;
  while ( i - (d1 + 1) >= 0  && field[(i - (d1 + 1)) *num_columns + j] == sen )
    d1++;

  while ( i + (d2 + 1) < num_rows  && field[(i + (d2 + 1)) *num_columns + j] == sen )
    d2++;

  if ((d1 - (-d2)) >= d-1)
  {
   // printf ("vertical win on i=%d, j=%d (d1=%d, d2=%d, diff=%d)\n",i,j,d1,d2,d1-(-d2));
    return 1;
  }

  /////////////////////////
  /* analyze descending */
  d1 = d2 = 0;
  while ( i - (d1 + 1) >= 0  && j - (d1 + 1) >= 0 && field[(i - (d1 + 1)) *num_columns + (j - (d1 + 1))] == sen )
    d1++;

  while ( i + (d2 + 1) < num_rows  &&  j + (d2 + 1) < num_columns && field[(i + (d2 + 1)) *num_columns + (j + (d2 + 1))] == sen )
    d2++;

  if ((d1 - (-d2)) >= d-1)
  {
   // printf ("descending win on i=%d, j=%d (d1=%d, d2=%d, diff=%d)\n",i,j,d1,d2,d1-(-d2));
    return 1;
  }

  /////////////////////////
  /* analyze ascending */
  d1 = d2 = 0;
  while ( i + (d1 + 1) < num_rows  && j - (d1 + 1) >= 0 && field[(i + (d1 + 1)) *num_columns + (j - (d1 + 1))] == sen )
    d1++;

  while ( i - (d2 + 1) >= 0 && j + (d2 + 1) < num_columns && field[(i - (d2 + 1)) *num_columns + (j + (d2 + 1))] == sen )
    d2++;

  if ((d1 - (-d2)) >= d-1)
  {
   // printf ("ascending win on i=%d, j=%d (d1=%d, d2=%d, diff=%d)\n",i,j,d1,d2,d1-(-d2));
    return 1;
  }


  return 0;
}


/*
 * Prints game field
 */
void 
print_field(void)
{
  unsigned int i,j;
  printf (" #| ");
  for (i=0; i<num_columns; i++)
    printf ("%u ",i);
  printf ("\n");
  for (i=0; i<num_columns; i++)
    printf ("--");
  printf ("---\n");
  for (i=0; i<num_rows; i++)
  {
    printf ("%2u| ",i);
    for (j=0; j<num_columns; j++)
      printf ("%c ",field[i*num_columns +j]);   
    printf ("\n");
  }
  for (i=0; i<num_columns; i++)
    printf ("--");
  printf ("---\n");
  fflush(NULL);
}

/*
 * Applies gravity on slot indexed
 * by row_idx and col_idx and sets
 * them properly after gravity
 */
void 
gravity(int *row_idx, int *col_idx)
{
  if (field[(*row_idx)*num_columns + *col_idx] == EMP)
    return;
  while (*row_idx < num_rows-1 && field[(*row_idx+1)*num_columns + *col_idx] == EMP)
  {
    field[(*row_idx+1)*num_columns + *col_idx] = field[(*row_idx)*num_columns + *col_idx];
    field[(*row_idx)*num_columns + *col_idx] = EMP;
    (*row_idx)++;
  }
}


/*
 * Send final task to all workers,
 */
void
send_kill(numproc)
{
  uint32_t kill = UINT_MAX;
  unsigned int i;

  for (i = 1; i < numproc ; i++)
  {
    printf ("[%d] killing (%d)\n",myid,i);
    MPI_Send(&kill,sizeof(kill),MPI_BYTE,i,TASK,MPI_COMM_WORLD);
  }
}

/*
 * Receives copy of game context 
 * and evaluates subtree of given
 * context. After that, it returns
 * result
 */
void
worker(void)
{
  int total_tasks = 0;
  int total_time = 0;
  int start,end;
  int bsize;
  struct result r;
  struct task *t;
  MPI_Status status;

  while (1)
  {
#ifdef DEBUG
    printf ("[%d] WAITING TASK\n",myid);
#endif
    MPI_Probe(ROOT,TASK,MPI_COMM_WORLD,&status);
    MPI_Get_count(&status, MPI_BYTE,&bsize); 

    t = malloc(bsize);
    if (t == NULL)
    {
      printf ("========MALLOC FAILURE==========\n"); fflush(NULL);
      perror("malloc");
      panic("malloc is asshole\n");
    }
  
    MPI_Recv(t,bsize,MPI_BYTE,ROOT,TASK, MPI_COMM_WORLD, &status);
#ifdef DEBUG
    printf ("[%d] RECEIVED TASK (id=%d)\n",myid,t->task_id);
#endif

    if ((*(unsigned int*)t) == KILL)
    {
#ifdef DEBUG
      printf ("[%d] WILL DIE IN PEACE \n",myid);
#endif
      break;
    }

    field = malloc(t->num_rows * t->num_columns);
    if (field == NULL)
    {
      printf ("========MALLOC FAILURE==========\n"); fflush(NULL);
      perror("malloc");
      panic("malloc is asshole\n");
    }

    t->field = (unsigned char*)t + sizeof(struct task);
    memcpy(field,t->field,t->num_rows * t->num_columns);

    //r.eval    = worker_evaluate(t->depth+1,t->next_move);
    start = MPI_Wtime();
    r.eval    = worker_evaluate(t->depth,t->next_move);
    end = MPI_Wtime();
    total_time += (end-start);
    r.task_id = t->task_id;

    MPI_Send(&r,sizeof(r),MPI_BYTE,ROOT,RES,MPI_COMM_WORLD);
#ifdef DEBUG
    printf ("[%d] SENDING RESULT (r=%lf)\n",myid,r.eval);
#endif
    free(field);
    total_tasks++;
  }
  printf ("[%d] total computed tasks = %d\n",myid,total_tasks);
  printf ("[%d] total time = %d s\n",myid,total_time);
  return;
}



/* 
 * Tries all possible columns and 
 * checks wheater it is a best move or not
 */
void 
make_cpu_move(int *row_idx,int *col_idx)
{
  int i,j,col,row;
  double eval=0,t_eval;
  unsigned int old_num_rows;

  /* check all columns (as they are only possible solutions) */
  check_overflow();
  for (i=0, j=0; j<num_columns; j++)
  {
    row = i; col = j;
    /* make move on column and apply gravity */
    field[row * num_columns + col] = CPU;
    gravity (&row,&col);

    old_num_rows = num_rows;

    /* evaluate this move */
    if (eval_winner(row,col,CPU) == TRUE )
      t_eval = 1;
    else
    {
      root_dispatch(0,USR);
      root_collect();
      t_eval = root_evaluate(0,USR);
      //free(res);
      t_id = -1;
      t_id2 = -1;
      //exit(EXIT_FAILURE);
    }

    if (num_rows != old_num_rows)
    {
      row      += num_rows - old_num_rows;
      *row_idx += num_rows - old_num_rows;
    }

    if (j==0 || t_eval >= eval)
    {
      eval = t_eval;
      *row_idx = row;
      *col_idx = col;
    }
    
    print_field();
    
    field[row * num_columns + col] = EMP;
  }
  /* apply move on best evaluated column (gravity is alrealy applied) */
  field[*row_idx * num_columns + *col_idx] = CPU;
  //printf ("best eval is %lf\n",eval);
}


/*
 * Interactive mode to talk to user
 */
void 
interactive(void)
{
  char buf[33];
  char *tok;
  int row_idx=0,col_idx=0;
  while (1)
  {
    print_field();
    printf ("choose your move (column index):");
    fgets(buf,33,stdin);

    tok = strtok(buf," ");
    if (tok == NULL)
    {
      fprintf (stderr,"can't parse your input...\n");
      continue;
    }
    col_idx = strtol (tok,NULL,0);
    row_idx = 0;

    if (row_idx < 0 || row_idx >= num_rows ||
        col_idx < 0 || col_idx >= num_columns)
    {
      fprintf (stderr,"(%d %d) indexes are illegal...\n",row_idx,col_idx);
      continue;
    }

    if (field[(row_idx)*num_columns + col_idx] != EMP)
    {
      fprintf (stderr,"slot not free...\n");
      continue;
    }
    else
    {
      field[(row_idx)*num_columns + col_idx] = USR;
      gravity (&row_idx,&col_idx);
    
      print_field();

      if (eval_winner(row_idx,col_idx,USR))
      {
        printf ("~ winner :) ~\n");
        break;
      }
      check_overflow();
    }

    // computer
    double start,end;
    start = MPI_Wtime();
    make_cpu_move(&row_idx,&col_idx);
    end = MPI_Wtime();
    printf ("==used %lf seconds==\n",end-start);
    print_field();
    if (eval_winner(row_idx,col_idx,CPU))
    {
      printf ("~ looser :( ~\n");
      break;
    }
  }
}

/*
 * Root process executes interactive
 * mode shell and decides of computer
 * moves
 */
void 
root(int numproc)
{
  field = malloc (num_rows * num_columns);
  if (field == NULL)
  {
    printf ("========MALLOC FAILURE==========\n"); fflush(NULL);
    perror("malloc");
  }
  memset(field,EMP,num_rows * num_columns);
  interactive();
  free(field);
  send_kill(numproc);
}


int 
main(int argc, char *argv[])
{
  if (argc != 1 && argc != 3)
  {
    fprintf (stderr,"usage: %s num_rows num_columns\n",argv[0]);
    return 1;
  }

  if (argc == 3)
  { 
    num_rows    = strtoul (argv[1],NULL,0);
    num_columns = strtoul (argv[2],NULL,0);
  } 
  else
  {
    num_rows    = DEFAULT_ROWS;
    num_columns = DEFAULT_COLUMNS;
  }

  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&numproc);
  MPI_Comm_rank(MPI_COMM_WORLD,&myid);
  MPI_Get_processor_name(processor_name,&namelen);

  srand(time(NULL) + myid);

  if (numproc < MIN_PROCS) {
    fprintf (stderr,"only %u of us, commiting suicide ...\n",numproc);
    exit(EXIT_FAILURE);
  }

  printf("[%d]: starting MPI process @%s\n", myid, processor_name); fflush(stdout);

  MPI_Barrier(MPI_COMM_WORLD);

  if (myid == ROOT)
    root(numproc);
  else
    worker();
  
  MPI_Finalize();

  return 0;
}

/*
 * This is simple evaluate that is used
 * in sequential implementation. Recursivly
 * evaluate subtree and return result
 */
double 
worker_evaluate(int c_depth, unsigned char move)
{
  int i,j,col,row;
  double eval=0,t_eval;
  unsigned int old_num_rows;


  if (c_depth > max_depth)
    return eval; // eval == 0 // divide with number of possible moves on this level 
  
  //printf ("eval depth = %d\n",c_depth);
  
  check_overflow();
  for (i=0, j=0; j<num_columns; j++)
  {
    row = i; col = j;

    /* make move on column and apply gravity */
    field[row * num_columns + col] = move;
    gravity (&row,&col);
  
    if (move == CPU && eval_winner(row,col,CPU) == TRUE )
    {
      //printf ("[%d] 888\n",myid);
      t_eval = 1;
    }
    else if (move == USR && eval_winner(row,col,USR) == TRUE)
    {
      //printf ("[%d] 999\n",myid);
      t_eval = -1;
    }
    else /* if there is no winner or looser, do the evaluation on deeper level */
    {
      old_num_rows = num_rows;
      /* change turn and evaluate this move */
      if (move == CPU)
        t_eval = worker_evaluate(c_depth+1,USR);
      else
        t_eval = worker_evaluate(c_depth+1,CPU);

      //printf ("teval = %lf\n",t_eval);
      if (num_rows != old_num_rows)
      {
        //printf ("inserted new %d rows\n",num_rows - old_num_rows);
        row += num_rows - old_num_rows;
      }
    }

    //printf ("evaluated as %lf\n",t_eval);
    eval += t_eval;
    field[row * num_columns + col] = EMP;
  }

  //printf ("eval = %lf\n",eval);
  //printf ("eeval = %lf\n",eval/(num_columns*c_depth));
  //return eval / pow(num_columns,c_depth);
  return eval/(num_columns*c_depth);
}

/* 
 * Root traverses tree until it reaches
 * depth where number of total subtrees
 * is greater then number of processors
 * powered by two. Then it creates copy
 * of game context and sends it to workers
 * in round-robin manner, adding incremental
 * ID to every task.
 */
void
root_dispatch(int c_depth, unsigned char move)
{
  int i,j,col,row;
  unsigned int old_num_rows;
  struct task *t;
  char *b;
  int now_send = FALSE;

  if (c_depth > max_depth)
    return; 
  
  if (pow(num_columns,c_depth+1) >= numproc*numproc)
  {
    now_send = TRUE;
    //printf ("on level %d, there are %d tasks\n",c_depth+1,(int)pow(num_columns,c_depth+1));
  }

  check_overflow();
  for (i=0, j=0; j<num_columns; j++)
  {
    row = i; col = j;

    /* make move on column and apply gravity */
    field[row * num_columns + col] = move;
    gravity (&row,&col);
  
    /*
    if (move == CPU && eval_winner(row,col,CPU) == TRUE )
      t_eval = 1;
    else if (move == USR && eval_winner(row,col,USR) == TRUE)
      t_eval = -1;
    else
    {
    */
    // this is evil to do

    if (now_send == TRUE)
    {
      // send tasks
      b = malloc(sizeof(struct task) + num_columns*num_rows);
      if (b==NULL)
      {
        perror("malloc");
        exit(EXIT_FAILURE);
      }
      t = (struct task*)b;
      t->num_rows      = num_rows;
      t->num_columns   = num_columns;
      if (move == CPU)
        t->next_move   = USR; 
      else
        t->next_move   = CPU;
      t->depth         = c_depth+1;
      t->task_id       = ++t_id;
      /*
      task_buffer[task_num - 1]->sender      = myid;
      task_buffer[task_num - 1]->solved      = FALSE;
      */
      t->field = NULL;
      memcpy(b+sizeof(struct task),field,num_columns*num_rows);
      
      t_dest++;
      while(t_dest == 0 || t_dest >= numproc)
        t_dest = (t_dest + 1) % numproc;
 
#ifdef DEBUG 
      printf ("[%d] SENDING TASK [%d] , ID = [%d]\n",myid,t_dest,t->task_id);
#endif
      MPI_Send(b,sizeof(struct task)+num_columns*num_rows,MPI_BYTE,t_dest,TASK,MPI_COMM_WORLD);
    }
    else
    {
      old_num_rows = num_rows;
      /* change turn and evaluate this move */
      if (move == CPU)
        root_dispatch(c_depth+1,USR);
      else
        root_dispatch(c_depth+1,CPU);

      if (num_rows != old_num_rows)
      {
        row += num_rows - old_num_rows;
      }
    }

    field[row * num_columns + col] = EMP;
  }
}


/*
 * After all tasks are dispatched,
 * they have to be collected. 
 */
void
root_collect(void)
{
  unsigned int i;
  struct result r;
  MPI_Status status;

  res = realloc(res,t_id*sizeof(double));
  for ( i = 0; i <= t_id; i++ )
  {
    MPI_Recv(&r,sizeof(r),MPI_BYTE,MPI_ANY_SOURCE,RES,MPI_COMM_WORLD,&status);
    //printf ("received %d\n",i);

    res[r.task_id] = r.eval;
  }

  /*
  printf ("results\n");
  for ( i = 0; i <= t_id; i++ )
  {
    printf (" %lf ",res[i]);
  }
  printf ("\n");
  */
}



/*
 * After all results are collected,
 * root traverses tree again, but 
 * on computed depth, now it just uses 
 * results computed by workers instead
 * of further recursive traversal. It 
 * is also possible to omit some of
 * results computed by workers, in case
 * that win or loose happens in upper
 * levels of tree.
 */
double 
root_evaluate(int c_depth, unsigned char move)
{
  int i,j,col,row;
  double eval=0,t_eval=0;
  unsigned int old_num_rows;
  unsigned int now_pick = FALSE;

  if (c_depth > max_depth)
    return eval; // eval == 0 // divide with number of possible moves on this level 
  
  if (pow(num_columns,c_depth+1) >= numproc*numproc)
  {
    now_pick = TRUE;
    //printf ("on level %d, there are %d tasks\n",c_depth+1,(int)pow(num_columns,c_depth+1));
  }

  check_overflow();
  for (i=0, j=0; j<num_columns; j++)
  {
    row = i; col = j;

    /* make move on column and apply gravity */
    field[row * num_columns + col] = move;
    gravity (&row,&col);
  
    if (move == CPU && eval_winner(row,col,CPU) == TRUE )
    {
      t_eval = 1;
      ++t_id2;
    }
    else if (move == USR && eval_winner(row,col,USR) == TRUE)
    {
      t_eval = -1;
      ++t_id2;
    }
    else /* if there is no winner or looser, do the evaluation on deeper level */
    {
      if (now_pick == TRUE)
      {
        t_eval = res[++t_id2];
        //printf ("picked %d\n",t_id2);
      }
      else
      {
        old_num_rows = num_rows;
        /* change turn and evaluate this move */
        if (move == CPU)
          t_eval = root_evaluate(c_depth+1,USR);
        else
          t_eval = root_evaluate(c_depth+1,CPU);

        if (num_rows != old_num_rows)
        {
          //printf ("inserted new %d rows\n",num_rows - old_num_rows);
          row += num_rows - old_num_rows;
        }
      }
    }

    //printf ("evaluated as %lf\n",t_eval);
    eval += t_eval;
    field[row * num_columns + col] = EMP;
  }
  return eval / pow(num_columns,c_depth);
}

