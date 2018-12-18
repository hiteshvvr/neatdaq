/***********************************************************************************************
 *
 * Name: frontend.c
 * Created by: Modified from the template of Dr. Stefan Ritt and Prof. Guy Ron
 *
 * Contents: Frontend for the readout of the experintal data of Neon trapping
 * experiment at SARAF
 *
 * $Id: frontend.c 0001 24-10-17    hitesh.rahangdale@mail.huji.ac.il
 *
 * ******************************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "midas.h"
// #include "mcstd.h"

#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <strings.h>

#include <CAENVMElib.h>
#include <CAENVMEtypes.h>
#include "experim.h"
#include "frontend.h"

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


void seq_callback(INT hDB, INT hseq, void *info)
{
  printf("odb ... trigger settings touched\n");
}
/****************************************************************************
                        INITIALIZATION OF FRONTEND 
*****************************************************************************/
INT frontend_init()
{
	printf("Initialize Frontend::'\n");

  set_equipment_status(equipment[0].name, "Initializing...","yellow");
  set_equipment_status(equipment[1].name, "Initializing...","yellow");
	int i, status, csr;
	status = mvme_open(&myvme, 0);

	printf("Hello world\n");

	CAENVME_DeviceReset((int) myvme->info);

	CAENVME_SetOutputConf((int) myvme->info, cvOutput0,cvDirect,cvDirect,cvManualSW);
	CAENVME_SetOutputConf((int) myvme->info, cvOutput1,cvDirect,cvDirect,cvManualSW);

  CAENVME_SetOutputRegister((int) myvme->info, cvOut1Bit);                    // Although using it now its used End of transmission

	CAENVME_SetInputConf((int) myvme->info, cvInput0, cvDirect, cvDirect);

	unsigned int InpDat;
	CAENVME_ReadRegister((int) myvme->info,0x0B,&InpDat);
	printf("Input Mux Register %4.4x\n", InpDat);

	CAENVME_ReadRegister((int) myvme->info, cvInputReg, &InpDat);
	printf("Input Register %4.4x\n", InpDat);

	mvme_set_am(myvme, MVME_AM_A32_ND);                // Setting the addressing mode to 32 Bit non privileged
	mvme_set_dmode(myvme, MVME_DMODE_D16);             // Setting the Data mode to 16 Bit

/******************************************************************************
 *
 * INITIALIZE VARIOUS DEVICES
 *
 *****************************************************************************/

//-----------------CODE To Initialize TDC'S----------------------------------//

#if defined V1290_CODE
	//check the TDC's
	printf("v1290N\n");
  set_equipment_status(equipment[0].name, "Setting TDC","yellow");

  csr = mvme_read_value(myvme, V1290N_BASE_ADDR, 0x100c);
  printf("IVR: 0x%x\n", csr);
	csr = mvme_read_value(myvme, V1290N_BASE_ADDR, V1290_FIRM_REV_RO);
	printf("Firmware Revision: 0x%x\n", csr);

	v1290_SoftClear(myvme, V1290N_BASE_ADDR);                            // Software reset of the TDC
  // v1290_Setup(myvme, V1290N_BASE_ADDR, 1);                             // Setting mode 1. It sets various parameters of the device.
  v1290_LEResolutionSet(myvme, V1290N_BASE_ADDR, LE_RESOLUTION_25);    // Setting the resoultion of TDC to 25 ps

// OFFSET                                                            // for exact rules check manuals.
  /**
  The window offset and width follow the following constraint so follow them
  window width + window offset ≤ 40 clock cycles = 1000 ns

  Firmware uses something called two's complement of signed integer
  where max 12 bit signed integer is represented by 2048 and minimum
  by 4096.
  **/
  int off = -5000 ;                                                        // offset is between -51000 to +1000 ns in multiple of 25 ns
  off=off/25;                                                              // Put value divisible by 25 (in the range)
  off=4096+off;                                                            // taking lower multiple of 25 ns
  off=61440+off;
  printf("Setting OFFSET: 0x%04x\t%d\n", off,off);
  v1290_OffsetSet(myvme, V1290N_BASE_ADDR, off);

  // WIDTH  
  /*
  The window offset and width follow the following constraint so follow them
  window width + window offset ≤ 40 clock cycles = 1000 ns
  */
  int width=5000;                                                   // Width value is between 25 ns to 52200 ns in multiple of 25 ns
  width = width/25;                                                 // Put value divisible by 25 (in the range)
   
  // width = 0x00EF;
  printf("Setting Width: 0x%04x\t%d\n", width,width );
  v1290_WidthSet(myvme, V1290N_BASE_ADDR, width);              
   
    //  printf("Received OS :code: 0x%04x  0x%08x\n", V1290_WINDOW_OFFSET_WO, value);
  
  // v1290_OffsetSet(myvme, V1290N_BASE_ADDR, 0xFF10);
  // v1290_WidthSet(myvme, V1290N_BASE_ADDR, 0x00EE);     
  // v1290_Status(myvme,V1290N_BASE_ADDR);

#endif

#if defined V2495_CODE
  //check the TDC's
  printf("Setting up FPGA\n");

  // v2495_FirmwareRevision(myvme,V2495_BASE_ADDR);
  v2495_EarlyWindow(myvme,V2495_BASE_ADDR, 0x0000);
  v2495_LateWindow(myvme,V2495_BASE_ADDR, 0x0000);

#endif

//-----------------CODE To Initialize CFD's--------------------------------//

#if defined V812_CODE
// Setting Threshholds for all 16 channels of 812 model, 0 does something weird, 255 (or 0xFF) is -250ms 
printf("Setting V812 Threshholds, Width, Deadtime\n");
for (i=0; i< 16; i++)
{
	v812_SetTh(myvme, V812_BASE_ADDR,0,150);
}

// Setting the Width of CFD's 
// 0x00 sets output width of 15ns and 0xFF sets width of 250ns
v812_SetWidth(myvme, V812_BASE_ADDR, 0, 0xAF);   // from 0-7 channels
v812_SetWidth(myvme, V812_BASE_ADDR, 1, 0xAF);   // from 8-15 channels

// Setting deadtime for all the channels 
// 0 sets deadtime of 150ns and 255 sets 2us
v812_SetDeadTime(myvme, V812_BASE_ADDR, 0, 255);   // from 0-7 channels
v812_SetDeadTime(myvme, V812_BASE_ADDR, 1, 255);   // from 8-15 channels

// Open the channel to take data
// 0x01 opens only channel 0 and 0xFF opens all 16 channels
v812_SetInhibit(myvme,V812_BASE_ADDR,0xFF);

printf("V812 Fixed Code: 0x%x\n", v812_Read16(myvme, V812_BASE_ADDR, V812_FIXED_CODE));
printf("V812 Version Serial: 0x%x\n", v812_Read16(myvme, V812_BASE_ADDR, V812_VER_SERIAL));

#endif
/****************Device Initialization Finished *************************/


int gemlatch=9;   // I do not understand the use of this function
IsRunStopped=true;
printf("End of Frontend Initialization\n");

///////////////////////////// NEW FUNCTIONALITIES//////////////////////////////

//****************************TRIGGER SETTINGS*********************************

set_equipment_status(equipment[0].name, "Started", "green");
set_equipment_status(equipment[1].name, "Started", "green");

// trigger_settings_str="hitesh";
char set_str[30];
int size;

TRIGGER_SETTINGS_STR(trigger_settings_str);
sprintf(set_str, "/Equipment/Trigger/Settings");
status = db_create_record(hDB,0,set_str,strcomb(trigger_settings_str));
status = db_find_key(hDB, 0, set_str, &hSet);
if (status != DB_SUCCESS){
  cm_msg(MINFO,"FE","Key %s not found", set_str);
  return status;
}
/*Enabling Hot-Linking on settings of the Equipments*/
size = sizeof(TRIGGER_SETTINGS);
status=db_open_record(hDB, hSet, &ts, size, MODE_READ, seq_callback, NULL);
if (status!= DB_SUCCESS) // printf("Hitesh2\n");
  return status;

//****************************PERIODIC SETTINGS*********************************
char per_str[30];

PERIODIC_DISPLAY_STR(periodic_display_str);
sprintf(per_str, "/Equipment/Periodic/Display");

status = db_create_record(hDB,0,per_str,strcomb(periodic_display_str));
status = db_find_key(hDB, 0, per_str, &hper);
if (status != DB_SUCCESS){
//   printf("Hitesh\n");
//   cm_msg(MINFO,"FE","Key %s not found", per_str);
  return status;
}
// // /*Enabling Hot-Linking on settings of the Equipments*/
size = sizeof(PERIODIC_DISPLAY);
status=db_open_record(hDB, hper, &pd, size, MODE_WRITE, NULL, NULL);
if (status!= DB_SUCCESS) // printf("Hitesh2\n");
  return status;


/////////////////////SOME TRIALS///////////////////////////////////////////////////////////
int datam=0;
char trig[10];
// trig="..........";
datam = v2495_Read16(myvme, V2495_BASE_ADDR, 0x1800);
printf("%d\n", datam);
sprintf(trig, "%d", datam);

/**********************FiRMWARE REVISION OF FPGA****************************************/
// int  v2495_FirmwareRevision(myvme,V2495_BASE_ADDR);
pd.v2495.fpgarevno = v2495_FirmwareRevision(myvme,V2495_BASE_ADDR);


// printf("staus====%d\n", status );
// printf("SUCCESS::::%d\n", SUCCESS);
// printf("DB_SUCCESS:::::%d\n", DB_SUCCESS);
// printf("DB_SUCCESS:::::%d\n", DB_SUCCESS);


// set_equipment_status(equipment[1].name, "OK", "green");
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
                        GETTING USER PARAMETER
*****************************************************************************/
INT get_user_parameters()
{
    INT Status,size;
    size = sizeof(TRIGGER_SETTINGS);
    Status = db_get_record (hDB, hSet, &ts, &size, 0); 
   
    if (Status != DB_SUCCESS)
    {
      cm_msg(MERROR, "get_exp_settings","Cannot retrive trigger Settings Record (size of ts = %d)", size);
     return Status;
    }

    if (Status == DB_SUCCESS)
    {
      //Setting FPGA
      printf("FPGA Early Window %d\n", ts.v2495.fpgaearlywindow);
      v2495_EarlyWindow(myvme,V2495_BASE_ADDR, ts.v2495.fpgaearlywindow);
      printf("FPGA Late Window %d\n", ts.v2495.fpgaearlywindow);
      v2495_LateWindow(myvme,V2495_BASE_ADDR, ts.v2495.fpgalatewindow);

      //Setting up TDC

      int off = ts.v1290.tdcoffset ;            // offset is between -51000 to +1000 ns in multiple of 25 ns
      if(ts.v1290.tdcoffset>1000) 
      {
        printf("You have entered wrong value for TDC. Offset range is from -51000 to 1000 ns\n");
        exit(0);
      }
      off=off/25;                            // Put value divisible by 25 (in the range)
      off=4096+off;                             // taking lower multiple of 25 ns
      off=61440+off;
      printf("Setting OFFSET: 0x%04x\t%d\n", off,off);
      // v1290_OffsetSet(myvme, V1290N_BASE_ADDR, off);

      int width=ts.v1290.tdcwidth;             // Width value is between 25 ns to 52200 ns in multiple of 25 ns
      width = width/25;                        // Put value divisible by 25 (in the range)
      printf("Setting Width: 0x%04x\t%d\n", width,width );
      // v1290_WidthSet(myvme, V1290N_BASE_ADDR, width);   
    }

    return SUCCESS;
}


/****************************************************************************
                        BEGINING THE RUN 
*****************************************************************************/

INT begin_of_run(INT run_number, char *error)
{

  INT Status,size;
  Status = get_user_parameters();

    // prepare to clear latch later // comment made by earlier people


  /* read Triggger settings */

    
    printf("Start the Run\n");
    int i, csr, status, evtcnt,temp;
    WORD threshold[32];

    mvme_set_am(myvme, MVME_AM_A32_ND);                // Setting the addressing mode to 32 Bit non privileged
    mvme_set_dmode(myvme, MVME_DMODE_D16);             // Setting the Data mode to 16 Bit


//******************Setting up Devices**********************************//
//
//
//******TDC
//
#if defined V1290_CODE
  printf("v1290N\n");
  csr = mvme_read_value(myvme, V1290N_BASE_ADDR, V1290_FIRM_REV_RO);    // Reading Firmware Revision
  printf("Firmware revision: 0x%x\n", csr);

  // Reset
  // v1290_SoftClear(myvme, V1290N_BASE_ADDR);
  evtcnt = v1290_EvtCounter(myvme, V1290N_BASE_ADDR);                   //Get event count
  printf("1290 Event Counter: %d\n",evtcnt);
  
  // v1290_Setup(myvme, V1290N_BASE_ADDR, 1);                           // Set mode 1
  // v1290_Status(myvme,V1290N_BASE_ADDR);

  v1290_LEResolutionSet(myvme, V1290N_BASE_ADDR, LE_RESOLUTION_25);     // Set Resolution to 25 ps
  evtcnt = v1290_EvtCounter(myvme, V1290N_BASE_ADDR);                   //Get event count
  printf("1290 Event Counter: %d\n",evtcnt);
  v1290_SoftClear(myvme, V1290N_BASE_ADDR);
#endif
// Other Device if available.

//******************Device setup Done*********************************//

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

CAENVME_SetOutputRegister((int) myvme->info,cvOut0Bit);
CAENVME_ClearOutputRegister((int) myvme->info,cvOut0Bit);
CAENVME_ClearOutputRegister((int) myvme->info,cvOut1Bit);

return SUCCESS;
}

/****************************************************************************
                        END OF RUN 
*****************************************************************************/

INT end_of_run(INT run_number, char *error)
{
    #if defined V2495_CODE
    //check the TDC's
    printf("Restoring FPGA so no trigger is accepted\n");

    // v2495_FirmwareRevision(myvme,V2495_BASE_ADDR);
    v2495_EarlyWindow(myvme,V2495_BASE_ADDR, 0x0000);
    v2495_LateWindow(myvme,V2495_BASE_ADDR, 0x0000);

    #endif

    IsRunStopped=true;
    fIsIRQEnabled=false;                                                      // used for disabling triggers

    CAENVME_SetOutputRegister((int) myvme->info,cvOut1Bit);

    printf("End of Run\n");
    return SUCCESS;
}

/****************************************************************************
                        PAUSE RUN 
*****************************************************************************/
// CHECK WHAT HAPPENS WHEN YOU REMOVE THIS FROM THE FRONTEND CODE
INT pause_run(INT run_number, char *error)
{
    v2495_EarlyWindow(myvme,V2495_BASE_ADDR, ts.v2495.fpgaearlywindow);
    v2495_LateWindow(myvme,V2495_BASE_ADDR, ts.v2495.fpgalatewindow);
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
    // if(IsRunStopped == true) {return 0;}                                  // Check if uncommenting this line helps
                                                                             // Specially with many dots problem.
    for (i=0; i < count; i++)
    {
        InpDat=v1290_DataReady(myvme, V1290N_BASE_ADDR);
        if (InpDat == 1) {return 1;}
    }
    return 0;
    // return 1;

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
    DWORD *tdata, *pdata1, *pdata2, *pdata3, *evdata, *pdata, *pdata4;
    int nentry, nextra, dummyevent, fevtcnt, temp;
    // Generating a pulse
    // CAENVME_SetOutputRegister((int) myvme->info,cvOut0Bit);
    // CAENVME_SetOutputRegister((int) myvme->info,cvOut1Bit);
    //initialize bank structure
    bk_init32(pevent);

//-----------------Take Data of TDC'S----------------------------------//
#if defined V1290_CODE
    nentry = 0;
    nextra = 0;
    fevtcnt = 0;

bk_create(pevent, "TDC0", TID_DWORD, &tdata);                            // Create data bank for TDC

v1290_DataRead(myvme, V1290N_BASE_ADDR, tdata, &nentry);
tdata += nentry;

// understand this part of the data taking code
fevtcnt = v1290_EvtCounter(myvme, V1290N_BASE_ADDR);
temp = 0xaa000000 + (fevtcnt & 0xffffff);
*tdata++ = temp;

/* check if there are any additional events in TDC, If they are available,
 *  then put the flag and number of extra events */

while(v1290_DataReady(myvme, V1290N_BASE_ADDR))
{
    nextra++;
    v1290_EventRead(myvme, V1290N_BASE_ADDR, tdata, &dummyevent);
}

// THIS part is also confusing
if (nextra>0)
{
    if(evtdebug)
 //     printf("ERROR Multiple Events in 1290: N words in FIFO = %d, Event Num = %d\n",nextra,fevtcnt); 
    temp = 0xdcfe0000 + (nextra & 0x7ff);
    *tdata++ = temp;
    temp = 0xbad0cafe;
    *tdata++ = temp;
}

bk_close(pevent, tdata);

#endif


//-----------------Bank for DAQ event counter----------------------------------//
bk_create(pevent, "EVNT", TID_DWORD, &evdata);
*evdata++ = event_counter_for_interface;
bk_close(pevent, evdata);

event_counter_for_interface++;

    // Generating a pulse
  CAENVME_SetOutputRegister((int) myvme->info,cvOut0Bit);
  CAENVME_ClearOutputRegister((int) myvme->info,cvOut0Bit);
 
return bk_size(pevent);
}


INT read_periodic_event(char *levent, INT off)
{
   float *pdata;
   int a;
   int trig, elec, ion;
   /* init bank structure */
   bk_init(levent);
   /* create SCLR bank */
   bk_create(levent, "PRDC", TID_FLOAT, (void **)&pdata);
   /* following code "simulates" some values */
   // for (a = 0; a < 4; a++)
      // *pdata++ = 100*sin(M_PI*time(NULL)/60+a/2.0);
   *pdata++ = 100;


   trig = v2495_GetValidEventRate(myvme, V2495_BASE_ADDR);
   elec = v2495_GetElecRate(myvme, V2495_BASE_ADDR);
   ion = v2495_GetIonRate(myvme, V2495_BASE_ADDR);

   // trig = trig*rand()/100000;
   // elec = elec*rand()/100000;
   // ion = ion*rand()/100000;

   // db_set_value(hDB,0,"/Equipment/Trigger/Settings/v2495/FPGAValidEvents", &a, sizeof(a), 1 , TID_INT);
   //********************PUTTING VALUES IN ODB TREE****************************** 
   pd.v2495.trigfreq = abs(trig);
   pd.v2495.elecfreq = abs(elec);
   pd.v2495.ionfreq = abs(ion);




   bk_close(levent, pdata);
   return bk_size(levent);

  //   INT Status=1,size;
  //   int istatus, val=314;
  //   // size = sizeof(PERIODIC_SETTINGS);
  //   // Status = db_get_record (hDB, hSet, &ps, &size, 0); 
   
  //   // if (Status != DB_SUCCESS)
  //   // {
  //   //   cm_msg(MERROR, "get_exp_settings","Cannot retrive trigger Settings Record (size of ts = %d)", size);
  //   // }

  //   if (Status == DB_SUCCESS)
  //   {
  //     // ps.v2495.fpgaearlywindow = val;
  //   }
  // // istatus = db_set_value(hDB, hSet, const char *key_name, const void *data, INT size, INT num_values, DWORD type);

  //    return Status;

}


/*************************************************************************************
 * DON'T KNOW EXACTLY
************************************************************************************/

int eventcounteroffset(int event_counter_for_interface, int gemlatch)
{
    return ( ( event_counter_for_interface & 0x7 ) - ((gemlatch & 0xf)>>1) ) & 0x7;
}
