#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <errno.h>
#include <ctype.h>

// 글자 굵기, 색
#define Bold_RED    "\033[1;31m"
#define Bold_GREEN  "\033[1;32m"
#define Bold_BLUE   "\033[1;34m"
#define RESET       "\033[0m"

#define MAX_ARGS 64
#define MAX_LUMPS 64

char *args[MAX_ARGS];       // 개별 단어 (ex. ls)
char *lumps[MAX_LUMPS];     // 단어 덩어리 (ex. ls -al)
char *crits [MAX_LUMPS];    // 기준 "|" "||" "&" "&&" ";" 들
int nl = 0, nc = 0;         // 덩어리(lumps) 수, 기호(crits) 수
int argc = 0;               // 덩어리에서의 단어 인덱스

// "username@hostname"
char *userhost(void){
    static char buf[256];
    char hostname[64];

    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    // 정상적으로 user와 host를 가져오면 username@hostname을 return.
    if ((pw) && (gethostname(hostname, sizeof(hostname))) == 0) {
        snprintf(buf, sizeof(buf), "%s@%s", pw->pw_name, hostname);
        return buf;
    }
    else {
        printf("사용자 정보 로드 실패.");
    }
    return NULL;
}

// current directory 구현
char *curdir(void){
    static char buf[256];
    char cwd[256];

    // cwd에 현재 directory 저장. (ex. /home/username/...)
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return NULL;    //실패
    }

    // home dir 경로 저장. (ex. /home/username )
    struct passwd *pw = getpwuid(getuid());
    if (!pw) {
        return NULL;    //실패
    }
    const char *home = pw->pw_dir;
    size_t hl = strlen(home); // home 경로 길이

    // cwd가 /home/username으로 시작하는지
    if (strncmp(cwd, home, hl) == 0) {
        if (cwd[hl] == '/' || cwd[hl] == '\0') {
            // "/home/username"인 경우
            if (cwd[hl] == '\0') {
                snprintf(buf, sizeof(buf), "~"); // ex) ~
            }
            // "/home/username/..." 인 경우
            else {
                snprintf(buf, sizeof(buf), "~%s", cwd + hl); // ex) ~/...
            }
            return buf;
        }
    }

    // /home/username으로 시작하지 않으면 그대로 저장.
    snprintf(buf, sizeof(buf), "%s", cwd);
    return buf;

}
/*
void run_external(char *args[]) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork 실패");
    }
    // 자식 프로세스
    else if (pid == 0) {
        execvp(args[0], args);

        printf("%s: command not found \n", args[0]);
        _exit(1);
    }
    // 부모 프로세스
    else {
        int status;
        waitpid(pid, &status, 0);
    }
}
*/

/*
void command(char *input) {
    if (!(strcmp(input, "pwd"))) {                  // pwd
        char cwd[256];
        // cwd에 현재 dir 저장
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            return;    //실패
        }

        printf("%s\n", cwd);

        return;
    }
    else if (!(strcmp(input, "cd"))) {              // cd
            // 인수 없을 때 ex) "cd"
        if (args[1] == NULL || *args[1] == '\0') {
            struct passwd *pw = getpwuid(getuid());
            args[1] = pw ? pw->pw_dir : "/";
            chdir(args[1]);
        }
        if (chdir(args[1]) != 0) {
            perror("cd 실패");
        }
        return;
    }
    else if (!(strcmp(input, "exit"))) {            // exit
        exit(0);
    }
    else {                                          // ls, cat 등 명령어
        run_external(args);
    }

}

// 덩어리를 단어로 토큰화 후 실행.
int run_cmd(char *cmd) {
    char *token = strtok(cmd, " \t");
    while (token != NULL && argc < MAX_ARGS-1) {
        args[argc++] = token;
        token = strtok(NULL, " \t");
    }
    args[argc] = NULL;

    if (args[0]) {
        command(args[0]);
    }
}
*/

int run_cmd(char *lump) {
    char *copy = strdup(lump);
    char *token = strtok(copy, " \t");
    while (token && argc < MAX_ARGS-1) {
        args[argc++] = token;
        token = strtok(NULL, " \t");
    }
    args[argc] = NULL;
    if (!args[0]) { free(copy); return -1; }
    if (strcmp(args[0], "exit") == 0) {
        free(copy);
          exit(0);
    }
    if (strcmp(args[0], "cd") == 0) {
        if (argc < 2 || args[1][0] == '\0') {
            struct passwd *pw = getpwuid(getuid());
            args[1] = pw ? pw->pw_dir : "/";
        }
        if (chdir(args[1]) != 0)
            perror("cd 실패");
        free(copy);
        return 0;
    }
    if (strcmp(args[0], "pwd") == 0) {
        char cwd[256];
        if (getcwd(cwd, sizeof(cwd)))
            printf("%s\n", cwd);
        free(copy);
        return 0;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork 실패"); free(copy); return -1;
    }
    if (pid == 0) {                     // 자식 프로세스
        execvp(args[0], args);
        fprintf(stderr, "%s: command not found\n", args[0]);
        _exit(127);
    }
    int status;
    waitpid(pid, &status, 0);
    free(copy);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

void run_cmd_bg(char *lump) {
    int argc = 0;
    char *copy = strdup(lump);
    char *tok = strtok(copy, " \t");
    while (tok && argc < MAX_ARGS-1) {
        args[argc++] = tok;
        tok = strtok(NULL, " \t");
    }
    args[argc] = NULL;
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork 실패");
    } else if (pid == 0) {      // 자식 프로세스
        execvp(args[0], args);
        _exit(1);
    } else {                    // 부모 프로세스
        printf("[BG] PID=%d\n", pid);
    }
    free(copy);
}

int run_pipeline(char *lumps[], int start, int end) {
    int n = end - start + 1;
    int pipefd[2*(n-1)];
    for (int i = 0; i < n-1; i++)
        if (pipe(pipefd+2*i) < 0) perror("pipe 실패");
    pid_t pid;
    int status;
    for (int i = 0; i < n; i++) {
        pid = fork();
        if (pid == 0) {
            if (i > 0)
                dup2(pipefd[2*(i-1)], STDIN_FILENO);
            if (i < n-1)
                dup2(pipefd[2*i+1], STDOUT_FILENO);
            for (int j = 0; j < 2*(n-1); j++) close(pipefd[j]);
            char *argv[MAX_ARGS]; int a=0;
            char *cp = strdup(lumps[start+i]);
            char *t = strtok(cp, " \t");
            while (t) { argv[a++] = t; t = strtok(NULL, " \t"); }
            argv[a] = NULL;
            execvp(argv[0], argv);
            _exit(127);
        }
    }
    for (int j = 0; j < 2*(n-1); j++) close(pipefd[j]);
    for (int i = 0; i < n; i++) wait(&status);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* -메모장-
지금 내가 해야 할 게 파이프라인 / 백그라운드 / 다중명령어
이렇게인데, 음... 일단 그 명령 기호가 있는지 검사(| || & && ;)를 해야 하는데, 
| 이거는 개수별로 pipe_count++ 해서 하나씩 줄여 나가도록..? 근데 || 이거는 2개로 취급하지 않도록 주의.
& 백그라운드는.. 나중에.
|| 이거랑 && 이거는..

"|" "||" "&" "&&" ";" -> 나오는 순서대로 std[0], std[1], std[2]... 에 저장 후 꺼내 쓰기.
-> "|" "||" "&" "&&" ";" 이 5개의 기호를 기준으로 토큰화. lump[0](ex. cat Cykor.txt), lump[1](ex. ls -al), lump[2]
-> 각각 토큰화된 명령 덩어리(ex. cat Cykor.txt)띄어쓰기, \t 기준 으로 다시 토큰화 후 명령 진행. args[0](cat), args[1](Cykor.txt).

crits[i] 값이 
"|" 이면 출력 -> 입력 연결
"&&" 또는 "||" 이면 바로 전 명령(lump)의 실행 여부 검사(어떻게?),
"&" 이면 waitpid 생략(백그라운드),
";" 이면 무조건 순차 실행 (그냥 다음 lump)
*/

// 덩어리로 분리 + 기호 분리(|| && 등)
void split_lumps(const char *line) {
    const char *p     = line;    // 순회용 포인터
    const char *start = p;       // 시작 지점
    nl = nc = 0;

    while (*p) {
        if ((p[0]=='|' && p[1]=='|') || (p[0]=='&' && p[1]=='&')) {     // || 또는 &&
            if (p > start)
                lumps[nl++] = strndup(start, p - start);
            crits [nc++] = strndup(p, 2);

            p     += 2;
            start = p;
        }
        else if (*p=='|' || *p=='&' || *p==';') {                       // | & ;
            if (p > start)
                lumps[nl++] = strndup(start, p - start);
            crits [nc++] = strndup(p, 1);

            p     += 1;
            start = p;
        }
        else {
            p++;
        }
    }

    if (p > start)
        lumps[nl++] = strndup(start, p - start);

    lumps[nl] = NULL;
    crits [nc] = NULL;
}


int main(void) {
    // "username@hostname:"
    char user[256];
    snprintf(user, sizeof(user), Bold_GREEN "%s" RESET":", userhost());

    while(1) {
        // 현재 디렉토리 표현 
        char cwd[256];
        snprintf(cwd, sizeof(cwd), Bold_RED "%s" RESET"$", curdir());

        // 기본 출력 ex) "username@hostname:~/projects$ "
        char base[512];
        snprintf(base, sizeof(base), "%s%s ", user, cwd);
        
        // 입력 받기
        char *line = readline(base);

        if (!line) break;
        if (*line) add_history(line);
        else {          // 엔터만 쳤을 경우.
            free(line);
            continue;
        }
        split_lumps(line);
        free(line);

        int last_status = 0;    // 이전 명령 실패(!0) / 성공(0)
        if (nc > 0) {           // 조건 연산자? ("|" "||" "&" "&&" ";")가 있으면.
            for (int i = 0; i < nl; i++) {      // lump 개수 번 반복.
                if (i>0 && strcmp(crits[i-1], "&&")==0) {           // &&: 이전 명령 실패 -> 이번 명령 건너뜀
                    if (last_status != 0) continue;
                } else if (i>0 && strcmp(crits[i-1], "||")==0) {    // ||: 이전 명령 성공 -> 이번 명령 건너뜀
                    if (last_status == 0) continue;
                }
                if (i < nc && strcmp(crits[i], "|")==0) {
                    int start = i;
                    int end = i;
                    while (end < nc && strcmp(crits[end], "|")==0) {
                        end++;
                    }
                    last_status = run_pipeline(lumps, start, end);
                    i = end + 1;       // 다음 명령어로 건너뛰기
                    continue;
                }
                if (i < nc && strcmp(crits[i], "&") == 0) {
                    run_cmd_bg(lumps[i]);
                    last_status = 0;
                    continue;
                } else {
                    last_status = run_cmd(lumps[i]);
                }
                argc = 0;
            }
            for (int i=0;i<nl;i++) free(lumps[i]);
            for (int i=0;i<nc;i++) free(crits[i]);

        }
        else {                  //("|" "||" "&" "&&" ";")가 없으면.
            run_cmd(*lumps);
        }
        argc = 0;
    }

    return 0;
}
