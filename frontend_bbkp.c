/***********************************************************************************************
 *
 * Name: frontend.c
 * Created by: Modified from the template of Stefan Ritt and Guy Ron
 *
 * Contents: Frontend for the readout of the experintal data of Neon Branching Ratio
 * experiment at SARAF
 *
 * $Id: frontend.c 0001 24-10-17    hitesh.rahangdale@mail.huji.ac.il
 *
 * ******************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "midas.h"
#include "mcstd.h"
#include <unistd.h>
#include "experim.h"
#include "frontend.h"

#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <strings.h>

#include <CAENVMElib.h>
#include <CAENVMEtypes.h>
#include <CAENComm.h>

unsigned int fAddrCTS;
unsigned int fTdcLastChannelNo;
unsigned int fTdcEpochCounter;
unsigned int fCTSExtTrigger;
unsigned int fCTSExtTriggerStatus;
bool fIsIRQEnabled;
bool IsRunStopped;

static const size_t fPacketMaxSize = 65507;
unsigned int fPacket[65507];

int DW;
int DataWaiting;
unsigned int DataToWrite[30000];
unsigned int offsetnow, offsetbefore;
/***********************************************************************************************************
 * SLEEP FUNCTION FOR DEBUG
***********************************************************************************************************/
void hhp(int num, int d)
{
  int i, j = 1, k;
  if (d == 0)
    j = 5;
  for (k = 0; k < j; k++)
  {
    for (i = 0; i < 66; i++)
      printf("%d", num);
    printf("\n");
  }
}
/***********************************************************************************************************
**********************************************************************************************************/


void seq_callback(INT hDB, INT hseq, void *info)
{
  printf("odb... Trigger settings Changed\n");
}

/****************************************************************************
                        INITIALIZATION OF FRONTEND 
*****************************************************************************/

INT frontend_init()
{
	printf("Start 'Frontend INTI'\n");

    set_equipment_status(equipment[0].name, "Initializing...", "yellow");

	int i, status, csr, aad,bad;
	status = mvme_open(&myvme, 0);
    printf("%d\n", status );
	printf("Hello world\n");

	CAENVME_DeviceReset((int) myvme->info);
	CAENVME_SetOutputConf((int) myvme->info, cvOutput0,cvDirect,cvDirect,cvManualSW);
	CAENVME_SetOutputConf((int) myvme->info, cvOutput1,cvDirect,cvDirect,cvManualSW);
	CAENVME_SetOutputRegister((int) myvme->info, cvOut0Bit);
	CAENVME_SetOutputRegister((int) myvme->info, cvOut1Bit);

	CAENVME_SetInputConf((int) myvme->info, cvInput0, cvDirect, cvDirect);

	unsigned int InpDat;
	CAENVME_ReadRegister((int) myvme->info,0x0B,&InpDat);
	printf("Input Mux Register %4.4x\n", InpDat);

	CAENVME_ReadRegister((int) myvme->info, cvInputReg, &InpDat);
	printf("Input Register %4.4x\n", InpDat);

	// Setting the addressing mode 
  mvme_set_am(myvme, MVME_AM_A32);
	// Setting the Data mode
	mvme_set_dmode(myvme, MVME_DMODE_D32);  

/******************************************************************************
 *
 * INITIALIZE VARIOUS DEVICES
 *
 *****************************************************************************/

//-----------------CODE To Initialize Digitizer-------------------------------//

#if defined V1720_CODE
	//check the Digitizer
	printf("V1720\n");

  // Status and Firmware version of Digitizer 
  printf("Status of the Digitizer::::\n");
  v1720_Status(myvme,V1720_BASE_ADDR);

  bad = v1720_RegisterRead(myvme, V1720_BASE_ADDR, 0xF080);
  aad = v1720_RegisterRead(myvme, V1720_BASE_ADDR, 0xF084);
  printf("Serial Number of Digitizer:::: 0x%x%x\n",bad,aad);  

	csr = mvme_read_value(myvme, V1720_BASE_ADDR, V1720_ROC_FW_VER);
	printf("V1720 ROC(motherboard)Firmware Revision: 0x%8.8x\n", csr);

	// Software reset of the Digitizer
  v1720_Reset(myvme, V1720_BASE_ADDR);
    
  // Setting mode 1. It sets various parameters of the device.
  v1720_Setup(myvme, V1720_BASE_ADDR, 0x03);

  printf("v1720 INITIALIZED\n");
#endif

/****************Device Initialization Finished *************************/

int gemlatch=9;   // I do not understand the use of this function
// ss_sleep(5000);
// ss_sleep(5000);
// IsRunStopped=true;
// printf("run stopped\n");
// InpDat=regRead(myvme, V1720_BASE_ADDR,V1720_ACQUISITION_CONTROL);
// printf("%x\n",InpDat);
printf("End of Frontend Initialization\n");

// int  add;
// for(i=0;i<8;i++)
// {
//   add = 0x118C;
// csr = v1720_RegisterRead(myvme, V1720_BASE_ADDR, add);
// printf("V1720_ACQUISITION_CONTROL::::%8.8x\n",csr);  
//   add = add + 0x100;
// }

// add = 0x8124;
// csr = v1720_RegisterRead(myvme, V1720_BASE_ADDR, add);
// printf("V1720_ACQUISITION_CONTROL::::%8.8x\n",csr);  


// v1720_RegisterWrite(myvme, 0xCCCC0000, 0x811C, 0x1);
//mvme_write_value(myvme, 0xCCCC0000, 0x811C, 0x1);
// for(i=0;i<8;i++)
// {
// bad = v1720_RegisterRead(myvme, V1720_BASE_ADDR, 0xF080);
// aad = v1720_RegisterRead(myvme, V1720_BASE_ADDR, 0xF084);
// printf("SCRATCH::::%x\n", aad);  
// // mvme_write_value(myvme, 0xCCCC0000, 0x1200, 0x11110000);
// printf("SCRATCH::::%x\n", aad);  
// }
// aad = v1720_RegisterRead(myvme, V1720_BASE_ADDR, V1720_ACQUISITION_CONTROL);
aad = v1720_RegisterRead(myvme, V1720_BASE_ADDR, 0x811C);
printf("V1720_ACQUISITION_CONTROL::::%x\n", aad);  

for(i=1;i<9;i++)
{
    v1720_ChannelThresholdSet(myvme, V1720_BASE_ADDR, i, 0x050);
}

// for(i=1;i<9;i++)
// {
//   bad = v1720_ChannelGet(myvme, V1720_BASE_ADDR, i, V1720_CHANNEL_THRESHOLD);
//   printf("Channel Threshold::::%x \t %x\n", V1720_BASE_ADDR,bad);  
// }

//****************************TRIGGER SETTINGS*********************************
    char set_str[30];
    int size;

    TRIGGER_SETTINGS_STR(trigger_settings_str);
    sprintf(set_str, "/Equipment/Trigger/Settings");
    status = db_create_record(hDB, 0, set_str, strcomb(trigger_settings_str));
    status = db_find_key(hDB, 0, set_str, &hSet);
    if (status != DB_SUCCESS) {
        cm_msg(MINFO, "FE", "Key %s not found", set_str);
        return status;
    }

    /*Enabling Hot-Linking on settings of the Equipments*/
    size = sizeof(TRIGGER_SETTINGS);
    status = db_open_record(hDB, hSet, &ts, size, MODE_READ, seq_callback, NULL);
    if (status != DB_SUCCESS)
        return status;
  set_equipment_status(equipment[0].name, "Started", "green");

ss_sleep(5000);
return SUCCESS;
}

/****************************************************************************
                     FRONTEND EXIT FUNCTION
*****************************************************************************/
INT frontend_exit()
{
    return SUCCESS;
}

/****************************************************************************
                        BEGINING THE RUN 
*****************************************************************************/

INT begin_of_run(INT run_number, char *error)
{
    // prepare to clear latch later // comment made by earlier people
    
    printf("Start the Run\n");
    int i, csr, status, evtcnt;
    WORD threshold[32];
    mvme_set_am(myvme, MVME_AM_A32_ND); //set addressing mode to A32 non-privileged data
    mvme_set_dmode(myvme,MVME_DMODE_D32); // Setting data mode to 16 Bit

#if defined V1720_CODE
  //check the Digitizer
  printf("V1720\n");
  csr = mvme_read_value(myvme, V1720_BASE_ADDR, V1720_ROC_FW_VER);
  printf("V1720 ROC(motherboard)Firmware Revision: 0x%x\n", csr);

  // Software reset of the Digitizer
  v1720_Reset(myvme, V1720_BASE_ADDR);
    
  // Setting mode 1. It sets various parameters of the device.
  v1720_Setup(myvme, V1720_BASE_ADDR, 0x03);

  // Setting the buffer size to 1k
  // v1720_RegisterWrite(myvme, V1720_BASE_ADDR, V1720_BUFFER_ORGANIZATION,  0x0A);
  
  // Setting the threshold 
  // v1720_ChannelConfig(myvme,V1720_BASE_ADDR,0x2);
  for(i=1;i<9;i++)
  {
    //v1720_ChannelThresholdSet(myvme, V1720_BASE_ADDR, i, 0x0868);
    v1720_ChannelThresholdSet(myvme, V1720_BASE_ADDR, i, 0x50);
  }

  // Individual Settings for the Run:
  // Setting the post trigger value
  // v1720_RegisterWrite(myvme, V1720_BASE_ADDR, V1720_POST_TRIGGER_SETTING, 80);
  // SEtting the sample length
  // v1720_RegisterWrite(myvme, V1720_BASE_ADDR, V1720_CUSTOM_SIZE, 200);

   // v1720_RegisterWrite(myvme, V1720_BASE_ADDR, V1720_VME_CONTROL, 64);




  printf("V1720 IS Configured\n");

  ss_sleep(5000);
#endif
//******************Device Setup Done*********************************//

event_counter_for_interface = 0;
offsetbefore = 0;

printf("Waiting\n");
for (i=0;i<3;i++)
  {
    ss_sleep(1000);
    printf(".\n"); 
  }
printf("Starting now\n");
printf("End 'Begin of Run'\n");

IsRunStopped=false;
// CAENVME_ClearOutputRegister((int) myvme->info,cvOut0Bit);
// CAENVME_ClearOutputRegister((int) myvme->info,cvOut1Bit);

v1720_AcqCtl(myvme, V1720_BASE_ADDR, V1720_RUN_START);
// v1720_RegisterWrite(myvme, V1720_BASE_ADDR, V1720_ACQUISITION_CONTROL, 0x4);
csr = v1720_RegisterRead(myvme, V1720_BASE_ADDR, V1720_ACQUISITION_CONTROL);
printf("V1720_ACQUISITION_CONTROL::::%8.8x\n",csr);  
printf("Begin of Run is done \n");
ss_sleep(5000);
return SUCCESS;
}

/****************************************************************************
                        END OF RUN 
*****************************************************************************/

INT end_of_run(INT run_number, char *error)
{
    IsRunStopped=true;
    fIsIRQEnabled=false;                // user for disabling triggers

  

    // CAENVME_SetOutputRegister((int) myvme->info,cvOut0Bit);
    // CAENVME_SetOutputRegister((int) myvme->info,cvOut1Bit);
    v1720_AcqCtl(myvme, V1720_BASE_ADDR, V1720_RUN_STOP);
    printf("End of Run\n");
    return SUCCESS;
}

/****************************************************************************
                        PAUSE RUN 
*****************************************************************************/
// CHECK WHAT HAPPENS WHEN YOU REMOVE THIS FROM THE FRONTEND CODE
INT pause_run(INT run_number, char *error)
{
    return SUCCESS;
}

/****************************************************************************
                        RESUME RUN 
*****************************************************************************/
// CHECK WHAT HAPPENS WHEN YOU REMOVE THIS FROM THE FRONTEND CODE
INT resume_run(INT run_number, char *error)
{
    return SUCCESS;
}

/****************************************************************************
                        FRONTEND LOOP
*****************************************************************************/
INT frontend_loop()
{
    /* if frontend_call_loop is true, this routine gets called when
     the frontend is idle or once between every event */
    return SUCCESS;
}


/****************************************************************************
 *
 *
 * DATA READOUT ROUTINES
 *
 *
****************************************************************************/

/****************************************************************************
                    Triggered event Data Availability Check
****************************************************************************/
//Checking if the events are available to be stored in the data bank
//*****************POLLING EVENT********************************************/
//Computer/Midas asks vme device if the data is available or not.
INT poll_event(INT source, INT count, BOOL test)
/* Plling routine for events. Returns TRUE if event is available. If the test
 * equals TRUE, don't return. The test flag is used to time tht polling*/
{
    int i;
    int latch = 0;
    int InpDat;

    InpDat=0;
// Following logic can be used to stop polling after run stops
//    if(IsRunStopped == true) {return 0;}

    for (i=0; i < count+2; i++)
    {
      InpDat = v1720_RegisterRead(myvme, V1720_BASE_ADDR, V1720_VME_STATUS);
      // printf("i= %d \t EFO4:::0x%x\n",i, InpDat);
      InpDat = InpDat & 0x00000001;
      if (InpDat == 1)
      {
        // printf("data available\n");
        return 1;
      }
    }
    return 0;
}


//*****************INTERRUPT EVENT********************************************/ 
// Actually we do not use our frontend in the polling mode. I am just keeping 
// it for historic reason.
// TEST : WHAT HAPPENS WHEN I REMOVE THIS BLOCK OF CODE

INT interrupt_configure(INT cmd, INT source, POINTER_T adr)
{
  switch (cmd) {
  case CMD_INTERRUPT_ENABLE:
    break;
  case CMD_INTERRUPT_DISABLE:
    break;
  case CMD_INTERRUPT_ATTACH:
    break;
  case CMD_INTERRUPT_DETACH:
    break;
  }
  return SUCCESS;
}

/****************************************************************************
 *                      DATA READOUT
****************************************************************************/

INT read_trigger_event(char *pevent, INT off)
{
    DWORD *tdata, *ddata, *pdata1, *pdata2, *pdata3, *evdata, *pdata, *pdata4;
    int nentry, nextra, dummyevent, fevtcnt, temp;
    int dtemp,dentry,dextra,devtcnt;
    int size_of_evt,nu_of_evt,nuofcycles;
    int transffered_cycles=0;


    // Generating a pulse
    CAENVME_SetOutputRegister((int) myvme->info,cvOut0Bit);
    CAENVME_SetOutputRegister((int) myvme->info,cvOut1Bit);
    //initialize bank structure
    bk_init32(pevent);

//-----------------Take Data from Digitizer----------------------------//
#if defined V1720_CODE
v1720_AcqCtl(myvme, V1720_BASE_ADDR, V1720_RUN_STOP);

dentry = 0;
dextra = 0;
devtcnt = 0;
size_of_evt=0;
nu_of_evt=0;
transffered_cycles=0;

size_of_evt =  mvme_read_value(myvme, V1720_BASE_ADDR, 0x814C);
nu_of_evt =  mvme_read_value(myvme, V1720_BASE_ADDR, 0x812C);
nuofcycles = size_of_evt * nu_of_evt * 4;     //chaning from 32bit to byte


bk_create(pevent, "DG01", TID_DWORD, &ddata);      // Create data bank for Digitizer

//************************EVENT READ ONE BY ONE**********************//
// mvme_set_am(myvme, MVME_AM_A32_ND); //set addressing mode to A32 non-privileged data
// mvme_set_dmode(myvme,MVME_DMODE_D32); 
// uint32_t first;
// first=v1720_RegisterRead(myvme,V1720_BASE_ADDR,V1720_EVENT_READOUT_BUFFER);
// int n32w=first&0x0FFFFFFF;
// printf("%d\n", n32w );
//************************EVENT READ ONE BY ONE**********************//
// printf("--\n");
// printf("EventSize=%d\tNuOfevents=%d\tNuOfcycles=%d\tTran Bef Read be Zero=%d\n", size_of_evt,nu_of_evt,nuofcycles,transffered_cycles);

// int aaa = CAENVME_BLTReadCycle((long *)myvme->info, V1720_BASE_ADDR+V1720_EVENT_READOUT_BUFFER, ddata, nuofcycles,cvA32_U_BLT,0x04,&transffered_cycles);

// size_of_evt =  mvme_read_value(myvme, V1720_BASE_ADDR, 0x814C);
// nu_of_evt =  mvme_read_value(myvme, V1720_BASE_ADDR, 0x812C);

// if(size_of_evt!=0){
//   printf("EventSize=%d\tNuOfevents=%d\tNuOfcycles=%d\tActually Transferred=%d\n", size_of_evt,nu_of_evt,nuofcycles,transffered_cycles);
// aaa = CAENVME_BLTReadCycle((long *)myvme->info, V1720_BASE_ADDR+V1720_EVENT_READOUT_BUFFER, ddata, size_of_evt*4,cvA32_U_BLT,0x04,&transffered_cycles);
// }

// printf("%d\n", aaa);

// if(aaa!=0)
// {
// printf("PROBLEM %d\n",aaa);
// printf("EventSize=%d\tNuOfevents=%d\tNuOfcycles=%d\tActually Transferred=%d\n", size_of_evt,nu_of_evt,nuofcycles,transffered_cycles);
// // v1720_RegisterWrite(myvme, V1720_BASE_ADDR, 0xEF28, 1);
// printf("PROBLEM Solved\n");
// }

// size_of_evt =  mvme_read_value(myvme, V1720_BASE_ADDR, 0x814C);
// nu_of_evt =  mvme_read_value(myvme, V1720_BASE_ADDR, 0x812C);
// nuofcycles = size_of_evt * nu_of_evt * 4;     //chaning from 32bit to byte
// if(size_of_evt!=0)


// if(aaa==0)
// {
// printf("SUCCESS\n");
// ddata += transffered_cycles;
// }

dtemp = v1720_DataRead(myvme, V1720_BASE_ADDR, ddata, 8);
ddata += dtemp;
//dentry=dentry*4*1000;
//dentry = 256;
//dtemp = v1720_DataBlockRead(myvme, V1720_BASE_ADDR, ddata, &dentry);

bk_close(pevent,ddata);
#endif

//-----------------Take Data of TDC'S----------------------------------//
// #if defined V1290_CODE
//     nentry = 0;
//     nextra = 0;
//     fevtcnt = 0;

// bk_create(pevent, "TDC", TID_DWORD, &tdata);       // Create data bank for TDC

// v1290_EventRead(myvme, V1290N_BASE_ADDR, tdata, &nentry);
// tdata += nentry;

// // understand this part of the data taking code
// fevtcnt = v1290_EvtCounter(myvme, V1290N_BASE_ADDR);
// temp = 0xaa000000 + (fevtcnt & 0xffffff);
// *tdata++ = temp;

//  check if there are any additional events in TDC, If they are available,
//  *  then put the flag and number of extra events 

// while(v1290_DataReady(myvme, V1290N_BASE_ADDR))
// {
//     nextra++;
    // v1290_EventRead(myvme, V1290N_BASE_ADDR, tdata, &dummyevent);
// }
// // THIS part is also confusing
// if (nextra>0)
// {
//     if(evtdebug)
//  //     printf("ERROR Multiple Events in 1290: N words in FIFO = %d, Event Num = %d\n",nextra,fevtcnt); 
//     temp = 0xdcfe0000 + (nextra & 0x7ff);
//     *tdata++ = temp;
//     temp = 0xbad0cafe;
//     *tdata++ = temp;
// }

// bk_close(pevent, tdata);

// #endif


//-----------------Bank for DAQ event counter----------------------------------//
bk_create(pevent, "EVNT", TID_DWORD, &evdata);
*evdata++ = event_counter_for_interface;
bk_close(pevent, evdata);

event_counter_for_interface++;

    // Generating a pulse
    CAENVME_SetOutputRegister((int) myvme->info,cvOut0Bit);
    CAENVME_SetOutputRegister((int) myvme->info,cvOut1Bit);
 
v1720_AcqCtl(myvme, V1720_BASE_ADDR, V1720_RUN_START);
return bk_size(pevent);
}


/*************************************************************************************
 * DON'T KNOW EXACTLY
************************************************************************************/

int eventcounteroffset(int event_counter_for_interface, int gemlatch)
{
    return ( ( event_counter_for_interface & 0x7 ) - ((gemlatch & 0xf)>>1) ) & 0x7;
}
