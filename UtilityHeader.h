// Contains all functions for handling packets
#include "IncludeUtility.h"

void OpenSwitchFIFOs(SwitchData* switchData)
{
  int switchId = switchData->mSwitchID;

  // open connection with left node
  if (switchData->mLeftSwitchNode != -1)
    {
      // open FIFO for write
      char writeFIFO[9];
      sprintf(writeFIFO, "fifo-%d-%d", switchId, switchId - 1);
      switchData->mFDOut[0] = open(writeFIFO, O_RDWR | O_NONBLOCK);
      printf("\nOpening outgoing connection for port 1 with FD: %d", switchData->mFDOut[0]);
      // open FIFO for read
      char readFIFO[9];
      sprintf(readFIFO, "fifo-%d-%d", switchId - 1, switchId);
      switchData->mFDIn[0] = open(readFIFO, O_RDONLY | O_NONBLOCK);
      printf("\nOpening incoming connection for port 1 with FD: %d", switchData->mFDIn[0]);
    }

    // open connection with right node
    if (switchData->mRightSwitchNode != -1)
      {
        // open FIFO for read
        char readFIFO[9];
        sprintf(readFIFO, "fifo-%d-%d", switchId + 1, switchId);
        switchData->mFDIn[1] = open(readFIFO, O_RDONLY | O_NONBLOCK);
        printf("\nOpening incoming connection for port 2 with FD: %d", switchData->mFDIn[1]);

        // open FIFO for write
        char writeFIFO[9];
        sprintf(writeFIFO, "fifo-%d-%d", switchId, switchId + 1);
        switchData->mFDOut[1] = open(writeFIFO, O_RDWR | O_NONBLOCK);
        printf("\nOpening outgoing connection for port 2 with FD: %d", switchData->mFDOut[1]);
      }
}

void CleanUp()
{
  // TODO: Close all FIFOs and text files
}

void SendOpenRequest(SwitchData* switchData)
{
  printf("Sending open request to FD: %d\n", switchData->mSocket);
  char Packet[MSG_BUF];
  int PacketSize = sprintf(Packet, "Open %d %d %d %d %d", switchData->mSwitchID,
                                         switchData->mLeftSwitchNode,
                                         switchData->mRightSwitchNode,
                                         switchData->mIPLow,
                                         switchData->mIPHigh);
  write(switchData->mSocket, Packet, PacketSize);
}

void SendAckPacket(int switchId, ControllerData* controllerData, PacketStatsForController* packetStats)
{
  char Packet[MSG_BUF];
  int PacketSize = sprintf(Packet, "Ack");
  write(controllerData->mSocketFD[switchId], Packet, PacketSize);
  packetStats->mNumAck++;
}


void AddFlowTableEntry(char packet[], int PacketSize, SwitchData* switchData, PacketStatsForSwitch* packetStats)
{
  const char delim[] = " ";
  char* running;
  running = packet;
  char* token;

  int flowEntryIndex = switchData->mNumFlowTablesEntries;
  // We are expecting 5 arguments
  token = strsep(&running, delim); // throw this one away
  token = strsep(&running, delim); // destination ip high
  switchData->mFlowTable[flowEntryIndex].mDestIPHigh = atoi(token);
  token = strsep(&running, delim); // destination ip low
  switchData->mFlowTable[flowEntryIndex].mDestIPLow = atoi(token);

  token = strsep(&running, delim);
  if (strncmp(token, "Relay", 5) == 0) switchData->mFlowTable[flowEntryIndex].mActionType = Relay;
  else if (strncmp(token, "Drop", 4) == 0) switchData->mFlowTable[flowEntryIndex].mActionType = Drop;

  token = strsep(&running, delim);
  switchData->mFlowTable[flowEntryIndex].mActionValue = atoi(token);
  switchData->mFlowTable[flowEntryIndex].mSrcIPLow = 0;
  switchData->mFlowTable[flowEntryIndex].mSrcIPHigh = 1000;
  switchData->mFlowTable[flowEntryIndex].mPriority = 4;
  switchData->mFlowTable[flowEntryIndex].mPacketCount = 0;
  printf("Added flow table entry:\n");
  PrintFlowTableEntry(flowEntryIndex, switchData->mFlowTable[flowEntryIndex]);
  switchData->mNumFlowTablesEntries++;
}

void SendQueryRequest(SwitchData* switchData, int destinationIP, PacketStatsForSwitch* packetStats)
{
  printf("Sending Query packet to controller\n");
  char Packet[MSG_BUF];
  int PacketSize = sprintf(Packet, "Query %d %d", switchData->mSwitchID, destinationIP);
  write(switchData->mSocket, Packet, PacketSize);
  packetStats->mNumQuery++;

  struct pollfd fdArray[1];
  int pollResult;
  char buffer[MSG_BUF];
  int bytes_in;

  fdArray[0].fd = switchData->mSocket;
  fdArray[0].events = 0;
  fdArray[0].events |= POLLIN;

  pollResult = poll(fdArray, 1, -1); //block until we receive the add packet
  if (pollResult == -1) printf("PollError\n");
  else if (fdArray[0].revents & POLLIN)
  {
    if ((bytes_in = read(fdArray[0].fd, buffer, MSG_BUF)) > 0)
    buffer[bytes_in] = '\0';
    printf("Received add from controller: %s\n", buffer);
    AddFlowTableEntry(buffer, bytes_in, switchData, packetStats);
    packetStats->mNumAddRule++;
  }
}

void SendAddPacket(ControllerData* controllerData, PacketStatsForController* packetStats, int sourceSwitchId, int destinationIP, int destinationSwitchIndex)
{
  char Packet[MSG_BUF];
  int PacketSize;
  if (destinationSwitchIndex == -1)
  {
    PacketSize = sprintf(Packet,"Add %d %d Drop -1", destinationIP, destinationIP);
  }
  else
  {
    int destinationIPHigh = controllerData->mSwitchData[destinationSwitchIndex].mIPHigh;
    int destinationIPLow = controllerData->mSwitchData[destinationSwitchIndex].mIPLow;
    int destinationPort = -1;
    if (controllerData->mSwitchData[destinationSwitchIndex].mSwitchId > sourceSwitchId)
    {
      destinationPort = 2;
    }
    else if (controllerData->mSwitchData[destinationSwitchIndex].mSwitchId < sourceSwitchId)
    {
      destinationPort = 1;
    }

    if (destinationPort == -1)
    {
      PacketSize = sprintf(Packet,"Add %d %d Drop -1", destinationIP, destinationIP);
    }

    PacketSize = sprintf(Packet, "Add %d %d Relay %d", destinationIPHigh, destinationIPLow, destinationPort);
  }

  write(controllerData->mSocketFD[sourceSwitchId], Packet, PacketSize);
  packetStats->mNumAdd++;
}

void RelayOrDropPacket(char packet[], int sourceIP, int destinationIP, SwitchData* switchData, PacketStatsForSwitch* packetStats)
{
  int foundFlowTableIndex = -1;
  for (int i = 0; i < switchData->mNumFlowTablesEntries; i++)
  {
    if (sourceIP > MAXIP)
    {
      continue;
    }
    if (destinationIP < switchData->mFlowTable[i].mDestIPLow)
    {
      continue;
    }
    if (destinationIP > switchData->mFlowTable[i].mDestIPHigh)
    {
      continue;
    }
    if (sourceIP <= MAXIP && destinationIP >= switchData->mFlowTable[i].mDestIPLow && destinationIP <= switchData->mFlowTable[i].mDestIPHigh)
    {
      switchData->mFlowTable[i].mPacketCount++;
      foundFlowTableIndex = i;
    }
  }
  if (foundFlowTableIndex == -1) printf("Error flow table entry not found after query return for IP: %d\n", destinationIP);

  else
  {
    if (switchData->mFlowTable[foundFlowTableIndex].mActionType == Relay && switchData->mFlowTable[foundFlowTableIndex].mActionValue < 3)
    {
      char Packet[MSG_BUF];
      int PacketSize = sprintf(Packet, "%s", packet);
      int fd;
      if (switchData->mFlowTable[foundFlowTableIndex].mActionValue == 2)
      {
        fd = switchData->mFDOut[1];
      }
      else if (switchData->mFlowTable[foundFlowTableIndex].mActionValue == 1)
      {
        fd = switchData->mFDOut[0];
      }
      else
      {
        printf("Not relaying to any port\n");
        return;
      }
      write(fd, Packet, PacketSize);
      packetStats->mNumRelayOut++;
      printf("Relaying Packet: %s to port: %d on FD: %d\n", Packet, switchData->mFlowTable[foundFlowTableIndex].mActionValue, fd);
    }
    else
    {
      printf("Dropping Packet: %s\n", packet);
    }
  }
}






