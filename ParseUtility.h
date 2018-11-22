#include "IncludeUtility.h"
#include "UtilityHeader.h"
// Contains all parsing functions

void InitializeSocketForSwitch(char hostName[], SwitchData* switchData)
{
   printf("InitializeSocketForSwitch() entered\n");
   struct hostent	*hp;
   hp = gethostbyname(hostName);
   if (hp == (struct hostent*) NULL)
   {
     printf("Error determining hostname\n");
     exit(1);
   }
   struct in_addr ** addrList = (struct in_addr**) hp->h_addr_list;
   if (addrList[0] != NULL) printf("InitializedSwitch with IP: %s\n", inet_ntoa(*addrList[0]));
   else { printf("addrList[0] is NULL\n"); }

   memset ((char*) &switchData->mServerSIN, 0, sizeof switchData->mServerSIN);
   memcpy ((char*) &switchData->mServerSIN.sin_addr, hp->h_addr, hp->h_length);
   switchData->mServerSIN.sin_family = hp->h_addrtype;
   switchData->mServerSIN.sin_port = htons(switchData->mPortNumber);

   switchData->mSocket = socket(hp->h_addrtype, SOCK_STREAM, 0);
   if (switchData->mSocket < 0)
   {
     printf("Error opening Socket for switch id: %d\n", switchData->mSwitchID);
     exit(1);
   }

   if (connect(switchData->mSocket, (struct sockaddr*) &switchData->mServerSIN, sizeof switchData->mServerSIN) < 0)
   {
     printf("Error connecting to controller with switch id: %d\n", switchData->mSwitchID);
     exit(1);
   }
}

void ProcessOpenRequest(ControllerData* controllerData, PacketStatsForController* packetStats, char buffer[], int buffer_size)
{
  // we are expecting 6 arguments separated by spaces
  const char delim[] = " ";
  char* running;
  running = buffer;
  char* token;
  SwitchDataForController switchData;

  token = strsep(&running, delim); // throw away open argument

  token = strsep(&running, delim); // Switch ID
  switchData.mSwitchId = atoi(token);

  token = strsep(&running, delim); // left node ID
  switchData.mLeftNode = atoi(token);

  token = strsep(&running, delim); // right node ID
  switchData.mRightNode = atoi(token);

  token = strsep(&running, delim); // IPLow node ID
  switchData.mIPLow = atoi(token);

  token = strsep(&running, delim); // IPHigh node ID
  switchData.mIPHigh = atoi(token);

  switchData.mActive = 1;

  controllerData->mSwitchData[switchData.mSwitchId - 1] = switchData;

  printf("Received Open Request from sw%d\n", switchData.mSwitchId);
  SendAckPacket(switchData.mSwitchId, controllerData, packetStats);
}

void ProcessQuery(ControllerData* controllerData, PacketStatsForController* packetStats, char buffer[], int buffer_size)
{
  int sourceSwitchId = -1;
  int destinationIP = -1;
  // Query packet format: Query sourceSwitchId destinationIP
  const char delim[] = " ";
  char* running;
  running = buffer;
  char* token;

  token = strsep(&running, delim); //throw away Query argument

  token = strsep(&running, delim); // source switch id
  sourceSwitchId = atoi(token);

  token = strsep(&running, delim); // destination IP
  destinationIP = atoi(token);

  int foundActiveTargetSwitch = -1;
  // create a add rule: dest_hi dest_lo, action, actionval
  for (int i = 0; i < controllerData->mNumSwitches; i++)
  {
    if (destinationIP >= controllerData->mSwitchData[i].mIPLow && destinationIP <= controllerData->mSwitchData[i].mIPHigh && controllerData->mSwitchData[i].mActive == 1)
    {
      foundActiveTargetSwitch = 1;
      SendAddPacket(controllerData, packetStats, sourceSwitchId, destinationIP, i); // add rule
    }
  }
  if (foundActiveTargetSwitch == -1)
  {
    SendAddPacket(controllerData, packetStats, sourceSwitchId, destinationIP, -1);
  }

  printf("Received a query request from sw%d about dest_IP:%d\n", sourceSwitchId, destinationIP);
}


void ProcessPacketForSwitch(char packet[], SwitchData* switchData, PacketStatsForSwitch* packetStats, int portNumber)
{
  printf("Received packet: %s from port: %d\n", packet, portNumber);
  if (portNumber <= 2) packetStats->mNumRelayIn++;
  int sourceIP = -1;
  int destinationIP = -1;
  int foundFlowTableIndex = -1;

  char localLine[MSG_BUF];
  const char delim[] = " ";
  char* running;
  running = packet;
  strncpy(localLine, packet, MSG_BUF);
  printf("Processing packet: %s\n", localLine);
  char* token;

  token = strsep(&running, delim); // throw away the first argument
  token = strsep(&running, delim); // source ip
  sourceIP = atoi(token);

  token = strsep(&running, delim); // destination ip
  destinationIP = atoi(token);
  // look at the FlowTable for what to do
  for (int i = 0; i < switchData->mNumFlowTablesEntries; i++)
  {
    if (sourceIP <= MAXIP && destinationIP >= switchData->mFlowTable[i].mDestIPLow && destinationIP <= switchData->mFlowTable[i].mDestIPHigh)
    {
      foundFlowTableIndex = i;
    }
  }
  if (foundFlowTableIndex != -1)
  {
    RelayOrDropPacket(localLine, sourceIP, destinationIP, switchData, packetStats);
  }
  else
  {
    SendQueryRequest(switchData, destinationIP, packetStats);
    RelayOrDropPacket(localLine, sourceIP, destinationIP, switchData, packetStats);
  }
}

double GetTimeInMilliSec()
{
  struct timeval time;
  gettimeofday(&time, NULL);
  return (time.tv_sec + (time.tv_usec / 1000000.0)) * 1000.0;
}

void ProcessTrafficFileLine(char line[], SwitchData* switchData, PacketStatsForSwitch* packetStats, double* start, int* delay)
{
  const char delim[] = " ";
  char localLine[MSG_BUF];
  char delayToken[MSG_BUF];
  strncpy(delayToken, line, MSG_BUF);
  char* running;
  char* delayRunning = delayToken;
  running = line;
  strncpy(localLine, line, MSG_BUF);
  char* token;
  int targetSwitchId;

  token = strsep(&running, delim); // the first arg in the line
  if (1 == sscanf(token, "%*[^123456789]%d", &targetSwitchId))
  {
    if (targetSwitchId == switchData->mSwitchID)
    {
      token = strsep(&delayRunning, delim); // first argument
      token = strsep(&delayRunning, delim); // second argument
      if (strncmp(token, "delay", 5) == 0)
      {
        int tempDelay = atoi(strsep(&delayRunning, delim));
        if (tempDelay > -1) 
        {
          *start = GetTimeInMilliSec();
          *delay = tempDelay;
          printf("Entering a delay period for %d millisec\n", tempDelay);
        }
        else printf("Error parsing delay packet\n");
      }
      else
      {
        packetStats->mNumAdmit++;
        ProcessPacketForSwitch(localLine, switchData, packetStats, 3);
      }
    }
  }
}


void ReceivePacketForController(ControllerData* controllerData, PacketStatsForController* packetStats, char buffer[], int buffer_size)
{
  if (strncmp(buffer, "Open", 4) == 0)
  {
    packetStats->mNumOpen++;
    ProcessOpenRequest(controllerData, packetStats, buffer, buffer_size);
  }
  else if (strncmp(buffer, "Query", 5) == 0)
  {
    packetStats->mNumQuery++;
    ProcessQuery(controllerData, packetStats, buffer, buffer_size);
  }
}

