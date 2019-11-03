# TCPclient
Simple TCPclient for testing out different TCP-based servers. Now supports Guacamole handshake procedure. Platform is either Unix or Windows.
## Compilation on Linux:
gcc tcpclient.c -o tcpclient
## Compilation on Windows:
gcc tcpclient.c -o tcpclient -lws2_32
## Usage:
tcpclient hostname_or_hostip port
