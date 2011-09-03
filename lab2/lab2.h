#include <stdint.h>

#define CPU 'C'
#define USR 'U'
#define EMP '.'

#define KILL UINT_MAX

#define TRUE  1
#define FALSE 0

#define SOLVING 433

#define INIT_DEPTH 0
#define MAX_DEPTH 8

#define DEFAULT_ROWS 6
#define DEFAULT_COLUMNS 7

#define ROOT               0
#define MIN_PROCS          0
#define STATUS_CLOSE       0
#define STATUS_NON_FREE    1
#define STATUS_AVAILABLE   2

#define TASK 1337
#define RES  1338

/*

#define TASK_NEW 23
#define TASK_ID  24

#define TASK_REQ 21
#define TASK_REP 22

#define RES_REQ 103
#define RES_RET 102
#define RES_REP 103

#define MAX_KILLS 10
#define WORK_SLEEP 1
*/


struct task 
{
  unsigned int num_rows;
  unsigned int num_columns;
  unsigned char next_move;
  unsigned int depth;
  unsigned int task_id;
  unsigned int solved;
  unsigned int sender;
  double eval;
  unsigned char * field;
};

struct result 
{
  unsigned int task_id;
  double eval;
};

/*
struct request
{
  uint8_t  id;
  uint16_t start_seat_id;
  uint16_t seat_num;
};

struct response
{
  uint8_t  status;
  uint16_t start_seat_id;
  uint16_t seat_num;
};
*/

void fatality (const char * msg);
void check_overflow(void);
void expand_field (void);
void print_field(void);
void root(int numproc);
void worker(void);
double evaluate(unsigned int c_depth, unsigned char move);
