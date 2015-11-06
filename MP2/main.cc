/*******************************************************
                          main.cc
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/
/*******************************************************
                          main.cc
                  		Mengqiu Dong
                           2015
                	  mdong3@ncsu.edu
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
#include <iostream>
using namespace std;

#include "cache.h"

void MSI(int num_processors, int proIndx, unsigned char operaType, unsigned long addr, Cache** cacheArr);
void MESI(int num_processors, int proIndx, unsigned char operaType, unsigned long addr, Cache** cacheArr);
void Dragon(int num_processors, int proIndx, unsigned char operaType, unsigned long addr, Cache** cacheArr);
int main(int argc, char *argv[])
{
	
	ifstream fin;
	FILE * pFile;

	if(argv[1] == NULL){
		 printf("input format: ");
		 printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
		 exit(0);
        }

	/*****uncomment the next five lines*****/
	int cache_size = atoi(argv[1]);
	int cache_assoc= atoi(argv[2]);
	int blk_size   = atoi(argv[3]);
	int num_processors = atoi(argv[4]);/*1, 2, 4, 8*/
	int protocol   = atoi(argv[5]);	 /*0:MSI, 1:MESI, 2:Dragon*/
	char *fname =  (char *)malloc(20);
 	fname = argv[6];

//    cout << fname << endl;
	
	//****************************************************//
	//**printf("===== Simulator configuration =====\n");**//
	//*******print out simulator configuration here*******//
	//****************************************************//

 
	//*********************************************//
        //*****create an array of caches here**********//
    
    Cache** cacheArr = new Cache*[num_processors];
    for (int i = 0; i < num_processors; i++) {
        *(cacheArr + i) = new Cache(cache_size, cache_assoc, blk_size);
    }
	//*********************************************//	

    //absolute path: /Users/admin/Documents/2015\ Fall/ECE506/Machine\ Problem/MP2/MP2/MP2///
	pFile = fopen (fname,"r");
//    cout << pFile << endl;
	if(pFile == 0)
	{
        perror("Erroe");
		printf("Trace file problem\n");
		exit(0);
	}
    
    int proIndx;
    unsigned char operaType;
    unsigned long addr;
    
    int count = 0;
    string protocalName;
    while (/*count < 10*/1) {
        if (feof(pFile)) {
            break;
        }
        fscanf(pFile, "%d %c %lx\n", &proIndx, &operaType, &addr);
//        cout << proIndx << " " << operaType << " " << addr << endl;
        switch (protocol) {
            case 0:
                protocalName = "MSI";
                MSI(num_processors, proIndx, operaType, addr, cacheArr);
                break;
            case 1:
                protocalName = "MESI";
                MESI(num_processors, proIndx, operaType, addr, cacheArr);
                break;
            case 2:
                protocalName = "Dragon";
                Dragon(num_processors, proIndx, operaType, addr, cacheArr);
                break;
            default:
                protocalName = "Unkown";
                printf("protocol is unknown.");
                break;
        }
        count ++;

    }
    printf("===== 506 Personal information =====\n");
    printf("Name: Mengqiu Dong\n");
    printf("UnityID: mdong3\n");
    printf("CSC506 Students section 001\n");
    printf("===== 506 SMP Simulator configuration =====\n");
    printf("L1_SIZE: %d\n", cache_size);
    printf("L1_ASSOC: %d\n", cache_assoc);
    printf("L1_BLOCKSIZE: %d\n", blk_size);
    printf("NUMBER OF PROCESSORS: %d\n", num_processors);
    printf("COHERENCE PROTOCOL: %s\n", protocalName.c_str());
    printf("TRACE FILE: %s\n", fname);
    for (int i = 0; i < num_processors; i++) {
//        printf("for processor: %d\n", i);
        cacheArr[i]->printStats(i);
    }
	///******************************************************************//
	//**read trace file,line by line,each(processor#,operation,address)**//
	//*****propagate each request down through memory hierarchy**********//
	//*****by calling cachesArray[processor#]->Access(...)***************//
	///******************************************************************//
    
		fclose(pFile);
		
	//********************************//
	//print out all caches' statistics //
	//********************************//
	
}
void MSI(int num_processors, int proIndx, unsigned char operaType, unsigned long addr, Cache** cacheArr) {
    unsigned long busFlags = cacheArr[proIndx]->MSIProtocolProc(addr, operaType);
    for (int i = 0; i < num_processors; i++) {
//        cout << proIndx << " is index, cache: " << i << endl;
        if (i != proIndx) {
            cacheArr[i]->MSIProtocolBus(busFlags, addr);
        }
    }
}
void MESI(int num_processors, int proIndx, unsigned char operaType, unsigned long addr, Cache** cacheArr) {
    bool hasCopy = false;
    if (cacheArr[proIndx]->isInvalid(addr)) {
        for (int i = 0; i < num_processors; i ++) {
            if (i != proIndx) {
                if (!cacheArr[i]->isInvalid(addr)) {
                    hasCopy = true;
                    break;
                }
            }
        }
    }
    unsigned long busFlags = cacheArr[proIndx]->MESIProtocolProc(addr, operaType, hasCopy);
    for (int i = 0; i < num_processors; i++) {
        //        cout << proIndx << " is index, cache: " << i << endl;
        if (i != proIndx) {
            cacheArr[i]->MESIProtocolBus(busFlags, addr);
        }
    }
}
void Dragon(int num_processors, int proIndx, unsigned char operaType, unsigned long addr, Cache** cacheArr) {
    bool hasCopy = false;
    for (int i = 0; i < num_processors; i ++) {
        if (i != proIndx) {
            if (!cacheArr[i]->isInvalid(addr)) {
                hasCopy = true;
                break;
            }
        }
    }
    unsigned long busFlags = cacheArr[proIndx]->DragonProtocolProc(addr, operaType, hasCopy);
    for (int i = 0; i < num_processors; i++) {
        //        cout << proIndx << " is index, cache: " << i << endl;
        if (i != proIndx) {
            cacheArr[i]->DragonProtocolBus(busFlags, addr);
        }
    }
}
