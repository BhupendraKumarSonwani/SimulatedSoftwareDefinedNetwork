#ifndef INCLUDE_UTILITY
#define INCLUDE_UTILITY

// Contains all macros, struct defintions and includes

#include <poll.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX_NSW 7
#define	CPU_LIMIT	200	// max cpu limit in seconds
#define MAXIP 1000
#define MAX_NUM_FLOWTABLE_ENTRIES 100
#define MAX_ARG_LENGTH 128 //max number of characters in an argument
#define MSG_BUF 256
#define NUM_SWITCH_FD 2 // this is the number of FIFOs each switch would use for each direction

enum ProgramMode { Unassigned, Controller, Switch };

enum ActionType { Relay, Drop };

typedef struct FlowTableEntry
{
  int mSrcIPLow;
  int mSrcIPHigh;
  int mDestIPLow;
  int mDestIPHigh;
  enum ActionType mActionType;
  int mActionValue;
  int mPriority;
  int mPacketCount;
} FlowTableEntry;

typedef struct SwitchData
{
  int mSwitchID;
  char mTrafficFileName[MAX_ARG_LENGTH];
  int mLeftSwitchNode;
  int mRightSwitchNode;
  int mIPLow;
  int mIPHigh;
  int mPortNumber;
  int mSocket; // the FD for the socket connection with the controller
  struct sockaddr_in mServerSIN; // holds the address info for the server
  // the order of FIFOs are with left node, right node
  int mFDIn[NUM_SWITCH_FD];
  int mFDOut[NUM_SWITCH_FD];
  FlowTableEntry mFlowTable[MAX_NUM_FLOWTABLE_ENTRIES];
  int mNumFlowTablesEntries;
} SwitchData;

typedef struct SwitchDataForController
{ // The controller holds data about switches here
  int mActive; //holds 1 if activated
  int mSwitchId;
  int mLeftNode;
  int mRightNode;
  int mIPLow;
  int mIPHigh;
} SwitchDataForController;

typedef struct ControllerData
{
  int mNumSwitches;
  int mPortNumber;
  int mSocket;
  struct sockaddr_in mSIN;
  struct sockaddr_in mSFROM;
  int mSocketFD[MAX_NSW+1]; // sw(i) is in mSocket[i]
  SwitchDataForController mSwitchData[MAX_NSW];
} ControllerData;

typedef struct ProgramStateData
{
  int mStateDataValid;
  enum ProgramMode mProgramMode;
  SwitchData mSwitchData;
  ControllerData mControllerData;
} ProgramStateData;

typedef struct PacketStatsForController
{
  // Received Packets
  int mNumOpen;
  int mNumQuery;

  // Transmitted Packets
  int mNumAck;
  int mNumAdd;
} PacketStatsForController;

typedef struct PacketStatsForSwitch
{
  // Received Packets
  int mNumAdmit;
  int mNumAck;
  int mNumAddRule;
  int mNumRelayIn;

  // Transmitted Packets
  int mNumOpen;
  int mNumQuery;
  int mNumRelayOut;
} PacketStatsForSwitch;

#endif
