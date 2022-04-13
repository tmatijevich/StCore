/*******************************************************************************
 * File: StCoreTarget.c
 * Author: Tyler Matijevich
 * Date: 2022-04-13
*******************************************************************************/

/* Headers */
#include "StCoreMain.h"

/* Function prototypes */
static void clearInstanceOutputs(StCoreTarget_typ *inst);

/* Target core interface */
void StCoreTarget(StCoreTarget_typ *inst) {
	
	/*********************** 
	 Declare local variables
	***********************/
	FormatStringArgumentsType args;
	unsigned char *pTargetStatus;
	
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
			
			/*******
			 Control
			*******/
			
			/******
			 Status
			******/
			pTargetStatus = pCoreCyclicStatus + coreInterfaceConfig.targetStatusOffset + 3 * inst->Internal.Select;
			
			inst->PalletPresent = GET_BIT(*pTargetStatus, 0);
			inst->PalletInPosition = GET_BIT(*pTargetStatus, 1);
			inst->PalletPreArrival = GET_BIT(*pTargetStatus, 2);
			inst->PalletOverTarget = GET_BIT(*pTargetStatus, 3);
			inst->PalletPositionUncertain = GET_BIT(*pTargetStatus, 6);
			
			inst->PalletID = *(pCoreCyclicStatus + coreInterfaceConfig.targetStatusOffset + 3 * inst->Internal.Select + 1);
			
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
