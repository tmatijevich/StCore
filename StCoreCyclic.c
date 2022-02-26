/*******************************************************************************
 * File: StCoreCyclic.c
 * Author: Tyler Matijevich
 * Date: 2022-02-25
*******************************************************************************/

#include "StCoreMain.h"

long StCoreCyclic(void) {
	
	/***********************
	 Declare local variables 
	***********************/
	struct FormatStringArgumentsType args;
	static unsigned long timerSave = 0, saveParameters = 0;
	
	configPLCInterface;
	
	if(timerSave >= 500000 && saveParameters == 0) {
		/* Save parameters */
		saveParameters = 0x000020; /* Global parameters, PLC interface configuration */
		args.i[0] = SuperTrakServChanWrite(0, stPAR_SAVE_PARAMETERS, 0, 1, (unsigned long)&saveParameters, sizeof(saveParameters));
		if(args.i[0] != scERR_SUCCESS) 
			StCoreLogServChan(args.i[0], stPAR_SAVE_PARAMETERS);
		else {
			args.i[0] = stPAR_SAVE_PARAMETERS;
			args.i[1] = saveParameters;
			CustomFormatMessage(USERLOG_SEVERITY_DEBUG, 2000, "Successful serv. chan. write to par %i with value %i", &args, "StCoreLog", 1);
		}
	}
	else
		timerSave += 800;
	
	return 0;
	
} /* Function definition */
