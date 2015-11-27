/*******************************************************
                          cache.cc
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/
/*******************************************************
                          cache.cc
                  		Mengqiu Dong
                           2015
                	  mdong3@ncsu.edu
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "cache.h"
using namespace std;

Cache::Cache(int s,int a,int b )
{
   ulong i, j;
   reads = readMisses = writes = 0; 
   writeMisses = writeBacks = currentCycle = 0;

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
  
   //*******************//
   //initialize your counters here//
   //*******************//
    CaToCaTransfer = memTrans = 0;
    interventions = invalidations = flushes = BusRdx = 0;
 
    BusFlags = 0;
    
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
		tagMask <<= 1;
        tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
	   cache[i][j].invalidate();
      }
   }      
   
}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
void Cache::Access(ulong addr,uchar op)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w') writes++;
	else          reads++;
	
	cacheLine * line = findLine(addr);
	if(line == NULL)/*miss*/
	{
		if(op == 'w') writeMisses++;
		else readMisses++;

		cacheLine *newline = fillLine(addr);
   		if(op == 'w') newline->setFlags(DIRTY);    
		
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if(op == 'w') line->setFlags(DIRTY);
	}
}

/*********************MSI***************************/
ulong Cache::MSIProtocolProc(ulong addr, uchar op) {
    currentCycle++;
    if(op == 'w') writes++;
    else          reads++;
    
    cacheLine * line = findLine(addr);
    if (line == NULL) { //if miss ---> I state
        cacheLine *newline = fillLine(addr);
        if (op == 'w') { //need write, transfer to Modify
            writeMisses ++;
            memTrans ++;
            newline->setFlags(DIRTY);
            setBusFlags(BUSRDX);
            BusRdx ++;
        }
        else { //need read, transfer to Shared
            readMisses ++;
            memTrans ++;
            newline->setFlags(SHARED);
            setBusFlags(BUSRD);
        }
    }
    else { //not miss --> M or S state
        updateLRU(line);
        if (line->getFlags() == DIRTY) { //if M state
            setBusFlags(NONE);
        }
        else if(line->getFlags() == SHARED){//if S state
            if (op == 'r') {//read on S sate
                setBusFlags(NONE);
            }
            else {//write on S state
                memTrans ++;
                line->setFlags(DIRTY);
                setBusFlags(BUSRDX);
                BusRdx ++;
            }
        }
    }
    return getBusFlags();
}

void Cache::MSIProtocolBus(ulong BusFlags, ulong addr) {
    cacheLine * line = findLine(addr);
    if (line == NULL) {
        return;
    }
    if (BusFlags == NONE) {
        return;
    }
    if (line->getFlags() == DIRTY) {//////////state is Modify/////////////
        if (BusFlags == BUSRD) {//M BUSRD/Flush S
            flushes ++;
            writeBacks ++;
            memTrans ++;
            interventions ++;
            line->setFlags(SHARED);
        }
        else if (BusFlags == BUSRDX) { //M BUSRDX/Flush I
            flushes ++;
            writeBacks ++;
            memTrans ++;
            invalidations ++;
            line->setFlags(INVALID);
        }
    }
    else if (line->getFlags() == SHARED) {//////////state is Share/////////////
        if (BusFlags == BUSRD) { //S BUSRD/- S
            return;
        }
        else if (BusFlags == BUSRDX) { //S BUSRDX/- I
            invalidations ++;
            line->setFlags(INVALID);
        }
    }
}
/*********************MSI***************************/


/*********************MESI***************************/
bool Cache::isInvalid(ulong addr) {
    cacheLine *testLine = findLine(addr);
    if (testLine == NULL) {
        return true;
    }
    else
        return false;
}
ulong Cache::MESIProtocolProc(ulong addr, uchar op, bool hasCopy) {
    currentCycle++;
    if(op == 'w') writes++;
    else          reads++;
    
    cacheLine * line = findLine(addr);
    if (line == NULL) { //if miss ---> I state
        cacheLine *newline = fillLine(addr);
        if (op == 'w') { //need write, transfer to Modify
            writeMisses ++;
            memTrans ++;
            newline->setFlags(DIRTY);
            if (hasCopy) {
                CaToCaTransfer ++;
                memTrans --;
            }
            setBusFlags(BUSRDX);
            BusRdx ++;
        }
        else { //need read, transfer to Shared
            readMisses ++;
            memTrans ++;
            if (hasCopy) {
                newline->setFlags(SHARED);
                CaToCaTransfer ++;
                memTrans --;
            }
            else
                newline->setFlags(EXCLUSIVE);
            setBusFlags(BUSRD);
        }
    }
    else { //not miss --> M or S or E state
        updateLRU(line);
        if (line->getFlags() == DIRTY) { //if M state
            setBusFlags(NONE);
        }
        else if(line->getFlags() == SHARED){//if S state
            if (op == 'r') {//read on S sate
                setBusFlags(NONE);
            }
            else {//write on S state
                line->setFlags(DIRTY);
                setBusFlags(BUSUPGR);
//                BusRdx ++;
            }
        }
        else if(line->getFlags() == EXCLUSIVE){
            if (op == 'r') {//read on E state
                setBusFlags(NONE);
            }
            else {//write on E state
                line->setFlags(DIRTY);
                setBusFlags(NONE);
            }
        }
    }
    return getBusFlags();
}

void Cache::MESIProtocolBus(ulong BusFlags, ulong addr) {
    cacheLine * line = findLine(addr);
    if (line == NULL) {
        return;
    }
    if (BusFlags == NONE) {
        return;
    }
    if (line->getFlags() == DIRTY) {//////////state is Modify/////////////
        if (BusFlags == BUSRD) {//M BUSRD/Flush S
            flushes ++;
            writeBacks ++;
            interventions ++;
            memTrans ++;
            line->setFlags(SHARED);
        }
        else if (BusFlags == BUSRDX) { //M BUSRDX/Flush I
            flushes ++;
            writeBacks ++;
            invalidations ++;
            memTrans ++;
            line->setFlags(INVALID);
        }
    }
    else if (line->getFlags() == SHARED) {//////////state is Share/////////////
        if (BusFlags == BUSRD) { //S BUSRD/FlushOpt S
            return;
        }
        else if (BusFlags == BUSRDX) { //S BUSRDX/FlushOpt I
            invalidations ++;
            line->setFlags(INVALID);
        }
        else if(BusFlags == BUSUPGR) { //S BUSUPGR/- I
            invalidations ++;
            line->setFlags(INVALID);
        }
    }
    else if (line->getFlags() == EXCLUSIVE) {///////////Stete is Exclusive/////////
        if (BusFlags == BUSRD) {//E BUSRD/FlushOpt S
            interventions ++;
            line->setFlags(SHARED);
        }
        else if(BusFlags == BUSRDX) {//E BUSRDX/FlushOpt I
            invalidations ++;
            line->setFlags(INVALID);
        }
    }
    //   cout << "states: " << line->getFlags() << endl; 
}
/*********************MESI***************************/

/*********************Dragon***************************/
ulong Cache::DragonProtocolProc(ulong addr, uchar op, bool hasCopy) {
    currentCycle++;
    if(op == 'w') writes++;
    else          reads++;
    
    cacheLine * line = findLine(addr);
    if (line == NULL) { //if miss
        cacheLine *newline = fillLine(addr);
        if (op == 'w') { //miss and need write
            writeMisses ++;
            memTrans ++;
            if (hasCopy) {
                newline->setFlags(SHARED_MODIFIED);
                setBusFlags(BUSRD_BUSUPD);
            }
            else {
                newline->setFlags(DIRTY);
                setBusFlags(BUSRD);
            }
        }
        else { //miss and need read
            readMisses ++;
            memTrans ++;
            if (hasCopy) {
                newline->setFlags(SHARED_CLEAN);
                setBusFlags(BUSRD);
            }
            else {
                newline->setFlags(EXCLUSIVE);
                setBusFlags(BUSRD);
            }
        }
    }
    else { //not miss --> M or Sc or Sm or E state
        updateLRU(line);
        if (line->getFlags() == DIRTY) { //if M state
            setBusFlags(NONE);
        }
        else if(line->getFlags() == SHARED_CLEAN){//if Sc state
            if (op == 'r') {//read on Sc sate
                setBusFlags(NONE);
            }
            else {//write on Sc state
                if (hasCopy) {
                    line->setFlags(SHARED_MODIFIED);
                    setBusFlags(BUSUPD);
                }
                else {
                    line->setFlags(DIRTY);
                    setBusFlags(BUSUPD);
                }
            }
        }
        else if(line->getFlags() == SHARED_MODIFIED){//if Sm state
            if (op == 'r') {//read on Sm sate
                setBusFlags(NONE);
            }
            else {//write on Sm state
                if (hasCopy) {
                    setBusFlags(BUSUPD);
                }
                else {
                    line->setFlags(DIRTY);
                    setBusFlags(BUSUPD);
                }
            }
        }
        else if(line->getFlags() == EXCLUSIVE){
            if (op == 'r') {//read on E state
                setBusFlags(NONE);
            }
            else {//write on E state
                line->setFlags(DIRTY);
                setBusFlags(NONE);
            }
        }
    }
    return getBusFlags();
}

void Cache::DragonProtocolBus(ulong BusFlags, ulong addr) {
    cacheLine * line = findLine(addr);
    if (line == NULL) {
        return;
    }
    if (BusFlags == NONE) {
        return;
    }
    if (line->getFlags() == DIRTY) {//////////state is Modify/////////////
        if (BusFlags == BUSRD) {//M BUSRD/Flush Sm
            flushes ++;
            interventions ++;
            line->setFlags(SHARED_MODIFIED);
        }
        else if (BusFlags == BUSRD_BUSUPD) { //M BUSRD_BUSUPD/Flush_Update Sc
            flushes ++;
            interventions ++;
            line->setFlags(SHARED_CLEAN);
        }
    }
    else if (line->getFlags() == SHARED_CLEAN) {//////////state is Sc/////////////
        if (BusFlags == BUSRD) { //Sc BUSRD/- Sc
            return;
        }
        else if (BusFlags == BUSUPD) { //Sc BUSUPD/update Sc
            return;
        }
    }
    else if (line->getFlags() == SHARED_MODIFIED) {//////////state is Sm/////////////
        if (BusFlags == BUSRD) { //Sm BUSRD/Flush Sm
            flushes ++;
        }
        else if (BusFlags == BUSUPD) { //Sm BUSUPD/update Sc
            line->setFlags(SHARED_CLEAN);
        }
        else if(BusFlags == BUSRD_BUSUPD) {//Sm BUSRD_BUSUPD/Flush_Update Sc
            line->setFlags(SHARED_CLEAN);
            flushes ++;
        }
    }
    else if (line->getFlags() == EXCLUSIVE) {///////////State is Exclusive/////////
        if (BusFlags == BUSRD) {//E BUSRD/- Sc
            interventions ++;
            line->setFlags(SHARED_CLEAN);
        }
    }
    //   cout << "states: " << line->getFlags() << endl;
}
/*********************Dragon***************************/

/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
	if(cache[i][j].isValid())
	        if(cache[i][j].getTag() == tag)
		{
		     pos = j; break; 
		}
   if(pos == assoc)
	return NULL;
   else
	return &(cache[i][pos]); 
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
  line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) return &(cache[i][j]);     
   }   
   for(j=0;j<assoc;j++)
   {
	 if(cache[i][j].getSeq() <= min) { victim = j; min = cache[i][j].getSeq();}
   } 
   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
//    if(victim->getFlags() == DIRTY) {
//        writeBacks ++;
//    }
    if(victim->getFlags() == DIRTY || victim->getFlags() == SHARED_MODIFIED) {
        writeBack(addr);
        memTrans ++;
    }
    	
   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(VALID);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::printStats(int proIndx)
{
	printf("============ Simulation results (Cache %d) ============\n", proIndx);
	/****print out the rest of statistics here.****/
	/****follow the ouput file format**************/
    printf("01. number of reads: %ld\n",  reads);
    printf("02. number of read misses: %ld\n", readMisses);
    printf("03. number of writes: %ld\n", writes);
    printf("04. number of write misses: %ld\n", writeMisses);
    printf("05. total miss rate: %.2lf%%\n",(float) (readMisses + writeMisses)/(reads + writes)*100);
    printf("06. number of writebacks: %ld\n", writeBacks/* + flushes*/);
    printf("07. number of cache-to-cache transfers: %ld\n", CaToCaTransfer);
    printf("08. number of memory transactions: %ld\n", memTrans/*memTrans + readMisses + writeMisses +writeBacks*/);
    printf("09. number of interventions: %ld\n", interventions);
    printf("10. number of invalidations: %ld\n", invalidations);
    printf("11. number of flushes: %ld\n", flushes);
    printf("12. number of BusRdX: %ld\n", BusRdx);
}
