
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
int instr_queue_size = 0;
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
static int fetch_index = 0;

/* ECE552 Assignment 3 - BEGIN CODE */
static int int_rs = 0;
static int flt_rs = 0;
static int issue_flag_int[RESERV_INT_SIZE] = {0};
static int issue_flag_fp[RESERV_FP_SIZE] = {0};
static int int_index[2];
static int fp_index;
static instruction_t* new_CDB = NULL;
static int cycle_counter = 1;
/* ECE552 Assignment 3 - END CODE */

/* FUNCTIONAL UNITS */


/* RESERVATION STATIONS */


/* ECE552 Assignment 3 - BEGIN CODE */
  
void update_ifq(int kol) {
	
	int i;
	for(i=0;i<instr_queue_size - 1;i++) {		  	
		instr_queue[i] = instr_queue[i+1];
	}
	instr_queue[instr_queue_size] = NULL;
	//instr_queue_size--;

	//printf("%d: Current IFQ -- %d  %d = %d \n",cycle_counter, kol, INSTR_QUEUE_SIZE, instr_queue_size);
	instr_queue_size--;                
	//printf("%d: Removed IFQ -- %d = %d \n",cycle_counter, kol, instr_queue_size);
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
  //printf("Done ?\n");
  if(fetch_index>=sim_num_insn){
		for (int i=0; i<RESERV_INT_SIZE;i++){
			if (reservINT[i]!=NULL)
				return false;
		}
		for (int i=0; i<RESERV_FP_SIZE;i++){
			if(reservFP[i]!=NULL)
				return false;
		}
		for (int i=0;i<FU_INT_SIZE;i++)	{
			if (fuINT[i]!=NULL)
				return false;		
		}
		for (int i=0;i<FU_FP_SIZE;i++){
				if 	(fuFP[i]!=NULL)
					return false;
		}
	return true;
  }
  else
	  return false;  
  
 /*  bool isFinished = true;
    // check IFQ
    if (instr_queue_size != 0) {
        //printf("instr_queue_size: %d\n", instr_queue_size);
        isFinished = false;
    }
    // check RS
    int i;
    for (i = 0; i < RESERV_INT_SIZE; i++)
    {
        if (reservINT[i] != NULL) {
            //PRINT_INST(stdout, reservINT[i], "reservINT is not empty: ", 0);
            isFinished = false;
        }
    }
    for (i = 0; i < RESERV_FP_SIZE; i++)
    {
        if (reservFP[i] != NULL) {
            //PRINT_INST(stdout, reservFP[i], "reservFP is not empty: ", 0);
            isFinished = false;
        }
    }
    // check CDB
    if (commonDataBus != NULL) {
        //PRINT_INST(stdout, commonDataBus, "CDB is not empty: ", 0);
        isFinished = false;
    }
	
	return isFinished; */
  
  /* ECE552 Assignment 3 - END CODE */

  //return true; //ECE552: you can change this as needed; we've added this so the code provided to you compiles
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
  /* ECE552 Assignment 3 - BEGIN CODE */
  
  //printf("Retire\n");
  commonDataBus=new_CDB;
  new_CDB=NULL;
  if(commonDataBus!=NULL){
      for (int i =0; i<RESERV_INT_SIZE;i++ )
          if (reservINT[i]!=NULL)
              for (int j=0; j<3;j++)
                  if(reservINT[i]->Q[j]==commonDataBus)
                      reservINT[i]->Q[j]=NULL;
      
      for (int i =0; i<RESERV_FP_SIZE;i++ ) 
          if(reservFP[i]!=NULL)
              for (int j=0; j<3;j++)
                  if(reservFP[i]->Q[j]==commonDataBus)
                      reservFP[i]->Q[j]=NULL;
          
      for (int i=0; i<MD_TOTAL_REGS;i++)
          if(map_table[i]==commonDataBus)
              map_table[i]=NULL;
     
      new_CDB=NULL;
      commonDataBus=NULL;

  }
  
  /* ECE552 Assignment 3 - END CODE */

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
  /* ECE552 Assignment 3 - BEGIN CODE */

  //printf("CDB\n");
  int i;
  for (i=0; i<FU_INT_SIZE; i++) {
	  if (fuINT[i] != NULL) {
		  if ((current_cycle - (fuINT[i]->tom_execute_cycle)) >= FU_INT_LATENCY) {
			  if(IS_STORE(fuINT[i]->op)){
				  reservINT[int_index[i]] = NULL;
				  fuINT[i]=NULL;
				  issue_flag_int[int_index[i]]=0;
				  int_rs--;
			  }
			  else {
				  if (new_CDB == NULL)
					  new_CDB=fuINT[i];
				  else {
					  if (fuINT[i]->index < new_CDB->index)
						  new_CDB=fuINT[i];
				  }
			  }
		  }
	  }
  }
  
  for (i=0; i<FU_FP_SIZE; i++) {
	  if (fuFP[i] != NULL) {
		  if ((current_cycle - (fuFP[i]->tom_execute_cycle)) >= FU_FP_LATENCY) {
				if (new_CDB == NULL)
				  new_CDB=fuFP[i];
				else {
				  if (fuFP[i]->index < new_CDB->index)
					  new_CDB=fuFP[i];
				}
			}
		}
	}
	
	if (new_CDB != NULL)
		new_CDB->tom_cdb_cycle=current_cycle+1;	
	
	for (i=0; i<FU_INT_SIZE; i++)
    {
        if ((fuINT[i]!=NULL)&&(new_CDB!=NULL)&&(fuINT[i]->index==new_CDB->index)) {
            fuINT[i]=NULL;
			reservINT[int_index[i]] = NULL;
			issue_flag_int[int_index[i]] = 0;
			int_rs--;
	    }
	}
    
	for (int i=0;i<FU_FP_SIZE;i++)
    {
        if ((fuFP[i]!=NULL)&&(new_CDB!=NULL)&&(fuFP[i]->index==new_CDB->index)) {
            fuFP[i]=NULL;
			reservFP[fp_index] = NULL;
			issue_flag_fp[fp_index] = 0;
			flt_rs--;
		}
	}
	
  /* ECE552 Assignment 3 - END CODE */

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
  /* ECE552 Assignment 3 - BEGIN CODE */
  //printf("Execute\n");
  int i, rs_size, limit_size, oldest_instr_index, temp1, temp2, rsrv_index[RESERV_INT_SIZE];
  instruction_t* rs_int[RESERV_INT_SIZE] = {NULL};
  instruction_t* rs_fp[RESERV_FP_SIZE] = {NULL};
  rs_size = 0;
  limit_size = 0;
  
  for (i=0; i < RESERV_INT_SIZE; i++) {
        if (reservINT[i] 	   != NULL &&
            reservINT[i]->Q[0] == NULL &&
            reservINT[i]->Q[1] == NULL &&
            reservINT[i]->Q[2] == NULL &&
            reservINT[i]->tom_execute_cycle == 0 ) {
			//reservINT[i]->tom_issue_cycle != 0) {
            rs_int[rs_size] = reservINT[i];
			rsrv_index[rs_size] = i;
			rs_size++;
			
			
		}
  }
  if (rs_size > 0) {
	  temp1 = rs_int[0]->index;
	  temp2 = rsrv_index[0];
	  oldest_instr_index = 0;
  
	  for (i=0; i < rs_size; i++) {
		  if ( (rs_int[i]->index) < temp1 ) {
			  temp1 = rs_int[i]->index;
			  temp2 = rsrv_index[i];
			  oldest_instr_index = i;
		  }
	  }
	  for (i=0; i < FU_INT_SIZE; i++) {
				if ((fuINT[i] == NULL) && (limit_size == 0)) {
					fuINT[i] = rs_int[oldest_instr_index];
					fuINT[i]->tom_execute_cycle = current_cycle;
					reservINT[temp2]->tom_execute_cycle = current_cycle;
					int_index[i] = temp2;
					limit_size++;
				}
	  }
  }
  
  rs_size = 0;
  limit_size = 0;
  
  for (i=0; i < RESERV_FP_SIZE; i++) {
        if (reservFP[i] 	  != NULL &&
            reservFP[i]->Q[0] == NULL &&
            reservFP[i]->Q[1] == NULL &&
            reservFP[i]->Q[2] == NULL &&
            reservFP[i]->tom_execute_cycle == 0 ) {
		//	reservINT[i]->tom_issue_cycle != 0) {
            rs_fp[rs_size] = reservFP[i];
			rsrv_index[rs_size] = i;
			rs_size++;
		}
  }
  if (rs_size > 0) {
	  temp1 = rs_fp[0]->index;
	  temp2 = rsrv_index[0];
	  oldest_instr_index = 0;
  
	  for (i=0; i < rs_size; i++) {
		  if ( (rs_int[i]->index) < temp1 ) {
			  temp1 = rs_fp[i]->index;
			  temp2 = rsrv_index[i];
			  oldest_instr_index = i;
		  }
	  }
	  for (i=0; i < FU_FP_SIZE; i++) {
				if ((fuFP[i] == NULL) && (limit_size == 0)) {
					fuFP[i] = rs_fp[oldest_instr_index];
					fuFP[i]->tom_execute_cycle = current_cycle;
					reservFP[temp2]->tom_execute_cycle = current_cycle;
					fp_index = temp2;
					limit_size++;
				}
	  }
  }
  
	  
	  
  
  
  /* ECE552 Assignment 3 - END CODE */
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
  /* ECE552 Assignment 3 - BEGIN CODE */
  //printf("Issue\n");
  int i,j,flag;
  
  for (i=0;i<RESERV_INT_SIZE;i++) {
	  flag = 0;
	if ((reservINT[i] != NULL) && issue_flag_int[i] < 1) {
		
		for (j=0;j<3;j++) {
			if (reservINT[i] -> r_in[j] != DNA) {
				if (map_table[reservINT[i]->r_in[j]] != NULL) {
                        reservINT[i]->Q[j] = map_table[reservINT[i]->r_in[j]];
						flag++;
                }
				else
					flag++;
			}
		}
		for (j = 0; j < 2; j++) {
            if (reservINT[i]->r_out[j] != DNA) {
                map_table[reservINT[i]->r_out[j]] = reservINT[i];
				flag++;
            }
			else
				flag++;
        }
	}
  
  if (flag == 5) {
	issue_flag_int[i]++;
	reservINT[i] -> tom_issue_cycle = current_cycle;
  }
  
  }
  
  
  
  
  
  
  
  for (i=0;i<RESERV_FP_SIZE;i++) {
	  flag = 0;
	if ((reservFP[i] != NULL) && issue_flag_fp[i] < 1) {
		
		for (j=0;j<3;j++) {
			if (reservFP[i] -> r_in[j] != DNA) {
				if (map_table[reservFP[i]->r_in[j]] != NULL) {
                        reservFP[i]->Q[j] = map_table[reservFP[i]->r_in[j]];
						flag++;
                }
				else
					flag++;
			}
		}
		for (j = 0; j < 2; j++) {
            if (reservFP[i]->r_out[j] != DNA) {
                map_table[reservFP[i]->r_out[j]] = reservFP[i];
				flag++;
            }
			else
				flag++;
        }
	}
  
   if (flag == 5) {
	issue_flag_fp[i]++;
	reservFP[i] -> tom_issue_cycle = current_cycle;
  }
  
  }
  
  
  
 
  
  /* ECE552 Assignment 3 - END CODE */
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
  /* ECE552 Assignment 3 - BEGIN CODE */
  
  //printf("Fetch\n IFQ = %d \n int_rs = %d flt_rs = %d \n", instr_queue_size, int_rs, flt_rs);
  int flag = 0;
  
  if (fetch_index <= sim_num_insn) {
	  if (instr_queue_size < INSTR_QUEUE_SIZE) {
		  while (IS_TRAP(get_instr(trace,fetch_index) -> op)) {
			  fetch_index++;
		  }
			while(!flag) {
				if (IS_UNCOND_CTRL(get_instr(trace,fetch_index) -> op)|| 
					IS_COND_CTRL(get_instr(trace,fetch_index) -> op)  ||
					USES_INT_FU(get_instr(trace,fetch_index) -> op)   ||
					USES_FP_FU(get_instr(trace,fetch_index) -> op)) {
					  
					//printf("%dskldjlaskdaskl",instr_queue_size);
					instr_queue[instr_queue_size] = get_instr(trace, fetch_index);
					PRINT_INST(stdout,instr_queue[instr_queue_size]," inst- ",cycle_counter);
					instr_queue_size++;
					
					fetch_index++;
					flag++;
			  }
			  
			  else
				  fetch_index++;
		  
			}
	  }
			//else
				//printf("over -----------------------------------------------------\n");
		  
	  
  }
		  
		  
  
  /* ECE552 Assignment 3 - END CODE */
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
  /* ECE552 Assignment 3 - BEGIN CODE */
  
	//printf("Decode\n");
	int i, empty_rs;
	
	
  
   	if (instr_queue_size > 0) {

	
		//PRINT_INST(stdout,instr_queue[0],"1",current_cycle);
	
		if (IS_UNCOND_CTRL(instr_queue[0] -> op) || 
			IS_COND_CTRL(instr_queue[0] -> op)) {
				
				update_ifq(1);
				
		}
		else if (USES_INT_FU(instr_queue[0] -> op) && (int_rs < RESERV_INT_SIZE)) {
			
			empty_rs = -1;
			
			for (i=0;i<RESERV_INT_SIZE;i++) {
				if (reservINT[i]==NULL)
					empty_rs = i;
			}
			if (empty_rs != -1)	{
				instr_queue[0]->tom_dispatch_cycle = current_cycle;
				reservINT[empty_rs] = instr_queue[0];
				int_rs++;
				update_ifq(2);
			}
				
		}
		
		else if (USES_FP_FU(instr_queue[0] -> op) && (flt_rs < RESERV_FP_SIZE)) {
			
			empty_rs = -1;
			
			for (i=0;i<RESERV_FP_SIZE;i++) {
				if (reservFP[i]==NULL)
					empty_rs = i;
			}
			
			if (empty_rs != -1) {
				instr_queue[0]->tom_dispatch_cycle = current_cycle;
				reservFP[empty_rs] = instr_queue[0];
				flt_rs++;
				update_ifq(3);
			}
				
		}
	}
		  
  /* ECE552 Assignment 3 - END CODE */
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