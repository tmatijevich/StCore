/*******************************************************************************
 * File: StCore\Exit.c
 * Author: Tyler Matijevich
 * Date: 2022-02-25
*******************************************************************************/

#include "Main.h"

/* Free internal memory */
long StCoreExit(void) {
	
	/* Declare local variables */
	unsigned long allocationSize;

	SuperTrakExit();
	
	/* Free global pointers */
	if(core.pCyclicControl) {
		allocationSize = core.interface.controlSize;
		TMP_free(allocationSize, (void**)core.pCyclicControl);
	}
	
	if(core.pCyclicStatus) {
		allocationSize = core.interface.statusSize;
		TMP_free(allocationSize, (void**)core.pCyclicStatus);
	}
	
	if(core.pSimpleRelease) {
		allocationSize = sizeof(coreCommandType) * core.targetCount;
		TMP_free(allocationSize, (void**)core.pSimpleRelease);
	}
	
	if(core.pCommandBuffer) {
		allocationSize = sizeof(coreCommandBufferType) * core.palletCount;
		TMP_free(allocationSize, (void**)core.pCommandBuffer);
	}
	
	if(core.pPalletData) {
		allocationSize = sizeof(SuperTrakPalletInfo_t) * core.palletCount;
		TMP_free(allocationSize, (void**)core.pPalletData);
	}
	
	coreLog(core.ident, CORE_LOG_SEVERITY_INFO, CORE_LOGBOOK_FACILITY, 1300, "Exit", "Allocated memory is free", NULL);
	
	return 0;
	
} /* End function */
