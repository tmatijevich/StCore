/*******************************************************************************
 * File: StCoreCyclic.c
 * Author: Tyler Matijevich
 * Date: 2022-02-25
*******************************************************************************/

#include "StCoreMain.h"

/* Process SuperTrak control interface */
long StCoreCyclic(void) {
	
	/***********************
	 Declare local variables
	***********************/
	static SuperTrakControlInterface_t controlInterface;
	static unsigned char firstLogAllocation = true;
	
	/**********
	 Check eror
	**********/
	/* Do not run anything if core error */
	if(coreError)
		return coreStatusID;
	
	/***********************
	 Process StCore commands
	***********************/
	coreProcessCommand();
	
	/*****************************
	 Process SuperTrak cyclic data
	*****************************/
	/* Guard pointers */
	if(pCoreCyclicControl == NULL || pCoreCyclicStatus == NULL) {
		if(firstLogAllocation) 
			coreLogMessage(USERLOG_SEVERITY_CRITICAL, coreEventCode(stCORE_ERROR_ALLOC), "StCoreCyclic() cannot reference cyclic control or status data due to null pointer");
		firstLogAllocation = false;
		coreError = true;
		return coreStatusID = stCORE_ERROR_ALLOC;
	}
	firstLogAllocation = true;
	
	controlInterface.pControl = (unsigned long)pCoreCyclicControl;
	controlInterface.controlSize = coreControlInterface.controlSize;
	controlInterface.pStatus = (unsigned long)pCoreCyclicStatus;
	controlInterface.statusSize = coreControlInterface.statusSize;
	controlInterface.connectionType = stCONNECTION_LOCAL;
	
	SuperTrakProcessControl(0, &controlInterface);
	SuperTrakCyclic1();
	SuperTrakProcessStatus(0, &controlInterface);
	
	return 0;
	
} /* Function definition */
