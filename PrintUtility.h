#include "IncludeUtility.h"

// Contains all the print functions

void PrintFlowTableEntry(int entryIndex, FlowTableEntry entry)
{
  char action[MSG_BUF];
  sprintf(action, entry.mActionType == Relay ? "RELAY" : "DROP");
  printf("[%d] (srcIP= %d-%d, dest_IP= %d-%d, action= %s-%d, pri=%d, pktCount=%d)\n", entryIndex, entry.mSrcIPLow, entry.mSrcIPHigh, entry.mDestIPLow, entry.mDestIPHigh, action, entry.mActionValue, entry.mPriority, entry.mPacketCount);
}

void PrintProgramData(ProgramStateData* programStateData)
{
  if (programStateData->mProgramMode == Switch)
  {
    printf("Successfully created a switch with following parameters: \n");
    printf("Switch ID: sw%d\n", programStateData->mSwitchData.mSwitchID);
    printf("Traffic File Name: %s\n", programStateData->mSwitchData.mTrafficFileName);
    printf("Left Node Number: %d\n", programStateData->mSwitchData.mLeftSwitchNode);
    printf("Right Node Number: %d\n", programStateData->mSwitchData.mRightSwitchNode);
    printf("IP High: %d\n", programStateData->mSwitchData.mIPHigh);
    printf("IP Low: %d\n", programStateData->mSwitchData.mIPLow);

    char addrstr[100];
    void *ptr;
    struct addrinfo* addressInfo = &programStateData->mSwitchData.mAddressInfo;
    switch (addressInfo->ai_family)
    {
    case AF_INET:
        ptr = &((struct sockaddr_in *) addressInfo->ai_addr)->sin_addr;
        break;
    case AF_INET6:
        ptr = &((struct sockaddr_in6 *) addressInfo->ai_addr)->sin6_addr;
        break;
    default:
        printf("Address family wasn't determined\n");
    }
    inet_ntop (addressInfo->ai_family, ptr, addrstr, 100);
    printf("Server Address: %s\n", addrstr);

    printf("Port Number: %d\n", programStateData->mSwitchData.mPortNumber);
  }
  else if (programStateData->mProgramMode == Controller)
  {
    // TODO: Remove the creation prints
    printf("Successfully created a controller with following parameters: \n");
    printf("Number of Switches: %d\n", programStateData->mControllerData.mNumSwitches);
    printf("Port Number: %d\n", programStateData->mControllerData.mPortNumber);
  }
}

void PrintSwitchPacketStats(PacketStatsForSwitch* packetStats)
{
  printf("\nPacket Stats: \n");
  printf("\tReceived:\tADMIT:%d, ACK:%d, ADDRUlE:%d, RELAYIN:%d\n", packetStats->mNumAdmit, packetStats->mNumAck, packetStats->mNumAddRule, packetStats->mNumRelayIn);
  printf("\tTransmitted:\tOPEN:%d, QUERY:%d, RELAYOUT:%d\n", packetStats->mNumOpen, packetStats->mNumQuery, packetStats->mNumRelayOut);
}

void PrintControllerPacketStats(PacketStatsForController* packetStats)
{
  printf("\nPacket Stats: \n");
  printf("\tReceived:\tOPEN:%d, QUERY:%d\n", packetStats->mNumOpen, packetStats->mNumQuery);
  printf("\tTransmitted:\tACK:%d, ADD:%d\n", packetStats->mNumAck, packetStats->mNumAdd);
}

void PrintSwitchInformationForController(ControllerData* controllerData)
{
  printf("Switch Information: \n");
  for (int i = 0; i < controllerData->mNumSwitches; i++)
  {
    if (controllerData->mSwitchData[i].mActive == 1)
    {
      SwitchDataForController switchData = controllerData->mSwitchData[i];
      printf("[sw%d] port1=%d, port2=%d, port3=%d-%d\n", switchData.mSwitchId, switchData.mLeftNode, switchData.mRightNode, switchData.mIPLow, switchData.mIPHigh);
    }
  }
}

void ListForController(ControllerData* controllerData, PacketStatsForController* packetStats)
{
  PrintSwitchInformationForController(controllerData);
  PrintControllerPacketStats(packetStats);
}

void ExitForController(ControllerData* controllerData, PacketStatsForController* packetStats)
{
  ListForController(controllerData, packetStats);
  exit(0);
}

void ListForSwitch(SwitchData* switchData, PacketStatsForSwitch* packetStats)
{
  printf("Flow Table: \n");
  for (int i = 0; i < switchData->mNumFlowTablesEntries; i++)
  {
    PrintFlowTableEntry(i, switchData->mFlowTable[i]);
  }
  PrintSwitchPacketStats(packetStats);
}


void ExitForSwitch(SwitchData* switchData, PacketStatsForSwitch* packetStats)
{
  ListForSwitch(switchData, packetStats);
  exit(0);
}