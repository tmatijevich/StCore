/*******************************************************************************
 * File: StCoreCommand.c
 * Author: Tyler Matijevich
 * Date: 2022-02-28
*******************************************************************************/

#include "StCoreMain.h"

void StCoreRunCommand(void) {
	
	SuperTrakCommand_t *pBufferData, *pCommand;
	StCoreBufferControlType *pBufferControl;
	unsigned char *pTrigger, *pComplete, *pSuccess, complete, success;
	unsigned short i;
	
	for(i = 0; i < userPalletCount; i++) {
		
		pBufferData = pCoreCommandBuffer + stCORE_COMMANDBUFFER_SIZE * i;
		pBufferControl = pCoreBufferControl + i;
		pTrigger = pCyclicControlData + coreControlInterfaceConfig.commandTriggerOffset + i / 8;
		pCommand = pCyclicControlData + coreControlInterfaceConfig.commandDataOffset + sizeof(SuperTrakCommand_t) * i;
		pComplete = pCyclicStatusData + coreControlInterfaceConfig.commandCompleteOffset + i / 8;
		pSuccess = pCyclicStatusData + coreControlInterfaceConfig.commandSuccessOffset + i / 8;
		complete = GET_BIT(*pComplete, i % 8);
		success = GET_BIT(*pSuccess, i % 8);
		
		/* While active, wait for command to complete, then clear */
		if(pBufferControl->active) {
			if(complete) {
				memset(pCommand, 0, sizeof(SuperTrakCommand_t));
				/* Clear trigger */
				CLEAR_BIT(*pTrigger, i % 8);
				pBufferControl->active = false; /* Should also ensure SuperTrakCyclic1() gets called */
			}
		}
		else if(pBufferControl->read != pBufferControl->write || pBufferControl->full) {
				
		}
	}
	
}
