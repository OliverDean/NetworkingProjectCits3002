#ifndef TM_H
#define TM_H

struct curUser;
struct Uri;
struct RequestLine;
struct HeaderField;
struct HttpRequest;
struct ContentType;

typedef struct curUser
{
    char username[32];
    char password[32];
    char *QB[10];         // Each question is c or python, indicating where its
    char *QuestionID[10]; // Indicating what the questionID is
    int attempted;        // Max of 10
    int questions[10];    // Each has a max of 3
    int score[10];        // Max of 3
    int total_score;      // Max of 30
    char user_filename[13];
} curUser;

typedef struct {
    char path[500];
    char queryString[500];
} Uri;

typedef struct {
    char method[20];
    Uri uri;
    char version[10];
} RequestLine;

typedef struct {
    char fieldname[500];
    char value[500];
} HeaderField;

typedef struct {
    RequestLine requestLine;
    HeaderField headers[50];
    int headerCount;
} HttpRequest;

typedef enum {
    HTML,
    CSS,
    JS,
    JPEG
} ContentType;

extern void QuestionBanks(int QBsocket, int pipe[2], char *QBversion);
extern void handleRequest(int socket_fd, HttpRequest httpRequest, curUser user);
extern int loadUser(curUser *user);
extern int login(char username[], char password[], curUser *user);
extern void generatenewfile(curUser *user);
extern char *randomStringGenerator();
extern void sendImageResponse(int socket_fd, const char *filePath, ContentType contentType);
extern void sendHttpResponse(int socket_fd, const char *filePath, ContentType contentType, const char *user_filename);
extern Uri parseUri(const char *uriString);
extern HttpRequest parseHttpRequest(const char *request);
const char* getContentTypeString(ContentType contentType);
extern void getQuestion(int cin, int cout, int pin, int pout, int questionIndex, curUser user)
#endif