/***********************************************************************************************
 *
 * Name: frontend.c
 * Created by: Modified from the template of Stefan Ritt and Guy Ron
 *
 * Contents: Frontend for the readout of the experintal data of Neon Branching Ratio
 * experiment at SARAF
 *
 * $Id: frontend.c 0001 24-10-17    hitesh.rahangdale@mail.huji.ac.il
 * $Id: frontend.c 0002 08-11-18    hitesh.rahangdale@mail.huji.ac.il
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

void seq_callback(INT hDB, INT hseq, void *info)
{
    printf("odb... Trigger settings Changed\n");
}

/****************************************************************************
  INITIALIZATION OF FRONTEND
 *****************************************************************************/
INT frontend_init()
{
    printf("Initialize Frontend::'\n");

    set_equipment_status(equipment[0].name, "Initializing...", "yellow");

    int i, status, csr;
    status = mvme_open(&myvme, 0);
    printf(" Status::: %d\n Hello world\n", status);

    CAENVME_DeviceReset((int) myvme->info);

    CAENVME_SetOutputConf((int) myvme->info, cvOutput0, cvDirect, cvDirect, cvManualSW);
    printf("Input Mux Register");
    CAENVME_SetOutputConf((int) myvme->info, cvOutput1, cvDirect, cvDirect, cvManualSW);

    CAENVME_SetOutputRegister((int) myvme->info, cvOut1Bit);     // Although using it now its used End of transmission

    CAENVME_SetInputConf((int) myvme->info, cvInput0, cvDirect, cvDirect);

    unsigned int InpDat;
    CAENVME_ReadRegister((int) myvme->info, 0x0B, &InpDat);
    printf("Input Mux Register %4.4x\n", InpDat);

    CAENVME_ReadRegister((int) myvme->info, cvInputReg, &InpDat);
    printf("Input Register %4.4x\n", InpDat);

    mvme_set_am(myvme, MVME_AM_A32); // Setting the addressing mode
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
    set_equipment_status(equipment[0].name, "Setting TDC", "yellow");

    // Status and Firmware version of Digitizer

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
#endif

    /****************Device Initialization Finished *************************/

    int gemlatch = 9; // I do not understand the use of this function
    IsRunStopped = true;
    printf("End of Frontend Initialization\n");

    int fp = v1720_RegisterRead(myvme, V1720_BASE_ADDR, 0x811C);
    if (fp & 0x00000000 == 0)
        printf("Front Panel NIM%x\n");

    for (i = 1; i < 9; i++)
    {
        v1720_ChannelThresholdSet(myvme, V1720_BASE_ADDR, i, 0x050);
    }

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
    mvme_set_dmode(myvme, MVME_DMODE_D32); // Setting data mode to 16 Bit

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
  /*  for (i = 1; i < 9; i++)
    {
        //v1720_ChannelThresholdSet(myvme, V1720_BASE_ADDR, i, 0x0868);
//        v1720_ChannelThresholdSet(myvme, V1720_BASE_ADDR, i, 0x50);
    }*/

    v1720_ChannelDACSet(myvme,V1720_BASE_ADDR,0,0xAA00);
    v1720_ChannelDACSet(myvme,V1720_BASE_ADDR,1,0xAA00);
    v1720_ChannelDACSet(myvme,V1720_BASE_ADDR,2,0xAA00);
    v1720_ChannelDACSet(myvme,V1720_BASE_ADDR,3,0xAA00);
    v1720_ChannelDACSet(myvme,V1720_BASE_ADDR,4,0x2000);
    
    v1720_ChannelThresholdSet(myvme,V1720_BASE_ADDR,4,0x6A4);
    v1720_ChannelOUThresholdSet(myvme,V1720_BASE_ADDR,4,8);
    // Individual Settings for the Run:
    // Setting the post trigger value
    // v1720_RegisterWrite(myvme, V1720_BASE_ADDR, V1720_POST_TRIGGER_SETTING, 80);
    // SEtting the sample length
    // v1720_RegisterWrite(myvme, V1720_BASE_ADDR, V1720_CUSTOM_SIZE, 200);

    // v1720_RegisterWrite(myvme, V1720_BASE_ADDR, V1720_VME_CONTROL, 64);




    printf("V1720 IS Configured\n");

    ss_sleep(1000);
#endif
    //******************Device Setup Done*********************************//

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

    v1720_AcqCtl(myvme, V1720_BASE_ADDR, V1720_RUN_START);
    // v1720_RegisterWrite(myvme, V1720_BASE_ADDR, V1720_ACQUISITION_CONTROL, 0x4);
    csr = v1720_RegisterRead(myvme, V1720_BASE_ADDR, V1720_ACQUISITION_CONTROL);
    printf("V1720_ACQUISITION_CONTROL::::%8.8x\n", csr);
    printf("Begin of Run is done \n");

    ss_sleep(1000);
    return SUCCESS;
}

/****************************************************************************
  END OF RUN
 *****************************************************************************/

INT end_of_run(INT run_number, char *error)
{
    IsRunStopped = true;
    fIsIRQEnabled = false;              // user for disabling triggers



    // CAENVME_SetOutputRegister((int) myvme->info,cvOut0Bit);
    // CAENVME_SetOutputRegister((int) myvme->info,cvOut1Bit);
    CAENVME_SetOutputRegister((int) myvme->info, cvOut1Bit);
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

    InpDat = 0;
    // Following logic can be used to stop polling after run stops
    //    if(IsRunStopped == true) {return 0;}

    for (i = 0; i < count + 2; i++)
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
    int dtemp, dentry, dextra, devtcnt;
    int size_of_evt, nu_of_evt;

    // Generating a pulse
    CAENVME_SetOutputRegister((int) myvme->info, cvOut0Bit);
    CAENVME_SetOutputRegister((int) myvme->info, cvOut1Bit);
    //initialize bank structure
    bk_init32(pevent);

    //-----------------Take Data from Digitizer----------------------------//
#if defined V1720_CODE
    v1720_AcqCtl(myvme, V1720_BASE_ADDR, V1720_RUN_STOP);

    dentry = 0;
    dextra = 0;
    devtcnt = 0;
    size_of_evt = 0;
    nu_of_evt = 0;
    int readmsg = 0;
    int cycle = 0,  maxcycles = 6;     // This varialble is important to read just the first event and not let ODB overflow

    int bytes_totransfer = 0, bytes_remaining = 0 ;
    int bytes_transferred =  0;
    int bytes_max =  4096;

    bk_create(pevent, "DG01", TID_DWORD, &ddata);      // Create data bank for Digitizer

    //************************EVENT READ ONE BY ONE**********************//
    // uint32_t first;
    // first=v1720_RegisterRead(myvme,V1720_BASE_ADDR,V1720_EVENT_READOUT_BUFFER);
    // int n32w=first&0x0FFFFFFF;
    // printf("%d\n", n32w );
    //************************EVENT READ ONE BY ONE**********************//
    // printf("--\n");
    // printf("EventSize=%d\tNuOfevents=%d\tNuOfcycles=%d\tTran Bef Read be Zero=%d\n", size_of_evt,nu_of_evt,nuofcycles,transffered_cycles);

    mvme_set_am(myvme, MVME_AM_A32_ND); //set addressing mode to A32 non-privileged data
    mvme_set_dmode(myvme, MVME_DMODE_D32);
    mvme_set_blt(myvme, MVME_BLT_BLT32);
    int InpDat = v1720_RegisterRead(myvme, V1720_BASE_ADDR, V1720_VME_STATUS);
    InpDat = InpDat & 0x00000001;

    if (InpDat == 1)
    {
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
    }




//       dtemp = v1720_DataRead(myvme, V1720_BASE_ADDR, ddata, 8);
//       ddata += dtemp;

    // int aaa = CAENVME_BLTReadCycle((long *)myvme->info, V1720_BASE_ADDR+V1720_EVENT_READOUT_BUFFER, ddata, size_of_evt*4,cvA32_U_BLT,0x04,&transffered_cycles);

    // ddata += transffered_cycles;

    // int aaa=CAENComm_BLTRead((long *)myvme->info, V1720_BASE_ADDR+V1720_EVENT_READOUT_BUFFER, ddata,size_of_evt*4,transffered_cycles);

    //dentry=dentry*4*1000;
    // dentry = 256;
    //dtemp = v1720_DataBlockRead(myvme, V1720_BASE_ADDR, ddata, &dentry);

    bk_close(pevent, ddata);
#endif


    //-----------------Bank for DAQ event counter----------------------------------//
    bk_create(pevent, "EVNT", TID_DWORD, &evdata);
    *evdata++ = event_counter_for_interface;
    bk_close(pevent, evdata);

    event_counter_for_interface++;

    // Generating a pulse
    CAENVME_SetOutputRegister((int) myvme->info, cvOut0Bit);
    CAENVME_SetOutputRegister((int) myvme->info, cvOut1Bit);
    v1720_AcqCtl(myvme, V1720_BASE_ADDR, V1720_RUN_START);
    return bk_size(pevent);
}


/*************************************************************************************
 * DON'T KNOW EXACTLY
 ************************************************************************************/

int eventcounteroffset(int event_counter_for_interface, int gemlatch)
{
    return ( ( event_counter_for_interface & 0x7 ) - ((gemlatch & 0xf) >> 1) ) & 0x7;
}
