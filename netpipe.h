
#ifndef __NETPIPE_H__
#define __NETPIPE_H__


#include "compat-header.h"

typedef enum _EOptionType {
	otUnknown,
	otSourceHost,
	otSourcePort,
	otTargetHost,
	otTargetPort,
	otIPv4Only,
	otIPv6Only,
	otLogError,
	otLogWarning,
	otLogInfo,
	otLogPacket,
	otOneConnection,
	otKeepAlive,
	otHelp,
	otVersion,
	otLogPacketData,
	otAuthSource,
	otAuthTarget,
	otLogFile,
	otReceiveDomain,
	otSendDomain,
#ifndef _WIN32
	otUnixSource,
	otUnixDest,
#endif
} EOptionType, *PEOptionType;

typedef struct _COMMAND_LINE_OPTION {
	EOptionType Type;
	int Specified;
	int ArgumentCount;
	size_t NameCount;
	char *Names[2];
	char *ArgumentType;
	char *Description;
} COMMAND_LINE_OPTION, *PCOMMAND_LINE_OPTION;

#ifdef _WIN32
extern COMMAND_LINE_OPTION _cmdOptions[21];
#endif


int NetPipeMain(int argc, char *argv[]);
void NetPipeTerminate(void);



#endif
