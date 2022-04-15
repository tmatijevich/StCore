/*******************************************************************************
 * File: StCoreTarget.c
 * Author: Tyler Matijevich
 * Date: 2022-04-13
*******************************************************************************/

/* Headers */
#include "StCoreMain.h"

/* Function prototypes */
static void clearInstanceOutputs(StCoreTarget_typ *inst);

/* Get target status */
long StCoreTargetStatus(unsigned char Target, StCoreTargetStatusType *Status) {
	
	/*********************** 
	 Declare local variables
	***********************/
	unsigned char *pTargetStatus;
	unsigned short dataUInt16;
	long dataInt32, i;
	static long timestamp;
	static unsigned short palletDataUInt16[256];
	
	/* Clear status structure */
	memset(Status, 0, sizeof(*Status));
	
	/* Check select */
	if(Target < 1 || coreTargetCount < Target)
		return stCORE_ERROR_INPUT;
	
	/* Check reference */
	if(pCoreCyclicStatus == NULL)
		return stCORE_ERROR_ALLOC;
	
	/**********
	 Set status
	**********/
	pTargetStatus = pCoreCyclicStatus + coreInterfaceConfig.targetStatusOffset + 3 * Target;
	
	Status->PalletPresent = GET_BIT(*pTargetStatus, 0);
	Status->PalletInPosition = GET_BIT(*pTargetStatus, 1);
	Status->PalletPreArrival = GET_BIT(*pTargetStatus, 2);
	Status->PalletOverTarget = GET_BIT(*pTargetStatus, 3);
	Status->PalletPositionUncertain = GET_BIT(*pTargetStatus, 6);
	
	Status->PalletID = *(pCoreCyclicStatus + coreInterfaceConfig.targetStatusOffset + 3 * Target + 1);
	
	SuperTrakServChanRead(0, stPAR_TARGET_SECTION, Target, 1, (unsigned long)&dataUInt16, sizeof(dataUInt16));
	Status->Info.Section = (unsigned char)dataUInt16;
	
	SuperTrakServChanRead(0, stPAR_TARGET_POSITION, Target, 1, (unsigned long)&dataInt32, sizeof(dataInt32));
	Status->Info.PositionUm = dataInt32;
	Status->Info.Position = ((double)dataInt32) / 1000.0; /* um to mm */
	
	/* Re-read pallet data if new scan */
	if(timestamp != AsIOTimeCyclicStart()) {
		timestamp = AsIOTimeCyclicStart();
		SuperTrakServChanRead(0, 1339, 0, corePalletCount, (unsigned long)&palletDataUInt16, sizeof(palletDataUInt16));
	}
	
	for(i = 0; i < corePalletCount; i++) {
		if((unsigned char)palletDataUInt16[i] == Target) 
			Status->Info.PalletCount++;
	}
	
	return 0;
	
} /* End function */

/* Target core interface */
void StCoreTarget(StCoreTarget_typ *inst) {
	
	/*********************** 
	 Declare local variables
	***********************/
	FormatStringArgumentsType args;
	StCoreTargetStatusType targetStatus;
	
	/************
	 Switch state
	************/
	/* Interrupt if disabled */
	if(inst->Enable == false)
		inst->Internal.State = CORE_FUNCTION_DISABLED;
	
	switch(inst->Internal.State) {
		case CORE_FUNCTION_DISABLED:
			clearInstanceOutputs(inst);
			
			/* Wait for enable */
			if(inst->Enable) {
				/* Check select */
				if(inst->Target < 1 || coreTargetCount < inst->Target) {
					args.i[0] = inst->Target;
					args.i[1] = coreTargetCount;
					coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_INST), "StCoreTarget call with target %i exceeds limits [1, %i]", &args);
					inst->Error = true;
					inst->StatusID = stCORE_ERROR_INST;
					inst->Internal.State = CORE_FUNCTION_ERROR;
				}
				else {
					inst->Internal.Select = inst->Target; /* Latch the selected target */
					inst->Internal.State = CORE_FUNCTION_EXECUTING;
				}
			}
			
			break;
			
		case CORE_FUNCTION_EXECUTING:
			/* Check references */
			if(pCoreCyclicControl == NULL || pCoreCyclicStatus == NULL) {
				/* Do not spam the logger, StCoreSystem logs this */
				clearInstanceOutputs(inst);
				inst->Error = true;
				inst->StatusID = stCORE_ERROR_ALLOC;
				inst->Internal.State = CORE_FUNCTION_ERROR;
				break;
			}
			
			/* Check select changes */
			if(inst->Target != inst->Internal.Select && inst->Target != inst->Internal.PreviousSelect) {
				args.i[0] = inst->Internal.Select;
				args.i[1] = inst->Target;
				coreLogFormatMessage(USERLOG_SEVERITY_WARNING, 555, "StCoreTarget target select %i change to %i is ignored until re-enabled", &args);
			}
			
			/*******
			 Control
			*******/
			
			/******
			 Status
			******/
			/* Call target status subroutine */
			StCoreTargetStatus(inst->Internal.Select, &targetStatus);
			
			/* Map target status data to function block */
			inst->PalletPresent = targetStatus.PalletPresent;
			inst->PalletInPosition = targetStatus.PalletInPosition;
			inst->PalletPreArrival = targetStatus.PalletPreArrival;
			inst->PalletOverTarget = targetStatus.PalletOverTarget;
			inst->PalletPositionUncertain = targetStatus.PalletPositionUncertain;
			inst->PalletID = targetStatus.PalletID;
			
			memcpy(&inst->Info, &targetStatus.Info, sizeof(inst->Info));
			
			inst->Valid = true;
			
			break;
			
		default:
			/* Set the error if state is changed externally */
			inst->Error = true;
			
			/* Wait for error reset */
			if(inst->ErrorReset && !inst->Internal.PreviousErrorReset) {
				clearInstanceOutputs(inst);
				inst->Internal.State = CORE_FUNCTION_EXECUTING;
			}
			
			break;
	}
	
	inst->Internal.PreviousSelect = inst->Target;
	inst->Internal.PreviousErrorReset = inst->ErrorReset;
	
} /* End function */

/* Clear all function block outputs */
void clearInstanceOutputs(StCoreTarget_typ *inst) {
	inst->Valid = false;
	inst->Error = false;
	inst->StatusID = 0;
	inst->PalletPresent = false;
	inst->PalletInPosition = false;
	inst->PalletPreArrival = false;
	inst->PalletOverTarget = false;
	inst->PalletPositionUncertain = false;
}
