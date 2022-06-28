/*******************************************************************************
 * File: StCore\Pallet.c
 * Author: Tyler Matijevich
 * Date: 2022-06-27
*******************************************************************************/

#include "Main.h"

/* Network output */
long StCoreGetNetworkIO(unsigned char Offset, unsigned char *pValue) {
	
	/************************************************
	 Dependencies:
	  Global:
	   core.pCyclicStatus
	   core.interface
	   core.error
	   core.statusID
	************************************************/
	
	/* Declare local variables */
	unsigned char *pNetwork;
	
	/* Check core */
	if(core.error)
		return core.statusID;
	
	if(core.pCyclicStatus == NULL)
		return stCORE_ERROR_ALLOCATION;
		
	if(pValue == NULL)
		return stCORE_ERROR_ALLOCATION;
	
	if(Offset >= core.networkIOCount)
		return stCORE_ERROR_INDEX;
		
	/* Access */
	pNetwork = core.pCyclicStatus + core.interface.networkOutputOffset + Offset / CORE_NETWORK_IO_PER_BYTE;
	*pValue = GET_BIT(*pNetwork, Offset % CORE_NETWORK_IO_PER_BYTE);
	
	return 0;
	
} /* End function */

/* Network input */
long StCoreSetNetworkIO(unsigned char Offset, unsigned char Value) {
	
	/************************************************
	 Dependencies:
	  Global:
	   core.pCyclicControl
	   core.interface
	   core.error
	   core.statusID
	************************************************/
	
	/* Declare local variables */
	unsigned char *pNetwork;
	
	/* Check core */
	if(core.error)
		return core.statusID;
	
	if(core.pCyclicControl == NULL)
		return stCORE_ERROR_ALLOCATION;
	
	if(Offset >= core.networkIOCount)
		return stCORE_ERROR_INDEX;
		
	/* Access */
	pNetwork = core.pCyclicControl + core.interface.networkInputOffset + Offset / CORE_NETWORK_IO_PER_BYTE;
	
	if(Value) SET_BIT(*pNetwork, Offset % CORE_NETWORK_IO_PER_BYTE);
	else CLEAR_BIT(*pNetwork, Offset % CORE_NETWORK_IO_PER_BYTE);
	
	return 0;
	
} /* End function */
