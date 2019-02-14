#include "predictor.h"

/////////////////////////////////////////////////////////////
// 2bitsat
/////////////////////////////////////////////////////////////
static int branch_history_buffer[4096];
void InitPredictor_2bitsat() {
	
	for(int i=0; i>4096; i++) {
		branch_history_buffer[i] = 0;
	}

}

bool GetPrediction_2bitsat(UINT32 PC) {
	int index = PC % 4096; 
	if (branch_history_buffer[index] < 2)
		return NOT_TAKEN;
	else
		return TAKEN;
}

void UpdatePredictor_2bitsat(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	int index = PC % 4096; 
	if ((resolveDir == 1) && branch_history_buffer[index] < 2)
		branch_history_buffer[index]++;
	else if ((resolveDir == 0) && branch_history_buffer[index] == 1)
		branch_history_buffer[index]--;
	else if((resolveDir == 0) && branch_history_buffer[index] > 1)
		branch_history_buffer[index]--;
	else if((resolveDir == 1) && branch_history_buffer[index] == 2)
		branch_history_buffer[index]++;
}

/////////////////////////////////////////////////////////////
// 2level
/////////////////////////////////////////////////////////////

static UINT32 BHT[512];
static UINT32 PHT[8][64];

void InitPredictor_2level() {
	int i,j;
	for(i=0;i<512;i++) {
		BHT[i] = 0;
	}
	for(i=0;i<8;i++) {
		for(j=0;j<64;j++) {
			PHT[i][j] = 1;
		}
	}
}

bool GetPrediction_2level(UINT32 PC) {
	UINT32 BHT_index = (PC & 0b111111111000) >> 3;
	UINT32 PHT_index = PC & 0b111;
	
	if(PHT[PHT_index][BHT[BHT_index]] > 1)
		return TAKEN;
	else
		return NOT_TAKEN;
}

void UpdatePredictor_2level(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	UINT32 BHT_index = (PC & 0b111111111000) >> 3;
	UINT32 PHT_index = PC & 0b111;
	
	
	
	if ((resolveDir == 1) && PHT[PHT_index][BHT[BHT_index]] < 2)
		PHT[PHT_index][BHT[BHT_index]]++;
	else if((resolveDir == 0) && PHT[PHT_index][BHT[BHT_index]] == 1)
		PHT[PHT_index][BHT[BHT_index]]--;
	else if((resolveDir == 0) && PHT[PHT_index][BHT[BHT_index]] > 1)
		PHT[PHT_index][BHT[BHT_index]]--;
	else if((resolveDir == 1) && PHT[PHT_index][BHT[BHT_index]] == 2)
		PHT[PHT_index][BHT[BHT_index]]++;
	
	
	BHT[BHT_index] = (((BHT[BHT_index] << 1) | resolveDir) & 0b111111);
}

/////////////////////////////////////////////////////////////
// openend
/////////////////////////////////////////////////////////////

/*
unsigned int gshare_pred_table [512][8];
unsigned int glob_hist = 0;

void InitPredictor_openend() {
	
	int i,j;
	for (i = 0; i < 512; i++)
		for (j=0; j < 8; j++)
			gshare_pred_table[i][j] = 2; 

}

bool GetPrediction_openend(UINT32 PC) {
	
	int index = (0b111111111000 & (glob_hist ^ PC)) >> 3; 
	int index2 = (0b111& (glob_hist ^ PC));
	return gshare_pred_table[index][index2] >= 3 ? TAKEN : NOT_TAKEN;
}

void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	
	int index = (0b111111111000 & (glob_hist ^ PC)) >> 3; 
	int index2 = (0b111 & (glob_hist ^ PC));
	
	if (resolveDir){
		if (gshare_pred_table[index][index2] < 5) {
			gshare_pred_table[index][index2]++; 
		}

	} else {
		if (gshare_pred_table[index][index2] > 0) {
			gshare_pred_table[index][index2]--; 
		}
	}
	glob_hist = (((glob_hist << 1) | resolveDir) & 0b11111111);

}
*/

static UINT32 GAp_predTable [8][512];
static UINT32 GAp_BHT = 0;

bool Store_GAp = TAKEN;
bool Store_TwoLevel = TAKEN;

void InitPredictor_openend() {
	
	int i,j;
	for (i = 0; i < 8; i++)
		for (j=0; j < 512; j++)
			GAp_predTable[i][j] = 2; 



}

bool GetPrediction_openend(UINT32 PC) {
	UINT32 PC_1 = PC >> 16;
	
	UINT32 index = (0b111111111000 & (GAp_BHT ^ PC_1)) >> 3; 
	UINT32 index2 = (0b111 & (GAp_BHT ^ PC_1));
	if (GAp_predTable[index2][index] > 2) 
		return TAKEN;
	else
		return NOT_TAKEN;
}

void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	
	UINT32 PC_1 = PC;
//	printf("---->%d \n", PC_1);
	PC_1 = PC_1 >> 16;
//	printf(">>>>>%d \n", PC_1);
	UINT32 index = (0b111111111000 & (GAp_BHT ^ PC_1)) >> 3; 
	UINT32 index2 = (0b111 & (GAp_BHT ^ PC_1));
	
	if (resolveDir == 1) {
		if (GAp_predTable[index2][index] < 5) {
			GAp_predTable[index2][index]++; 
		}
	} 
	
	else {
		if (GAp_predTable[index2][index] > 0) {
			GAp_predTable[index2][index]--; 
		}
	}
	GAp_BHT = (((GAp_BHT << 1) | resolveDir) & 0b11111111);
}


