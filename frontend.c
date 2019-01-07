/***********************************************************************************************
 *
 * Name: frontend.c
 * Created by: Modified from the template of Dr. Stefan Ritt and Prof. Guy Ron
 *
 * Contents: Frontend for the readout of the experintal data of Neon trapping
 * experiment at SARAF
 *
 * $Id: frontend.c 0001 24-10-17    hitesh.rahangdale@mail.huji.ac.il
 * $Id: frontend.c 0002 24-11-18    hitesh.rahangdale@mail.huji.ac.il
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
#include <CAENComm.h>
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
int datmode, addmode;


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

  set_equipment_status(equipment[0].name, "Initializing...", "yellow");
  set_equipment_status(equipment[1].name, "Initializing...", "yellow");

  int i, status, csr;
  status = mvme_open(&myvme, 0);
  printf(" Status::: %d\n Hello world\n",status);
  
  CAENVME_DeviceReset((int) myvme->info);

  CAENVME_SetOutputConf((int) myvme->info, cvOutput0, cvDirect, cvDirect, cvManualSW);
  CAENVME_SetOutputConf((int) myvme->info, cvOutput1, cvDirect, cvDirect, cvManualSW);
  CAENVME_SetOutputRegister((int) myvme->info, cvOut1Bit);
  // Although using output register now its used for End of transmission
  CAENVME_SetInputConf((int) myvme->info, cvInput0, cvDirect, cvDirect);

  unsigned int InpDat;
  CAENVME_ReadRegister((int) myvme->info, 0x0B, &InpDat);
  printf("Input Mux Register %4.4x\n", InpDat);

  CAENVME_ReadRegister((int) myvme->info, cvInputReg, &InpDat);
  printf("Input Register %4.4x\n", InpDat);

  mvme_set_am(myvme, MVME_AM_A32_ND);                // Setting the addressing mode to 32 Bit non privileged
  mvme_set_dmode(myvme, MVME_DMODE_D16);             // Setting the Data mode to 16 Bit
  printf("I am here\n");
  /******************************************************************************
   *
   * INITIALIZE VARIOUS DEVICES
   *
   *****************************************************************************/

//-----------------CODE To Initialize TDC'S----------------------------------//

#if defined V1290_CODE
  //check the TDC's
  printf("v1290N\n");
  set_equipment_status(equipment[0].name, "Setting TDC", "yellow");

  csr = mvme_read_value(myvme, V1290N_BASE_ADDR, 0x100c);
  printf("IVR: 0x%x\n", csr);
  csr = mvme_read_value(myvme, V1290N_BASE_ADDR, V1290_FIRM_REV_RO);
  printf("Firmware Revision: 0x%x\n", csr);

  v1290_SoftClear(myvme, V1290N_BASE_ADDR);                            // Software reset of the TDC
  v1290_Setup(myvme, V1290N_BASE_ADDR, 1);                          // Setting mode 1. It sets various parameters of the device.
  v1290_LEResolutionSet(myvme, V1290N_BASE_ADDR, LE_RESOLUTION_25);    // Setting the resoultion of TDC to 25 ps

  // OFFSET                                                            // for exact rules check manuals.
  /**
  The window offset and width follow the following constraint so follow them
  window width + window offset ≤ 40 clock cycles = 1000 ns

  Firmware uses something called two's complement of signed integer
  where max 12 bit signed integer is represented by 2048 and minimum
  by 4096.
  **/
//---------
  // int off = -5000 ;                                                        // offset is between -51000 to +1000 ns in multiple of 25 ns
  // off=off/25;                                                              // Put value divisible by 25 (in the range)
  // off=4096+off;                                                            // taking lower multiple of 25 ns
  // off=61440+off;
  // printf("Setting OFFSET: 0x%04x\t%d\n", off,off);
  // v1290_OffsetSet(myvme, V1290N_BASE_ADDR, off);

  // // WIDTH
  // /*
  // The window offset and width follow the following constraint so follow them
  // window width + window offset ≤ 40 clock cycles = 1000 ns
  // */
  // int width=5000;                                                   // Width value is between 25 ns to 52200 ns in multiple of 25 ns
  // width = width/25;                                                 // Put value divisible by 25 (in the range)

  // // width = 0x00EF;
  // printf("Setting Width: 0x%04x\t%d\n", width,width );
  // v1290_WidthSet(myvme, V1290N_BASE_ADDR, width);

//---------

  //  printf("Received OS :code: 0x%04x  0x%08x\n", V1290_WINDOW_OFFSET_WO, value);

  // v1290_OffsetSet(myvme, V1290N_BASE_ADDR, 0xFF10);
  // v1290_WidthSet(myvme, V1290N_BASE_ADDR, 0x00EE);
  v1290_Status(myvme, V1290N_BASE_ADDR);

#endif

//-----------------CODE To Initialize FPGA--------------------------------//

#if defined V2495_CODE
  //check the TDC's
  printf("Setting up FPGA\n");

  // v2495_FirmwareRevision(myvme,V2495_BASE_ADDR);
  v2495_EarlyWindow(myvme, V2495_BASE_ADDR, 0x0000);
  v2495_LateWindow(myvme, V2495_BASE_ADDR, 0x0000);

#endif


//-----------------CODE To Initialize Digitizer--------------------------------//
#if defined V1720_CODE
    mvme_get_dmode(myvme, &datmode);
    mvme_set_dmode(myvme, MVME_DMODE_D32);

    printf("Setting Up Digitizer\n");
    // Status and Firmware version of Digitizer
    //check the Digitizer
    printf("Status of the Digitizer::::\n");
    v1720_Status(myvme, V1720_BASE_ADDR);

    int ser_h = v1720_RegisterRead(myvme, V1720_BASE_ADDR, 0xF080);
    int ser_l = v1720_RegisterRead(myvme, V1720_BASE_ADDR, 0xF084);
    printf("Serial Number of Digitizer:::: 0x%x%x\n", ser_l, ser_h);

    csr = mvme_read_value(myvme, V1720_BASE_ADDR, V1720_ROC_FW_VER);
    printf("V1720 ROC(motherboard)Firmware Revision: 0x%8.8x\n", csr);

    // Software reset of the Digitizer
    v1720_Reset(myvme, V1720_BASE_ADDR);

    // Setting mode 1. It sets various parameters of the device.
    v1720_Setup(myvme, V1720_BASE_ADDR, 0x03);

    printf("v1720 INITIALIZED\n");

    int frontp = v1720_RegisterRead(myvme, V1720_BASE_ADDR, 0x811C);
    if (frontp & 0x00000000 == 0)
        printf("Front Panel NIM%x\n");

    ss_sleep(5000);
    printf("I Successfully setup the Digitizer\n");
    mvme_set_dmode(myvme, datmode);
    ss_sleep(5000);
#endif



//-----------------CODE To Initialize CFD's--------------------------------//

#if defined V812_CODE
// Setting Threshholds for all 16 channels of 812 model, 0 does something weird, 255 (or 0xFF) is -250ms
  printf("Setting V812 Threshholds, Width, Deadtime\n");
  for (i = 0; i < 16; i++)
  {
    v812_SetTh(myvme, V812_BASE_ADDR, 0, 150);
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
  v812_SetInhibit(myvme, V812_BASE_ADDR, 0xFF);

  printf("V812 Fixed Code: 0x%x\n", v812_Read16(myvme, V812_BASE_ADDR, V812_FIXED_CODE));
  printf("V812 Version Serial: 0x%x\n", v812_Read16(myvme, V812_BASE_ADDR, V812_VER_SERIAL));

#endif
  /****************Device Initialization Finished *************************/


  int gemlatch = 9; // I do not understand the use of this function
  IsRunStopped = true;
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
  status = db_create_record(hDB, 0, set_str, strcomb(trigger_settings_str));
  status = db_find_key(hDB, 0, set_str, &hSet);
  if (status != DB_SUCCESS) {
    cm_msg(MINFO, "FE", "Key %s not found", set_str);
    return status;
  }
  /*Enabling Hot-Linking on settings of the Equipments*/
  size = sizeof(TRIGGER_SETTINGS);
  status = db_open_record(hDB, hSet, &ts, size, MODE_READ, seq_callback, NULL);
  if (status != DB_SUCCESS) // printf("Hitesh2\n");
    return status;

//****************************PERIODIC SETTINGS*********************************
  char per_str[30];

  PERIODIC_DISPLAY_STR(periodic_display_str);
  sprintf(per_str, "/Equipment/Periodic/Display");

  status = db_create_record(hDB, 0, per_str, strcomb(periodic_display_str));
  status = db_find_key(hDB, 0, per_str, &hper);
  if (status != DB_SUCCESS) {
//   printf("Hitesh\n");
//   cm_msg(MINFO,"FE","Key %s not found", per_str);
    return status;
  }
// // /*Enabling Hot-Linking on settings of the Equipments*/
  size = sizeof(PERIODIC_DISPLAY);
  status = db_open_record(hDB, hper, &pd, size, MODE_WRITE, NULL, NULL);
  if (status != DB_SUCCESS) // printf("Hitesh2\n");
    return status;


/////////////////////SOME TRIALS///////////////////////////////////////////////////////////
  int datam = 0;
  char trig[10];
// trig="..........";
  datam = v2495_Read16(myvme, V2495_BASE_ADDR, 0x1800);
  printf("OUR FPGA FRONTEND:%d\n", datam);
  sprintf(trig, "%d", datam);

  /**********************FiRMWARE REVISION OF FPGA****************************************/
// int  v2495_FirmwareRevision(myvme,V2495_BASE_ADDR);
  pd.v2495.fpgarevno = v2495_FirmwareRevision(myvme, V2495_BASE_ADDR);


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
  INT Status, size;
  size = sizeof(TRIGGER_SETTINGS);
  Status = db_get_record (hDB, hSet, &ts, &size, 0);

  if (Status != DB_SUCCESS)
  {
    cm_msg(MERROR, "get_exp_settings", "Cannot retrive trigger Settings Record (size of ts = %d)", size);
    return Status;
  }

  if (Status == DB_SUCCESS)
  {
    //Setting FPGA
    printf("FPGA Early Window %d\n", ts.v2495.fpgaearlywindow);
    v2495_EarlyWindow(myvme, V2495_BASE_ADDR, ts.v2495.fpgaearlywindow);
    printf("FPGA Late Window %d\n", ts.v2495.fpgaearlywindow);
    v2495_LateWindow(myvme, V2495_BASE_ADDR, ts.v2495.fpgalatewindow);

    //Setting up TDC
//--------------
    // int off = ts.v1290.tdcoffset ;            // offset is between -51000 to +1000 ns in multiple of 25 ns
    // if(ts.v1290.tdcoffset>1000)
    // {
    //   printf("You have entered wrong value for TDC. Offset range is from -51000 to 1000 ns\n");
    //   exit(0);
    // }
    // off=off/25;                            // Put value divisible by 25 (in the range)
    // off=4096+off;                             // taking lower multiple of 25 ns
    // off=61440+off;
    // printf("Setting OFFSET: 0x%04x\t%d\n", off,off);
    // // v1290_OffsetSet(myvme, V1290N_BASE_ADDR, off);

    // int width=ts.v1290.tdcwidth;             // Width value is between 25 ns to 52200 ns in multiple of 25 ns
    // width = width/25;                        // Put value divisible by 25 (in the range)
    // printf("Setting Width: 0x%04x\t%d\n", width,width );
    // // v1290_WidthSet(myvme, V1290N_BASE_ADDR, width);
//--------------
  }

  return SUCCESS;
}

/****************************************************************************
                       SETUP DIGITIZER 
*****************************************************************************/

INT setupdigitizer()
{
    int Status, size;
    size = sizeof(TRIGGER_SETTINGS);
    Status = db_get_record(hDB, hSet, &ts, &size, 0);

    if (Status != DB_SUCCESS)
    {
        printf("\n\n\n");
        printf("\n\n\n");
        cm_msg(MERROR, "get_exp_settings", "Cannot retrive trigger settings REcord(Size of Trigger SEttings = %d)", size);
        printf("Setup Digitizer Failed\n");
        printf("\n\n\n");
        return Status;
    }

    if(Status == DB_SUCCESS)
    {
    /************************************
     * BOARD SETTINGS
     * *********************************/

    int buffersize = ts.v1720.boardsettings.bufferorganization;
    int acqcontrol = ts.v1720.boardsettings.acquisitioncontrol;

    if(buffersize!= 0xA) { ss_sleep(5000);
        printf("Be Careful you are putting different buffer size than regular 1k Buffer\n");
    }
    v1720_RegisterWrite(myvme,V1720_BASE_ADDR,V1720_BUFFER_ORGANIZATION,buffersize);
    printf("Be Careful you are putting different buffer size than regular 1k Buffer\n");

    int trigsource = ts.v1720.boardsettings.triggersource[1];
    /* if(trigsource == 1) */
    v1720_RegisterWrite(myvme,V1720_BASE_ADDR,V1720_TRIG_SRCE_EN_MASK,0x40000000);
    printf("Be Careful you are putting different buffer size than regular 1k Buffer\n");

    int postsample = ts.v1720.boardsettings.posttriggersamples;
    if(postsample <0 || postsample >> 500)
    {
        printf("Input proper post sample number\n");
        exit(0);
    }
    v1720_RegisterWrite(myvme,V1720_BASE_ADDR,V1720_POST_TRIGGER_SETTING,postsample);


    /************************************
     * CHANNEL SETTINGS
     * *********************************/

    int i, val;

    // Enable Channel
    int chanmask=0;

    for(i = 0; i<8; i++)
    {
        val = ts.v1720.channelsettings.enable[i];
        chanmask = chanmask + val * 2^i;
    }
    ss_sleep(5000);
    printf("\n\n \nYou are Enabling %d Channels \n",chanmask);

    if(ts.v1720.channelsettings.enable[8] == 0)
        v1720_RegisterWrite(myvme,V1720_BASE_ADDR,V1720_CHANNEL_EN_MASK ,0xFF);
    else
        v1720_RegisterWrite(myvme,V1720_BASE_ADDR,V1720_CHANNEL_EN_MASK ,chanmask);

    // SEt DC Offset
    int alldac = ts.v1720.channelsettings.dcoffset[8];
    if(alldac!= 0 )
    {
        for(i = 0; i<8; i++)
            v1720_ChannelDACSet(myvme, V1720_BASE_ADDR, i, alldac);
    }
    else
    {
        for(i = 0; i<8; i++)
        {
            int dac = ts.v1720.channelsettings.dcoffset[i];
            v1720_ChannelDACSet(myvme, V1720_BASE_ADDR, i, dac);
        }

    }

    if(ts.v1720.channelsettings.enable[9] == 0)
        v1720_RegisterWrite(myvme,V1720_BASE_ADDR,V1720_CHANNEL_EN_MASK ,0xFF);
    else
        v1720_RegisterWrite(myvme,V1720_BASE_ADDR,V1720_CHANNEL_EN_MASK ,chanmask);

    // SEt Channel Threshold
 /*   int alldc = ts.v1720.channelsettings.dcoffset[8];
    if(alldc != 0 )
    {
        for(i = 0; i<8; i++)
            v1720_ChannelDACSet(myvme, V1720_BASE_ADDR, i, alldac);
    }
    else
    {
        for(i = 0; i<8; i++)
        {
            int dac = ts.v1720.channelsettings.dcoffset[i];
            v1720_ChannelDACSet(myvme, V1720_BASE_ADDR, i, dac);
        }


    }



    int a = V1720_RegisterRead(myvme,V1720_BASE_ADDR,mything);
    */
    }

    return 0;
}


/****************************************************************************
                        BEGINING THE RUN
*****************************************************************************/

INT begin_of_run(INT run_number, char *error)
{

  INT Status, size;
  Status = get_user_parameters();

  /* read Triggger settings */

  printf("Start the Run\n");
  int i, csr, status, evtcnt, temp;
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
  printf("1290 Event Counter: %d\n", evtcnt);

  // v1290_Setup(myvme, V1290N_BASE_ADDR, 1);                           // Set mode 1
  // v1290_Status(myvme,V1290N_BASE_ADDR);

  v1290_LEResolutionSet(myvme, V1290N_BASE_ADDR, LE_RESOLUTION_25);     // Set Resolution to 25 ps
  evtcnt = v1290_EvtCounter(myvme, V1290N_BASE_ADDR);                   //Get event count
  printf("1290 Event Counter: %d\n", evtcnt);
  v1290_SoftClear(myvme, V1290N_BASE_ADDR);
  // v1290_Setup(myvme, V1290N_BASE_ADDR, 1);                           // Set mode 1


  //Setting up TDC
  int off = ts.v1290.tdcoffset ;            // offset is between -51000 to +1000 ns in multiple of 25 ns
  printf(" Your offset is :::: %d\n", off);
  if (ts.v1290.tdcoffset > 1000)
  {
    printf("You have entered wrong value for TDC. Offset range is from -51000 to 1000 ns\n");
    exit(0);
  }
  off = off / 25;                        // Put value divisible by 25 (in the range)
  off = 4096 + off;                         // taking lower multiple of 25 ns
  off = 61440 + off;
  printf("Setting OFFSET: 0x%04x\t%d\n", off, off);
  v1290_OffsetSet(myvme, V1290N_BASE_ADDR, off);

  int width = ts.v1290.tdcwidth;           // Width value is between 25 ns to 52200 ns in multiple of 25 ns
  width = width / 25;                      // Put value divisible by 25 (in the range)
  printf("Setting Width: 0x%04x\t%d\n", width, width );
  v1290_WidthSet(myvme, V1290N_BASE_ADDR, width);

#endif

#if defined V1720_CODE
    
    mvme_get_dmode(myvme, &datmode);
    mvme_set_dmode(myvme, MVME_DMODE_D32);
    
    //check the Digitizer
    printf("V1720\n");
    csr = mvme_read_value(myvme, V1720_BASE_ADDR, V1720_ROC_FW_VER);
    printf("V1720 ROC(motherboard)Firmware Revision: 0x%x\n", csr);

    // Software reset of the Digitizer
    v1720_Reset(myvme, V1720_BASE_ADDR);

    // Setting mode 1. It sets various parameters of the device.
    v1720_Setup(myvme, V1720_BASE_ADDR, 0x03);
    v1720_ChannelDACSet(myvme,V1720_BASE_ADDR,0,0xAA00);
    v1720_ChannelDACSet(myvme,V1720_BASE_ADDR,1,0xAA00);
    v1720_ChannelDACSet(myvme,V1720_BASE_ADDR,2,0xAA00);
    v1720_ChannelDACSet(myvme,V1720_BASE_ADDR,3,0xAA00);
    v1720_ChannelDACSet(myvme,V1720_BASE_ADDR,4,0x2000);

    v1720_ChannelThresholdSet(myvme,V1720_BASE_ADDR,4,0x9C4);
    v1720_ChannelOUThresholdSet(myvme,V1720_BASE_ADDR,4,8);

    // Setting the buffer size to 1k
    // v1720_RegisterWrite(myvme, V1720_BASE_ADDR, V1720_BUFFER_ORGANIZATION,  0x0A);

    // Setting the threshold
    // v1720_ChannelConfig(myvme,V1720_BASE_ADDR,0x2);
 /*   for (i = 1; i < 9; i++)
    {
        //v1720_ChannelThresholdSet(myvme, V1720_BASE_ADDR, i, 0x0868);
        v1720_ChannelThresholdSet(myvme, V1720_BASE_ADDR, i, 0x50);
    }
*/
    // Individual Settings for the Run:
    // Setting the post trigger value
    // v1720_RegisterWrite(myvme, V1720_BASE_ADDR, V1720_POST_TRIGGER_SETTING, 80);
    // SEtting the sample length
    // v1720_RegisterWrite(myvme, V1720_BASE_ADDR, V1720_CUSTOM_SIZE, 200);

    // v1720_RegisterWrite(myvme, V1720_BASE_ADDR, V1720_VME_CONTROL, 64);

    v1720_AcqCtl(myvme, V1720_BASE_ADDR, V1720_RUN_START);
    printf("V1720 IS Configured AGain\n");
    mvme_set_dmode(myvme, datmode);
    ss_sleep(1000);
#endif



// Other Device if available.

//******************Device setup Done*********************************//

  event_counter_for_interface = 0;
  offsetbefore = 0;

  printf("Waiting\n");
  for (i = 0; i < 3; i++)
  {
    ss_sleep(1000);
    printf(".\n");
  }

  printf("Starting now\n");
  printf("End 'Begin of Run'\n");


  IsRunStopped = false;

  CAENVME_SetOutputRegister((int) myvme->info, cvOut0Bit);
  CAENVME_ClearOutputRegister((int) myvme->info, cvOut0Bit);
  CAENVME_ClearOutputRegister((int) myvme->info, cvOut1Bit);

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
  v2495_EarlyWindow(myvme, V2495_BASE_ADDR, 0x0000);
  v2495_LateWindow(myvme, V2495_BASE_ADDR, 0x0000);

#endif

  IsRunStopped = true;
  fIsIRQEnabled = false;                                                    // used for disabling triggers
  ss_sleep(1000);

  CAENVME_SetOutputRegister((int) myvme->info, cvOut1Bit);
  v1720_AcqCtl(myvme, V1720_BASE_ADDR, V1720_RUN_STOP);
  v1720_Reset(myvme, V1720_BASE_ADDR);
  v1290_SoftReset(myvme, V1290N_BASE_ADDR);
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
  int digidat;

  InpDat = 0;
// Following logic can be used to stop polling after run stops
  // if(IsRunStopped == true) {return 0;}                                  // Check if uncommenting this line helps
  // Specially with many dots problem.
  for (i = 0; i < count+2; i++)
  {
    InpDat = v1290_DataReady(myvme, V1290N_BASE_ADDR);
    digidat = v1720_RegisterRead(myvme, V1720_BASE_ADDR, V1720_VME_STATUS);
    digidat = digidat & 0x00000001;

//    InpDat = digidat & InpDat;

    InpDat = digidat;  // Polling only on digitizer
       
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
  //DWORD *tdata, *pdata1, *pdata2, *pdata3, *evdata, *pdata, *pdata4;
  DWORD *tdata, *ddata, *evdata;
  int nentry, nextra, dummyevent, fevtcnt, temp;
  int dtemp, dentry, dextra, devtcnt;
  int size_of_evt, nu_of_evt;



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

  while (v1290_DataReady(myvme, V1290N_BASE_ADDR))
  {
    nextra++;
    v1290_EventRead(myvme, V1290N_BASE_ADDR, tdata, &dummyevent);
  }

// THIS part is also confusing
  if (nextra > 0)
  {
    if (evtdebug)
//     printf("ERROR Multiple Events in 1290: N words in FIFO = %d, Event Num = %d\n",nextra,fevtcnt);
      temp = 0xdcfe0000 + (nextra & 0x7ff);
    *tdata++ = temp;
    temp = 0xbad0cafe;
    *tdata++ = temp;
  }

  bk_close(pevent, tdata);

#endif

  //-----------------Take Data from Digitizer----------------------------//

#if defined V1720_CODE
    v1720_AcqCtl(myvme, V1720_BASE_ADDR, V1720_RUN_STOP);
    mvme_get_dmode(myvme, &datmode);
    mvme_set_am(myvme, MVME_AM_A32_ND); //set addressing mode to A32 non-privileged data
    mvme_set_dmode(myvme, MVME_DMODE_D32);
    mvme_set_blt(myvme, MVME_BLT_BLT32);

    dentry = 0;
    dextra = 0;
    devtcnt = 0;
    size_of_evt = 0;
    nu_of_evt = 0;
    int readmsg = 0;
    int cycle = 0,  maxcycles = 10;     // This varialble is important to read just the first event and not let ODB overflow

    int bytes_totransfer = 0, bytes_remaining = 0 ;
    int bytes_transferred =  0;
    int bytes_max =  4096;


    int InpDat = v1720_RegisterRead(myvme, V1720_BASE_ADDR, V1720_VME_STATUS);
    InpDat = InpDat & 0x00000001;

    if (InpDat == 1)
    {
        bk_create(pevent, "DG01", TID_DWORD, &ddata);      // Create data bank for Digitizer
        size_of_evt =  mvme_read_value(myvme, V1720_BASE_ADDR, 0x814C);
        nu_of_evt =  mvme_read_value(myvme, V1720_BASE_ADDR, 0x812C);
        bytes_remaining = size_of_evt * nu_of_evt * 4;     //chaning from 32bit to byte
        int toread = bytes_remaining;

        if (bytes_remaining > 1 )
        {
//            printf("\n\n\n\nNew Event:: \t Bytes Remaining:: %d\n", bytes_remaining);
            *ddata = (DWORD *) malloc(bytes_remaining);
            cycle = 0;
            int buffer_address = V1720_EVENT_READOUT_BUFFER;

            while(bytes_remaining > 1 && cycle < maxcycles)   // the cycle setting is important untill we have smart 
            {                                                  // trigger oterwise it will read many cylcles 
                                                               // and return odb fragmentation  error 
    //            printf("Bytes Remaining:: %d Cycle :: %d\n", bytes_remaining, cycle);
                if (bytes_remaining > 4096)
                {
                    bytes_totransfer = bytes_max;
//                    printf("Bytes Remaining:: %d\n", bytes_remaining);
                }
               else
                    bytes_totransfer = bytes_remaining;

                //        to_transfer = bytes_remaining > bytes_max ? bytes_max : bytes_remaining;
                //V1720_EVENT_READOUT_BUFFER
                readmsg = CAENVME_BLTReadCycle((long *)myvme->info, V1720_BASE_ADDR + V1720_EVENT_READOUT_BUFFER , ddata, bytes_totransfer, cvA32_U_BLT, 0x04, &bytes_transferred);
                bytes_remaining = bytes_remaining - bytes_transferred;
                if (readmsg == 0)
                {
//                    printf("BLT success::%d\n",readmsg);
                    ddata += bytes_transferred/4;
//                  printf("Cycles Requ:: %d \t Cycles READ:: %d \n", bytes_totransfer, bytes_transferred);
//                  printf("To REad:: %d \t ACt. READ:: %d \t Rem. to Read:: %d \n", bytes_totransfer, bytes_transferred, bytes_remaining);
//                    printf("First Data Word:: %d \n", ddata[0]);
                }
                if (readmsg != 0)
                {
                    printf("\n\n\n\nBLT REad Message::%d\n", readmsg);
                    printf("Cycles Requ:: %d \t Cycles READ:: %d \n", bytes_totransfer, bytes_transferred);
                }
                cycle++;
            }
        }
        bk_close(pevent, ddata);
    }




//       dtemp = v1720_DataRead(myvme, V1720_BASE_ADDR, ddata, 8);
//       ddata += dtemp;

    mvme_set_dmode(myvme, datmode);
    v1720_AcqCtl(myvme, V1720_BASE_ADDR, V1720_RUN_START);
#endif



//-----------------Bank for DAQ event counter----------------------------------//
  bk_create(pevent, "EVNT", TID_DWORD, &evdata);
  *evdata++ = event_counter_for_interface;
  bk_close(pevent, evdata);

  event_counter_for_interface++;

  // Generating a pulse
  CAENVME_SetOutputRegister((int) myvme->info, cvOut0Bit);
  CAENVME_ClearOutputRegister((int) myvme->info, cvOut0Bit);

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
  return ( ( event_counter_for_interface & 0x7 ) - ((gemlatch & 0xf) >> 1) ) & 0x7;
}
