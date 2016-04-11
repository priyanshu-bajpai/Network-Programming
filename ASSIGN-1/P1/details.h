
struct buffer
{
char data[504];
int datagram_no;
int status;
/*
status = 1 - data
status = -1 - file not found, error
status = 0 - end of file
*/
};