#define MAX_SEATS        100
#define NUM_ROWS          10
#define EXIT_PERCENTAGE   10
#define MIN_PROCS          0
#define SEAT_FREE          0
#define SEAT_KEEPER        0
#define SEAT_ARBITER_ID    0
#define STATUS_CLOSE       0
#define STATUS_NON_FREE    1
#define STATUS_AVAILABLE   2
#define TAG_LEET 		1337

#include <stdint.h>

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

