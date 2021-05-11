#include <env.h>
#include <pmap.h>
#include <printf.h>

/* Overview:
 *  Implement simple round-robin scheduling.
 *  Search through 'envs' for a runnable environment ,
 *  in circular fashion statrting after the previously running env,
 *  and switch to the first such environment found.
 *
 * Hints:
 *  The variable which is for counting should be defined as 'static'.
 */
void sched_yield(void)
{
    static int count = 0; // remaining time slices of current env
    static int point = 0; // current env_sched_list index
//u_int *addr = 0x0040500c;
//printf("\nsch_start\n");
    /*  hint:
     *  1. if (count==0), insert `e` into `env_sched_list[1-point]`
     *     using LIST_REMOVE and LIST_INSERT_TAIL.
     *  2. if (env_sched_list[point] is empty), point = 1 - point;
     *     then search through `env_sched_list[point]` for a runnable env `e`,
     *     and set count = e->env_pri
     *  3. count--
     *  4. env_run()
     *
     *  functions or macros below may be used (not all):
     *  LIST_INSERT_TAIL, LIST_REMOVE, LIST_FIRST, LIST_EMPTY
     */
    static struct Env *e;
    --count;
    while ((count <= 0) || (e->env_status != ENV_RUNNABLE)) {
        // Env e is end, need scheduling, or env e is blocked, not runnable.
//printf("sched!");
        if (e != NULL) {
            LIST_REMOVE(e, env_sched_link);
            LIST_INSERT_TAIL(&env_sched_list[1 - point], e, env_sched_link);
        }
/*      if (LIST_EMPTY(&env_sched_list[point])) { // Current list is empty, switch to another list
            point = 1 - point;
        }*/
        LIST_FOREACH(e, &env_sched_list[point], env_sched_link) {
            if (e->env_status == ENV_RUNNABLE) {
                break;
            }
        }
        if (e != NULL) {
            count = e->env_pri;
        } else {
            point = 1 - point;
            count = -1;
        }
//printf("count:%d\n", count);
    }
//printf("\nsched!run:%x,pc at %x\n", e->env_id, e->env_tf.cp0_epc);
    env_run(e);
}
/*
void sched_yield_from_int(void) {
        printf("sched_yield call from interrupt!\n");
        return;
}*/
    
