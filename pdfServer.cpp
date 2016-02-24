/*-------------------------------------------------------------------------*
 *---									---*
 *---		pdfServer.cpp						---*
 *---									---*
 *---	    This file defines a server that receives a joDoc word	---*
 *---	processor file over a socket, creates a PDF file of it, and	---*
 *---	sends that PDF file back over the socket to the client.		---*
 *---									---*
 *---	----	----	----	----	----	----	----	---	---*
 *---									---*
 *---	Version 1.0		2012 May 25		Joseph Phillips	---*
 *---									---*
 *-------------------------------------------------------------------------*
 *---									---*
 *---		pdfServer.c						---*
 *---									---*
 *---	    Revised to:							---*
 *---	(1) receive a text file over a socket, create a PDF file of it,	---*
 *---	    and to send that PDF file back to the client.		---*
 *---	(2) be implemented in C instead of C++.				---*
 *---	(3) use more modern library fncs (strtol() instead of atoi(),	---*
 *---	     sigaction() instead of signal(), etc.)			---*
 *---									---*
 *---	----	----	----	----	----	----	----	---	---*
 *---									---*
 *---	Version 2.0		2014 February 23	Joseph Phillips	---*
 *---									---*
 *---				2015 March 2		Joseph Phillips	---*
 *---									---*
 *---	    Officially removed ncurses functionality.			---*
 *---									---*
 *-------------------------------------------------------------------------*/

//	Compile with:
// $ gcc pdfServer.c -o pdfServer


//Moon Lee
//Systems2
//HW 4

#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>		// For memset()
#include	<sys/types.h>
#include	<sys/stat.h>		// For stat()
#include	<unistd.h>
#include	<fcntl.h>		// For open()
#include	<sys/socket.h>		// For socket()
#include	<netinet/in.h>		// For INADDR_ANY
#include	<signal.h>		// For sigaction
#include	<sys/wait.h>		// For WNOHANG
#include <errno.h>

#include	"headers.h"

//  PURPOSE:  To keep track of the current number of client handling children
//	that exist.
int 	numChildren	= 0;


//  PURPOSE:  To acknowledge the ending of child processes so they don't
//	stay zombies.  Uses a while-loop to get one or more processes that
//	have ended, and decrements 'numChildren'.  Ignores 'sig'.
//	No return value.
void	childHandler	(int	sig)
{
  //  I.  Application validity check:

  //  II.  'wait()' for finished child(ren):
  int status;
  pid_t childId;

  while ( (childId = waitpid(-1,&status,WNOHANG)) > 0)
  {
    numChildren--;
    if (WIFEXITED(status))
    {
      if (WEXITSTATUS(status) == EXIT_SUCCESS)
        printf("Child %d succeeded.\n",childId);
      else
        printf("Child %d failed.\n",childId);
    }
    else
      printf("Child %d crashed.\n",childId);
  }

  //  III.  Finished:
}


//  PURPOSE:  To install 'childHandler()' as the 'SIGCHLD' handler.  No
//	parameters.  No return value.
void	installChildHandler	()
{
  //  I.  Application validity check:

  //  II.  Install handler:

  // Set up struct to specify the new action.
  struct sigaction act;
  memset(&act,'\0',sizeof(struct sigaction));
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_NOCLDSTOP | SA_RESTART;;
  act.sa_handler = childHandler;
  // Handle with simpleHandler()
  sigaction(SIGCHLD,&act,NULL);

  //  III.  Finished:
}


//  PURPOSE:  To note that signal 'SIGCHLD' is to be ignored.  No parameters.
//	No return value.
void	ignoreSigChld	()
{
  //  I.  Application validity check:

  //  II.  Ignore SIGCHLD:
  struct sigaction act;
  memset(&act,'\0',sizeof(struct sigaction));
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_NOCLDSTOP | SA_RESTART;;
  act.sa_handler = SIG_DFL;
  // Handle with simpleHandler()
  sigaction(SIGCHLD,&act,NULL);
  //  III.  Finished:
}


//  PURPOSE:  To return a port number to monopolize, either from the
//	'argv[1]' (if 'argc' > 1), or by asking the user (otherwise).
int	obtainPortNumber	(int	argc,
				 char*	argv[]
				)
{
  //  I.  Application validity check:

  //  II.  Obtain port number:
  int	portNum;

  if  (argc > 1)
  {
    //  II.A.  Attempt to obtain port number from 'argv[1]':
    portNum = strtol(argv[1],NULL,10);
  }
  else
  {
    //  II.B.  Attempt to obtain port number from user:
    char	numText[NUM_TEXT_LEN];

    do
    {
      printf("Port number (%d-%d)? ",LO_LEGAL_PORT_NUM,HI_LEGAL_PORT_NUM);
      fgets(numText,NUM_TEXT_LEN,stdin);
      portNum = strtol(numText,NULL,10);
    }
    while  ((portNum < LO_LEGAL_PORT_NUM) || (portNum > HI_LEGAL_PORT_NUM));

  }

  //  III.  Finished:
  return(portNum);
}

void cleanFilesysAfterHandlingClient(const char* inFilename,const char* psFilename,const char* pdfFilename){
  remove(inFilename);
  remove(psFilename);
  remove(pdfFilename);
}

//  PURPOSE:  To return a socket for listening for clients if obtained one from
//	OS, or to send an appropriate error msg to 'stderr' and return -1
//	otherwise.  Sets '*portPtr' to the number of the port to use.
//	No parameters.  No parameters.
int 	createListeningSocket	(int	argc,
				 char*	argv[],
				 int*	portPtr
				)
{
  //  I.  Applicability validity check:

  //  II.  Create server socket:
  int socketDescriptor = ERROR_FD;
  //  YOUR CODE HERE
  socketDescriptor =socket(AF_INET, // AF_INET domain
              SOCK_STREAM, // Reliable TCP
              0);

 // We'll fill in this datastruct
  struct sockaddr_in socketInfo;
  // Fill socketInfo with 0's
  memset(&socketInfo,'\0',sizeof(socketInfo));
  // Use std TCP/IP
  socketInfo.sin_family = AF_INET;
  // Tell port in network endian with htons()
  socketInfo.sin_port = htons(*portPtr);
  // Allow machine to connect to this service
  socketInfo.sin_addr.s_addr = INADDR_ANY;
  // Try to bind socket with port and other specifications
  int status = bind(socketDescriptor, // from socket()
        (struct sockaddr*)&socketInfo,
        sizeof(socketInfo)
       );

  if  (status < 0)
  {
    fprintf(stderr,"Could not bind to port %d\n",*portPtr);
    exit(EXIT_FAILURE);
  }

  listen(socketDescriptor,5);

  //  III.  Finished:
  return(socketDescriptor);
}


//  PURPOSE:  To attempt to obtain the filename and file size from the client
//	via 'clientDescriptor' and to put them into 'filename' and
//	'*filesizePtr' respectively.  'maxFilenameLen' tells the length of the
//	'filename' char array.  Returns '1' if successful or '0' otherwise.
int	didReadNameAndSize
			(int	clientDescriptor,
			 char*	filename,
			 int	maxFilenameLen,
			 int*	filesizePtr
			)
{
  //  I.  Application validity check:

  //  II.  Read filename and file size:
  int numRead = read(clientDescriptor,filename,maxFilenameLen);
  if (-1 == numRead){
    fprintf(stderr,"Could not read filename.\n");
    return 0;
  }

  char filesize[NUM_TEXT_LEN];
  numRead = read(clientDescriptor,filesize,NUM_TEXT_LEN);
  if (-1 == numRead){
    fprintf(stderr,"Could not read size of file %s.\n",filename);
    return 0;
  }

  int temp = strtol(filesize,NULL,10);
  if (temp <=0){
    fprintf(stderr,"Illegal file size for file %s.\n",filename);
    return 0;
  }

  *filesizePtr = temp;
  //  III.  Finished:
  return(1);
}


//  PURPOSE:  To do the work of handling the client with whom socket descriptor
//	'clientDescriptor' communicates.  Returns 'EXIT_SUCCESS' on success or
//	'EXIT_FAILURE' otherwise.
int	serveClient	(int		clientDescriptor,
			 const char*	filename,
			 int		fileSize
			)
{
  //  I.  Application validity check:

  //  II.  Serve client:
  //  II.A.  Create names of temporary files:
  //  Open file for writing

  printf("serveClient\n");
  char tempFilename[FILENAME_LEN];
  snprintf(tempFilename,FILENAME_LEN,TEMP_FILENAME_PREFIX,getpid());
  strcat(tempFilename,filename);
  strcat(tempFilename,".tmp");
  printf("tempFilename: %s\n",tempFilename);
  char psFilename[FILENAME_LEN];
  snprintf(psFilename,FILENAME_LEN,TEMP_FILENAME_PREFIX,getpid());
  strcat(psFilename,filename);
  strcat(psFilename,".ps");
  printf("psFilename: %s\n",psFilename);
  char pdfFilename[FILENAME_LEN];
  snprintf(pdfFilename,FILENAME_LEN,TEMP_FILENAME_PREFIX,getpid());
  strcat(pdfFilename,filename);
  strcat(pdfFilename,".pdf");
  printf("pdfFilename: %s\n",pdfFilename);

  int tempFd = open(tempFilename,O_WRONLY|O_TRUNC|O_CREAT,0660); 
  int psFd = open(psFilename,O_WRONLY|O_TRUNC|O_CREAT,0660); 
  int pdfFd = open(pdfFilename,O_WRONLY|O_TRUNC|O_CREAT,0660);

  
  close(psFd);
  close(pdfFd);

  int numBytes;
  char fileBuffer[LINE_LEN];
  int bytesRead = 0;
  printf("readFile %d\n",fileSize);
  while ( bytesRead < fileSize && (numBytes = read(clientDescriptor,fileBuffer,LINE_LEN)) > 0){
      printf("reading %d\n",fileSize);
      bytesRead += numBytes;
      int numWritten = write(tempFd,fileBuffer,numBytes);
      printf("numBytes %d numWritten %d\n", numBytes,numWritten);
      if (numBytes != numWritten){
        fprintf(stderr,"Could not write file %s\n",tempFilename);
        return(EXIT_FAILURE);
      }
  }

  close(tempFd);

  printf("enscript\n");
  if ( 0 == fork() ){
    execl("/usr/bin/enscript","enscript","-B",tempFilename,"-p",psFilename,"-q",NULL);
  }
  else{
    int status;
    wait(&status);
  }

  printf("ps2pdf12\n");
  if ( 0 == fork() ){
    execl("/usr/bin/ps2pdf12","ps2pdf12",psFilename,pdfFilename,NULL);
  }
  else{
    int status;
    wait(&status);
  }

  
  struct stat statBuffer;
  stat(pdfFilename,&statBuffer);
  char sizeBuf[NUM_TEXT_LEN];
  snprintf(sizeBuf,NUM_TEXT_LEN, "%d", (int)statBuffer.st_size);

  printf("write file size in clientDescriptor %s\n", sizeBuf);
  //Write the size of the file
  int numWritten = write(clientDescriptor,sizeBuf,NUM_TEXT_LEN);
  if (-1 == numWritten){
    fprintf(stderr,"Could not write size of file %s: %s into clientDescriptor.\n",pdfFilename,sizeBuf);
    return(EXIT_FAILURE);
  }


  //Read the pdf file
  pdfFd = open(pdfFilename,O_RDONLY,0660);
  int readBytes;
  char pdfFileBuffer[LINE_LEN];
  printf("read pdfFilename and write in clientDescriptor\n");
  int checkWrittenByte = 0;
  while ( (readBytes = read(pdfFd,pdfFileBuffer,LINE_LEN)) > 0 ){
    printf("read %d bytes\n", readBytes);
    numWritten = write(clientDescriptor,pdfFileBuffer,readBytes);
    if (-1 == numWritten){
      fprintf(stderr,"Could not write file %s into clientDescriptor.\n",pdfFilename);
      return(EXIT_FAILURE);
    }
    checkWrittenByte += numWritten;
  }
  printf("wrote %d bytes into clientDescriptor\n",checkWrittenByte);



  if (readBytes != 0){
    fprintf(stderr,"Could not read %s.\n",pdfFilename);
    return(EXIT_FAILURE);
  }

  close(tempFd);
  close(psFd);
  close(pdfFd);

  cleanFilesysAfterHandlingClient(tempFilename,psFilename,pdfFilename);

  //  III.  Finished:
  return(EXIT_SUCCESS);
}



//  PURPOSE:  To do the work of the server: waiting for clients, fork()-ing
//	child processes to handle them, and going back to wait for more
//	clients.  No return value.
void	doServer	(int	socketDescriptor
			)
{
  //  I.  Application validity check:

  //  II.  Serve clients:
  //  II.A.  Each iteration serves one client:
  char	text[LINE_LEN];
  int	i;

  for  (i = 0;  i < NUM_SERVER_ITERS;  i++)
  {
    int		clientDescriptor = accept(socketDescriptor,NULL,NULL); // <-- CHANGE THAT INTEGER!
    char	filename[FILENAME_LEN];
    int		fileSize;
    pid_t	childPid;

    if  (clientDescriptor < 0)
      continue;

    if  (!didReadNameAndSize(clientDescriptor,filename,FILENAME_LEN,&fileSize))
      continue;

    childPid	= fork(); // <-- CHANGE THAT INTEGER!

    if  (childPid < 0)
      continue;
    else
    if  (childPid == 0)
    {
      ignoreSigChld();
      close(socketDescriptor);
      int status = serveClient(clientDescriptor,filename,fileSize);
      close(clientDescriptor);
      exit(status);
    }
    else
    {
      numChildren++;
      printf("%d: %s (%d bytes) (process %d) ...\n",i,filename,fileSize,childPid);
      close(clientDescriptor);
    }

  }

  //  II.B.  Wait for all children to finish:
  while  (numChildren > 0)
    sleep(1);

  //  III.  Finished:
}


int	main	(int	argc,
		 char*	argv[]
		)
{
  //  I.  Application validity check:

  //  II.  Serve clients
  //  II.A.  Get socket file descriptor:
  int port = obtainPortNumber(argc,argv);
  int socketDescriptor = createListeningSocket(argc,argv,&port);

  if  (socketDescriptor == -1)
    return(EXIT_FAILURE);

  //  II.B.  Set up SIGCHLD handler:
  installChildHandler();

  //  II.C.  Handle clients:
  doServer(socketDescriptor);

  close(socketDescriptor);

  //  III.  Will get here if 'doServer()' quits:
  return(EXIT_SUCCESS);
}

