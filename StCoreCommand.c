/*******************************************************************************
 * File: StCoreCommand.c
 * Author: Tyler Matijevich
 * Date: 2022-02-28
*******************************************************************************/

#include "StCoreMain.h"
#define FORMAT_SIZE 125

long StCoreReleaseToTarget(unsigned char target, unsigned char palletID, unsigned short direction, unsigned char destinationTarget) {
	
	unsigned char index, context, commandID, *pTargetPalletID, *pCommand;
	StCoreBufferControlType *pBufferControl;
	char format[FORMAT_SIZE + 1];
	FormatStringArgumentsType args;
	
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
	if(pBufferControl->write == pBufferControl->read) {
		pBufferControl->full = true;
		/* Write to logger */
		format[FORMAT_SIZE] = '\0';
		strncpy(format, "Pallet %i command buffer full (size=%i)", FORMAT_SIZE);
		args.i[0] = index;
		args.i[1] = stCORE_COMMANDBUFFER_SIZE;
		StCoreFormatMessage(USERLOG_SEVERITY_WARNING, 5000, format, &args);
	}
	
}

void StCoreRunCommand(void) {
	
	SuperTrakCommand_t *pBufferData, *pCommand;
	StCoreBufferControlType *pBufferControl;
	unsigned char *pTrigger, *pComplete, *pSuccess, complete, success;
	unsigned short i;
	char format[FORMAT_SIZE + 1];
	FormatStringArgumentsType args;
	
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
			
			/* Write to logger */
			format[FORMAT_SIZE] = '\0';
			if(pCommand->u1[0] == 16 || pCommand->u1[0] == 17)
				strncpy(format, "Target %i release to target direction=%s destination=T%i", FORMAT_SIZE);
			else if(pCommand->u1[0] == 18 || pCommand->u1[0] == 19)
				strncpy(format, "Pallet %i release to target direction=%s destination=T%i", FORMAT_SIZE);
			// Target %i release to offset direction=%s destination=T%i offset=%ium
			if(pCommand->u1[0] == 16 || pCommand->u1[0] == 18)
				strncpy(args.s[0], "left", IECSTRING_FORMATARGS_LEN);
			else
				strncpy(args.s[0], "right", IECSTRING_FORMATARGS_LEN);
				
			args.i[0] = pCommand->u1[1];
			args.i[1] = pCommand->u1[2];
			
			StCoreFormatMessage(USERLOG_SEVERITY_DEBUG, 5000, format, &args);
			
		} /* Active? */
	} /* Loop pallets */
	
}
