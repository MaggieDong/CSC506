/*******************************************************
                          cache.h
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/
/*******************************************************
                          cache.h
                  		Mengqiu Dong
                           2015
                	  mdong3@ncsu.edu
********************************************************/

#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;

/****add new states, based on the protocol****/
enum{
	INVALID = 0,
	VALID,
	DIRTY,
    SHARED,
    EXCLUSIVE,
    SHARED_MODIFIED,
    SHARED_CLEAN
};


class cacheLine 
{
protected:
   ulong tag;
   ulong Flags;   // 0:invalid, 1:valid, 2:dirty
//   ulong BusFlags;
   ulong seq;
 
public:
   cacheLine()            { tag = 0; Flags = 0; }
   ulong getTag()         { return tag; }
   ulong getFlags()			{ return Flags;}
//   ulong getBusFlags()     {return BusFlags;}
   ulong getSeq()         { return seq; }
   void setSeq(ulong Seq)			{ seq = Seq;}
   void setFlags(ulong flags)			{  Flags = flags;}
//    void setBusFlags(ulong Busflags)        {BusFlags = Busflags;}
   void setTag(ulong a)   { tag = a; }
   void invalidate()      { tag = 0; Flags = INVALID; }//useful function
   bool isValid()         { return ((Flags) != INVALID); }
};

enum{
    NONE = 0,
    BUSRD,
    BUSRDX,
    BUSUPGR,
    BUSUPD,
    BUSRD_BUSUPD
};
class Cache
{
protected:
   ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
    ulong reads,readMisses,writes,writeMisses,writeBacks;
    ulong BusFlags;

   //******///
   //add coherence counters here///
   //******///
    ulong CaToCaTransfer, memTrans, interventions, invalidations, flushes, BusRdx;

   cacheLine **cache;
   ulong calcTag(ulong addr)     { return (addr >> (log2Blk) );}
   ulong calcIndex(ulong addr)  { return ((addr >> log2Blk) & tagMask);}
   ulong calcAddr4Tag(ulong tag)   { return (tag << (log2Blk));}
   
public:
    ulong currentCycle;  
     
    Cache(int,int,int);
   ~Cache() { delete cache;}
   
   cacheLine *findLineToReplace(ulong addr);
   cacheLine *fillLine(ulong addr);
   cacheLine * findLine(ulong addr);
   cacheLine * getLRU(ulong);
   
   ulong getRM(){return readMisses;} ulong getWM(){return writeMisses;} 
   ulong getReads(){return reads;}ulong getWrites(){return writes;}
   ulong getWB(){return writeBacks;}
    ulong getBusFlags()     {return BusFlags;}

   void writeBack(ulong)   {writeBacks++;}
   void Access(ulong,uchar);
   void printStats(int);
   void updateLRU(cacheLine *);
    void setBusFlags(ulong Busflags)        {BusFlags = Busflags;}

   //******///
   //add other functions to handle bus transactions///
   //******///
    ulong MSIProtocolProc(ulong, uchar);
    void MSIProtocolBus(ulong, ulong);
    bool isInvalid(ulong addr);
    ulong MESIProtocolProc(ulong, uchar, bool);
    void MESIProtocolBus(ulong, ulong);
    ulong DragonProtocolProc(ulong, uchar, bool);
    void DragonProtocolBus(ulong, ulong);
};

#endif
