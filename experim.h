/********************************************************************\

  Name:         experim.h
  Created by:   ODBedit program

  Contents:     This file contains C structures for the "Experiment"
                tree in the ODB and the "/Analyzer/Parameters" tree.

                Additionally, it contains the "Settings" subtree for
                all items listed under "/Equipment" as well as their
                event definition.

                It can be used by the frontend and analyzer to work
                with these information.

                All C structures are accompanied with a string represen-
                tation which can be used in the db_create_record function
                to setup an ODB structure which matches the C structure.

  Created on:   Tue Nov  6 14:03:49 2018

\********************************************************************/

#ifndef EXCL_TRIGGER

#define TRIGGER_COMMON_DEFINED

typedef struct {
  WORD      event_id;
  WORD      trigger_mask;
  char      buffer[32];
  INT       type;
  INT       source;
  char      format[8];
  BOOL      enabled;
  INT       read_on;
  INT       period;
  double    event_limit;
  DWORD     num_subevents;
  INT       log_history;
  char      frontend_host[32];
  char      frontend_name[32];
  char      frontend_file_name[256];
  char      status[256];
  char      status_color[32];
  BOOL      hidden;
} TRIGGER_COMMON;

#define TRIGGER_COMMON_STR(_name) const char *_name[] = {\
"[.]",\
"Event ID = WORD : 1",\
"Trigger mask = WORD : 0",\
"Buffer = STRING : [32] SYSTEM",\
"Type = INT : 2",\
"Source = INT : 0",\
"Format = STRING : [8] MIDAS",\
"Enabled = BOOL : y",\
"Read on = INT : 1",\
"Period = INT : 500",\
"Event limit = DOUBLE : 0",\
"Num subevents = DWORD : 0",\
"Log history = INT : 0",\
"Frontend host = STRING : [32] localhost",\
"Frontend name = STRING : [32] neatdaq",\
"Frontend file name = STRING : [256] frontend.h",\
"Status = STRING : [256] Started",\
"Status color = STRING : [32] green",\
"Hidden = BOOL : n",\
"",\
NULL }

#define TRIGGER_SETTINGS_DEFINED

typedef struct {
  struct {
    INT       tdcwidth;
    INT       tdcoffset;
    INT       tdcresolution;
  } v1290;
  struct {
    INT       fpgaearlywindow;
    INT       fpgalatewindow;
    INT       fpgavalidevents;
  } v2495;
  struct {
    struct {
      INT       bufferorganization;
      INT       triggersource[4];
      INT       posttriggersamples;
      INT       acquisitioncontrol;
    } boardsettings;
    struct {
      INT       enable[9];
      float     dcoffset[9];
      float     threshold[9];
    } channelsettings;
  } v1720;
} TRIGGER_SETTINGS;

#define TRIGGER_SETTINGS_STR(_name) const char *_name[] = {\
"[v1290]",\
"TDCWidth = INT : 4000",\
"TDCOffset = INT : -4025",\
"TDCResolution = INT : 0",\
"",\
"[v2495]",\
"FPGAEarlyWindow = INT : 10",\
"FPGALateWindow = INT : 200",\
"FPGAValidEvents = INT : 16169",\
"",\
"[v1720/BoardSettings]",\
"BufferOrganization = INT : 10",\
"TriggerSource = INT[4] :",\
"[0] 0",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"PostTriggerSamples = INT : 0",\
"AcquisitionControl = INT : 0",\
"",\
"[v1720/ChannelSettings]",\
"Enable = INT[9] :",\
"[0] 1",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"[4] 0",\
"[5] 0",\
"[6] 0",\
"[7] 0",\
"[8] 0",\
"DCOffset = FLOAT[9] :",\
"[0] 0",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"[4] 0",\
"[5] 0",\
"[6] 0",\
"[7] 0",\
"[8] 0",\
"Threshold = FLOAT[9] :",\
"[0] 0",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"[4] 0",\
"[5] 0",\
"[6] 0",\
"[7] 0",\
"[8] 0",\
"",\
NULL }

#endif

#ifndef EXCL_PERIODIC

#define PERIODIC_COMMON_DEFINED

typedef struct {
  WORD      event_id;
  WORD      trigger_mask;
  char      buffer[32];
  INT       type;
  INT       source;
  char      format[8];
  BOOL      enabled;
  INT       read_on;
  INT       period;
  double    event_limit;
  DWORD     num_subevents;
  INT       log_history;
  char      frontend_host[32];
  char      frontend_name[32];
  char      frontend_file_name[256];
  char      status[256];
  char      status_color[32];
  BOOL      hidden;
} PERIODIC_COMMON;

#define PERIODIC_COMMON_STR(_name) const char *_name[] = {\
"[.]",\
"Event ID = WORD : 2",\
"Trigger mask = WORD : 0",\
"Buffer = STRING : [32] SYSTEM",\
"Type = INT : 1",\
"Source = INT : 0",\
"Format = STRING : [8] MIDAS",\
"Enabled = BOOL : y",\
"Read on = INT : 255",\
"Period = INT : 1000",\
"Event limit = DOUBLE : 0",\
"Num subevents = DWORD : 0",\
"Log history = INT : 1",\
"Frontend host = STRING : [32] localhost",\
"Frontend name = STRING : [32] neatdaq",\
"Frontend file name = STRING : [256] frontend.h",\
"Status = STRING : [256] Started",\
"Status color = STRING : [32] green",\
"Hidden = BOOL : n",\
"",\
NULL }

#define PERIODIC_DISPLAY_DEFINED

typedef struct {
  struct {
    INT       trigfreq;
    INT       elecfreq;
    INT       ionfreq;
    INT       fpgarevno;
  } v2495;
} PERIODIC_DISPLAY;

#define PERIODIC_DISPLAY_STR(_name) const char *_name[] = {\
"[v2495]",\
"TrigFreq = INT : 65535",\
"ElecFreq = INT : 41182",\
"IonFreq = INT : 24567",\
"FPGARevNo = INT : 0",\
"",\
NULL }

#endif

