# Server-Client-Search
Practicing POSIX message queues and multi threaded programing

Server:
Takes request from clients and create a child process for each request.In multi threaded manner, it opens given files and finds the matched words.Result is given back to the client. In communication, POSIX message queues are used.

Client:
Passes the command line arguments to the server in a struct via message queue. It prints the results which is sent by server through another message queue.So all client has to do is arranging an object contains the request information, passing it server, recieving the result and print it.

For running these programs, command line arguments are used.

For server:  server <serverQueueName> 

For client:  client <serverQueueName> <keyword> N <file1> …<fileN>


