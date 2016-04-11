typedef struct{
	char name[255];
	uint16_t type;
	uint16_t class; //=1
	int32_t ttl; //=0
	uint16_t rdlength;
	char rdata[64];
}RR;

typedef struct{
uint16_t id; 
uint16_t param; // qr:0 for query , 1 for response, opcode[4], many more
uint16_t noques,noans,noauth,noadd;
}Header;

typedef struct{
uint8_t length;
char label[64];

}Question;

typedef struct{
Header header;
//one question support
Question question[20];
uint16_t qtype;
uint16_t qclass;//=1

RR answer[20];
RR authority[20];
RR additional[20];
}Msg;
