#define CLIENTS 5
#define SERVERS 2
#define CLIENT_OPERATIONS 100
#define POOL_SIZE 15

typedef struct {
	int	 code;
	long index;
	long value;
} MessageBody;

typedef struct {
	long type;
	MessageBody data;
} Message;

typedef struct {
	int value, dirty;
} Cell;