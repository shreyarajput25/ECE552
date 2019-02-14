#include "predictor.h"

/////////////////////////////////////////////////////////////
// 2bitsat
/////////////////////////////////////////////////////////////
static UINT32 branch_history_buffer[4096];
void InitPredictor_2bitsat() {
	
	for(int i=0; i>4096; i++) {
		branch_history_buffer[i] = 0;
	}

}

bool GetPrediction_2bitsat(UINT32 PC) {
	UINT32 index = PC % 4096; 
	if (branch_history_buffer[index] < 2)
		return NOT_TAKEN;
	else
		return TAKEN;
}

void UpdatePredictor_2bitsat(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	UINT32 index = PC % 4096; 
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

static uint16_t BHT_open[512];
static uint8_t  PHT_open[8][4096];
static uint8_t  HybridSelector[512];
static uint8_t  GAp_predTable [8][512];
static uint16_t GAp_BHT = 0;

bool Store_GAp = 1;
bool Store_TwoLevel = 1;



///----------------------------------------------------------------------------------------------------------------------



void InitPredictor_Gs_openend() {
	
	int i,j;
	for (i = 0; i < 8; i++)
		for (j=0; j < 512; j++)
			GAp_predTable[i][j] = 2; 



}

bool GetPrediction_Gs_openend(UINT32 PC) {
	
	UINT32 index = (0b111111111000 & (GAp_BHT ^ (PC >> 16))) >> 3; 
	UINT32 index2 = (0b111 & (GAp_BHT ^ (PC>>16)));
	if (GAp_predTable[index2][index] > 7) 
		return TAKEN;
	else
		return NOT_TAKEN;
}

void UpdatePredictor_Gs_openend(UINT32 PC, bool resolveDir) {
	
	UINT32 index = (0b111111111000 & (GAp_BHT ^ (PC >> 16))) >> 3; 
	UINT32 index2 = (0b111 & (GAp_BHT ^ (PC>>16)));
	
	if (resolveDir == 1) {
		if (GAp_predTable[index2][index] < 15) {
			GAp_predTable[index2][index]++; 
		}
	} 
	
	else {
		if (GAp_predTable[index2][index] > 0) {
			GAp_predTable[index2][index]--; 
		}
	}
	GAp_BHT = (((GAp_BHT << 1) | resolveDir) & 0b111111111111);
}

//----------------------------------------------------------------------------------------------------------------------



void InitPredictor_2level_open() {
	int i,j;
	for(i=0;i<512;i++) {
		BHT_open[i] = 0;
	}
	for(i=0;i<8;i++) {
		for(j=0;j<4096;j++) {
			PHT_open[i][j] = 1;
		}
	}
}

bool GetPrediction_2level_open(UINT32 PC) {
	UINT32 BHT_open_index = (PC & 0b111111111000) >> 3;
	UINT32 PHT_open_index = PC & 0b111;
	
	if(PHT_open[PHT_open_index][BHT_open[BHT_open_index]] > 3)
		return TAKEN;
	else
		return NOT_TAKEN;
}

void UpdatePredictor_2level_open(UINT32 PC, bool resolveDir) {
	UINT32 BHT_open_index = (PC & 0b111111111000) >> 3;
	UINT32 PHT_open_index = PC & 0b111;
	
	
	
	if ((resolveDir == 1) && PHT_open[PHT_open_index][BHT_open[BHT_open_index]] < 7)
		PHT_open[PHT_open_index][BHT_open[BHT_open_index]]++;
	
	else if((resolveDir == 0) && PHT_open[PHT_open_index][BHT_open[BHT_open_index]] > 0)
		PHT_open[PHT_open_index][BHT_open[BHT_open_index]]--;
	
	
	
	BHT_open[BHT_open_index] = (((BHT_open[BHT_open_index] << 1) | resolveDir) & 0b111111111111);
}

//----------------------------------------------------------------------------------------------------------------------


void InitSelector_open() {
for (int i = 0; i < 512; i++)
    {
        HybridSelector[i] = 8;
    }
}

void UpdateHybridSelector(UINT32 PC, bool resolveDir)
{
    UINT32 hsindex = PC & 0b111111111;
    UINT32 selector = HybridSelector[hsindex];
    
    if (resolveDir == Store_TwoLevel && resolveDir != Store_GAp) {
        if (selector < 15)
            HybridSelector[hsindex] ++;
    }
    if (resolveDir != Store_TwoLevel && resolveDir == Store_GAp) {
        if (selector > 0)
            HybridSelector[hsindex]--;
    }
}


//----------------------------------------------------------------------------------------------------------------------

void InitPredictor_openend() {
	
	InitPredictor_Gs_openend();
	InitPredictor_2level_open();
	InitSelector_open();

}

bool GetPrediction_openend(UINT32 PC) {
	
	bool GAp_Result = GetPrediction_Gs_openend(PC);
	bool two_level = GetPrediction_2level_open(PC);
	
	Store_GAp = GAp_Result;
	Store_TwoLevel = two_level;
	
    UINT32 selector = HybridSelector[(PC & 0b111111111)];
    if(selector > 7){
        return Store_TwoLevel;
    }
    else{
        return Store_GAp;
    }
}


void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	
	UpdatePredictor_Gs_openend(PC, resolveDir);
	UpdatePredictor_2level_open(PC, resolveDir);
	UpdateHybridSelector(PC, resolveDir);

}