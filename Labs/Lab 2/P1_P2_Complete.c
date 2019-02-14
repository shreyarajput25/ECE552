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

void InitPredictor_openend() {

}

bool GetPrediction_openend(UINT32 PC) {

  return TAKEN;
}

void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {

}

