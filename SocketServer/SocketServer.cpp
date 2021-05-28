#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <winsock.h>
#include <windows.h>
#include <time.h>




#pragma comment(lib, "wsock32.lib")   
#define PORT 5000         // 사용포트는 5000
#define MAX_CLIENT 3  // 최대 허용 인원 수 3개
#define ALLOW 65535         // 최대 생성 가능 소켓 번호 65535
#define MAX_CARD_FRUIT 4
#define MAX_CARD_NUM 5

int COUNT = 0; // 카드 낸 횟수 세기


void recv_client(void* ns);   // 스레드 함수 프로토 타입
void playgame();
void broadcast_all(char* mess);      //전체 메세지 보내는 함수
void broadcast_cardnum();         //남은 카드 숫자 알려주는 함수
void pressBell(int i);              //카드 또는 벨을 입력했을 때 실행되는 함수
void randomcard(int i);             //카드를 랜덤하게 생성해주는 함수
void broadcast_cardinfo();          //카드의 정보를 보여주는 함수
void resetFruitState();             //벨을 눌렸을 때, 사용자의 카드 정보를 초기화 시켜주는 함수
void finish(int i);                      //카드를 눌렀을 때, 카드의 수가 적거나 0이되었을때 알려주기
void winner();
void voidBuffer(SOCKET s);
int client_num = 0;         // 점유 횟수 (클라이언트 갯수)
int seat = 0;            // 클라언트 번호
char welcome_ok[] = "User Welcome.\n\0";   // Welcome 정상 초기 글
char welcome_full[] = "Can't Connected.(FULLY OCCUPIED)\n";               // Welcome 사용자 초가시 생기는 글
char game_start[] = "\n-------------------------------------------\n3/3 Game Start.\n card / bell Enter \n";  //게임을 시작한다고 사용자에게 전송
int client_sock[ALLOW];      // client_sock (클라이언트 Welcome Socket)
HANDLE hMutex = 0;         // 뮤택스
int po = 0;      //포트넘버 받아주는 변수
int turn = 9;


typedef struct card_state {
    char Fruit[10] = { 0 };
    int F_num = 0;
};

typedef struct player {
    int socket_num = 0;
    int card_num = 30;
    int player_id = 0;
    struct card_state Card_state;
}PLAYER;


const char* fruit[MAX_CARD_FRUIT] = { "Banana", "Strawberry", "Grape", "Apple" };


PLAYER p[MAX_CLIENT];

int main()
{
    int i = 0;
    p[0].player_id = 1;
    p[1].player_id = 2;
    p[2].player_id = 3;
    // Welcome Screen
    printf("+---------------------------+\n");
    printf("+ Halli Galli Game          +\n");
    printf("+ Server                    +\n");
    printf("+---------------------------+\n");



    // 뮤택스 생성
    hMutex = CreateMutex(NULL, FALSE, NULL);   // 생성 실패시 오류
    if (!hMutex)
    {
        printf("Mutex error\n");
        CloseHandle(hMutex);
        return 1;
    }

    // 윈속 초기화
    WSADATA wsd;
    if (WSAStartup(MAKEWORD(1, 1), &wsd) != 0)   // 사용 소켓 버전은 1.1
    {
        printf("Winsock error\n");
        WSACleanup();
        return 1;

    }

    // Listen 소켓 생성
    int s, addrsize, ret;
    sockaddr_in server, client;

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == SOCKET_ERROR)
    {
        printf("socket() error\n");
        closesocket(s);
        WSACleanup();
        return 1;
    }

    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    // Bind 하기
    if (bind(s, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        printf("bind() error\n");
        closesocket(s);
        WSACleanup();
        return 1;

    }

    printf("Waiting for client.. %d left\n", MAX_CLIENT - client_num);
    listen(s, 10);
    addrsize = sizeof(client);

    // 사용자의 접속을 기다립니다.
    while (1)
    {
        // Blocking 방식으로 Client 를 기다립니다.
        client_sock[seat] = accept(s, (sockaddr*)&client, &addrsize);

        // accept 시(중요, client_num 가 accept() 함수 수행중 에 변할수 있으므로
        // MAX_CLIENT 도달시랑 따로 accept() 시 문제 발생 가능성 있음
        if (client_num < MAX_CLIENT - 1)      // 정상 맞이 하기
        {

            if (client_sock[seat] != INVALID_SOCKET || client_sock[seat] != SOCKET_ERROR) {}
            _beginthread(recv_client, 0, &client_sock[seat]);
            Sleep(10);
            printf("User %d client connected from %s:%d\n", seat, inet_ntoa(client.sin_addr), ntohs(client.sin_port));

        }

        else if (client_num == MAX_CLIENT - 1) //게임 시작
        {
            if (client_sock[seat] != INVALID_SOCKET || client_sock[seat] != SOCKET_ERROR) {}
            _beginthread(recv_client, 0, &client_sock[seat]);
            Sleep(10);
            printf("User %d client connected from %s:%d \n", seat, inet_ntoa(client.sin_addr), ntohs(client.sin_port));
            printf(game_start);
            seat = 0;
            for (int i = 0; i < MAX_CLIENT; i++) {
                ret = send(client_sock[seat], game_start, sizeof(game_start), 0);
                seat++;
            }

            //게임 시작
            if (seat == MAX_CLIENT) {
                playgame();
                if (seat == MAX_CLIENT - 1) {           //추가
                    playgame();
                }
            }
        }

        else   // 가득 찼다. 더이상 못들어오게
        {
            addrsize = sizeof(client);
            if (client_sock[seat] == INVALID_SOCKET)
            {
                printf("accept() error\n");
                closesocket(client_sock[seat]);
                closesocket(s);
                WSACleanup();
                return 1;
            }

            ret = send(client_sock[seat], welcome_full, sizeof(welcome_full), 0);
            closesocket(client_sock[seat]);

            // 메시지 보내고 바로 끊는다.

        }
    }

    return 0;

}


void recv_client(void* ns)
{
    // 정상적으로 받아 드릴때, 스레드 실행
    // 클라이언트의 숫자를 늘림
    WaitForSingleObject(hMutex, INFINITE);
    client_num++;				// 클라이언트 갯수 증가
    seat++;						// 클라이언트 번호 증가
    printf("%d Users left.. \n", MAX_CLIENT - client_num);    	// 갯수로 판단

    ReleaseMutex(hMutex);


    char welcome[100] = { 0 };		// accept 된 소켓에게 줄 버퍼 생성
//    char buff[1000000] = { 0 };
    int ret, i, j = 0;


    _itoa_s(seat, welcome, 10);			// 클라이언트 번호
    strcat_s(welcome, welcome_ok);		// 정상 환영 메시지 환영
    ret = send(*(SOCKET*)ns, welcome, sizeof(welcome), 0);		// 전송
    p[po].socket_num = *(SOCKET*)ns;                  //클라이언트 포트 넘버 저장
    po++;
    while (ret != SOCKET_ERROR || ret != INVALID_SOCKET)
    {
        static char buff[1000000] = { 0 };
        ret = recv(*(SOCKET*)ns, buff, 1000000, 0);		// 클라이언트의 메시지를 받음

        // broadcast 부분
        for (i = 0; i < MAX_CLIENT; i++)
        {
            // 받은 클라이언트 소켓의 메모리 주소와 보내는 클라이언트 소켓 메모리 주소가 다를때만 전송
            char bell[10] = "bell";
            char card[10] = "card";
            char textmessage[100];              //카드와 벨을 제외한 사용자가 보내는 채팅메세지
            if (strstr(buff, card) != NULL) {                       //특정 사용자가 카드를 눌렀을 때 사용자에게 알려주고 카드수를 줄인다

                if (*(SOCKET*)ns == p[0].socket_num)
                {
                    finish(0);
                    printf("** USER %d CHOSSE CARD! ** \n", p[0].player_id);
                    if (p[0].card_num != 0) {
                        if (turn == 1) {
                            randomcard(0);
                            broadcast_cardinfo();
                            p[0].card_num--;
                            break;
                        }
                        else {
                            char message1[100] = "NOT YOUR TURN! \n";
                            int ret = send(p[0].socket_num, message1, sizeof(message1), 0);
                            break;
                        }
                    }
                }
                else if (*(SOCKET*)ns == p[1].socket_num)
                {
                    finish(1);
                    printf("** USER %d CHOSSE CARD! ** \n", p[1].player_id);
                    if (p[1].card_num != 0) {
                        if (turn == 2) {
                            randomcard(1);
                            broadcast_cardinfo();
                            p[1].card_num--;
                            break;
                        }
                        else {
                            char message1[100] = "NOT YOUR TURN! .\n";
                            int ret = send(p[1].socket_num, message1, sizeof(message1), 0);
                            break;
                        }

                    }
                }
                else if (*(SOCKET*)ns == p[2].socket_num)
                {
                    finish(2);
                    printf("** USER %d CHOOSE CARD! ** \n", p[2].player_id);
                    if (p[2].card_num != 0) {
                        if (turn == 3) {
                            randomcard(2);
                            broadcast_cardinfo();
                            p[2].card_num--;
                            break;
                        }
                        else {
                            char message1[100] = "NOT YOUR TURN! .\n";
                            int ret = send(p[2].socket_num, message1, sizeof(message1), 0);
                            break;
                        }

                    }
                }

            }
            if (strstr(buff, bell) != NULL) {                   //특정 사용자가 벨을 눌렀을 때 

                if (*(SOCKET*)ns == p[0].socket_num)
                {
                    printf("** User %d choose BELL! ** \n", p[0].player_id);
                    if (p[0].card_num >= 0) {
                        pressBell(0);
                        finish(0);
                        resetFruitState();
                        broadcast_cardinfo();

                        broadcast_cardnum();
                    }


                    break;
                }
                else if (*(SOCKET*)ns == p[1].socket_num)
                {
                    printf("** User %d choose BELL! ** \n", p[1].player_id);
                    if (p[1].card_num >= 0) {
                        pressBell(1);
                        finish(1);
                        resetFruitState();
                        broadcast_cardinfo();
                        broadcast_cardnum();
                    }

                    break;
                }
                else
                {
                    printf("** User %d choose BELL! ** \n", p[2].player_id);
                    if (p[2].card_num >= 0) {
                        pressBell(2);
                        finish(2);
                        resetFruitState();
                        broadcast_cardinfo();
                        broadcast_cardnum();
                    }


                    break;
                }

            }

            if (strstr(buff, card) == NULL && strstr(buff, bell) == NULL) {                     //채팅 보내기
                if (*(SOCKET*)ns == p[0].socket_num)
                {
                    printf("User %d : %s \n", p[0].player_id, buff);


                    char message2[100];
                    _snprintf(message2, sizeof(message2), "User %d : %s", p[0].player_id, buff);
                    for (int j = 0; j < MAX_CLIENT; j++) {
                        if (j != 0) {
                            int ret = send(p[j].socket_num, message2, sizeof(message2), 0);
                        }
                    }

                    break;
                }
                else if (*(SOCKET*)ns == p[1].socket_num)
                {
                    printf("User %d : %s \n", p[1].player_id, buff);

                    char message2[100];
                    _snprintf(message2, sizeof(message2), "User %d : %s", p[1].player_id, buff);
                    for (int j = 0; j < MAX_CLIENT; j++) {
                        if (j != 1) {
                            int ret = send(p[j].socket_num, message2, sizeof(message2), 0);
                        }
                    }

                    break;
                }
                else
                {
                    printf("User %d : %s \n", p[2].player_id, buff);
                    char message2[100];
                    _snprintf(message2, sizeof(message2), "User %d : %s", p[2].player_id, buff);
                    for (int j = 0; j < MAX_CLIENT; j++) {
                        if (j != 2) {
                            int ret = send(p[j].socket_num, message2, sizeof(message2), 0);
                        }
                    }

                    break;
                }
            }


            ReleaseMutex(hMutex);

        }



        memset(buff, 0, 1000000);

    }

    // 접속된 소켓이 연결을 해제 시켰을때
    WaitForSingleObject(hMutex, INFINITE);
    client_num--;
    printf("User %d gone! \n %d Users left ..\n", seat, MAX_CLIENT - client_num);
    ReleaseMutex(hMutex);

    closesocket(*(int*)ns);

    return;
}

void voidBuffer(SOCKET s) {
    u_long tmpl, i;
    char tmpc;
    ioctlsocket(s, FIONREAD, &tmpl);
    for (i = 0; i < tmpl; i++) recv(s, &tmpc, sizeof(char), 0);
}

void finish(int i) { // card 냈을 때 처리문
    /*if (p[i].card_num < 6 && p[i].card_num >=0 ) { // 5개 이하 경고문
        char message[50];
        sprintf_s(message, "%d player's Number Of Cards: %d", p[i].player_id, p[i].card_num);
        for (int j = 0; j < MAX_CLIENT; j++) { // 모두에게 알리기
            int ret = send(p[j].socket_num, message, sizeof(message), 0);
        }
    }*/
    if (p[i].card_num < 0) { // 카드 없음 게임 종료


        printf("%d player kicked! \n", p[i].player_id);
        for (int j = 0; j < MAX_CLIENT; j++) { // 모두에게 알리기
            char message[50] = { 0 };
            _snprintf(message, sizeof(message), "%d player kicked! \n", p[i].player_id);

            int ret = send(p[j].socket_num, message, sizeof(message), 0);
        }
        p[i].Card_state.F_num = 0;
        p[i].socket_num = 0;
        closesocket(p[i].socket_num);
    }
}

void winner() {
    int k = 0;
    for (int i = 0; i < MAX_CLIENT; i++) {              //나간 사용자의 소켓넘버는 0으로 초기화되어있음 -> 합이 1이라면 우승자가 나옴

        if (p[i].socket_num != 0) {
            k++;
        }
    }

    if (k == 1) {
        for (int j = 0; j < MAX_CLIENT; j++) {
            if (p[j].socket_num != 0) {
                char message[50] = { 0 };
                _snprintf(message, sizeof(message), "%d player is the winner!!!!!!!!!!!!\n", p[j].player_id);
                printf("%d player is the winner!!!!!!!!!!!!", p[j].player_id);
                int ret = send(p[j].socket_num, message, sizeof(message), 0);
                closesocket(p[j].socket_num);
                WSACleanup();
            }
        }
    }
}


void playgame() {
    char message[200] = "Start from User 1 .\n"; //사용자 1 부터 시작
    broadcast_all(message);
    broadcast_cardnum();
    int ret;
    while (1) {
        if (p[0].card_num > 0) {
            winner();
            char message1[200] = "User 1's turn\n";
            broadcast_all(message1);
            turn = 1;
            Sleep(10);
            ret = send(p[0].socket_num, "Enter!\n", sizeof("Enter!\n"), 0);

            Sleep(10000);
        }


        if (p[1].card_num > 0) {
            winner();
            char message2[200] = "User 2's turn\n";

            broadcast_all(message2);
            turn = 2;
            Sleep(10);
            ret = send(p[1].socket_num, "Enter!\n", sizeof("Enter!\n"), 0);


            Sleep(10000);
        }

        if (p[2].card_num > 0) {
            winner();
            char message3[200] = "User 3's turn\n";

            broadcast_all(message3);
            turn = 3;
            Sleep(10);
            ret = send(p[2].socket_num, "Enter!\n", sizeof("Enter!\n"), 0);

            Sleep(10000);
        }



    }
}


void broadcast_all(char* mess) {
    int ret;


    //strcpy_s(message, sizeof(message), mess);

    for (int i = 0; i < MAX_CLIENT; i++) {
        char message[300] = { 0 };
        _snprintf(message, sizeof(message), mess);
        ret = send(p[i].socket_num, message, sizeof(message), 0);
    }

    return;
}

void broadcast_cardnum() {

    int ret;
    for (int j = 0; j < MAX_CLIENT; j++) {
        if (p[j].card_num >= 0) {
            char message[50] = { 0 };
            p[j].player_id = j + 1;
            _snprintf(message, sizeof(message), "User ID : %d / Number of Cards : %d\n", p[j].player_id, p[j].card_num);
            printf("%s\n", message);
            for (int i = 0; i < MAX_CLIENT; i++) {
                Sleep(100);
                ret = send(p[i].socket_num, message, sizeof(message), 0);
            }
        }
    }

    return;
}

void broadcast_cardinfo() {
    int ret;
    for (int j = 0; j < MAX_CLIENT; j++) {
        char message[50] = { 0 };
        p[j].player_id = j + 1;
        if (p[j].Card_state.F_num != 0) {
            _snprintf(message, sizeof(message), "User ID : %d / Fruits : %s, Number : %d\n", p[j].player_id, p[j].Card_state.Fruit, p[j].Card_state.F_num);
            printf("%s\n", message);
            for (int i = 0; i < MAX_CLIENT; i++) {
                Sleep(20);
                ret = send(p[i].socket_num, message, sizeof(message), 0);
            }
        }
    }

    return;
}

void resetFruitState() {
    for (int i = 0; i < MAX_CLIENT; i++) {
        p[i].player_id = i + 1;
        //strcpy_s(p[i].Card_state.Fruit, NULL);
        p[i].Card_state.F_num = 0;
    }
}

void randomcard(int i) {
    srand(time(NULL));
    //strcpy_s(p[i].Card_state.Fruit, sizeof(p[i].Card_state.Fruit), fruit[rand() % 4]);
    _snprintf(p[i].Card_state.Fruit, sizeof(p[i].Card_state.Fruit), fruit[rand() % 4]);
    p[i].Card_state.F_num = rand() % 5 + 1;
    COUNT++;
}


void pressBell(int i) {
    // 같은 종류의 과일 개수가 5개인지 판단

    //{ "바나나", "딸기", "포도", "사과" };
    //벨을 제대로 눌렀을 경우
    int banana = 0;
    int strawb = 0;
    int grape = 0;
    int apple = 0;




    for (int ii = 0; ii < MAX_CLIENT; ii++) { // 총 개수 구하기
        if (strcmp(p[ii].Card_state.Fruit, "Banana") == 0) {
            banana += p[ii].Card_state.F_num;

        }
        else if (strcmp(p[ii].Card_state.Fruit, "Strawberry") == 0) {
            strawb += p[ii].Card_state.F_num;
        }
        else if (strcmp(p[ii].Card_state.Fruit, "Grape") == 0) {
            grape += p[ii].Card_state.F_num;
        }
        else { // 사과
            apple += p[ii].Card_state.F_num;
        }
    }

    // 5개 넘는거 있는지 구하기
    int check = 0; // 1이면 True
    if (banana == 5 || strawb == 5 || grape == 5 || apple == 5) {
        check = 1;
    }

    if (check == 1) {
        printf("** User %d takes ALL CARDS! **", p[i].player_id);
        if (p[i].card_num >= 0) {
            p[i].card_num += COUNT;
        }

        COUNT = 0;
    }

    else {                                   //벨을 잘못눌렀을 경우

        int nowmem = 0;         //빼야할 카드수 구하기
        for (int j = 0; j < MAX_CLIENT; j++) {
            if (p[j].card_num >= 0) {
                nowmem++;
            }
        }
        if (p[i].card_num >= nowmem) {          //나눠줄 카드 수가 충분할때
            printf("** User %d's mistake! Give your CARDS! **\n", p[i].player_id);
            p[i].card_num = p[i].card_num - (nowmem - 1);
            for (int j = 0; j < MAX_CLIENT; j++) {
                if (p[j].card_num >= 0 && j != i) {       //게임에 참여중인 사람에게만 카드 주기
                    p[j].card_num = p[j].card_num + 1;
                }
            }
        }
        else {
            p[i].card_num = -1;
            finish(i);
        }
    }

}