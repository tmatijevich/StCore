/*******************************************************************************
 * File: StCoreCommand.c
 * Author: Tyler Matijevich
 * Date: 2022-02-28
*******************************************************************************/

#include "StCoreMain.h"

long StCoreReleaseToTarget(unsigned char target, unsigned char palletID, unsigned short direction, unsigned char destinationTarget) {
	
	unsigned char index, context, commandID, *pTargetPalletID, *pCommand;
	StCoreBufferControlType *pBufferControl;
	
	if(target != 0) {
		if(target > coreInitialTargetCount)
			return -1;
		
		if(pCyclicStatusData == NULL)
			return -1;
					
		pTargetPalletID = pCyclicStatusData + coreControlInterfaceConfig.targetStatusOffset + 3 * target + 1;
		if(*pTargetPalletID < 1 || userPalletCount < *pTargetPalletID)
			return -1;
		
		index = *pTargetPalletID;
		context = target;
		if(direction == stDIRECTION_LEFT)
			commandID = 16;
		else
			commandID = 17;
	}
	else {
		if(palletID < 1 || userPalletCount < palletID)
			return -1;
		
		index = palletID;
		context = palletID;
		if(direction == stDIRECTION_LEFT)
			commandID = 18;
		else
			commandID = 19;
	}
	
	pBufferControl = pCoreBufferControl + index - 1;
	pCommand = (unsigned char *)(pCoreCommandBuffer + stCORE_COMMANDBUFFER_SIZE * (index - 1) + pBufferControl->write);
	
	if(pBufferControl->full)
		return -1;
	
	pCommand[0] = commandID;
	pCommand[1] = context;
	pCommand[2] = destinationTarget;
	
	pBufferControl->write = (pBufferControl->write + 1) % stCORE_COMMANDBUFFER_SIZE;
	if(pBufferControl->write == pBufferControl->read)
		pBufferControl->full = true;
	
}

void StCoreRunCommand(void) {
	
	SuperTrakCommand_t *pBufferData, *pCommand;
	StCoreBufferControlType *pBufferControl;
	unsigned char *pTrigger, *pComplete, *pSuccess, complete, success;
	unsigned short i;
	
	if(pCoreCommandBuffer == NULL || pCoreBufferControl == NULL || pCyclicControlData == NULL || pCyclicStatusData == NULL)
		return;
	
	for(i = 0; i < userPalletCount; i++) {
		
		pBufferData = pCoreCommandBuffer + stCORE_COMMANDBUFFER_SIZE * i;
		pBufferControl = pCoreBufferControl + i;
		pTrigger = pCyclicControlData + coreControlInterfaceConfig.commandTriggerOffset + i / 8;
		pCommand = pCyclicControlData + coreControlInterfaceConfig.commandDataOffset + sizeof(SuperTrakCommand_t) * i;
		pComplete = pCyclicStatusData + coreControlInterfaceConfig.commandCompleteOffset + i / 8;
		pSuccess = pCyclicStatusData + coreControlInterfaceConfig.commandSuccessOffset + i / 8;
		complete = GET_BIT(*pComplete, i % 8);
		success = GET_BIT(*pSuccess, i % 8);
		
		if(pBufferControl->active) {
			/* While active, wait for command to complete, then clear */
			if(complete) {
				memset(pCommand, 0, sizeof(SuperTrakCommand_t));
				/* Clear trigger */
				CLEAR_BIT(*pTrigger, i % 8);
				pBufferControl->active = false; /* Should also ensure SuperTrakCyclic1() gets called */
			}
		}
		else if(pBufferControl->read != pBufferControl->write || pBufferControl->full) {
			/* Pull command off buffer at the read index */
			memcpy(pCommand, pBufferData + pBufferControl->read, sizeof(SuperTrakCommand_t));
			/* Update read index */
			pBufferControl->read = (pBufferControl->read + 1) % stCORE_COMMANDBUFFER_SIZE;
			/* Reset full flag */
			pBufferControl->full = false;
			/* Set the trigger and activate */
			SET_BIT(*pTrigger, i % 8);
			pBufferControl->active = true;
		} /* Active? */
	} /* Loop pallets */
	
}
