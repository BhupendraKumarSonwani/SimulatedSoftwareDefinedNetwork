#include "PrintUtility.h"
#include "ParseUtility.h"

void SetSwitchData(ProgramStateData* programStateData, char* arguments[])
{
  if (1 != sscanf(arguments[1], "%*[^123456789]%d", &programStateData->mSwitchData.mSwitchID))
  {
    printf("Invalid Switch ID input\n");
    return;
  }
  strncpy(programStateData->mSwitchData.mTrafficFileName, arguments[2], MAX_ARG_LENGTH);
  if (1 != sscanf(arguments[3], "%*[^123456789]%d", &programStateData->mSwitchData.mLeftSwitchNode))
  {
    if (strncmp(arguments[3], "null", 4) == 0)
    {
      programStateData->mSwitchData.mLeftSwitchNode = -1;
    }
    else
    {
      printf("Invalid left node number!\n");
      return;
    }
  }
  if (1 != sscanf(arguments[4], "%*[^123456789]%d", &programStateData->mSwitchData.mRightSwitchNode))
  {
    if (strncmp(arguments[4], "null", 4) == 0)
    {
      programStateData->mSwitchData.mRightSwitchNode = -1;
    }
    else
    {
      printf("Invalid left node number!\n");
      return;
    }
  }

  const char delim[2] = "-";
  programStateData->mSwitchData.mIPLow = atoi(strtok(arguments[5], delim));
  programStateData->mSwitchData.mIPHigh = atoi(strtok(NULL, delim));

  ParseHostName(arguments[6], &programStateData->mSwitchData.mAddressInfo);
  programStateData->mSwitchData.mPortNumber = atoi(arguments[7]);

  programStateData->mStateDataValid = 1;
}

void InitializeSocketsForController(ProgramStateData* programStateData)
{
  // creating a managing socket
  ControllerData* contollerData = &programStateData->mControllerData;
  contollerData->mSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (contollerData->mSocket < 0)
  {
    printf("Error opening socket\n");
    exit(1);
  }

  // binding the managing socket to a name
  contollerData->mSIN.sin_family = AF_INET;
  contollerData->mSIN.sin_addr.s_addr = htonl(INADDR_ANY);
  contollerData->mSIN.sin_port = contollerData->mPortNumber;

  if (bind (contollerData->mSocket, (struct sockaddr*) &contollerData->mSIN, sizeof contollerData->mSIN) < 0)
  {
    printf("Error binding socket to name\n");
    exit(1);
  }

  // listening to connection requests
  listen(contollerData->mSocket, contollerData->mNumSwitches);
}

void SetControllerData(ProgramStateData* programStateData, char* arguments[])
{
  programStateData->mControllerData.mNumSwitches = atoi(arguments[2]);
  programStateData->mControllerData.mPortNumber = atoi(arguments[3]);
  if (programStateData->mControllerData.mNumSwitches > 0 && programStateData->mControllerData.mNumSwitches < 8)
  { 
    programStateData->mStateDataValid = 1; 
    InitializeSocketsForController(programStateData);
  }
  else
  {
    printf("Invalid number of switches. Must lie in range [1,7]\n");
    exit(1);
  }
}

enum ProgramMode SetProgramMode(int argCount, char * arguments[])
{
  enum ProgramMode programMode = Unassigned;

  if (argCount == 4 && strncmp(arguments[1], "cont", 4) == 0)
  {
    programMode = Controller;
  }

  else if (argCount == 8 && strncmp(arguments[1], "sw", 2) == 0)
  {
    programMode = Switch;
  }

  return programMode;
}

ProgramStateData SetProgramStateData(int argCount, char* arguments[])
{
  ProgramStateData programStateData;
  programStateData.mStateDataValid = -1;
  programStateData.mProgramMode = SetProgramMode(argCount, arguments);

  if (programStateData.mProgramMode == Unassigned) return programStateData;

  if (programStateData.mProgramMode == Switch) SetSwitchData(&programStateData, arguments);
  else if (programStateData.mProgramMode == Controller) SetControllerData(&programStateData, arguments);

  PrintProgramData(&programStateData);
  return programStateData;
}


void PerformControllerLoop(ControllerData* controllerData)
{
  PacketStatsForController packetStats;

  // poll the connections
  struct pollfd fdArray[controllerData->mNumSwitches + 1]; // the first element is the master socket
  int pollResult;
  char buffer[MSG_BUF];
  int bytes_in;
  int numConnectedSwitches = 1;

  fdArray[0].fd = controllerData->mSocket;
  fdArray[0].events = 0;
  fdArray[0].revents = 0;
  fdArray[0].events = POLLIN;

  packetStats.mNumOpen = 0; packetStats.mNumQuery = 0;
  packetStats.mNumAck = 0; packetStats.mNumAdd = 0;

  while (1)
  {
    pollResult = poll(fdArray, numConnectedSwitches, 100); // wait 100 until timeout

    if (pollResult == -1) printf("Poll Error\n");
    else
    {
      if ((numConnectedSwitches < MAX_NSW) && fdArray[0].revents & POLLIN)
      {
        int mSFROMLen = sizeof (controllerData->mSFROM);
        controllerData->mSocketFD[numConnectedSwitches] = accept(controllerData->mSocket, (struct sockaddr*) &controllerData->mSFROM, mSFROMLen);

        fdArray[numConnectedSwitches].fd = controllerData->mSocketFD[numConnectedSwitches];
        fdArray[numConnectedSwitches].events = POLLIN;
        fdArray[numConnectedSwitches].revents = 0;
        numConnectedSwitches++;
      }

      for (int i = 1; i < numConnectedSwitches; i++)
      {
        // checking the data sockets
        if (fdArray[i].revents & POLLIN)
        {
            if ((bytes_in = read(fdArray[i].fd, buffer, MSG_BUF)) > 0)
            {
              buffer[bytes_in] = '\0';
            ReceivePacketForController(controllerData, &packetStats, buffer, bytes_in + 1);
            }
        }
      }
    }

    {
      // poll the keyboard for input
      struct pollfd keyboardFDArray[0];
      int keyboardPollResult;
      char keyboardBuffer[MSG_BUF];
      int bytes_in;

      keyboardFDArray[0].fd = STDIN_FILENO;
      keyboardFDArray[0].events = 0;
      keyboardFDArray[0].events |= POLLIN;

      keyboardPollResult = poll(keyboardFDArray, 1, 100);
      if (keyboardPollResult == -1) printf("Poll error on keyboard\n");
      else if (keyboardFDArray[0].revents & POLLIN)
      {
        if ((bytes_in = read(keyboardFDArray[0].fd, keyboardBuffer, MSG_BUF)) > 0)
        keyboardBuffer[bytes_in] = '\0';
        if (strncmp(keyboardBuffer, "list", bytes_in - 1) == 0)
        {
          ListForController(controllerData, &packetStats);
        }
        else if (strncmp(keyboardBuffer, "exit", bytes_in - 1) == 0)
        {
          ExitForController(controllerData, &packetStats);
        }
      }
    }
  }
}

void PerformSwitchLoop(SwitchData* switchData, PacketStatsForSwitch* packetStats)
{
  FILE* trafficFile = fopen(switchData->mTrafficFileName, "r");
  if (trafficFile == NULL) printf("Error opening file\n");
  char line[MSG_BUF];
  while(1)
  {
    // read one line from Traffic File
    if (fgets(line, sizeof(line), trafficFile))
    {
      ProcessTrafficFileLine(line, switchData, packetStats);
    }

    {
      // poll the keyboard for input
      struct pollfd keyboardFDArray[1];
      int keyboardPollResult;
      char keyboardBuffer[MSG_BUF];
      int bytes_in;

      keyboardFDArray[0].fd = STDIN_FILENO;
      keyboardFDArray[0].events = 0;
      keyboardFDArray[0].events |= POLLIN;

      keyboardPollResult = poll(keyboardFDArray, 1, 10);
      if (keyboardPollResult == -1) printf("Poll error on keyboard\n");
      else if (keyboardFDArray[0].revents & POLLIN)
      {
        if ((bytes_in = read(keyboardFDArray[0].fd, keyboardBuffer, MSG_BUF)) > 0)
        keyboardBuffer[bytes_in] = '\0';
        if (strncmp(keyboardBuffer, "list", bytes_in - 1) == 0)
        {
          ListForSwitch(switchData, packetStats);
        }
        else if (strncmp(keyboardBuffer, "exit", bytes_in - 1) == 0)
        {
          ExitForSwitch(switchData, packetStats);
        }
      }
    }
    // poll the left node if it exists
    if (switchData->mLeftSwitchNode != -1)
    {
      struct pollfd leftNodeFdArray[1];
      int leftNodePollResult;
      char leftNodeBuffer[MSG_BUF];
      int bytes_in;

      leftNodeFdArray[0].fd = switchData->mFDIn[1];
      leftNodeFdArray[0].events = 0;
      leftNodeFdArray[0].events |= POLLIN;

      leftNodePollResult = poll(leftNodeFdArray, 1, 10);

      if (leftNodePollResult == -1) printf("Poll error on left node\n");
      else if (leftNodeFdArray[0].revents & POLLIN)
      {
        if ((bytes_in = read(leftNodeFdArray[0].fd, leftNodeBuffer, MSG_BUF)) > 0)
        leftNodeBuffer[bytes_in] = '\0';
        ProcessPacketForSwitch(leftNodeBuffer, switchData, packetStats, 1);
      }
    }
    // poll the right node if it exists
    if (switchData->mRightSwitchNode != -1)
    {
      struct pollfd rightNodeFdArray[1];
      int rightNodePollResult;
      char rightNodeBuffer[MSG_BUF];
      int bytes_in;

      rightNodeFdArray[0].fd = switchData->mFDIn[2];
      rightNodeFdArray[0].events = 0;
      rightNodeFdArray[0].events |= POLLIN;

      rightNodePollResult = poll(rightNodeFdArray, 1, 10);

      if (rightNodePollResult == -1) printf("Poll error on left node\n");
      else if (rightNodeFdArray[0].revents & POLLIN)
      {
        if ((bytes_in = read(rightNodeFdArray[0].fd, rightNodeBuffer, MSG_BUF)) > 0)
        rightNodeBuffer[bytes_in] = '\0';
        ProcessPacketForSwitch(rightNodeBuffer, switchData, packetStats, 2);
      }
    }
  }
}

void InitializeSwitch(SwitchData* switchData, PacketStatsForSwitch* packetStats)
{
  // initialize the packetStats
  packetStats->mNumAdmit = 0; packetStats->mNumAck = 0;
  packetStats->mNumAddRule = 0; packetStats->mNumRelayIn = 0;
  packetStats->mNumOpen = 0; packetStats->mNumQuery = 0;
  packetStats->mNumRelayOut = 0;

  switchData->mFlowTable[0].mSrcIPLow = 0; switchData->mFlowTable[0].mSrcIPHigh = MAXIP;
  switchData->mFlowTable[0].mDestIPLow = switchData->mIPLow; switchData->mFlowTable[0].mDestIPHigh = switchData->mIPHigh;
  switchData->mFlowTable[0].mActionType = Relay; switchData->mFlowTable[0].mActionValue = 3;
  switchData->mFlowTable[0].mPriority = 4; switchData->mFlowTable[0].mPacketCount = 0;

  switchData->mNumFlowTablesEntries = 1;
  // Even though the FIFOs to/from controller is opened in
  // non-blocking mode, we will still block for this`
  struct pollfd fdArray[1];
  fdArray[0].fd = switchData->mFDIn[0]; // the FIFO to read from Controller
  fdArray[0].events = 0;
  fdArray[0].events |= POLLIN;
  int pollResult;
  char buffer[MSG_BUF];

  // send the Open request to controller
  SendOpenRequest(switchData);
  packetStats->mNumOpen++;

  // wait for Ack packet
  pollResult = poll(fdArray, 1, -1); //block until Ack received
  if (pollResult == -1) printf("Poll Error\n");
  else if (fdArray[0].revents & POLLIN)
  {
    int bytes_in = read(fdArray[0].fd, buffer, MSG_BUF);
    buffer[bytes_in] = '\0';
    if (strncmp(buffer, "Ack", bytes_in) == 0)
    printf ("Received Ack packet from controller\n");
    packetStats->mNumAck++;
  }

}

void OpenFIFOs(ProgramStateData* programStateData)
{
  if (programStateData->mProgramMode == Controller) OpenControllerFIFOs(&programStateData->mControllerData);
  else if (programStateData->mProgramMode == Switch) OpenSwitchFIFOs(&programStateData->mSwitchData);
}

int main(int argc, char *argv[])
{
  // setting CPU limits
  struct rlimit	cpuLimit;
  cpuLimit.rlim_cur= cpuLimit.rlim_max= CPU_LIMIT;
  if (setrlimit (RLIMIT_CPU, &cpuLimit) < 0 ) 
  {
    fprintf (stderr, "%s: setrlimit \n", argv[0]);
    exit (1);
  }    	
  getrlimit (RLIMIT_CPU, &cpuLimit);
  printf ("cpuLimit: current (soft)= %lud, maximum (hard)= %lud \n", cpuLimit.rlim_cur, cpuLimit.rlim_max);

  ProgramStateData mProgramStateData = SetProgramStateData(argc, argv);
  if (mProgramStateData.mStateDataValid == -1) { printf("Invalid input parameters"); }
  else
  {
    OpenFIFOs(&mProgramStateData);
  }
  if (mProgramStateData.mProgramMode == Controller) PerformControllerLoop(&mProgramStateData.mControllerData);
  else if (mProgramStateData.mProgramMode == Switch)
  {
    PacketStatsForSwitch packetStats;
    InitializeSwitch(&mProgramStateData.mSwitchData, &packetStats);
    PerformSwitchLoop(&mProgramStateData.mSwitchData, &packetStats);
  }
}
