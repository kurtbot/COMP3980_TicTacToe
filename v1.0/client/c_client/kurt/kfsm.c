#include "kfsm.h"
#include <stdio.h>

int fsm_run(Environment *env, state_t *curr_state, state_t *next_state, const Transition transition_table[])
{
    state_t c_state;
    state_t n_state;

    c_state = *curr_state;
    n_state = *next_state;

    // main transition loop
    do
    {
        state_func action;
        
        action = state_transition(c_state, n_state, transition_table);

        if(action == NULL)
        {
            *curr_state = c_state;
            *next_state = n_state;
            return -1;
        }

        // update environment states
        env->from_state = c_state;
        env->to_state = n_state;

        // update local function states
        c_state = n_state;
        n_state = action(env); // do action

    } while (n_state != STATE_EXIT);
    
    *curr_state = c_state;
    *next_state = n_state;

    return 0;
}

static state_func state_transition(state_t curr_state, state_t next_state, const Transition transition_table[]) {

    const Transition *tran;

    // select first transition from transition table for transition verification
    tran = &transition_table[0];

    // while current transition is not a null state
    while(tran->from_state != STATE_NULL)
    {
        if (tran->from_state == curr_state && tran->to_state == next_state)
        {
            return tran->action;
        }

        tran = transition_table++;
    }

    return NULL;
}
