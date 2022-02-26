/*******************************************************************************
 * File: StCoreLog.c
 * Author: Tyler Matijevich
 * Date: 2022-02-17
*******************************************************************************/

#include "StCoreMain.h"

#define FORMAT_SIZE 125

void StCoreLogPosition(enum SuperTrakPositionErrorEnum error, struct SuperTrakPositionInfoType info) {
	
	char format[FORMAT_SIZE];
	struct FormatStringArgumentsType args;
	
	if(error == stPOS_ERROR_NONE)
		return;
	
	format[FORMAT_SIZE - 1] = '\0';
	strncpy(format, "Error #%i: ", FORMAT_SIZE - 1);
	args.i[0] = error;
	
	switch(error) {
		case stPOS_ERROR_SERVCHAN:
			strncat(format, "Service channel read error %i from parameter %i", FORMAT_SIZE - 1);
			args.i[1] = info.serviceChannelResult;
			args.i[2] = info.serviceChannelParameter;
			break;
		case stPOS_ERROR_SECTIONCOUNT:
			strncat(format, "Section count %i exceeds limits [1, %i]", FORMAT_SIZE - 1);
			args.i[1] = info.sectionCount;
			args.i[2] = info.sectionCountMax;
			break;
		case stPOS_ERROR_SECTIONADDRESS:
			strncat(format, "Section user address %i (network index %i) exceeds section count [1, %i]", FORMAT_SIZE - 1);
			args.i[1] = info.section;
			args.i[2] = info.networkIndex;
			args.i[3] = info.sectionCount;
			break;
		case stPOS_ERROR_SECTIONTYPE:
			strncat(format, "Section type %i exceeds [0, 5] for section %i", FORMAT_SIZE - 1);
			args.i[1] = info.sectionType;
			args.i[2] = info.section;
			break;
		case stPOS_ERROR_HEADSECTION:
			strncat(format, "Head section %i exceeds section count [1, %i]", FORMAT_SIZE - 1);
			args.i[1] = info.headSection;
			args.i[2] = info.sectionCount;
			break;
		case stPOS_ERROR_FLOWDIRECTION:
			strncat(format, "Invalid flow direction %i (0 - left, 1 - right)", FORMAT_SIZE - 1);
			args.i[1] = info.flowDirection;
			break;
		case stPOS_ERROR_NETWORKORDER:
			strncat(format, "Invalid network order", FORMAT_SIZE - 1);
			break;
		case stPOS_ERROR_INPUTORIGIN:
			strncat(format, "Origin section input %i exceeds section count [1, %i]", FORMAT_SIZE - 1);
			args.i[1] = info.originSection;
			args.i[2] = info.sectionCount;
			break;
		case stPOS_ERROR_INPUTSECTION:
			strncat(format, "Section input %i exceeds section count [1, %i]", FORMAT_SIZE - 1);
			args.i[1] = info.section;
			args.i[2] = info.sectionCount;
			break;
		case stPOS_ERROR_INPUTSECTIONPOS:
			strncat(format, "Section %i position input %i um exceeds limits [0, %i] um", FORMAT_SIZE - 1);
			args.i[1] = info.section;
			args.i[2] = info.sectionPosition;
			args.i[3] = info.sectionPositionMax;
			break;
		case stPOS_ERROR_INPUTGLOBALPOS:
			strncat(format, "Global position input %i um exceeds limits [0, %i] um", FORMAT_SIZE - 1);
			args.i[1] = info.globalPosition;
			args.i[2] = info.totalLength;
			break;
		case stPOS_ERROR_NETWORKGLOBAL:
			strncat(format, "Unexpected global position", FORMAT_SIZE - 1);
			break;
		default:
			strncat(format, "Unknown error %i", FORMAT_SIZE - 1);
			break;
	}
	
	CustomFormatMessage(USERLOG_SEVERITY_ERROR, 50000, format, &args, "StCoreLog", 1);
	
} /* Function definition */

long StCoreLogServChan(unsigned short result, unsigned short parameter) {
	
	char format[FORMAT_SIZE];
	struct FormatStringArgumentsType args;
	
	if(result == scERR_SUCCESS)
		return 0;
		
	format[FORMAT_SIZE - 1] = '\0';
	strncpy(format, "Serv. chan. error accessing par %i: ", FORMAT_SIZE - 1);
	args.i[0] = parameter;
	
	switch(result) {
		case scERR_INVALID_SECTION:	
			strncat(format, "An invalid section number was specified", FORMAT_SIZE - 1);
			break;
		case scERR_INVALID_PARAM:
			strncat(format, "The specified parameter number was not recognized", FORMAT_SIZE - 1);
			break;
		case scERR_INVALID_TASK:
			strncat(format, "The specified task code was not recognized", FORMAT_SIZE - 1);
			break;
		case scERR_TASK_UNAVAILABLE:
			strncat(format, "The requested operation is currently unavailable", FORMAT_SIZE - 1);
			break;
		case scERR_INVALID_INDEX:
			strncat(format, "The specified starting index is invalid", FORMAT_SIZE - 1);
			break;
		case scERR_INVALID_VALUE:
			strncat(format, "The value to be written was outside of the valid range", FORMAT_SIZE - 1);
			break;
		case scERR_INVALID_COUNT:
			strncat(format, "The requested number of elements is invalid", FORMAT_SIZE - 1);
			break;
		case scERR_INVALID_ARGUMENT:
			strncat(format, "An invalid argument was specified", FORMAT_SIZE - 1);
			break;
		case scERR_COMMAND_TIMEOUT:
			strncat(format, "The command did not complete in a timely manner", FORMAT_SIZE - 1);
			break;
		case scERR_UNAUTHORIZED:
			strncat(format, "The request was denied due to a lack of permissions", FORMAT_SIZE - 1);
			break;
		case scERR_BUFFER_SIZE:
			strncat(format, "The buffer size is insufficient", FORMAT_SIZE - 1);
			break;
		case scERR_INVALID_PACKET:
			strncat(format, "The packet is malformed, orits length is incorrect", FORMAT_SIZE - 1);
			break;
		case scERR_INTERNAL_ERROR:
			strncat(format, "An internal error occurred while processing the request", FORMAT_SIZE - 1);
			break;
		default:
			strncat(format, "Unknown error", FORMAT_SIZE - 1);
			break;
	}
	
	CustomFormatMessage(USERLOG_SEVERITY_ERROR, 50100, format, &args, "StCoreLog", 1);
	
	return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, 1, 50100);
	
} /* Function defintion */
