/*********************************************************************
Name:           vmicvme.c
Created by:     Pierre-Andre Amaudruz

Modified by:    Vincent Sulkosky

Contents:       VME interface for the VMIC VME board processor
                using the mvmestd VME call convention.

Modifications:  Changed for PVIC interface using the mvmestd VME call 
                convention.

$Id$

*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include <inttypes.h>
#include <stdint.h>

#include "v2718.h"
#include <CAENVMElib.h>
#include <CAENVMEoslib.h>
#include <CAENVMEtypes.h>

vme_bus_handle_t bus_handle;
vme_interrupt_handle_t int_handle;

struct sigevent event;
struct sigaction handler_act;

INT_INFO int_info;

int vmicvme_max_dma_nbytes = DEFAULT_DMA_NBYTES;
/*
mvme_size_t FullWsze (int am);
int vmic_mmap(MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_size_t size);
int vmic_unmap(MVME_INTERFACE *mvme, mvme_addr_t vmE_addr, mvme_size_t size);
mvme_addr_t vmic_mapcheck (MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_size_t n_bytes);
mvme_addr_t convert(mvme_addr_t address);
*/
#define A32_CHUNK 0xFFFFFFFF

/********************************************************************/
/**
Return the largest window size based on the address modifier.
*/
mvme_size_t FullWsze (int am) {
  switch (am & 0xF0) {
  case 0x00:
    return A32_CHUNK; // map A32 space in chunks of 16Mibytes
  case 0x30:
    return 0xFFFFFF; // map all of the A24 address space: 16Mibytes
  case 0x20:
    return 0xFFFF; // map all of the A16 address space: 64kibytes
  default:
    return 0;
  }
}

mvme_addr_t convert(mvme_addr_t address)
{  
   int newaddr;
   if (address%4==0) {
       newaddr=address+2; }
   else {
       newaddr=address-2;
   }
   return newaddr;
}

/********************************************************************/
/**
Open a VME channel
One bus handle per crate.
@param *mvme     VME structure.
@param index    VME interface index
@return MVME_SUCCESS, ERROR
*/
int mvme_open(MVME_INTERFACE **mvme, int index)
{

  //int *Handle;
  int32_t BHandle;
  /* index can be only 0 for VMIC */
 
  
  /* Allocate MVME_INTERFACE */
  *mvme = (MVME_INTERFACE *) calloc(1, sizeof(MVME_INTERFACE));
  
  /* Allocte VME_TABLE */
  (*mvme)->table = (char *) calloc(MAX_VME_SLOTS, sizeof(VME_TABLE));
  
  /* Set default parameters */
  (*mvme)->am = MVME_AM_DEFAULT;

  /* Allocate DMA_INFO */
  /* DMA_INFO *info;
     info = (DMA_INFO *) calloc(1, sizeof(DMA_INFO));
  */
   // for (BHandle=0;BHandle<20;BHandle++) printf("\n");
   //We Will use the info structure to store the V2718 Handle
    printf("Trying to access V2718...\n");
    if (CAENVME_Init(cvV2718,0,0,&BHandle)!=0) {assert("Problem Opening Link~\n");}
    printf("************** Handle:%x\n",BHandle);
    (*mvme)->info = BHandle;
//    exit(-1);
 
  printf("mvme_open:\n");
   return(MVME_SUCCESS);
}


/********************************************************************/
/**
Close and release ALL the opened VME channel.
@param *mvme     VME structure.
@return MVME_SUCCESS, ERROR
*/
int mvme_close(MVME_INTERFACE *mvme)
{
  long j;
  VME_TABLE *table;
  
  table = ((VME_TABLE *)mvme->table);
 
   j=(long )mvme->info;
    CAENVME_End(j);

    printf("mvme_close:\n");
  
  
  free (mvme->table);
  mvme->table = NULL;
  
  
  /* Free mvme block */
  free (mvme);
  
  return(MVME_SUCCESS);
}


/********************************************************************/
/**
Read from VME bus. Uses MVME_BLT_BLT32 for enabling the DMA
or size to transfer larger the 128 bytes (32 DWORD)
VME access measurement on a VMIC 7805 is :
Single read access : 1us
DMA    read access : 300ns / DMA latency 28us
@param *mvme VME structure
@param *dst  destination pointer
@param vme_addr source address (VME location).
@param vme_off source offset (opcode or register)
@param n_bytes requested transfer size.
@return MVME_SUCCESS, ERROR
*/
int mvme_read(MVME_INTERFACE *mvme, void *dst, mvme_addr_t vme_addr, mvme_locaddr_t vme_off, mvme_size_t n_bytes)
{
  //int status =0;
  //DWORD *II,*II2;
  //CVAddressModifier AM;
  //CVDataWidth DM;  
  mvme_addr_t addr;
  addr = vme_addr + vme_off;
  int TSize,count;
  TSize=n_bytes;
  CAENVME_BLTReadCycle((long *)mvme->info,addr,dst,TSize,cvA32_U_BLT,0x04,&count);

  n_bytes=count;
  return(count);//MVME_SUCCESS);
}

/********************************************************************/
/**
Read single data from VME bus.
@param *mvme VME structure
@param vme_addr source address (VME location).
@param vme_off source offset (opcode or register)
@return value return VME data
*/
DWORD mvme_read_value(MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_locaddr_t vme_off)
{
  mvme_addr_t addr;
  DWORD dst = 0xFFFFFFFF;
  CVAddressModifier AM;
  CVDataWidth DM;
  CAENVME_API Stat;
  /* Perform read */
/*  if (mvme->dmode == MVME_DMODE_D8) { // D8 should not be used
    addr = vme_addr + vme_off;
    char cdst = (char)dst;
    memcpy(&cdst,(char *)addr,1);
    return(cdst);
  } 
  // Need to convert address for PVIC using D16
  else if (mvme->dmode == MVME_DMODE_D16) {
    addr = vme_addr + convert(vme_off);
    WORD wdst = (WORD)dst;
    memcpy(&wdst,(char *)addr,2);
    return(wdst);
  }
  else if (mvme->dmode == MVME_DMODE_D32) {
    addr = vme_addr + vme_off;
    memcpy(&dst,(char *)addr,4);
  }
  */
  DM=mvme->dmode;
  if (DM==3) DM=4;
  addr = vme_addr + vme_off;

  Stat=CAENVME_ReadCycle((long *)mvme->info,(unsigned long)addr,&dst,mvme->am,DM);
  if (Stat!=cvSuccess) 	{ 
   }
  return(dst);
}

/********************************************************************/
/**
Write data to VME bus. Uses MVME_BLT_BLT32 for enabling the DMA
or size to transfer larger the 128 bytes (32 DWORD)
@param *mvme VME structure
@param vme_addr source address (VME location).
@param *src source array
@param n_bytes  size of the array in bytes
@return MVME_SUCCESS, MVME_ACCESS_ERROR
*/
int mvme_write(MVME_INTERFACE *mvme, mvme_addr_t vme_addr, void *src, mvme_size_t n_bytes)
{

  return MVME_SUCCESS;
}

/********************************************************************/
/**
Write single data to VME bus.
@param *mvme VME structure
@param vme_addr destination address (VME location).
@param vme_off source offset (opcode or register)
@param value  data to write to the VME address.
@return MVME_SUCCESS
*/
int mvme_write_value(MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_locaddr_t vme_off, DWORD value)
{
  mvme_addr_t addr;
  CVDataWidth DM;
    addr=vme_addr+vme_off;
  
  /* Perform write */
  if (mvme->dmode == MVME_DMODE_D8) { // D8 should not be used  
      char value_c = (char)value;
      // CAENVME_WriteCycle((long*)mvme->info,addr,&value_c,mvme->am,mvme->dmode);
  }
  else if (mvme->dmode == MVME_DMODE_D16) {
    WORD value_w = (WORD)value;
      // printf("I am writing to 16bit mode:::\n");
      // printf("INFO:%x \n",(long*)mvme->info);
      // printf("Addr:%x \n",addr);
      // printf("Value:%x \n",value_w);
      // printf("Address Modifier%x \n",mvme->am);
      // printf("Data Mode:%x \n",mvme->dmode);

    CAENVME_WriteCycle((long*)mvme->info,addr,&value_w,mvme->am,mvme->dmode);
  }
  else if (mvme->dmode == MVME_DMODE_D32) {
    DWORD value_dw = (DWORD)value;
    DM = 4;
      // printf("I am writing to 32bit mode:::\n");
      // printf("INFO:%x \n",(long*)mvme->info);
      // printf("Addr:%x \n",addr);
      // printf("Value:%x \n",value_dw);
      // printf("Address Modifier%x \n",mvme->am);
      // printf("Data Mode:%x \n",mvme->dmode);
    CAENVME_WriteCycle((long*)mvme->info,addr,&value_dw,mvme->am,DM);
  }
  
  return MVME_SUCCESS;
}

/********************************************************************/
int mvme_set_am(MVME_INTERFACE *mvme, int am)
{
  mvme->am = am;
  return MVME_SUCCESS;
}

/********************************************************************/
int EXPRT mvme_get_am(MVME_INTERFACE *mvme, int *am)
{
  *am = mvme->am;
  return MVME_SUCCESS;
}

/********************************************************************/
int mvme_set_dmode(MVME_INTERFACE *mvme, int dmode)
{
  mvme->dmode = dmode;
  return MVME_SUCCESS;
}

/********************************************************************/
int mvme_get_dmode(MVME_INTERFACE *mvme, int *dmode)
{
  *dmode = mvme->dmode;
  return MVME_SUCCESS;
}

/********************************************************************/
int mvme_set_blt(MVME_INTERFACE *mvme, int mode)
{
  mvme->blt_mode = mode;
  return MVME_SUCCESS;
}

/********************************************************************/
int mvme_get_blt(MVME_INTERFACE *mvme, int *mode)
{
  *mode = mvme->blt_mode;
  return MVME_SUCCESS;
}

/********************************************************************/
/*
static void myisr(int sig, siginfo_t * siginfo, void *extra)
{
  fprintf(stderr, "myisr interrupt received \n");
  fprintf(stderr, "interrupt: level:%d Vector:0x%x\n", int_info.level, siginfo->si_value.sival_int & 0xFF);
}
*/
/********************************************************************/
int mvme_interrupt_generate(MVME_INTERFACE *mvme, int level, int vector, void *info)
{
//  if (vme_interrupt_generate((vme_bus_handle_t )mvme->handle, level, vector)) {
//    perror("vme_interrupt_generate");
//    return MVME_ERROR;
//  }

  return MVME_SUCCESS;
}

/********************************************************************/
/**

<code>
static void handler(int sig, siginfo_t * siginfo, void *extra)
{
	print_intr_msg(level, siginfo->si_value.sival_int);
	done = 0;
}
</endcode>

*/
int mvme_interrupt_attach(MVME_INTERFACE *mvme, int level, int vector
					, void (*isr)(int, void*, void *), void *info)
{

  return MVME_SUCCESS;
}

/********************************************************************/
int mvme_interrupt_detach(MVME_INTERFACE *mvme, int level, int vector, void *info)
{
   return MVME_SUCCESS;
}

/********************************************************************/
int mvme_interrupt_enable(MVME_INTERFACE *mvme, int level, int vector, void *info)
{
  return MVME_SUCCESS;
}

/********************************************************************/
int mvme_interrupt_disable(MVME_INTERFACE *mvme, int level, int vector, void *info)
{
  return MVME_SUCCESS;
}

/********************************************************************/
/**
VME Memory map, uses the driver MVME_INTERFACE for storing the map information.
@param *mvme VME structure
@param vme_addr source address (VME location).
@param n_bytes  data to write
@return MVME_SUCCESS, MVME_ACCESS_ERROR
*/
int vmic_mmap(MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_size_t n_bytes)
{
  
  return(MVME_SUCCESS);
}

/********************************************************************/
/**
Unmap VME region.
VME Memory map, uses the driver MVME_INTERFACE for storing the map information.
@param *mvme VME structure
@param  nbytes requested transfer size.
@return MVME_SUCCESS, ERROR
*/
int vmic_unmap(MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_size_t size)
{
  return(MVME_SUCCESS);
}
/********************************************************************/
/**
Retrieve mapped address from mapped table.
Check if the given address belongs to an existing map. If not will
create a new map based on the address, current address modifier and
the given size in bytes.
@param *mvme VME structure
@param  nbytes requested transfer size.
@return MVME_SUCCESS, ERROR
*/
mvme_addr_t vmic_mapcheck (MVME_INTERFACE *mvme, mvme_addr_t vme_addr, mvme_size_t n_bytes)
{
//  return addr;
  return 0;
}

