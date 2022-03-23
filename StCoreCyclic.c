/*******************************************************************************
 * File: StCoreCyclic.c
 * Author: Tyler Matijevich
 * Date: 2022-02-25
*******************************************************************************/

#include "StCoreMain.h"

long StCoreCyclic(void) {
	
	static SuperTrakControlInterface_t controlInterface;
	
	controlInterface.pControl = (unsigned long)pCoreCyclicControl;
	controlInterface.controlSize = coreControlInterface.controlSize;
	controlInterface.pStatus = (unsigned long)pCoreCyclicStatus;
	controlInterface.statusSize = coreControlInterface.statusSize;
	controlInterface.connectionType = stCONNECTION_LOCAL;
	
	coreProcessCommand();
	
	SuperTrakProcessControl(0, &controlInterface);
	SuperTrakCyclic1();
	SuperTrakProcessStatus(0, &controlInterface);
	
	return 0;
	
} /* Function definition */
