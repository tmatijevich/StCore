/*******************************************************************************
 * File: StCoreLog.c
 * Author: Tyler Matijevich
 * Date: 2022-02-17
*******************************************************************************/

#include "StCoreMain.h"

void StCoreLogMessage(UserLogSeverityEnum severity, unsigned short code, char *message) {
	CustomMessage(severity, code, message, stCORE_LOGBOOK_NAME, stCORE_LOGBOOK_FACILITY);
}

void StCoreFormatMessage(UserLogSeverityEnum severity, unsigned short code, char *message, FormatStringArgumentsType *args) {
	CustomFormatMessage(severity, code, message, args, stCORE_LOGBOOK_NAME, stCORE_LOGBOOK_FACILITY);
}

unsigned short StCoreEventCode(long eventID) {
	return (unsigned short)eventID;
}

/* Log service channel error responses for all StCore functions */
void StCoreLogServChan(unsigned short result, unsigned short parameter) {
	
	/***********************
	 Declare local variables
	***********************/
	char format[stCORE_FORMAT_SIZE + 1];
	FormatStringArgumentsType args;
	unsigned long remainingLength;
	
	/*****
	 Abort
	*****/
	if(result == scERR_SUCCESS)
		return;
		
	/******************
	 Safely copy prefix
	******************/
	format[stCORE_FORMAT_SIZE] = '\0';
	strncpy(format, "Serv. chan. error %i accessing par %i: ", stCORE_FORMAT_SIZE);
	args.i[0] = result;
	args.i[1] = parameter;
	remainingLength = stCORE_FORMAT_SIZE - strlen(format);
	
	/**********************************
	 Concatenate specific error message
	**********************************/
	switch(result) {
		case scERR_INVALID_SECTION:	
			strncat(format, "An invalid section number was specified", remainingLength);
			break;
		case scERR_INVALID_PARAM:
			strncat(format, "The specified parameter number was not recognized", remainingLength);
			break;
		case scERR_INVALID_TASK:
			strncat(format, "The specified task code was not recognized", remainingLength);
			break;
		case scERR_TASK_UNAVAILABLE:
			strncat(format, "The requested operation is currently unavailable", remainingLength);
			break;
		case scERR_INVALID_INDEX:
			strncat(format, "The specified starting index is invalid", remainingLength);
			break;
		case scERR_INVALID_VALUE:
			strncat(format, "The value to be written was outside of the valid range", remainingLength);
			break;
		case scERR_INVALID_COUNT:
			strncat(format, "The requested number of elements is invalid", remainingLength);
			break;
		case scERR_INVALID_ARGUMENT:
			strncat(format, "An invalid argument was specified", remainingLength);
			break;
		case scERR_COMMAND_TIMEOUT:
			strncat(format, "The command did not complete in a timely manner", remainingLength);
			break;
		case scERR_UNAUTHORIZED:
			strncat(format, "The request was denied due to a lack of permissions", remainingLength);
			break;
		case scERR_BUFFER_SIZE:
			strncat(format, "The buffer size is insufficient", remainingLength);
			break;
		case scERR_INVALID_PACKET:
			strncat(format, "The packet is malformed, orits length is incorrect", remainingLength);
			break;
		case scERR_INTERNAL_ERROR:
			strncat(format, "An internal error occurred while processing the request", remainingLength);
			break;
		default:
			strncat(format, "Unknown error", remainingLength);
			break;
	}
	
	StCoreFormatMessage(USERLOG_SEVERITY_ERROR, StCoreEventCode(stCORE_ERROR_SERVCHAN), format, &args);
	
} /* Function defintion */
