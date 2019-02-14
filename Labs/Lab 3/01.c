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
static int instr_queue_size = 0;

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
//static int fetch_index = 0;

/*ECE552 Assignment 3 - BEGIN CODE*/
static int fetch_index = 1;
static int rs_int_size = 0;//number of instructions in integer reservation stations
static int rs_fp_size = 0; //number of instrctions in floating point reservation stations	

static instruction_t* fINT_ready[FU_INT_SIZE];
static instruction_t* fFP_ready;

static int finish_cycle_INT[FU_INT_SIZE];
static int finish_cycle_FP[FU_FP_SIZE];

static instruction_t* cdb_ready;

/*ECE552 Assignment 3 - END CODE*/

/* FUNCTIONAL UNITS */


/* RESERVATION STATIONS */

/*ECE552 Assignment 3 - BEGIN CODE*/
//helper function to update IFQ
void update_instr_queue(){

for (int i=0;i<instr_queue_size-1;i++)
{
instr_queue[i]=instr_queue[i+1];
}
instr_queue[instr_queue_size-1]=NULL;
instr_queue_size--;

}
/*ECE552 Assignment 3 - END CODE*/
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
/*ECE552 Assignment 3 - BEGIN CODE*/

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
/*if (fetch_index>=sim_num_insn){
	for (int i=0; i<INSTR_QUEUE_SIZE;i++){
		if (instr_queue[i]!=NULL)
			return false;
	
}
return true;
}*/
else
return false;
/*ECE552 Assignment 3 - END CODE*/

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
/*ECE552 Assignment 3 - BEGIN CODE*/
commonDataBus=cdb_ready;
    cdb_ready=NULL;
    if(commonDataBus!=NULL){
        for (int i =0; i<RESERV_INT_SIZE;i++ )
        {
            if (reservINT[i]!=NULL){
                for (int j=0; j<3;j++)
                {
                    if(reservINT[i]->Q[j]==commonDataBus)
                    {
                        reservINT[i]->Q[j]=NULL;
                    }
                }
            }
            
        }
        
        for (int i =0; i<RESERV_FP_SIZE;i++ ){
            
            if(reservFP[i]!=NULL){
                for (int j=0; j<3;j++)
                {
                    if(reservFP[i]->Q[j]==commonDataBus)
                    {
                        reservFP[i]->Q[j]=NULL;
                    }
                }
            }
            
        }
        for (int i=0; i<MD_TOTAL_REGS;i++){
            if(map_table[i]==commonDataBus){
                map_table[i]=NULL;
            }
        }
       
        cdb_ready=NULL;
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
    for (int i=0; i<FU_INT_SIZE;i++)
    {
        if (fuINT[i]!=NULL)
            finish_cycle_INT[i]++;
    }
    for(int i=0; i<FU_FP_SIZE;i++)
    {
        if (fuFP[i]!=NULL)
            finish_cycle_FP[i]++;
        
    }
for (int i=0;i<FU_INT_SIZE;i++)
	{
		if(finish_cycle_INT[i]>3)
			{
                
                if(IS_STORE(fuINT[i]->op)){
                    for (int j=0; j<RESERV_INT_SIZE;j++){
                        if(reservINT[j]==fuINT[i]){
                            reservINT[j]=NULL;
                            rs_int_size--;
                        }
                       
                    }
                    fuINT[i]=NULL;
                    finish_cycle_INT[i]=0;
                }
                else{
                    if (cdb_ready==NULL)
                        cdb_ready=fuINT[i];
                    else
					{
						if(cdb_ready->index>fuINT[i]->index)
							cdb_ready=fuINT[i];
                    }
                    
                }
                /*else{
                 
                    for (int j=0; j<RESERV_INT_SIZE;j++){
                        if(reservINT[j]==fuINT[i]){
                            reservINT[j]=NULL;
                            rs_int_size--;
                        }
                        fuINT[i]=NULL;
                        finish_cycle_INT[i]=0;
                    }
                    
                }*/
	
            }
        
    }
for (int i=0;i<FU_FP_SIZE;i++)
	{
		if(finish_cycle_FP[i]>8)
			{
                //if(WRITES_CDB(fuFP[i]->op)){
                    if (cdb_ready==NULL)
                        cdb_ready=fuFP[i];
                    else
                        {
                            if(cdb_ready->index>fuFP[i]->index)
                                cdb_ready=fuFP[i];
                        }
			
                //}
                /*else{
                 
                    for (int j=0; j<RESERV_FP_SIZE;j++){
                        if(reservFP[j]==fuFP[i]){
                            reservFP[j]=NULL;
                            rs_fp_size--;
                        }
                    }
                    fuFP[i]=NULL;
                    finish_cycle_FP[i]=0;
                    
                }*/
            }
	}
    if(cdb_ready!=NULL)
    cdb_ready->tom_cdb_cycle=current_cycle+1;
    //clear the cdb_ready entry in FU and reservation station
    for (int i=0;i<FU_INT_SIZE;i++)
    {
        if ((fuINT[i]!=NULL)&&(cdb_ready!=NULL)&&(fuINT[i]->index==cdb_ready->index))
        {
            fuINT[i]=NULL;
            finish_cycle_INT[i]=0;
            break;
        }
        
        
    }
    for (int i=0;i<FU_FP_SIZE;i++)
    {
        if ((fuFP[i]!=NULL)&&(cdb_ready!=NULL)&&(fuFP[i]->index==cdb_ready->index))
        {
            fuFP[i]=NULL;
            finish_cycle_FP[i]=0;
            break;
        }
        
        
    }
    for (int i=0; i<RESERV_INT_SIZE;i++){
        if(reservINT[i]==cdb_ready&&cdb_ready!=NULL){
            reservINT[i]=NULL;
            rs_int_size--;
        }
    }
    for (int i=0; i<RESERV_FP_SIZE;i++){
        if(reservFP[i]==cdb_ready&&cdb_ready!=NULL){
            reservFP[i]=NULL;
            rs_fp_size--;
        }
    }
/*ECE552 Assignment 3 - END CODE*/

}

/*ECE552 Assignment 3 - BEGIN CODE*/
/*
 Description: insert the issued instrction into fINT_ready queue. For the purpose of
 mutiple instructions can enter the function unit at the same cycle.
 
 */
void insert(instruction_t* fint_ready){
    if (fINT_ready[0]==NULL){
        fINT_ready[0]=fint_ready;
        
    }
    else {
        for (int i=0; i<FU_INT_SIZE; i++){
            if(fINT_ready[i]!=NULL){
                if (fint_ready->index<fINT_ready[i]->index) {
                    instruction_t* temp1;
                    instruction_t* temp2;
                    temp1=fint_ready;
                    for(int j=i;j<FU_INT_SIZE;j++){
                        temp2=fINT_ready[j];
                        fINT_ready[j]=temp1;
                        temp1=temp2;
                    }
                    break;
                }
            }
            else{
                fINT_ready[i]=fint_ready;
                break;
            }
        }
        
        
    }
}


/*ECE552 Assignment 3 - END CODE*/
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
/*ECE552 Assignment 3 - BEGIN CODE*/
    for (int i=0; i<RESERV_INT_SIZE;i++)
    {
		if(reservINT[i]!=NULL&&(reservINT[i]->tom_execute_cycle==0)){
        if ((reservINT[i]->Q[0]==NULL)&&(reservINT[i]->Q[1]==NULL)&&(reservINT[i]->Q[2]==NULL))
        {
            /*if(fINT_ready==NULL)
                fINT_ready=reservINT[i];
            else
            {
                if(fINT_ready->index>reservINT[i]->index)
                    fINT_ready=reservINT[i];
                
                
            }*/
            insert(reservINT[i]);
        }
        }
    }
    for (int i=0; i<RESERV_FP_SIZE;i++)
    {
		if(reservFP[i]!=NULL&&(reservFP[i]->tom_execute_cycle==0)){
        if ((reservFP[i]->Q[0]==NULL)&&(reservFP[i]->Q[1]==NULL)&&(reservFP[i]->Q[2]==NULL))
        {
            if(fFP_ready==NULL)
                fFP_ready=reservFP[i];
            else
            {
                if(fFP_ready->index>reservFP[i]->index)
                    fFP_ready=reservFP[i];
                
            }
        }}
        
        
        
    }

    int k=0;
    if(fINT_ready[0]!=NULL){

        for (int i=0; i<FU_INT_SIZE;i++){
            if (fuINT[i]==NULL&&fINT_ready[k]!=NULL)
			{
				fuINT[i]=fINT_ready[k];
                
                fINT_ready[k]->tom_execute_cycle=current_cycle+1;
                //fINT_ready[k]=NULL;
                k=k+1;
				//fINT_ready->tom_execute_cycle=current_cycle;
				finish_cycle_INT[i]=0;
				//break;
			}			
    
        }
        
    }
    for(int i=0;i<FU_INT_SIZE;i++){
        fINT_ready[i]=NULL;
    }
    if(fFP_ready!=NULL){
for (int i=0; i<FU_FP_SIZE;i++)
	{
		if (fuFP[i]==NULL)
			{
				fuFP[i]=fFP_ready;
				fFP_ready->tom_execute_cycle=current_cycle+1;
                fFP_ready=NULL;
				finish_cycle_FP[i]=0;
				break;			
			}			


    }}


/*ECE552 Assignment 3 - END CODE*/
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
/*ECE552 Assignment 3 - BEGIN CODE*/
    if(instr_queue_size>0){
    if(IS_UNCOND_CTRL(instr_queue[0]->op) ||IS_COND_CTRL(instr_queue[0]->op)){
        update_instr_queue();
        
    }
    else if(USES_INT_FU(instr_queue[0]->op)&&(rs_int_size<RESERV_INT_SIZE))
        
        
    {
        //reservINT[rs_int_size]=instr_queue[0];
        for (int i=0; i<RESERV_INT_SIZE;i++)
        {
            if ((reservINT[i]==NULL))
            {
                for(int j=0 ;j<3; j++)
                {
                    if(instr_queue[0]->r_in[j]!=-1){
                        if ((map_table[instr_queue[0]->r_in[j]]!=NULL))
                            //INT_T[j][i]=instr_queue[0];
                            instr_queue[0]->Q[j]=map_table[instr_queue[0]->r_in[j]];
                        else
                            instr_queue[0]->Q[j]=NULL;
                    }
                    else
                        //INT_T[j][i]=NULL;
                        instr_queue[0]->Q[j]=NULL;
                }
                reservINT[i]=instr_queue[0];
                rs_int_size++;
                if(instr_queue[0]->r_out[0]!=-1){
                    map_table[instr_queue[0]->r_out[0]]=instr_queue[0];}
                if(instr_queue[0]->r_out[1]!=-1){
                    map_table[instr_queue[0]->r_out[1]]=instr_queue[0];}
                //instr_queue[0]->tom_dispatch_cycle=current_cycle;
                instr_queue[0]->tom_issue_cycle=current_cycle+1;
                //&(map_table[reservINT[i]->r_in[1]]==NULL)&(map_table[reservINT[i]->r_in[2]]==NULL))
                update_instr_queue();
                break;
            }
        }
        
        
    }
    else if(USES_FP_FU(instr_queue[0]->op)&&(rs_fp_size<RESERV_FP_SIZE))
    {
        //reservFP[rs_fp_size]=instr_queue[0];
        //rs_fp_size++;
        //instr_queue[0]->tom_dispatch_cycle=current_cycle;
        //update_instr_queue();
        for (int i=0; i<RESERV_FP_SIZE;i++)
        {
            if ((reservFP[i]==NULL))
            {
                for(int j=0 ;j<3; j++)
                {
                    if(instr_queue[0]->r_in[j]!=-1){
                        if ((map_table[instr_queue[0]->r_in[j]]!=NULL))
                            //FP_T[j][i]=instr_queue;
                            instr_queue[0]->Q[j]=map_table[instr_queue[0]->r_in[j]];
                        else
                            instr_queue[0]->Q[j]=NULL;
                        //FP_T[j][i]=NULL;
                    }
                    else
                        instr_queue[0]->Q[j]=NULL;
                }
                reservFP[i]=instr_queue[0];
                rs_fp_size++;
                if(instr_queue[0]->r_out[0]!=-1){
                    map_table[instr_queue[0]->r_out[0]]=instr_queue[0];}
                if(instr_queue[0]->r_out[1]!=-1){
                    map_table[instr_queue[0]->r_out[1]]=instr_queue[0];}
               // instr_queue[0]->tom_dispatch_cycle=current_cycle;
                instr_queue[0]->tom_issue_cycle=current_cycle+1;
                
                update_instr_queue();
                break;
            }
        }
    }
   // else
    //printf("wromng\n");
    //  printf("%x\n",(instr_queue[0]->op));
    
    }
/*ECE552 Assignment 3 - END CODE*/
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
/*ECE552 Assignment 3 - BEGIN CODE*/
if(fetch_index<=sim_num_insn){
	if(instr_queue_size<INSTR_QUEUE_SIZE)
	{	while(IS_TRAP(get_instr(trace, fetch_index)->op))
			{
			fetch_index++;
			//fetch(trace);
			}
		
			{
			instr_queue[instr_queue_size]=get_instr(trace, fetch_index);
			instr_queue_size++;
			fetch_index++;
			}
		
	}
}
/*ECE552 Assignment 3 - END CODE*/
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
 /*ECE552 Assignment 3 - BEGIN CODE*/
    if(instr_queue_size>0){
        if(instr_queue[instr_queue_size-1]->tom_dispatch_cycle==0)
        instr_queue[instr_queue_size-1]->tom_dispatch_cycle=current_cycle;}
//update_instr_queue();
  

//for (int i=0; i<instr_queue_size; i++)
//printf("%x\n",(instr_queue[i]->op));
//print_all_instr(trace, sim_num_insn);
		
/*ECE552 Assignment 3 - END CODE*/
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
      fetch_To_dispatch(trace, cycle);
      dispatch_To_issue(cycle);
     //print_all_instr(trace, sim_num_insn);
     /*ECE552 Assignment 3 - END CODE*/

     cycle++;

     if (is_simulation_done(sim_num_insn))
        break;
  }
  
  return cycle;
}