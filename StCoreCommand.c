/*******************************************************************************
 * File: StCoreCommand.c
 * Author: Tyler Matijevich
 * Date: 2022-02-28
*******************************************************************************/

#include "StCoreMain.h"

/* Command target or pallet to release to target */
long StCoreReleaseToTarget(unsigned char Target, unsigned char Pallet, unsigned short Direction, unsigned char DestinationTarget) {
	
	/***********************
	 Declare local variables
	***********************/
	unsigned char index, commandID, context, *pPalletPresent;
	SuperTrakCommand_t *pCommand;
	coreBufferControlType *pBuffer;
	FormatStringArgumentsType args;
	
	/* Check references */
	if(pCoreCyclicStatus == NULL || pCoreCommandBuffer == NULL || pCoreBufferControl == NULL) {
		coreLogMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_ALLOC), "StCoreReleaseToTarget is unable to reference cyclic data or command buffers");
		return stCORE_ERROR_ALLOC;
	}
	
	/**************
	 Create command
	**************/
	if(Target != 0) {
		if(Target > coreTargetCount) {
			args.i[0] = Target;
			args.i[1] = coreTargetCount;
			coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_INPUT), "Target %i context exceeds count [1, %i] of release to target command", &args);
			return stCORE_ERROR_INPUT;
		}
					
		pPalletPresent = pCoreCyclicStatus + coreInterfaceConfig.targetStatusOffset + 3 * Target + 1;
		if(*pPalletPresent < 1 || corePalletCount < *pPalletPresent) {
			args.i[0] = *pPalletPresent;
			args.i[1] = Target;
			args.i[2] = corePalletCount;
			coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_INPUT), "Pallet %i present at target %i exceeds count [1, %i] of release to target command", &args);
			return stCORE_ERROR_INPUT;
		}
		
		index = *pPalletPresent;
		context = Target;
		if(Direction == stDIRECTION_RIGHT)
			commandID = 17;
		else
			commandID = 16;
	}
	else {
		if(Pallet < 1 || corePalletCount < Pallet) {
			args.i[0] = Pallet;
			args.i[1] = corePalletCount;
			coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_INPUT), "Pallet %i context exceeds count [1, %i] of release to target command", &args);
			return stCORE_ERROR_INPUT;
		}
		
		index = Pallet;
		context = Pallet;
		if(Direction == stDIRECTION_RIGHT)
			commandID = 19;
		else
			commandID = 18;
	}
	
	/*********************
	 Access command buffer
	*********************/
	pBuffer = pCoreBufferControl + index - 1;
	pCommand = pCoreCommandBuffer + CORE_COMMANDBUFFER_SIZE * (index - 1) + pBuffer->write;
	
	if(Target) {
		coreStringCopy(args.s[0], "Target", sizeof(args.s[0]));
		args.i[0] = Target;
	}
	else {
		coreStringCopy(args.s[0], "Pallet", sizeof(args.s[0]));
		args.i[0] = Pallet;
	}
	if(Direction == stDIRECTION_RIGHT) coreStringCopy(args.s[1], "right", sizeof(args.s[1]));
	else coreStringCopy(args.s[1], "left", sizeof(args.s[1]));
	args.i[1] = DestinationTarget;
	
	if(pBuffer->full) {
		coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_BUFFER), "Release to target command of context %s %i rejected by full buffer", &args);
		return stCORE_ERROR_BUFFER;
	}
	
	pCommand->u1[0] = commandID;
	pCommand->u1[1] = context;
	pCommand->u1[2] = DestinationTarget;
	
	coreLogFormatMessage(USERLOG_SEVERITY_DEBUG, 5200, "%s %i release to target %s to destination target %i", &args);
	
	pBuffer->write = (pBuffer->write + 1) % CORE_COMMANDBUFFER_SIZE;
	if(pBuffer->write == pBuffer->read) {
		pBuffer->full = true;
		args.i[0] = index;
		args.i[1] = CORE_COMMANDBUFFER_SIZE;
		coreLogFormatMessage(USERLOG_SEVERITY_WARNING, coreEventCode(stCORE_WARNING_BUFFER), "Pallet %i commad buffer is full (%i)", &args);
	}
	
	return 0;
	
}

void coreProcessCommand(void) {
	
	SuperTrakCommand_t *pBufferData, *pCommand;
	coreBufferControlType *pBufferControl;
	unsigned char *pTrigger, *pComplete, *pSuccess, complete, success;
	unsigned short i;
	char format[CORE_FORMAT_SIZE + 1];
	FormatStringArgumentsType args;
	
	if(pCoreCommandBuffer == NULL || pCoreBufferControl == NULL || pCoreCyclicControl == NULL || pCoreCyclicStatus == NULL)
		return;
	
	for(i = 0; i < corePalletCount; i++) {
		
		pBufferData = pCoreCommandBuffer + CORE_COMMANDBUFFER_SIZE * i;
		pBufferControl = pCoreBufferControl + i;
		pTrigger = pCoreCyclicControl + coreInterfaceConfig.commandTriggerOffset + i / 8;
		pCommand = pCoreCyclicControl + coreInterfaceConfig.commandDataOffset + sizeof(SuperTrakCommand_t) * i;
		pComplete = pCoreCyclicStatus + coreInterfaceConfig.commandCompleteOffset + i / 8;
		pSuccess = pCoreCyclicStatus + coreInterfaceConfig.commandSuccessOffset + i / 8;
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
			pBufferControl->read = (pBufferControl->read + 1) % CORE_COMMANDBUFFER_SIZE;
			/* Reset full flag */
			pBufferControl->full = false;
			/* Set the trigger and activate */
			SET_BIT(*pTrigger, i % 8);
			pBufferControl->active = true;
			
			/* Write to logger */
			format[CORE_FORMAT_SIZE] = '\0';
			if(pCommand->u1[0] == 16 || pCommand->u1[0] == 17)
				strncpy(format, "Target %i release to target direction=%s destination=T%i", CORE_FORMAT_SIZE);
			else if(pCommand->u1[0] == 18 || pCommand->u1[0] == 19)
				strncpy(format, "Pallet %i release to target direction=%s destination=T%i", CORE_FORMAT_SIZE);
			// Target %i release to offset direction=%s destination=T%i offset=%ium
			if(pCommand->u1[0] == 16 || pCommand->u1[0] == 18)
				strncpy(args.s[0], "left", IECSTRING_FORMATARGS_LEN);
			else
				strncpy(args.s[0], "right", IECSTRING_FORMATARGS_LEN);
				
			args.i[0] = pCommand->u1[1];
			args.i[1] = pCommand->u1[2];
			
			coreLogFormatMessage(USERLOG_SEVERITY_DEBUG, 5000, format, &args);
			
		} /* Active? */
	} /* Loop pallets */
	
}
