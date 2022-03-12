/*******************************************************************************
 * File: StCoreCyclic.c
 * Author: Tyler Matijevich
 * Date: 2022-02-25
*******************************************************************************/

#include "StCoreMain.h"

long StCoreCyclic(void) {
	
	static SuperTrakControlInterface_t controlInterface;
	
	controlInterface.pControl = (unsigned long)pCyclicControlData;
	controlInterface.controlSize = coreControlInterfaceConfig.controlSize;
	controlInterface.pStatus = (unsigned long)pCyclicStatusData;
	controlInterface.statusSize = coreControlInterfaceConfig.statusSize;
	controlInterface.connectionType = stCONNECTION_LOCAL;
	
	SuperTrakProcessControl(0, &controlInterface);
	SuperTrakCyclic1();
	SuperTrakProcessStatus(0, &controlInterface);
	
	return 0;
	
} /* Function definition */
