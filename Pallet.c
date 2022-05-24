/*******************************************************************************
 * File: StCore\Pallet.c
 * Author: Tyler Matijevich
 * Date: 2022-05-19
*******************************************************************************/

#include "Main.h"
#define LOG_OBJECT "Pallet"

/* Prototypes */
static long logMessage(coreLogSeverityEnum severity, unsigned short code, char *message, coreFormatArgumentType *args);
static void resetOutput(StCorePallet_typ *inst);
static void activateCommand(StCorePallet_typ *inst, unsigned char select);
static void resetCommand(StCorePallet_typ *inst);
static void recordInput(StCorePallet_typ *inst, unsigned short *pData);

/* Get pallet status */
long StCorePalletStatus(unsigned char Pallet, StCorePalletStatusType *Status) {
	
	/*********************** 
	 Declare Local Variables
	***********************/
	SuperTrakPalletInfo_t *pPalletData;
	static long timestamp;
	static short velocity[UCHAR_MAX];
	static unsigned short destination[UCHAR_MAX];
	static unsigned short setSection[UCHAR_MAX];
	static long setPosition[UCHAR_MAX];
	static float setVelocity[UCHAR_MAX];
	static float setAcceleration[UCHAR_MAX];
	
	/*****
	 Clear
	*****/
	memset(Status, 0, sizeof(*Status));
	
	/******
	 Verify
	******/
	/* Check core */
	if(core.error)
		return stCORE_ERROR_CRITICAL;
	
	/* Check reference */
	if(core.pPalletData == NULL)
		return stCORE_ERROR_ALLOC;
		
	/* Check user input */
	if(core.palletMap[Pallet] == -1)
		return stCORE_ERROR_PALLET;
		
	/******
	 Status
	******/
	/* Access the pallet information structure */
	pPalletData = core.pPalletData + core.palletMap[Pallet];
	
	/* Copy data */
	Status->Present = GET_BIT(pPalletData->status, stPALLET_PRESENT);
	Status->Recovering = GET_BIT(pPalletData->status, stPALLET_RECOVERING);
	Status->AtTarget = GET_BIT(pPalletData->status, stPALLET_AT_TARGET);
	Status->InPosition = GET_BIT(pPalletData->status, stPALLET_IN_POSITION);
	Status->ServoEnabled = GET_BIT(pPalletData->status, stPALLET_SERVO_ENABLED);
	Status->Initializing = GET_BIT(pPalletData->status, stPALLET_INITIALIZING);
	Status->Lost = GET_BIT(pPalletData->status, stPALLET_LOST);
	
	Status->Section = pPalletData->section;
	Status->Position = ((double)pPalletData->position) / 1000.0;
	Status->Info.PositionUm = pPalletData->position;
	Status->Info.ControlMode = pPalletData->controlMode;
	
	/* Wait for new scan to read parameters */
	if(timestamp != AsIOTimeCyclicStart()) {
		timestamp = AsIOTimeCyclicStart();
		SuperTrakServChanRead(0, 1314, 0, core.palletCount, (unsigned long)&velocity, sizeof(velocity));
		SuperTrakServChanRead(0, 1339, 0, core.palletCount, (unsigned long)&destination, sizeof(destination));
		SuperTrakServChanRead(0, 1306, 0, core.palletCount, (unsigned long)&setSection, sizeof(setSection));
		SuperTrakServChanRead(0, 1311, 0, core.palletCount, (unsigned long)&setPosition, sizeof(setPosition));
		SuperTrakServChanRead(0, 1313, 0, core.palletCount, (unsigned long)&setVelocity, sizeof(setVelocity));
		SuperTrakServChanRead(0, 1312, 0, core.palletCount, (unsigned long)&setAcceleration, sizeof(setAcceleration));
	}
	
	/* Copy data */
	Status->Info.Velocity = (float)velocity[core.palletMap[Pallet]];
	Status->Info.DestinationTarget = (unsigned char)destination[core.palletMap[Pallet]];
	Status->Info.SetSection = (unsigned char)setSection[core.palletMap[Pallet]];
	Status->Info.SetPosition = ((double)setPosition[core.palletMap[Pallet]]) / 1000.0;
	Status->Info.SetPositionUm = setPosition[core.palletMap[Pallet]];
	Status->Info.SetVelocity = setVelocity[core.palletMap[Pallet]];
	Status->Info.SetAcceleration = setAcceleration[core.palletMap[Pallet]] * 1000.0;
	
	return 0;
	
} /* End function */

/* Pallet interface */
void StCorePallet(StCorePallet_typ *inst) {
	
	/*********************** 
	 Declare Local Variables
	***********************/
	coreFormatArgumentType args;
	StCorePalletStatusType status;
	unsigned short input;
	coreCommandType *pCommand;
	
	/*************
	 State Machine
	*************/
	/* Interrupt if disabled */
	if(inst->Enable == false)
		inst->Internal.State = CORE_FUNCTION_DISABLED;
	
	switch(inst->Internal.State) {
		case CORE_FUNCTION_DISABLED:
			/* Reset outputs */
			resetOutput(inst);
			
			/* Wait for enable */
			if(inst->Enable) {
				inst->Internal.Select = inst->Pallet;
				inst->Internal.State = CORE_FUNCTION_EXECUTING;
			}
			
			break;
			
		case CORE_FUNCTION_EXECUTING:
			/* Cyclic error checks */
			if(core.error) {
				args.i[0] = inst->Internal.Select;
				logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_CRITICAL), "Cannot execute StCorePallet pallet %i due to critical StCore error", &args);
				resetOutput(inst);
				inst->Error = true;
				inst->StatusID = stCORE_ERROR_CRITICAL;
				inst->Internal.State = CORE_FUNCTION_ERROR;
				break;
			}
			/* Do not need to check reference because they are not used directly */
			else if(core.palletMap[inst->Internal.Select] == -1) {
				args.i[0] = inst->Internal.Select;
				logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_PALLET), "Pallet %i not found on system during StCorePallet enable", &args); 
				resetOutput(inst);
				inst->Error = true;
				inst->StatusID = stCORE_ERROR_PALLET;
				inst->Internal.State = CORE_FUNCTION_ERROR;
				break;
			}
			
			/* Monitor select */
			if(inst->Internal.Select != inst->Pallet && inst->Pallet != inst->Internal.PreviousSelect) {
				args.i[0] = inst->Internal.Select;
				args.i[1] = inst->Pallet;
				logMessage(CORE_LOG_SEVERITY_WARNING, coreLogCode(stCORE_WARNING_SELECT), "StCorePallet pallet change from %i to %i ignored until re-enabled", &args);
				inst->StatusID = stCORE_WARNING_SELECT;
			}
			
			/********
			 Commands
			********/
			/* Set motion parameters */
			if(inst->SetMotionParameters && !GET_BIT(inst->Internal.PreviousCommand, CORE_COMMAND_MOTION)) {
				/* Run and activate */
				coreSetMotionParameters(inst->Internal.Select, 0, inst->Parameters.Motion.Velocity, inst->Parameters.Motion.Acceleration, (void*)inst, (coreCommandType**)&inst->Internal.pCommand);
				activateCommand(inst, CORE_COMMAND_MOTION);
			}
			
			/* Set mechanical parameters */
			if(inst->SetMechanicalParameters && !GET_BIT(inst->Internal.PreviousCommand, CORE_COMMAND_MECHANICAL)) {
				/* Run and activate */
				coreSetMechanicalParameters(inst->Internal.Select, 0, inst->Parameters.Mechanical.ShelfWidth, inst->Parameters.Mechanical.CenterOffset, (void*)inst, (coreCommandType**)&inst->Internal.pCommand);
				activateCommand(inst, CORE_COMMAND_MECHANICAL);
			}
			
			/* Set mechanical parameters */
			if(inst->SetControlParameters && !GET_BIT(inst->Internal.PreviousCommand, CORE_COMMAND_CONTROL)) {
				/* Run and activate */
				coreSetControlParameters(inst->Internal.Select, 0, inst->Parameters.Control.ControlGainSet, inst->Parameters.Control.MovingFilter, inst->Parameters.Control.StationaryFilter, (void*)inst, (coreCommandType**)&inst->Internal.pCommand);
				activateCommand(inst, CORE_COMMAND_CONTROL);
			}
			
			/* Release pallet */
			if(inst->ReleasePallet && !GET_BIT(inst->Internal.PreviousCommand, CORE_COMMAND_RELEASE)) {
				/* Run and activate */
				coreReleasePallet(inst->Internal.Select, 0, inst->Parameters.Release.Direction, inst->Parameters.Release.DestinationTarget, (void*)inst, (coreCommandType**)&inst->Internal.pCommand);
				activateCommand(inst, CORE_COMMAND_RELEASE);
			}
			
			/* Release target offset */
			if(inst->ReleaseTargetOffset && !GET_BIT(inst->Internal.PreviousCommand, CORE_COMMAND_OFFSET)) {
				/* Run and activate */
				coreReleaseTargetOffset(inst->Internal.Select, 0, inst->Parameters.Release.Direction, inst->Parameters.Release.DestinationTarget, inst->Parameters.Release.Offset, (void*)inst, (coreCommandType**)&inst->Internal.pCommand);
				activateCommand(inst, CORE_COMMAND_OFFSET);
			}
			
			/* Release incremental offset */
			if(inst->ReleaseIncrementalOffset && !GET_BIT(inst->Internal.PreviousCommand, CORE_COMMAND_INCREMENT)) {
				/* Run and activate */
				coreReleaseIncrementalOffset(inst->Internal.Select, 0, inst->Parameters.Release.Offset, (void*)inst, (coreCommandType**)&inst->Internal.pCommand);
				activateCommand(inst, CORE_COMMAND_INCREMENT);
			}
			
			/* Continue move */
			if(inst->ContinueMove && !GET_BIT(inst->Internal.PreviousCommand, CORE_COMMAND_CONTINUE)) {
				/* Run and activate */
				coreContinueMove(inst->Internal.Select, 0, (void*)inst, (coreCommandType**)&inst->Internal.pCommand);
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
									logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_COMMAND), "StCorePallet pallet %i command error", &args);
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
							logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_ACKNOWLEDGE), "StCorePallet pallet %i command cannot be acknowledged", &args);
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
					logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_ACKNOWLEDGE), "StCorePallet pallet %i command cannot be acknowledged", &args);
					resetOutput(inst);
					inst->Error = true;
					inst->StatusID = stCORE_ERROR_ACKNOWLEDGE;
					inst->Internal.State = CORE_FUNCTION_ERROR;
					break;
				}
			}
			
			/* Allow warning reset */
			if(inst->ErrorReset && !inst->Internal.PreviousErrorReset)
				inst->StatusID = 0;
				
			/* Report valid */
			inst->Valid = true;
			
			break;
			
		default:
			/* Force error if state modified externally */
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
	
	inst->Internal.PreviousErrorReset = inst->ErrorReset;
	inst->Internal.PreviousSelect = inst->Pallet;
	recordInput(inst, &inst->Internal.PreviousCommand);
	
} /* End function */

/* Create local logging function */
long logMessage(coreLogSeverityEnum severity, unsigned short code, char *message, coreFormatArgumentType *args) {
	return coreLog(core.ident, severity, CORE_LOGBOOK_FACILITY, code, LOG_OBJECT, message, args);
}

/* Clear function block outputs */
void resetOutput(StCorePallet_typ *inst) {
	inst->Valid = false;
	inst->Error = false;
	inst->StatusID = 0;
	inst->Present = false;
	inst->Recovering = false;
	inst->AtTarget = false;
	inst->InPosition = false;
	inst->ServoEnabled = false;
	inst->Initializing = false;
	inst->Lost = false;
	memset(&inst->Info, 0, sizeof(inst->Info));
	inst->Busy = false;
	inst->Acknowledged = false;
	/* Do not clear internal data */
} 

/* Set command statuses and tracking */
void activateCommand(StCorePallet_typ *inst, unsigned char select) {
	/* Protect bit set */
	if(select > CORE_COMMAND_CONTROL) /* This is the last command in enumeration */
		return;
	inst->Busy = true;
	inst->Acknowledged = false;
	inst->Internal.CommandSelect = select;
}

/* Reset command statuses and tracking */
void resetCommand(StCorePallet_typ *inst) {
	inst->Busy = false;
	inst->Acknowledged = false;
	inst->Internal.CommandSelect = false;
	inst->Internal.pCommand = NULL;
}

/* Aggregate command inputs */
void recordInput(StCorePallet_typ *inst, unsigned short *pData) {
	coreAssign16(pData, CORE_COMMAND_RELEASE, inst->ReleasePallet);
	coreAssign16(pData, CORE_COMMAND_OFFSET, inst->ReleaseTargetOffset);
	coreAssign16(pData, CORE_COMMAND_INCREMENT, inst->ReleaseIncrementalOffset);
	coreAssign16(pData, CORE_COMMAND_CONTINUE, inst->ContinueMove);
	coreAssign16(pData, CORE_COMMAND_MOTION, inst->SetMotionParameters);
	coreAssign16(pData, CORE_COMMAND_MECHANICAL, inst->SetMechanicalParameters);
	coreAssign16(pData, CORE_COMMAND_CONTROL, inst->SetControlParameters);
}
