#ifndef KFSM_H
#define KFSM_H

// to be treated as global variables being passed around the system
typedef struct
{
    int from_state;
    int to_state;
} Environment;

typedef int state_t;
typedef int (*state_func) (Environment *env);

typedef struct 
{
    int from_state;
    int to_state;
    state_func action;
} Transition;

typedef enum 
{
    STATE_NULL = -1,    // no state
    STATE_INIT,         // initialization state
    STATE_EXIT,         // end of program state
    STATE_START,        // main start state
} Default_States;

int fsm_run(Environment *env, state_t *curr_state, state_t *next_state, const Transition transition_table[]);
static state_func state_transition(state_t curr_state, state_t next_state, const Transition transition_table[]); 

#endif
