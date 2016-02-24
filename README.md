# client.server-application
Client-server application. Unix I/O. Unix client/server networking

In this application the server's main process monopolizes a port and waits for a client to connect (with accept()). After the client (which I have already written for you) connects it sends a text file to the server (see the Protocol Outline) below. The server then fork()s a child process to handle that child. The child:

    receives the text file (according to the protocol),
    writes it as its first temporary file,
    converts the first temporary file into a second PS file (with enscript),
    converts the second temporary PS file into a third PDF file (with ps2pdf12),
    sends the third temporary file back to the client (according to the protocol),
    erases all three temporary files
