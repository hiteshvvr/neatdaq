/** MAIN CHANGE IS frontend end name is now SARAF1
Digitizer is refferred as  DG */
#include "vmicvme.h"
// #include "v2495.h"
// #include "v965.h"
// #include "qdc.h"
#include "v1720.h"
#include "v812.h"


#define V1720_ROC_FW_VER	0x8124      /* R/W       ; D32 */ 
#define V1720_CODE

const int evtdebug = 1;

#ifdef __cplusplus
extern "C" {
#endif


/**********Globals*******************/

/* THE frontend name (client name ) as seen by other MIDAS clients */
char *frontend_name = "NEAT DAQ";

/* The frontend file name, "they" say don't change it */
char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE */
BOOL frontend_call_loop = FALSE;

/* A frontend status page is displayed with this frequency in ms */
INT display_period = 1000;

/*maximum event size produced by this frontend */
INT max_event_size = 200000;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 *1024;

/* buffer size to hold events */
INT event_buffer_size = 10 * 100000;

/* Hardware */
MVME_INTERFACE *myvme;



/* GLOBALS */

#define N_DG 128
#define N_QDC  32
#define N_TDC1 16
#define N_TDC2 128 
#define N_EVNT 2

//I DO NOT "understand" THIS part
extern HNDLE hDB;
HNDLE hSet;
// HNDLE hper;
TRIGGER_SETTINGS ts;
// PERIODIC_DISPLAY pd;
//continue

/*VME BASE Addresses */
DWORD V1720_BASE_ADDR = 0xEEEE0000;
DWORD V812_BASE_ADDR = 0xFFFF0000;

// DWORD V2495_BASE_ADDR = 0x00000000;
// DWORD V965_BASE_ADDR = 0x00000000;

//I DO NOT "understand" THIS part
int pvmeRemoteVmeAccessF ( int verbose, 
	                       int unit_number,
	                       unsigned int remote_vme_address,
	                       unsigned int remote_vme_am,
	                       unsigned int pvic_page_number,
	                       unsigned int pvme_page_number,
	                       int data_bytes,
	                       int data_type,
	                       int write_access,
	                       int write_data,
	                       int repeat_count);

int vmePioAccessExecute (int vme_address,
	                     char *pointer,
	                     int data_bytes,
	                     int data_type,
	                     int write_access,
	                     int write_data,
	                     int repeat_count);
//continue

// new event counter for interfacing with GEM DAQ 
INT event_counter_for_interface;

/* Function declarations */

INT frontend_init();
INT frontend_exit();
INT begin_of_run( INT run_number, char *error);
INT end_of_run(INT run_number, char *error);
INT pause_run(INT run_number, char *error);
INT resume_run(INT run_number, char *error);
INT frontend_loop();
extern void interrupt_routine(void);
INT read_trigger_event(char *pevent, INT off);
INT read_scaler_event(char *pevent, INT off);

//I DO NOT "understand" THIS part
void register_cnaf_callback(int debug);

int pvmeLocalSetup(int localNodeId);
char *GetVMEPtr(int reomote_vme_adress, int remote_vme_am, int bsize, int pvmenum, int pvicnum);
char *GetVMEPtr32(int remote_vme_adress, int remote_vme_am, int bsize, int pvmenum, int pvicnum);
int pvmeRemoteSetup(int remoteNodeId);
//continue

BANK_LIST trigger_bank_list[] = {
  // online data banks
 {"DG01", TID_WORD, N_DG, NULL} // V1720
 // ,
 // {"QDC1", TID_WORD, N_QDC, NULL} // V965
 ,
};


// Equipment List

EQUIPMENT equipment[] = {
	{"Trigger",              /*Equipment Name */
	  {1,0,					 /*Event ID, trigger mask */
	   "SYSTEM",             /* Event Buffer */
	   EQ_POLLED,            /*EQUIPMENT TYPE*/
	   LAM_SOURCE(0, 0X0),   /* Event Source Crate 0, all stations */
	   "MIDAS",              /* FORMAT */
	   TRUE,                 /*ENABLED*/
	   RO_RUNNING,           /* READ ONLY WHEN RUNNING */
	   500,                  /* POLL FOR 500ms*/
	   0,                    /* stop run after this event limit */
	   0,                    /* number of sub events */
	   0,                    /* don't log history */
	   "", "", "",},
       read_trigger_event,    /*readout routine */
	   NULL, NULL, 
	   trigger_bank_list,
	  },

    {""}

};

#ifdef __cplusplus
}
#endif


