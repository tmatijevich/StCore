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
		allocationSize = sizeof(coreSimpleTargetReleaseType) * core.targetCount;
		TMP_free(allocationSize, (void**)core.pSimpleRelease);
	}
	
	if(core.pCommandBuffer) {
		allocationSize = sizeof(SuperTrakCommand_t) * CORE_COMMAND_BUFFER_SIZE * core.palletCount;
		TMP_free(allocationSize, (void**)core.pCommandBuffer);
	}
	
	return 0;
	
} /* Function definition */
