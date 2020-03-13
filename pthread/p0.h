
#define NUM_THREADS 16
#define BIG_ARRAY_DIM0 256
#define BIG_ARRAY_DIM1 65536

/*
 * struct to pass params to dispatched thread
 */
typedef struct {
  uint32_t t_num; /* this is the thread number */
  int sleep_time, loop0, loop1;
  double ret_val; /* some sort of a return value */
  uint64_t big_array[BIG_ARRAY_DIM0][BIG_ARRAY_DIM1];
} thread_parm_t;

