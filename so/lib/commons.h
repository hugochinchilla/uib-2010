#define CLIENTS 500
#define SERVERS 2
#define CLIENT_OPERATIONS 1000
#define POOL_SIZE 15000

#define binary_filename "data.bin"

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