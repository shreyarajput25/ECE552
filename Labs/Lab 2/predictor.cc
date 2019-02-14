#include "predictor.h"
#include "string.h"
/////////////////////////////////////////////////////////////
// 2bitsat
/////////////////////////////////////////////////////////////
#define BYTE unsigned char
#define HWORD short
#define WORD int
#define BYTE_SIZE sizeof(BYTE)
#define BHT_ENTRY_COUNT 512
#define PHT_ENTRY_COUNT 8
#define STRONG_TAKEN 3
#define WEAK_TAKEN 2
#define WEAK_NOT_TAKEN 1
#define STRONG_NOT_TAKEN 0


int two_bit_cont [4096];

void InitPredictor_2bitsat() {

    for (int i =0; i <4096; i++){
        two_bit_cont[i] = WEAK_NOT_TAKEN;
    }
}

int GetIndex_2bitsat(UINT32 PC){
    return PC % 4096;
}

bool GetPrediction_2bitsat(UINT32 PC) {
    int index =  GetIndex_2bitsat(PC);
    if (two_bit_cont[index] == STRONG_TAKEN || two_bit_cont[index] == WEAK_TAKEN)
        return TAKEN;
    else
        return NOT_TAKEN;
}

void UpdatePredictor_2bitsat(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
    int index = GetIndex_2bitsat(PC);
    if (resolveDir == TAKEN) {
        if (two_bit_cont[index] != STRONG_TAKEN)
            two_bit_cont[index] ++;
    }
    if (resolveDir == NOT_TAKEN) {
        if (two_bit_cont[index] != STRONG_NOT_TAKEN)
            two_bit_cont[index] --;
    }

}

/////////////////////////////////////////////////////////////
// 2level
/////////////////////////////////////////////////////////////

BYTE gBHT[512];
BYTE gPHT[64][8];

int GetPHTIndex(UINT32 PC){
    return PC & 0b111;
}

int GetBHTIndex(UINT32 PC){
    return (PC & 0b111111111000) >> 3;
}

int GetHistoryIndex(BYTE history){
    return history & 0b00111111;
}

void InitPredictor_2level() {
    memset(gBHT,0, BYTE_SIZE * BHT_ENTRY_COUNT);
    int i = 0;
    for(i =0 ; i < 64; i++)
        memset(gPHT[i],WEAK_NOT_TAKEN, 8);
}

bool GetPrediction_2level(UINT32 PC) {
    int pht_index = GetPHTIndex(PC);
    int bht_index = GetBHTIndex(PC);
    if (gPHT[GetHistoryIndex(gBHT[bht_index])][pht_index] >= WEAK_TAKEN)
        return TAKEN;
    else
        return NOT_TAKEN;
}

void UpdatePredictor_2level(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
    int bht_index = GetBHTIndex(PC);
    int pht_index = GetPHTIndex(PC);
    int current_history = gPHT[GetHistoryIndex(gBHT[bht_index])][pht_index];
    int modified_result = current_history;

    if (resolveDir == TAKEN) {
        if (modified_result != STRONG_TAKEN)
            modified_result ++;
    }
    if (resolveDir == NOT_TAKEN) {
        if (modified_result != STRONG_NOT_TAKEN)
             modified_result --;
    }
    gPHT[GetHistoryIndex(gBHT[bht_index])][pht_index] = modified_result;
    gBHT[bht_index] <<= 1;
    gBHT[bht_index] |= (resolveDir == TAKEN) ? TAKEN : NOT_TAKEN;
}

#define GSHARE_HISTORY_LENGTH 19
#define GSHARE_SIZE 59999

int gShare;

BYTE gGlobalSelect[GSHARE_SIZE];

int GetPredictionIndex_Global(UINT32 PC){
    return ((PC >>2) ^ (gShare & ((1<<GSHARE_HISTORY_LENGTH)-1)))%GSHARE_SIZE;

}

void InitPredictor_Global(){
    gShare = 0;
    memset(gGlobalSelect, WEAK_NOT_TAKEN,GSHARE_SIZE);
}

bool GetPrediction_Global(UINT32 PC){
    int index = GetPredictionIndex_Global(PC);
    if(gGlobalSelect[index] >= WEAK_TAKEN)
        return TAKEN;
    else{
        return NOT_TAKEN;
    }
}
void UpdatePredictor_Global(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
    int index = GetPredictionIndex_Global(PC);
    int modified_result = gGlobalSelect[index];
    if (resolveDir == TAKEN) {
        if (modified_result != STRONG_TAKEN)
            modified_result ++;
    }
    if (resolveDir == NOT_TAKEN) {
        if (modified_result != STRONG_NOT_TAKEN)
            modified_result --;
    }
    gGlobalSelect[index] = modified_result;
    gShare <<= 1;
    gShare |= resolveDir == TAKEN? 1 : 0;
}


BYTE RP_cnt[256];
void InitPredictor_RP() {\
    for (int i = 0; i<256;i++){
        RP_cnt[i]=128;
    }

}

bool GetPrediction_RP(UINT32 PC) {
    int index = (PC>>4) % 255;
    if (RP_cnt[index] >= 128)
        return TAKEN;
    else
        return NOT_TAKEN;
}

void UpdatePredictor_RP(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
    int index = (PC>>4) % 255;
    if (resolveDir == TAKEN) {
        if (RP_cnt[index] < 256)
            RP_cnt[index]++;
    } else {
        if (RP_cnt[index] > 0)
            RP_cnt[index] --;
    }
}


/////////////////////////////////////////////////////////////
// openend
/////////////////////////////////////////////////////////////
//
//#define PCPT_ROW 512
//#define PCPT_COL 31
//#define PCPT_LIMIT 73
//
//char pcpt_weight[PCPT_ROW][PCPT_COL];
//bool pcpt_hst[PCPT_COL];
//
//void InitPredictor_PerC(){
//    int col_cont = 0;
//    int row_cont = 0;
//    for (col_cont=0;col_cont<PCPT_COL; col_cont++) {
//        pcpt_hst [col_cont]= TAKEN;
//    }
//    for (row_cont=0;row_cont<PCPT_ROW; row_cont++) {
//        for (col_cont =0; col_cont<PCPT_COL; col_cont++){
//            pcpt_weight [row_cont][col_cont]= 0;
//        }
//    }
//
//}
//int GetPrediction_PerC(UINT32 PC) {
//    int index = (PC>>4) % PCPT_ROW; // mod or and, need to be determine
//    int sum = 0;
//    for (int i=0; i < PCPT_COL; i++) {
//        if (pcpt_hst[i] == TAKEN) {
//            sum += pcpt_weight [index][i];
//        } else {
//            sum -= pcpt_weight [index][i];
//        }
//    }
//
//    return (sum >= 0) ? TAKEN : NOT_TAKEN;
//
//}
//void UpdatePredictor_PerC(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
//    int index = (PC>>4) % PCPT_ROW; // mod or and, need to be determine
//    int col_cont =0;
//
//    for (col_cont =0; col_cont < PCPT_COL -1; col_cont++) {
//        if(resolveDir == pcpt_hst[col_cont]){
//            pcpt_weight[index][col_cont] += 1;
//        }
//        else{
//            pcpt_weight[index][col_cont] += -1;
//        }
//    }
//    for (col_cont =0; col_cont < PCPT_COL -1; col_cont++) {
//        pcpt_hst[col_cont] = pcpt_hst[col_cont + 1];
//    }
//        pcpt_hst[PCPT_COL-1]= resolveDir;
//}

//#define MAP_SIZE 1024
//#define WINDOW_SIZE 7
//#define HISTORY_SIZE 32
//int gMAP[MAP_SIZE];
//
//void InitPredicor_MAP(){
//    memset(gMAP,0,MAP_SIZE);
//}
//
//int GetIndex_MAP(UINT32 PC){
//    return PC>>4 & (MAP_SIZE-1);
//}
//
//int GetPrediction_MAP(UINT32 PC){
//    int index = GetIndex_MAP(PC);
//    short history = gMAP[index];
//    short current_pattern = history & ((1<<WINDOW_SIZE)-1);
//    int shift_count = HISTORY_SIZE-WINDOW_SIZE-1;
//    int starting_shift_count = 1;
//    int potential_result = -1;
//    int final_result = -1;
//    int history_pattern = 0;
//    while(starting_shift_count<=shift_count){
//        potential_result = history & 0b1;
//        history_pattern  = (history >> 1) & ((1<<WINDOW_SIZE)-1);
//        if(history_pattern == current_pattern){
//            final_result = potential_result;
//            break;
//        }
//        starting_shift_count++;
//        history_pattern>>=1;
//    }
//    return final_result;
//}
//
//void UpdatePredictor_MAP(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
//    int index = GetIndex_MAP(PC);
//    short value = gMAP[index];
//    value <<=1;
//    value |= resolveDir==TAKEN? 1:0;
//    gMAP[index] = value;
//}


#define SELECTION_COUNTER_SIZE 997
BYTE gSELECTION_COUNTER[SELECTION_COUNTER_SIZE];
bool gMaster_last = TAKEN;
bool gAltpred_last = TAKEN;

int GetIndex_MasterSlave(UINT32 PC){
    return (PC>>2) % SELECTION_COUNTER_SIZE;
}

void InitPredictor_MasterSlave(){
    memset(gSELECTION_COUNTER,WEAK_TAKEN,SELECTION_COUNTER_SIZE);
    InitPredictor_RP();
    InitPredictor_Global();
}

bool GetPrediction_MasterSlave(UINT32 PC){
    int map_value = GetPrediction_Global(PC);
    int alt_value = GetPrediction_RP(PC);
    int selection_counter_value = gSELECTION_COUNTER[GetIndex_MasterSlave(PC)];
    gAltpred_last=alt_value;
    gMaster_last = map_value;
    if(selection_counter_value >=WEAK_TAKEN){
        return map_value;
    }
    else{
        return alt_value;
    }

}
void UpdatePredictor_MasterSlave(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
    UpdatePredictor_Global(PC,resolveDir,predDir,branchTarget);
    UpdatePredictor_RP(PC,resolveDir,predDir,branchTarget);
    int counter_value = gSELECTION_COUNTER[GetIndex_MasterSlave(PC)];
    if (resolveDir == gMaster_last && resolveDir != gAltpred_last) {
        if (counter_value < STRONG_TAKEN)
            counter_value ++;
    }
    if (resolveDir != gMaster_last && resolveDir == gAltpred_last) {
        if (counter_value > STRONG_NOT_TAKEN)
            counter_value--;
    }
    gSELECTION_COUNTER[GetIndex_MasterSlave(PC)] = counter_value;
}




void InitPredictor_openend() {
    InitPredictor_MasterSlave();
    //InitPredictor_Global();
}

bool GetPrediction_openend(UINT32 PC) {
    return GetPrediction_MasterSlave(PC);
    //return GetPrediction_Global(PC);
}

void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
    UpdatePredictor_MasterSlave(PC,resolveDir,predDir,branchTarget);
    //UpdatePredictor_Global(PC,resolveDir,predDir,branchTarget);
}
