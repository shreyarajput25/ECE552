
#include <limits.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "sim.h"
#include "decode.def"

#include "instr.h"

/* PARAMETERS OF THE TOMASULO'S ALGORITHM */

#define INSTR_QUEUE_SIZE         10

#define RESERV_INT_SIZE    4
#define RESERV_FP_SIZE     2
#define FU_INT_SIZE        2
#define FU_FP_SIZE         1

#define FU_INT_LATENCY     4
#define FU_FP_LATENCY      9

/* IDENTIFYING INSTRUCTIONS */

//unconditional branch, jump or call
#define IS_UNCOND_CTRL(op) (MD_OP_FLAGS(op) & F_CALL || \
                         MD_OP_FLAGS(op) & F_UNCOND)

//conditional branch instruction
#define IS_COND_CTRL(op) (MD_OP_FLAGS(op) & F_COND)

//floating-point computation
#define IS_FCOMP(op) (MD_OP_FLAGS(op) & F_FCOMP)

//integer computation
#define IS_ICOMP(op) (MD_OP_FLAGS(op) & F_ICOMP)

//load instruction
#define IS_LOAD(op)  (MD_OP_FLAGS(op) & F_LOAD)

//store instruction
#define IS_STORE(op) (MD_OP_FLAGS(op) & F_STORE)

//trap instruction
#define IS_TRAP(op) (MD_OP_FLAGS(op) & F_TRAP) 

#define USES_INT_FU(op) (IS_ICOMP(op) || IS_LOAD(op) || IS_STORE(op))
#define USES_FP_FU(op) (IS_FCOMP(op))

#define WRITES_CDB(op) (IS_ICOMP(op) || IS_LOAD(op) || IS_FCOMP(op))

/* FOR DEBUGGING */

//prints info about an instruction
#define PRINT_INST(out,instr,str,cycle)	\
  myfprintf(out, "%d: %s", cycle, str);		\
  md_print_insn(instr->inst, instr->pc, out); \
  myfprintf(stdout, "(%d)\n",instr->index);

#define PRINT_REG(out,reg,str,instr) \
  myfprintf(out, "reg#%d %s ", reg, str);	\
  md_print_insn(instr->inst, instr->pc, out); \
  myfprintf(stdout, "(%d)\n",instr->index);

/* VARIABLES */

//instruction queue for tomasulo
static instruction_t* instr_queue[INSTR_QUEUE_SIZE];

//number of instructions in the instruction queue

/* ECE552 Assignment 3 - BEGIN CODE */
static int instr_queue_size = 0;
/* ECE552 Assignment 3 - END CODE */

//reservation stations (each reservation station entry contains a pointer to an instruction)
static instruction_t* reservINT[RESERV_INT_SIZE];
static instruction_t* reservFP[RESERV_FP_SIZE];

//functional units
static instruction_t* fuINT[FU_INT_SIZE];
static instruction_t* fuFP[FU_FP_SIZE];

//common data bus
static instruction_t* commonDataBus = NULL;

//The map table keeps track of which instruction produces the value for each register
static instruction_t* map_table[MD_TOTAL_REGS];

//the index of the last instruction fetched
/* ECE552 Assignment 3 - BEGIN CODE */
static int fetch_index = 0;
/* ECE552 Assignment 3 - END CODE */

/* ECE552 Assignment 3 - BEGIN CODE */
static instruction_t* new_cdb = NULL;
static int cycle_counter = 1;
static int int_latency[FU_INT_SIZE];
static int flt_latency[FU_FP_SIZE];
static int ifq_head = 0;
static int ifq_tail = 0;
/* ECE552 Assignment 3 -END CODE*/


/* ECE552 Assignment 3 - END CODE */

/* FUNCTIONAL UNITS */


/* RESERVATION STATIONS */


/* ECE552 Assignment 3 - BEGIN CODE */


void add_ifq(instruction_t *insn) {
  if (instr_queue_size != 0) 
    ifq_tail = (ifq_tail + 1) % INSTR_QUEUE_SIZE;        
  instr_queue[ifq_tail] = insn;
  instr_queue_size++;
}

void remove_ifq() {
	instr_queue[ifq_head] = NULL;
	if (ifq_head != ifq_tail)
		ifq_head = (ifq_head + 1) % INSTR_QUEUE_SIZE;
	instr_queue_size--;
}
 
/* ECE552 Assignment 3 - END CODE */


/* 
 * Description: 
 * 	Checks if simulation is done by finishing the very last instruction
 *      Remember that simulation is done only if the entire pipeline is empty
 * Inputs:
 * 	sim_insn: the total number of instructions simulated
 * Returns:
 * 	True: if simulation is finished
 */
static bool is_simulation_done(counter_t sim_insn) {

  /* ECE552: YOUR CODE GOES HERE */
  /* ECE552 Assignment 3 - BEGIN CODE */
  
  int i;
  if(fetch_index>=sim_num_insn){
	  
	if(instr_queue_size != 0)
	  return false;	
	for (i=0; i<RESERV_INT_SIZE;i++)
		if (reservINT[i]!=NULL)
			return false;
	for (i=0; i<RESERV_FP_SIZE;i++)
		if(reservFP[i]!=NULL)
			return false;
	for (i=0;i<FU_INT_SIZE;i++)	
		if (fuINT[i]!=NULL)
			return false;		
	for (i=0;i<FU_FP_SIZE;i++)
			if 	(fuFP[i]!=NULL)
				return false;
	return true;
  }
  else {
	  
	  // if (cycle_counter > 1750000) {
		  
		  // printf("**************cycle - %d *************** \n", cycle_counter);
		 // printf("***  %d  *** \n", (cycle_counter - (fuINT[i]->tom_execute_cycle)));
		  // printf("**************queue - %d *************** \n", instr_queue_size);
		  
		 // for (int i=0; i<RESERV_INT_SIZE;i++){
			// printf("***  %d  *** \n", (cycle_counter - (reservINT[i]->tom_execute_cycle)));
		// }
	  
		// for (int i=0; i<RESERV_INT_SIZE;i++){
			// if (reservINT[i]!=NULL)
				// printf("reservINT[%d]---- %d \n",i, reservINT[i]->tom_execute_cycle);
		// }
		// for (int i=0; i<RESERV_FP_SIZE;i++){
			// if(reservFP[i]!=NULL)
				// printf("reservFP[%d]\n",i);
		// }
		// for (int i=0;i<FU_INT_SIZE;i++)	{
			// if (fuINT[i]!=NULL)
				// printf("fuINT[%d]\n",i);		
		// }
		// for (int i=0;i<FU_FP_SIZE;i++){
				// if 	(fuFP[i]!=NULL)
					// printf("fuFP[%d]\n",i);
		// }
	  // }
  
	  return false;  
  }
  /* ECE552 Assignment 3 - END CODE */
}

/* 
 * Description: 
 * 	Retires the instruction from writing to the Common Data Bus
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void CDB_To_retire(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */
  /*ECE552 Assignment 3 - BEGIN CODE*/
	int i, j;
	commonDataBus=new_cdb;
	new_cdb=NULL;
	if(commonDataBus!=NULL){
		for (i =0; i<RESERV_INT_SIZE;i++)
			if (reservINT[i]!=NULL)
				for (j=0; j<3;j++)
					if(reservINT[i]->Q[j]==commonDataBus)
						reservINT[i]->Q[j]=NULL;
			
    
		for (i =0; i<RESERV_FP_SIZE;i++ )				
			if(reservFP[i]!=NULL)
				for (j=0; j<3;j++)
					if(reservFP[i]->Q[j]==commonDataBus)
						reservFP[i]->Q[j]=NULL;
			
		
		for (i=0; i<MD_TOTAL_REGS;i++)
			if(map_table[i]==commonDataBus)
				map_table[i]=NULL;
	   
		new_cdb=NULL;
		commonDataBus=NULL;

    }

  /*ECE552 Assignment 3 - END CODE*/

}

/* 
 * Description: 
 * 	Moves an instruction from the execution stage to common data bus (if possible)
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */

void execute_To_CDB(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */
  /*ECE552 Assignment 3 - BEGIN CODE*/
  int i, j;
    for (i=0; i<FU_INT_SIZE;i++) {
        if (fuINT[i]!=NULL)
            int_latency[i]++;
	}
   
	for(i=0; i<FU_FP_SIZE;i++) { 
			if (fuFP[i]!=NULL)
				flt_latency[i]++;
	}
	
	for (i=0;i<FU_INT_SIZE;i++) {
		if(int_latency[i]>3) {                
			if(IS_STORE(fuINT[i]->op)){
				for (j=0; j<RESERV_INT_SIZE;j++)
					if(reservINT[j]==fuINT[i])
						reservINT[j]=NULL;
				fuINT[i]=NULL;
				int_latency[i]=0;
			}
			else {
				if (new_cdb==NULL)
					new_cdb=fuINT[i];
				else
					if(new_cdb->index>fuINT[i]->index)
						new_cdb=fuINT[i];
			}	
        }       
    }
	for (i=0;i<FU_FP_SIZE;i++) {
		if(flt_latency[i]>8) {
            if (new_cdb==NULL)
                new_cdb=fuFP[i];
            else {
              if(new_cdb->index>fuFP[i]->index)
                new_cdb=fuFP[i];
            }
        }
	}
    if(new_cdb!=NULL)
    new_cdb->tom_cdb_cycle=current_cycle+1;
    for (i=0;i<FU_INT_SIZE;i++)
        if ((fuINT[i]!=NULL)&&(new_cdb!=NULL)&&(fuINT[i]->index==new_cdb->index)) {
            fuINT[i]=NULL;
            int_latency[i]=0;
            break;
        }    
		
    for (i=0;i<FU_FP_SIZE;i++)
        if ((fuFP[i]!=NULL)&&(new_cdb!=NULL)&&(fuFP[i]->index==new_cdb->index)) {
            fuFP[i]=NULL;
            flt_latency[i]=0;
            break;
        }    

    for (i=0; i<RESERV_INT_SIZE;i++){
        if(reservINT[i]==new_cdb&&new_cdb!=NULL)
            reservINT[i]=NULL;
    }
	
    for (i=0; i<RESERV_FP_SIZE;i++){
        if(reservFP[i]==new_cdb&&new_cdb!=NULL)
            reservFP[i]=NULL;
    }
/*ECE552 Assignment 3 - END CODE*/
} 
 
 /* 
 * Description: 
 * 	Moves instruction(s) from the issue to the execute stage (if possible). We prioritize old instructions
 *      (in program order) over new ones, if they both contend for the same functional unit.
 *      All RAW dependences need to have been resolved with stalls before an instruction enters execute.
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */ 
 void issue_To_execute(int current_cycle) {
  /* ECE552: YOUR CODE GOES HERE */
  /* ECE552 Assignment 3 -BEGIN CODE*/
    
	int i,j;
    int fu_head = 0;
    int fu_queue_size = 0;
	instruction_t* fu_queue[RESERV_INT_SIZE] = {NULL};
	instruction_t* swap = NULL;
	
    for (i = 0; i < RESERV_INT_SIZE; i++) {
        if (reservINT[i] != NULL)
           if  (reservINT[i]->Q[0] == NULL &&
				reservINT[i]->Q[1] == NULL &&
				reservINT[i]->Q[2] == NULL &&
				reservINT[i]->tom_execute_cycle == 0) {
				fu_queue[fu_queue_size++] = reservINT[i];
        }
    }
    for (i = 0; i < fu_queue_size; i++) {
        for (j = 0; j < fu_queue_size - 1; j++) {
            if (fu_queue[j]->index > fu_queue[j+1]->index) {
                swap = fu_queue[j+1];
                fu_queue[j+1] = fu_queue[j];
                fu_queue[j] = swap;
            }
        }
    }
    if (fu_queue_size > 0) {
        for (i = 0; i < FU_INT_SIZE; i++) {
            if (fuINT[i] == NULL && fu_head < fu_queue_size) {
                fuINT[i] = fu_queue[fu_head];
                fu_head++;
                fuINT[i]->tom_execute_cycle = current_cycle;
            }
        }
    }
    for (i = 0; i < RESERV_INT_SIZE; i++) {
        fu_queue[i] = NULL;
    }
    fu_head = 0;
    fu_queue_size = 0;
    for (i = 0; i < RESERV_FP_SIZE; i++) {
        if (reservFP[i] != NULL)
            if (reservFP[i]->Q[0] == NULL &&
				reservFP[i]->Q[1] == NULL &&
				reservFP[i]->Q[2] == NULL &&
				reservFP[i]->tom_execute_cycle == 0) {
				fu_queue[fu_queue_size++] = reservFP[i];
        }
    }
    for (i = 0; i < fu_queue_size; i++) {
        int j;
        for (j = 0; j < fu_queue_size - 1; j++) {
            if (fu_queue[j]->index > fu_queue[j+1]->index) {
                swap = fu_queue[j+1];
                fu_queue[j+1] = fu_queue[j];
                fu_queue[j] = swap;
            }
        }
    }
    if (fu_queue_size > 0) {
        for (i = 0; i < FU_FP_SIZE; i++) {
            if (fuFP[i] == NULL && fu_head < fu_queue_size) {
                fuFP[i] = fu_queue[fu_head];
                fu_head++;
                fuFP[i]->tom_execute_cycle = current_cycle;
            }
        }
    }
/* ECE552 Assignment 3 -END CODE*/

} 

/* 
 * Description: 
 * 	Moves instruction(s) from the dispatch stage to the issue stage
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void dispatch_To_issue(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */
  /* ECE552 Assignment 3 -BEGIN CODE*/
	
	int i, j;
	if (instr_queue_size == 0)
        return;
   
    if (IS_COND_CTRL(instr_queue[ifq_head]->op) || IS_UNCOND_CTRL(instr_queue[ifq_head]->op)) {
        instr_queue[ifq_head] = NULL;
        remove_ifq();
    } 
	
	else if (USES_INT_FU(instr_queue[ifq_head]->op)) {
        for (i = 0; i < RESERV_INT_SIZE; i++) {
            if (reservINT[i] == NULL) {
                break;
            }
        }

        if (i < RESERV_INT_SIZE) {
            instr_queue[ifq_head]->tom_issue_cycle = current_cycle;
            reservINT[i] = instr_queue[ifq_head];
            remove_ifq();

            for (j = 0; j < 3; j++) {
                if (reservINT[i]->r_in[j] != DNA) {
                    if (map_table[reservINT[i]->r_in[j]] != NULL) {
                        reservINT[i]->Q[j] = map_table[reservINT[i]->r_in[j]];
                    }
                }
            }
            for (j = 0; j < 2; j++) {
                if (reservINT[i]->r_out[j] != DNA) {
                    map_table[reservINT[i]->r_out[j]] = reservINT[i];
                }
            }
        }
    } else if (USES_FP_FU(instr_queue[ifq_head]->op)) {
        for (i = 0; i < RESERV_FP_SIZE; i++) {
            if (reservFP[i] == NULL) {
                break;
            }
        }
        if (i < RESERV_FP_SIZE) {
            instr_queue[ifq_head]->tom_issue_cycle = current_cycle;
            reservFP[i] = instr_queue[ifq_head];
            remove_ifq();
            
            for (j = 0; j < 3; j++) {
                if (reservFP[i]->r_in[j] != DNA) {
                    if (map_table[reservFP[i]->r_in[j]] != NULL) {
                        reservFP[i]->Q[j] = map_table[reservFP[i]->r_in[j]];
                    }
                }
            }
            for (j = 0; j < 2; j++) {
                if (reservFP[i]->r_out[j] != DNA) {
                    map_table[reservFP[i]->r_out[j]] = reservFP[i];
                }
            }
        }
    }
    /* ECE552 Assignment 3 -END CODE*/
}

/* 
 * Description: 
 * 	Grabs an instruction from the instruction trace (if possible)
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * Returns:
 * 	None
 */
void fetch(instruction_trace_t* trace) {
  /* ECE552: YOUR CODE GOES HERE */
  /* ECE552 Assignment 3 -BEGIN CODE*/
   
   if (fetch_index + 1 > sim_num_insn)
        return;
    while (IS_TRAP(get_instr(trace, fetch_index + 1)->op)) {
        fetch_index++;
    }
	
    if (instr_queue_size < INSTR_QUEUE_SIZE) {
		fetch_index++;
        instruction_t* instr = get_instr(trace, fetch_index);
		int i;
        for (i = 0; i < 3; i++)
            instr->Q[i] = NULL;
        add_ifq(instr);
    }
	
  /* ECE552 Assignment 3 -END CODE*/

}

/* 
 * Description: 
 * 	Calls fetch and dispatches an instruction at the same cycle (if possible)
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void fetch_To_dispatch(instruction_trace_t* trace, int current_cycle) {

  fetch(trace);

  /* ECE552: YOUR CODE GOES HERE */
  /* ECE552 Assignment 3 -BEGIN CODE*/

    instruction_t* instr_tail = instr_queue[ifq_tail];
    if (instr_tail != NULL && instr_tail->tom_dispatch_cycle == 0) {
        instr_tail->tom_dispatch_cycle = current_cycle;
    }
	
   /* ECE552 Assignment 3 -END CODE*/
}

/* 
 * Description: 
 * 	Performs a cycle-by-cycle simulation of the 4-stage pipeline
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * Returns:
 * 	The total number of cycles it takes to execute the instructions.
 * Extra Notes:
 * 	sim_num_insn: the number of instructions in the trace
 */
 
counter_t runTomasulo(instruction_trace_t* trace)
{
  //initialize instruction queue
  int i;
  for (i = 0; i < INSTR_QUEUE_SIZE; i++) {
    instr_queue[i] = NULL;
  }

  //initialize reservation stations
  for (i = 0; i < RESERV_INT_SIZE; i++) {
      reservINT[i] = NULL;
  }

  for(i = 0; i < RESERV_FP_SIZE; i++) {
      reservFP[i] = NULL;
  }

  //initialize functional units
  for (i = 0; i < FU_INT_SIZE; i++) {
    fuINT[i] = NULL;
  }

  for (i = 0; i < FU_FP_SIZE; i++) {
    fuFP[i] = NULL;
  }

  //initialize map_table to no producers
  int reg;
  for (reg = 0; reg < MD_TOTAL_REGS; reg++) {
    map_table[reg] = NULL;
  }
  
  int cycle = 1;
  while (true) {

     /* ECE552: YOUR CODE GOES HERE */
	 /*ECE552 Assignment 3 - BEGIN CODE*/
	 
	 CDB_To_retire(cycle);
     execute_To_CDB(cycle);
     issue_To_execute(cycle);
     dispatch_To_issue(cycle);
     fetch_To_dispatch(trace, cycle);
	 cycle_counter++;
	 
	 
	 /*ECE552 Assignment 3 - END CODE*/
     cycle++;

     if (is_simulation_done(sim_num_insn))
        break;
  }
  
  return cycle;
}