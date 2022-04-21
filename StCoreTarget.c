/*******************************************************************************
 * File: StCoreTarget.c
 * Author: Tyler Matijevich
 * Date: 2022-04-13
*******************************************************************************/

/* Headers */
#include "StCoreMain.h"

/* Function prototypes */
static void clearInstanceOutputs(StCoreTarget_typ *inst);
static void clearCommandOutputs(StCoreTarget_typ *inst);

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
	if(Target < 1 || core.targetCount < Target)
		return stCORE_ERROR_INPUT;
	
	/* Check reference */
	if(core.pCyclicStatus == NULL)
		return stCORE_ERROR_ALLOC;
	
	/**********
	 Set status
	**********/
	pTargetStatus = core.pCyclicStatus + core.interface.targetStatusOffset + 3 * Target;
	
	Status->PalletPresent = GET_BIT(*pTargetStatus, 0);
	Status->PalletInPosition = GET_BIT(*pTargetStatus, 1);
	Status->PalletPreArrival = GET_BIT(*pTargetStatus, 2);
	Status->PalletOverTarget = GET_BIT(*pTargetStatus, 3);
	Status->PalletPositionUncertain = GET_BIT(*pTargetStatus, 6);
	
	Status->PalletID = *(core.pCyclicStatus + core.interface.targetStatusOffset + 3 * Target + 1);
	
	SuperTrakServChanRead(0, stPAR_TARGET_SECTION, Target, 1, (unsigned long)&dataUInt16, sizeof(dataUInt16));
	Status->Info.Section = (unsigned char)dataUInt16;
	
	SuperTrakServChanRead(0, stPAR_TARGET_POSITION, Target, 1, (unsigned long)&dataInt32, sizeof(dataInt32));
	Status->Info.PositionUm = dataInt32;
	Status->Info.Position = ((double)dataInt32) / 1000.0; /* um to mm */
	
	/* Re-read pallet data if new scan */
	if(timestamp != AsIOTimeCyclicStart()) {
		timestamp = AsIOTimeCyclicStart();
		SuperTrakServChanRead(0, 1339, 0, core.palletCount, (unsigned long)&palletDataUInt16, sizeof(palletDataUInt16));
	}
	
	for(i = 0; i < core.palletCount; i++) {
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
	unsigned short commandInput;
	coreCommandType *pCommandEntry;
	
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
				if(inst->Target < 1 || core.targetCount < inst->Target) {
					args.i[0] = inst->Target;
					args.i[1] = core.targetCount;
					coreLogFormat(USERLOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INST), "StCoreTarget call with target %i exceeds limits [1, %i]", &args);
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
			if(core.pCyclicControl == NULL || core.pCyclicStatus == NULL) {
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
				coreLogFormat(USERLOG_SEVERITY_WARNING, 555, "StCoreTarget target select %i change to %i is ignored until re-enabled", &args);
			}
			
			/*******
			 Control
			*******/
			/* Release pallet */
			if(inst->ReleasePallet && !GET_BIT(inst->Internal.PreviousCommand, 1)) {
				/* Run */
				coreReleasePallet(inst->Internal.Select, 0, inst->Parameters.Release.Direction, inst->Parameters.Release.DestinationTarget, (void*)inst, (coreCommandType**)&inst->Internal.CommandEntry);
				/* Activate */
				inst->Internal.ActiveCommand = 1;
				inst->Busy = true;
				inst->Acknowledged = false;
			}
			
			/* Monitor active command */
			coreAssignUInt16(&commandInput, 1, inst->ReleasePallet);
			coreAssignUInt16(&commandInput, 2, inst->ReleaseTargetOffset);
			coreAssignUInt16(&commandInput, 3, inst->ReleaseIncrementalOffset);
			coreAssignUInt16(&commandInput, 4, inst->ContinueMove);
			coreAssignUInt16(&commandInput, 5, inst->SetPalletID);
			coreAssignUInt16(&commandInput, 6, inst->SetMotionParameters);
			coreAssignUInt16(&commandInput, 7, inst->SetMechanicalParameters);
			coreAssignUInt16(&commandInput, 8, inst->SetControlParameters);
			
			if(inst->Busy || inst->Acknowledged) {
				/* Check command */
				if(!GET_BIT(commandInput, inst->Internal.ActiveCommand))
					clearCommandOutputs(inst);
				
				/* Check instance */
				if(inst->Internal.CommandEntry != 0) { /* Command request shared command entry */
					if(!inst->Acknowledged) {
						pCommandEntry = (coreCommandType*)inst->Internal.CommandEntry;
						if(pCommandEntry->pInstance == inst) {
							if(GET_BIT(pCommandEntry->status, CORE_COMMAND_DONE)) {
								if(GET_BIT(pCommandEntry->status, CORE_COMMAND_ERROR)) {
									args.i[0] = inst->Internal.Select;
									coreLogFormat(USERLOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_CMDFAILURE), "StCoreTarget target %i command failure", &args);
									clearInstanceOutputs(inst);
									inst->Error = true;
									inst->StatusID = stCORE_ERROR_CMDFAILURE;
									inst->Internal.State = CORE_FUNCTION_ERROR;
									break;
								}
								else {
									inst->Busy = false;
									inst->Acknowledged = true;
								}
							}
						}
						else { /* Function block instance does not match command entry */
							clearCommandOutputs(inst);
						}
					}
				}
				
				/* De-activate */
				else if(!inst->Acknowledged)
					clearCommandOutputs(inst);
			}
			
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
	coreAssignUInt16(&inst->Internal.PreviousCommand, 1, inst->ReleasePallet);
	coreAssignUInt16(&inst->Internal.PreviousCommand, 2, inst->ReleaseTargetOffset);
	coreAssignUInt16(&inst->Internal.PreviousCommand, 3, inst->ReleaseIncrementalOffset);
	coreAssignUInt16(&inst->Internal.PreviousCommand, 4, inst->ContinueMove);
	coreAssignUInt16(&inst->Internal.PreviousCommand, 5, inst->SetPalletID);
	coreAssignUInt16(&inst->Internal.PreviousCommand, 6, inst->SetMotionParameters);
	coreAssignUInt16(&inst->Internal.PreviousCommand, 7, inst->SetMechanicalParameters);
	coreAssignUInt16(&inst->Internal.PreviousCommand, 8, inst->SetControlParameters);
	
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
	inst->PalletID = 0;
	memset(&inst->Info, 0, sizeof(inst->Info));
	inst->Busy = false;
	inst->Acknowledged = false;
	inst->Internal.Select = 0;
	inst->Internal.ActiveCommand = 0;
	inst->Internal.CommandEntry = 0;
	/* Do not clear state or previous value storage */
}

/* Assign bits of unsigned integer */
void coreAssignUInt16(unsigned short *pInteger, unsigned char bit, unsigned char value) {
	if(bit >= 16) return;
	value ? SET_BIT(*pInteger, bit) : CLEAR_BIT(*pInteger, bit);
}

/* Clear function block command data */
void clearCommandOutputs(StCoreTarget_typ *inst) {
	inst->Busy = false;
	inst->Acknowledged = false;
	inst->Internal.ActiveCommand = 0;
	inst->Internal.CommandEntry = 0;
}
