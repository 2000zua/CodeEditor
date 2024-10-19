#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0};

/*** data ***/
struct editorConfig{
    int screenrows;
    int screencols;
    struct termios orig_termios;
};

struct editorConfig E;

/*** append buffer ***/
struct abuf {
    char *b;
    int len;
};

/*** terminal ***/
// Tratamento de erros
void die(const char *s){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

// Desabilitar modo raw na saída
void disableRawMode(){
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1){
        die("tcsetattr");
    }
}

// desligar eco
void enableRawMode(){
    // limpar o terminal
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);
    struct termios raw = E.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_iflag &= ~(IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

// editar a cada caracter recebido
char editorReadKey(){
    int nread;
    char c;
    while ((nread = read(STDERR_FILENO, &c, 1)) != 1){
        if (nread == -1 && errno != EAGAIN) die("read");   
    }
    return c;
}

// pegar o tamanho da tela
int getWindowSize(int *rows, int *cols){
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return getCursorPosition(rows, cols);
    }else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

// pegar a posicao do curso
int getCursorPosition(int *rows, int *cols) {
    char buff[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
    while (i < sizeof(buff) -1){
        if (read(STDIN_FILENO, &buff[i], 1) != 1) break;
        if (buff[i] == 'R') break;
        i++;
    }
    buff[i] == '\0';
    if(buff[0] != '\x1b' || buff[1] != '[') return -1;
    if(sscanf(&buff[2], "%d;%d", rows, cols) != 2) return -1;
    //printf("\r\n&buf[1]: '%s' \r\n", &buff[1]);
    
    return 0;
}

/*** input ***/
void editorProcessKeypress(){
    char c = editorReadKey();
    switch (c){
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(1);
            break;
    }
}

/*** output ***/
// Limpar a tela
void editorRefreshScreen(){
    write(STDOUT_FILENO, "\x1b[2j", 4);
    write(STDOUT_FILENO, "\x1b[H", 3); // Reposicione o cursor no topo
    editorDrawRows();
    write(STDOUT_FILENO, "\x1b[H", 3);
}

// desenmhar uma linha
void editorDrawRows(){
    int y;
    for (y = 0; y < E.screenrows; y++){
        write(STDOUT_FILENO, "~", 1);
        if (y < E.screenrows - 1){
            write(STDOUT_FILENO, "\r\n", 2);
        }
    }
}

/*** init ***/
void initEditor(){
    if(getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);
  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}
void abFree(struct abuf *ab) {
  free(ab->b);
}

int main(int argc, char const *argv[]){
    enableRawMode();
    initEditor();
    while (1){
        editorRefreshScreen();
        editorProcessKeypress();
    } 
    return 0;
}




