/*
 * "vars.h"
 *
 * Part of Pro-Wizard-1 Package
 * (c) Sylvain "Asle" Chipaux
*/


FILE      *PW_in,*PW_out;
int64_t   PW_Start_Address=0;
uint32_t  OutputSize=0;
int64_t   PW_in_size=0;
int32_t   Cpt_Filename=0;
uint32_t  PW_i;
uint32_t  PW_j,PW_k,PW_l,PW_m,PW_n,PW_o;
uint8_t   *in_data;
/*uint8_t OutName[5]={'.','-','-','-',0x00};*/
char      OutName_final[33];
char      Depacked_OutName[33];
char      User_OutName[256];
uint8_t   Save_Status = GOOD;
uint32_t  PW_WholeSampleSize=0;
char      Extensions[_KNOWN_FORMATS+1][33];
uint8_t   CONVERT = BAD;
uint8_t   Amiga_EXE_Header = GOOD;
uint8_t   Scan_Only = BAD;
int32_t   Requested_Index = 0;
int32_t   Current_Found_Index = 0;
uint8_t   Filter_By_Index = BAD;
uint8_t   First_JSON_Entry = GOOD;
uint8_t   Do_Data_Mode = BAD;
uint8_t   Do_Module_Mode = BAD;
uint8_t   Do_Rip = BAD;
uint8_t   Do_Depack = BAD;
uint8_t   Script_Mode = BAD;
uint8_t   Module_Found = BAD;
uint8_t   Current_Is_Module = GOOD;
uint8_t   PW_LibMode = 0;
PW_FindResult PW_Results[PW_MAX_RESULTS];
int32_t   PW_ResultCount = 0;
