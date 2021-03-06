/*******************************************************************************
 * File: StCore\Target.c
 * Author: Tyler Matijevich
 * Date: 2022-04-13
*******************************************************************************/

#include "Main.h"
#define LOG_OBJECT "Target"

/* Prototypes */
static long logMessage(coreLogSeverityEnum severity, unsigned short code, char *message, coreFormatArgumentType *args);
static void resetOutput(StCoreTarget_typ *inst);
static void activateCommand(StCoreTarget_typ *inst, unsigned char select);
static void resetCommand(StCoreTarget_typ *inst);
static void recordInput(StCoreTarget_typ *inst, unsigned short *pData);
static void controlError(StCoreTarget_typ *inst, long status);
static void statusError(StCoreTarget_typ *inst, long status);

/* Get target status */
long StCoreTargetStatus(unsigned char Target, StCoreTargetStatusType *Status) {
	
	/************************************************
	 Dependencies:
	  Global:
	   core.pCyclicStatus
	   core.interface
	   core.targetCount
	   core.palletCount
	   core.error
	   core.statusID
	  Subroutines:
	   resetOutput
	   logMessage
	************************************************/
	
	/*********************** 
	 Declare Local Variables
	***********************/
	unsigned char *pTargetStatus;
	long i;
	static long timestamp;
	static unsigned short targetSection[CORE_TARGET_MAX], destinationTarget[CORE_PALLET_MAX];
	static long targetPosition[CORE_TARGET_MAX];
	
	/* Clear status structure */
	memset(Status, 0, sizeof(*Status));
	
	/******
	 Verify
	******/
	/* Check core */
	if(core.error)
		return core.statusID;
	
	/* Check select */
	if(Target < 1 || core.targetCount < Target)
		return stCORE_ERROR_INDEX;
	
	/* Check reference */
	if(core.pCyclicStatus == NULL)
		return stCORE_ERROR_ALLOCATION;
	
	/**********
	 Set Status
	**********/
	pTargetStatus = core.pCyclicStatus + core.interface.targetStatusOffset + 3 * Target;
	
	Status->PalletPresent = GET_BIT(*pTargetStatus, stTARGET_PALLET_PRESENT);
	Status->PalletInPosition = GET_BIT(*pTargetStatus, stTARGET_PALLET_IN_POSITION);
	Status->PalletPreArrival = GET_BIT(*pTargetStatus, stTARGET_PALLET_PRE_ARRIVAL);
	Status->PalletOverTarget = GET_BIT(*pTargetStatus, stTARGET_PALLET_OVER);
	Status->PalletPositionUncertain = GET_BIT(*pTargetStatus, stTARGET_PALLET_POS_UNCERTAIN);
	
	Status->PalletID = *(core.pCyclicStatus + core.interface.targetStatusOffset + CORE_TARGET_STATUS_BYTE_COUNT * Target + 1);
	
	/* Pull new data on new scan */
	if(timestamp != AsIOTimeCyclicStart()) {
		timestamp = AsIOTimeCyclicStart();
		SuperTrakServChanRead(0, stPAR_TARGET_SECTION, 0, core.targetCount, (unsigned long)&targetSection, sizeof(targetSection));
		SuperTrakServChanRead(0, stPAR_TARGET_POSITION, 0, core.targetCount, (unsigned long)&targetPosition, sizeof(targetPosition));
		SuperTrakServChanRead(0, 1339, 0, core.palletCount, (unsigned long)&destinationTarget, sizeof(destinationTarget));
	}
	
	/* Target section and position */
	Status->Info.Section = (unsigned char)targetSection[Target];
	Status->Info.PositionUm = targetPosition[Target];
	Status->Info.Position = ((double)targetPosition[Target]) / 1000.0; /* um to mm */
	
	/* Aggregate pallet count */
	for(i = 0; i < core.palletCount; i++) {
		if((unsigned char)destinationTarget[i] == Target) 
			Status->Info.PalletCount++;
	}
	
	return 0;
	
} /* End function */

/* Target core interface */
void StCoreTarget(StCoreTarget_typ *inst) {
	
	/************************************************
	 Dependencies:
	  Global:
	   core.targetCount
	   core.error
	   core.statusID
	  Subroutines:
	   resetOutput
	   logMessage
	************************************************/
	
	/*********************** 
	 Declare Local Variables
	***********************/
	coreFormatArgumentType args;
	long status;
	StCoreTargetStatusType targetStatus;
	unsigned short input;
	coreCommandType *pCommand;
	
	/************
	 Switch State
	************/
	/* Interrupt if disabled */
	if(inst->Enable == false)
		inst->Internal.State = CORE_FUNCTION_DISABLED;
	
	switch(inst->Internal.State) {
		case CORE_FUNCTION_DISABLED:
			resetOutput(inst);
			
			/* Wait for enable */
			if(inst->Enable) {
				/* Check index */
				/* The number of targets are counted during initialization */
				if(inst->Target < 1 || core.targetCount < inst->Target) {
					args.i[0] = inst->Target;
					args.i[1] = core.targetCount;
					logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INDEX), "StCoreTarget target %i index exceeds initial count [1, %i]", &args);
					inst->Error = true;
					inst->StatusID = stCORE_ERROR_INDEX;
					inst->Internal.State = CORE_FUNCTION_ERROR;
				}
				else {
					inst->Internal.Select = inst->Target; /* Latch the selected target */
					inst->Internal.State = CORE_FUNCTION_EXECUTING;
				}
			}
			
			break;
			
		case CORE_FUNCTION_EXECUTING:
			/* Cyclic error checks */
			if(core.error) {
				args.i[0] = inst->Internal.Select;
				logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(core.statusID), "Cannot execute StCoreTarget target %i due to critical error in StCore", &args);
				resetOutput(inst);
				inst->Error = true;
				inst->StatusID = core.statusID;
				inst->Internal.State = CORE_FUNCTION_ERROR;
				break;
			}
			
			/* Check select changes */
			if(inst->Target != inst->Internal.Select && inst->Target != inst->Internal.PreviousSelect && !inst->StatusID) {
				args.i[0] = inst->Internal.Select;
				args.i[1] = inst->Target;
				logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_WARNING_INDEX), "StCoreTarget target %i index change to %i is ignored until re-enabled", &args);
				inst->StatusID = stCORE_WARNING_INDEX;
			}
			
			/*******
			 Control
			*******/
			/* Run configuration commands before release commands */
			/* Set pallet ID */
			if(inst->SetPalletID && !GET_BIT(inst->Internal.PreviousCommand, CORE_COMMAND_ID)) {
				/* Run and activate */
				status = coreSetPalletID(inst->Internal.Select, inst->Parameters.PalletID, (void*)inst, (coreCommandType**)&inst->Internal.pCommand);
				if(status) { 
					controlError(inst, status);
					break;
				}
				else
					activateCommand(inst, CORE_COMMAND_ID);
			}
			
			/* Set motion parameters */
			if(inst->SetMotionParameters && !GET_BIT(inst->Internal.PreviousCommand, CORE_COMMAND_MOTION)) {
				/* Run and activate */
				status = coreSetMotionParameters(inst->Internal.Select, 0, inst->Parameters.Motion.Velocity, inst->Parameters.Motion.Acceleration, (void*)inst, (coreCommandType**)&inst->Internal.pCommand);
				if(status) { 
					controlError(inst, status);
					break;
				}
				else
					activateCommand(inst, CORE_COMMAND_MOTION);
			}
			
			/* Set mechanical parameters */
			if(inst->SetMechanicalParameters && !GET_BIT(inst->Internal.PreviousCommand, CORE_COMMAND_MECHANICAL)) {
				/* Run and activate */
				status = coreSetMechanicalParameters(inst->Internal.Select, 0, inst->Parameters.Mechanical.ShelfWidth, inst->Parameters.Mechanical.CenterOffset, (void*)inst, (coreCommandType**)&inst->Internal.pCommand);
				if(status) { 
					controlError(inst, status);
					break;
				}
				else
					activateCommand(inst, CORE_COMMAND_MECHANICAL);
			}
			
			/* Set mechanical parameters */
			if(inst->SetControlParameters && !GET_BIT(inst->Internal.PreviousCommand, CORE_COMMAND_CONTROL)) {
				/* Run and activate */
				status = coreSetControlParameters(inst->Internal.Select, 0, inst->Parameters.Control.ControlGainSet, inst->Parameters.Control.MovingFilter, inst->Parameters.Control.StationaryFilter, (void*)inst, (coreCommandType**)&inst->Internal.pCommand);
				if(status) { 
					controlError(inst, status);
					break;
				}
				else
					activateCommand(inst, CORE_COMMAND_CONTROL);
			}
			
			/* Release pallet */
			if(inst->ReleasePallet && !GET_BIT(inst->Internal.PreviousCommand, CORE_COMMAND_RELEASE)) {
				/* Run and activate */
				status = coreReleasePallet(inst->Internal.Select, 0, inst->Parameters.Release.Direction, inst->Parameters.Release.DestinationTarget, (void*)inst, (coreCommandType**)&inst->Internal.pCommand);
				if(status) { 
					controlError(inst, status);
					break;
				}
				else
					activateCommand(inst, CORE_COMMAND_RELEASE);
			}
			
			/* Release target offset */
			if(inst->ReleaseTargetOffset && !GET_BIT(inst->Internal.PreviousCommand, CORE_COMMAND_OFFSET)) {
				/* Run and activate */
				status = coreReleaseTargetOffset(inst->Internal.Select, 0, inst->Parameters.Release.Direction, inst->Parameters.Release.DestinationTarget, inst->Parameters.Release.Offset, (void*)inst, (coreCommandType**)&inst->Internal.pCommand);
				if(status) { 
					controlError(inst, status);
					break;
				}
				else
					activateCommand(inst, CORE_COMMAND_OFFSET);
			}
			
			/* Release incremental offset */
			if(inst->ReleaseIncrementalOffset && !GET_BIT(inst->Internal.PreviousCommand, CORE_COMMAND_INCREMENT)) {
				/* Run and activate */
				status = coreReleaseIncrementalOffset(inst->Internal.Select, 0, inst->Parameters.Release.Offset, (void*)inst, (coreCommandType**)&inst->Internal.pCommand);
				if(status) { 
					controlError(inst, status);
					break;
				}
				else
					activateCommand(inst, CORE_COMMAND_INCREMENT);
			}
			
			/* Continue move */
			if(inst->ContinueMove && !GET_BIT(inst->Internal.PreviousCommand, CORE_COMMAND_CONTINUE)) {
				/* Run and activate */
				status = coreContinueMove(inst->Internal.Select, 0, (void*)inst, (coreCommandType**)&inst->Internal.pCommand);
				if(status) { 
					controlError(inst, status);
					break;
				}
				else
					activateCommand(inst, CORE_COMMAND_CONTINUE);
			}
			
			/* Record command inputs */
			recordInput(inst, &input);
			
			/* Monitor active command */
			if(inst->Busy || inst->Acknowledged) {
				/* Clear if user resets input */
				if(!GET_BIT(input, inst->Internal.CommandSelect))
					resetCommand(inst);
				
				/* Monitor buffered command address */
				else if(inst->Internal.pCommand != 0) { 
					if(!inst->Acknowledged) { /* Command request acknowledged */
						pCommand = (coreCommandType*)inst->Internal.pCommand;
						if(pCommand->pInstance == inst) { /* Buffered command matches this instance */
							if(GET_BIT(pCommand->status, CORE_COMMAND_DONE)) {
								if(GET_BIT(pCommand->status, CORE_COMMAND_ERROR)) {
									args.i[0] = inst->Internal.Select;
									logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_COMMAND), "StCoreTarget target %i command error", &args);
									resetOutput(inst);
									inst->Error = true;
									inst->StatusID = stCORE_ERROR_COMMAND;
									inst->Internal.State = CORE_FUNCTION_ERROR;
									break;
								}
								else {
									/* Report acknowledged to user */
									inst->Busy = false;
									inst->Acknowledged = true;
									inst->Internal.pCommand = 0; /* No further need to monitor buffered command address */
								}
							}
						}
						else { 
							/* Error if the instance tracking does not match */
							args.i[0] = inst->Internal.Select;
							logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_ACKNOWLEDGE), "StCoreTarget target %i command cannot be acknowledged", &args);
							resetOutput(inst);
							inst->Error = true;
							inst->StatusID = stCORE_ERROR_ACKNOWLEDGE;
							inst->Internal.State = CORE_FUNCTION_ERROR;
							break;
							
						} /* Instance matches buffered command? */
					} /* Command request acknowledged? */
					
					/* Wait here while acknowledged for user command to reset */
					
				} /* Non-zero buffered command address? */
				
				/* Error if buffered command address was not shared */
				else if(!inst->Acknowledged) {
					args.i[0] = inst->Internal.Select;
					logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_ACKNOWLEDGE), "StCoreTarget target %i command cannot be acknowledged", &args);
					resetOutput(inst);
					inst->Error = true;
					inst->StatusID = stCORE_ERROR_ACKNOWLEDGE;
					inst->Internal.State = CORE_FUNCTION_ERROR;
					break;
				}
			}
			
			/******
			 Status
			******/
			/* Call target status subroutine */
			status = StCoreTargetStatus(inst->Internal.Select, &targetStatus);
			if(status) {
				statusError(inst, status);
				break;
			}
			
			/* Map target status data to function block */
			inst->PalletPresent = targetStatus.PalletPresent;
			inst->PalletInPosition = targetStatus.PalletInPosition;
			inst->PalletPreArrival = targetStatus.PalletPreArrival;
			inst->PalletOverTarget = targetStatus.PalletOverTarget;
			inst->PalletPositionUncertain = targetStatus.PalletPositionUncertain;
			inst->PalletID = targetStatus.PalletID;
			
			memcpy(&inst->Info, &targetStatus.Info, sizeof(inst->Info));
			
			/* Allow warning reset */
			if(inst->ErrorReset && !inst->Internal.PreviousErrorReset)
				inst->StatusID = 0;
				
			/* Report valid */
			inst->Valid = true;
			
			break;
			
		default:
			/* Set the error if state is changed externally */
			inst->Error = true;
			
			/* Wait for error reset */
			if(inst->ErrorReset && !inst->Internal.PreviousErrorReset) {
				/* The function block is enabled otherwise the state would be disabled (interrupt) */
				/* Maintain the select index when originally enabled */
				resetOutput(inst);
				inst->Internal.State = CORE_FUNCTION_EXECUTING;
			}
			
			break;
	}
	
	inst->Internal.PreviousSelect = inst->Target;
	inst->Internal.PreviousErrorReset = inst->ErrorReset;
	recordInput(inst, &inst->Internal.PreviousCommand);
	
} /* End function */

/* Create local logging function */
long logMessage(coreLogSeverityEnum severity, unsigned short code, char *message, coreFormatArgumentType *args) {
	return coreLog(core.ident, severity, CORE_LOGBOOK_FACILITY, code, LOG_OBJECT, message, args);
}

/* Clear all function block outputs */
void resetOutput(StCoreTarget_typ *inst) {
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
	inst->Internal.CommandSelect = 0;
	inst->Internal.pCommand = 0;
	/* Do not clear state or previous value storage */
}

/* Assign bits of unsigned integer */
void coreAssign16(unsigned short *pInteger, unsigned char bit, unsigned char value) {
	if(bit >= 16) return;
	value ? SET_BIT(*pInteger, bit) : CLEAR_BIT(*pInteger, bit);
}

/* Clear function block command data */
void resetCommand(StCoreTarget_typ *inst) {
	inst->Busy = false;
	inst->Acknowledged = false;
	inst->Internal.CommandSelect = 0;
	inst->Internal.pCommand = 0;
}

/* Set command selection and statuses when activated */
void activateCommand(StCoreTarget_typ *inst, unsigned char select) {
	inst->Internal.CommandSelect = select;
	inst->Busy = true;
	inst->Acknowledged = false;
}

/* Record bits from user inputs */
void recordInput(StCoreTarget_typ *inst, unsigned short *pData) {
	coreAssign16(pData, CORE_COMMAND_RELEASE, inst->ReleasePallet);
	coreAssign16(pData, CORE_COMMAND_OFFSET, inst->ReleaseTargetOffset);
	coreAssign16(pData, CORE_COMMAND_INCREMENT, inst->ReleaseIncrementalOffset);
	coreAssign16(pData, CORE_COMMAND_CONTINUE, inst->ContinueMove);
	coreAssign16(pData, CORE_COMMAND_ID, inst->SetPalletID);
	coreAssign16(pData, CORE_COMMAND_MOTION, inst->SetMotionParameters);
	coreAssign16(pData, CORE_COMMAND_MECHANICAL, inst->SetMechanicalParameters);
	coreAssign16(pData, CORE_COMMAND_CONTROL, inst->SetControlParameters);
}

/* Activate error if command request returns error */
void controlError(StCoreTarget_typ *inst, long status) {
	/* Declare local variables */
	coreFormatArgumentType args;
	
	args.i[0] = inst->Internal.Select;
	logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(status), "StCoreTarget target %i received an error from command request", &args);
	resetOutput(inst);
	inst->Error = true;
	inst->StatusID = status;
	inst->Internal.State = CORE_FUNCTION_ERROR;
}

/* Activate error if status request returns error */
void statusError(StCoreTarget_typ *inst, long status) {
	/* Declare local variables */
	coreFormatArgumentType args;
	
	args.i[0] = inst->Internal.Select;
	logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(status), "StCoreTarget target %i received an error from status read", &args);
	resetOutput(inst);
	inst->Error = true;
	inst->StatusID = status;
	inst->Internal.State = CORE_FUNCTION_ERROR;
}
