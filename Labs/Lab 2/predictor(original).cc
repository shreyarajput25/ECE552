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
	
	
	
	if((PHT[PHT_index][BHT[BHT_index]] == 0) & (resolveDir == 1))
		PHT[PHT_index][BHT[BHT_index]] = PHT[PHT_index][BHT[BHT_index]] + 1;

	else if((PHT[PHT_index][BHT[BHT_index]] == 1) & (resolveDir == 0))
		PHT[PHT_index][BHT[BHT_index]] = PHT[PHT_index][BHT[BHT_index]] - 1;
	else if((PHT[PHT_index][BHT[BHT_index]] == 1) & (resolveDir == 1))
		PHT[PHT_index][BHT[BHT_index]] = PHT[PHT_index][BHT[BHT_index]] + 1;
	
	else if((PHT[PHT_index][BHT[BHT_index]] == 2) & (resolveDir == 0))
		PHT[PHT_index][BHT[BHT_index]] = PHT[PHT_index][BHT[BHT_index]] - 1;
	else if((PHT[PHT_index][BHT[BHT_index]] == 2) & (resolveDir == 1))
		PHT[PHT_index][BHT[BHT_index]] = PHT[PHT_index][BHT[BHT_index]] + 1;
	
	else if((PHT[PHT_index][BHT[BHT_index]] == 3) & (resolveDir == 0))
		PHT[PHT_index][BHT[BHT_index]] = PHT[PHT_index][BHT[BHT_index]] - 1;
	
	
	BHT[BHT_index] = (((BHT[BHT_index] << 1) | resolveDir) & 0b111111);
}

/////////////////////////////////////////////////////////////
// openend
/////////////////////////////////////////////////////////////



static int branch_history_buffer_open[4096];
static UINT32 BHT_open[512];
static UINT32 PHT_open[8][64];
static UINT32 HybridSelector[4096];
bool V2bitsat_Result = TAKEN;
bool V2Level_Result = TAKEN;


//---------------------------------------------------------------------------------------------------------------------


void InitPredictor_2bitsat_open() {
	
	for(int i=0; i>4096; i++) {
		branch_history_buffer_open[i] = 0;
	}

}

bool GetPrediction_2bitsat_open(UINT32 PC) {
	int index = PC % 4096; 
	if (branch_history_buffer_open[index] < 2)
		return NOT_TAKEN;
	else
		return TAKEN;
}

void UpdatePredictor_2bitsat_open(UINT32 PC, bool resolveDir) {
	int index = PC % 4096; 
	if ((resolveDir == 1) && branch_history_buffer_open[index] < 2)
		branch_history_buffer_open[index]++;
	else if ((resolveDir == 0) && branch_history_buffer_open[index] == 1)
		branch_history_buffer_open[index]--;
	else if((resolveDir == 0) && branch_history_buffer_open[index] > 1)
		branch_history_buffer_open[index]--;
	else if((resolveDir == 1) && branch_history_buffer_open[index] == 2)
		branch_history_buffer_open[index]++;
}

//----------------------------------------------------------------------------------------------------------------------



void InitPredictor_2level_open() {
	int i,j;
	for(i=0;i<512;i++) {
		BHT_open[i] = 0;
	}
	for(i=0;i<8;i++) {
		for(j=0;j<64;j++) {
			PHT_open[i][j] = 1;
		}
	}
}

bool GetPrediction_2level_open(UINT32 PC) {
	UINT32 BHT_open_index = (PC & 0b111111111000) >> 3;
	UINT32 PHT_open_index = PC & 0b111;
	
	if(PHT_open[PHT_open_index][BHT_open[BHT_open_index]] > 1)
		return TAKEN;
	else
		return NOT_TAKEN;
}

void UpdatePredictor_2level_open(UINT32 PC, bool resolveDir) {
	UINT32 BHT_open_index = (PC & 0b111111111000) >> 3;
	UINT32 PHT_open_index = PC & 0b111;
	
	
	
	if((PHT_open[PHT_open_index][BHT_open[BHT_open_index]] == 0) & (resolveDir == 1))
		PHT_open[PHT_open_index][BHT_open[BHT_open_index]] = PHT_open[PHT_open_index][BHT_open[BHT_open_index]] + 1;

	else if((PHT_open[PHT_open_index][BHT_open[BHT_open_index]] == 1) & (resolveDir == 0))
		PHT_open[PHT_open_index][BHT_open[BHT_open_index]] = PHT_open[PHT_open_index][BHT_open[BHT_open_index]] - 1;
	else if((PHT_open[PHT_open_index][BHT_open[BHT_open_index]] == 1) & (resolveDir == 1))
		PHT_open[PHT_open_index][BHT_open[BHT_open_index]] = PHT_open[PHT_open_index][BHT_open[BHT_open_index]] + 1;
	
	else if((PHT_open[PHT_open_index][BHT_open[BHT_open_index]] == 2) & (resolveDir == 0))
		PHT_open[PHT_open_index][BHT_open[BHT_open_index]] = PHT_open[PHT_open_index][BHT_open[BHT_open_index]] - 1;
	else if((PHT_open[PHT_open_index][BHT_open[BHT_open_index]] == 2) & (resolveDir == 1))
		PHT_open[PHT_open_index][BHT_open[BHT_open_index]] = PHT_open[PHT_open_index][BHT_open[BHT_open_index]] + 1;
	
	else if((PHT_open[PHT_open_index][BHT_open[BHT_open_index]] == 3) & (resolveDir == 0))
		PHT_open[PHT_open_index][BHT_open[BHT_open_index]] = PHT_open[PHT_open_index][BHT_open[BHT_open_index]] - 1;
	
	
	BHT_open[BHT_open_index] = (((BHT_open[BHT_open_index] << 1) | resolveDir) & 0b111111);
}

//----------------------------------------------------------------------------------------------------------------------


void InitSelector_open() {
for (int i = 0; i < 4096; i++)
    {
        HybridSelector[i] = 1;
    }
}

void UpdateHybridSelector(UINT32 PC, bool resolveDir)
{
    UINT32 hsindex = (PC >> 2) & 0b111111111111;
    UINT32 selector = HybridSelector[hsindex];
    
    if (resolveDir == V2Level_Result && resolveDir != V2bitsat_Result) {
        if (selector < 3)
            HybridSelector[hsindex] ++;
    }
    if (resolveDir != V2Level_Result && resolveDir == V2bitsat_Result) {
        if (selector > 0)
            HybridSelector[hsindex]--;
    }
}


//--------------------------------------------------------------------------------------------------------

void InitPredictor_openend() {
	
	InitPredictor_2bitsat_open();
	InitPredictor_2level_open();
	InitSelector_open();

}

bool GetPrediction_openend(UINT32 PC) {
	
	bool two_bitsat = GetPrediction_2bitsat_open(PC);
	bool two_level = GetPrediction_2level_open(PC);
	
	V2bitsat_Result = two_bitsat;
	V2Level_Result = two_level;
	
    UINT32 selector = HybridSelector[((PC >> 2) & 0b111111111111)];
    if(selector >= 2){
        return V2Level_Result;
    }
    else{
        return V2bitsat_Result;
    }
}


void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
	
	UpdatePredictor_2bitsat_open(PC, resolveDir);
	UpdatePredictor_2level_open(PC, resolveDir);
	UpdateHybridSelector(PC, resolveDir);
	
	

}

