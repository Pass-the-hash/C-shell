#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "mygrep.h"

#define TOK_DELIM " \t\r\n"
#define RED "\033[0;31m"
#define RESET "\e[0m"
#define TK_BUFF_SIZE 256

char *line, *login, cwd[PATH_MAX];
char **args;
FILE* fp;

void clean(){ //Συνάρτηση που απελευθερώνει καθολικούς πόρους και αποθηκεύει τις εντολές στο log αρχείο
    free(line);
    free(args);
    fprintf(fp, "**************\n");
    fclose(fp);
}

void error(const char *msg) //Συνάρτηση που πραγματοποιεί εμφάνιση μηνυμάτων και έξοδο με εκκαθάριση σε "μοιραία" σφάλματα
{
    perror(msg);
    clean();
    exit(1);
}

int mysh_exit() { //Συνάρτηση σε περίπτωση ομαλής εξόδου
    printf("GOODBYE! :)\n");
    clean();
    exit(0);
}

int mysh_execute(char **args, char *line) { //Συνάρτηση εκτέλεσης των εντολών
  pid_t cpid;
  int status;

  if (strcmp(args[0], "exit") == 0) { // Ομαλή έξοδος στην περίπτωση που δοθεί "exit"
    return mysh_exit(args);
  }
  if (strcmp(args[0], "cd") == 0) { // Κλήση της chdir() αν δοθεί cd
      if (chdir(args[1]) != 0){
          perror("Couldn't change directory");
          status = 1;
      } else {
          if (getcwd(cwd, sizeof(cwd)) == NULL) {
              error("getcwd() error");
          }
          status = 0;
      }
      printf("Exit code: %d\n", status);
      return 0;
  }
  cpid = fork(); // Δημιουργία νέας θυγατρικής διεργασίας

  if (cpid == 0) {
    if (strcmp(args[0], "mygrep") == 0) { // Αν δοθεί "mygrep" καλείται η συνάρτηση μέσω του header
        //char* word;
        FILE* file;
        if ((file=fopen(args[2], "r")) == NULL){ // Άνοιγμα του αρχείου που δόθηκε ως παράμετρος
            error("Unable to open given file");
        }
        int count = mygrep(args[1], file);
        if (count == -1){
            perror("Couldn't initialize thread");
            return 1;
        }
        printf("\nTotal occurrences of '%s': %d\n", args[1], count);
        return 0;
    } else {                 // Κάθε άλλη εντολή εκτελείται με αυτόν τον τρόπο για υποστήριξη διοχετεύσεων και ανακατευθύνσεων
        char *command[4];
        command[0] = "/bin/bash";
        command[1] = "-c";
        command[2] = line;
        command[3] = NULL;
        if (execvp(command[0], command) < 0) {
            printf("mysh: command not found: %s\n", args[0]);
            exit(EXIT_FAILURE);
        }
    }
  } else if (cpid < 0)
    printf(RED "Error forking"
      RESET "\n");
  else { // Η γονική διεργασία περιμένει για τον τερματισμό της θυγατρικής και εκτυπώνει την κατάσταση εξόδου
    waitpid(cpid, & status, WUNTRACED);
    printf("Exit code: %d\n", status);
  }
  //return 1;
}

char **split_line(char * line) { // Συνάρτηση διάσπασης της γραμμής (tokenization) για την υποστήριξη επιπλέον εντολών, όπως cd, mygrep, exit
  int buffsize = TK_BUFF_SIZE, position = 0;
  char **tokens = malloc(buffsize * sizeof(char *));
  char *token;

  if (!tokens) error("Allocation error");

  token = strtok(line, TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= buffsize) { // Απλή μέθοδος για αύξηση του μεγέθους της προσωρινής αποθήκευσης
      buffsize += TK_BUFF_SIZE;
      tokens = realloc(tokens, buffsize * sizeof(char * ));
      if (!tokens) error("Allocation error");
    }
    token = strtok(NULL, TOK_DELIM);  // Ο διαχωρισμός γίνεται με βάση τους συνήθης κενούς χαρακτήρες (μακροεντολή)
  }
  tokens[position] = NULL;

  return tokens;
}

void loop() { // Βρόγχος για τη συνεχή εκτέλεση εντολών
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
     error("getcwd() error");
     //exit(1);
  }
    line = (char *) calloc(TK_BUFF_SIZE, sizeof(char));
    char command[TK_BUFF_SIZE];
    while(printf("%s:%s> ", login, cwd), fgets(line, TK_BUFF_SIZE, stdin)){   //Προτροπή για είσοδο εντολών από το χρήστη
        if (line[0]=='\n') continue;
        line[strcspn(line, "\n")] = 0;
        strcpy(command, line); // Αντιγραφή της εντολής που δόθηκε και ταυτόχρονη εκχώρηση στη συνάρτηση εκτέλεσης
        args = split_line(line);
        if (isalpha(command[1]) ) {     // Προστασία από κενούς χαρακτήρες
            fprintf(fp, "%s\n", line); // Καταγραφή εντολών
            mysh_execute(args, command);
        } else printf("Bad command format\n");
        //free(line);
  }
}

void handler()   // Συνάρτηση για διαχείριση σημάτων από το πληκτρολόγιο
{
    do {
        printf("\nDo you really want to quit? (y)/(n) ");
        char c;
        scanf(" %c", &c);
        if (c != 'y' && c != 'n') {
            continue;
        } else if (c == 'n') {
            //line = calloc(TK_BUFF_SIZE, sizeof(char));
            return;
        } else {
            clean();
            printf("GOODBYE! :)\n");
            exit(0);
        }
    } while(1);
}

int main() {
    login = (char *) malloc(TK_BUFF_SIZE); // Αποθήκευση του ονόματος χρήστη
    login=getlogin();
    char home[PATH_MAX]="/home/";
    strcat(home, login); // Συνδυασμός με τον αρχικό κατάλογο
    login = (char *) malloc(TK_BUFF_SIZE);
    login=getlogin();
    if (chdir(home) != 0){  // και μετάβαση σε αυτόν.
        perror("Couldn't change directory");
    }
    if ((fp=fopen(".myshlog", "a+")) == NULL){ // Άνοιγμα αρχείου για καταγραφή
        error("Unable to open log file");
    }

    signal(SIGTERM, handler); // Εκκίνηση των διαχειριστών σημάτων
    signal(SIGINT, handler);

    time_t raw = time(0);
    char time[30];
    struct tm buffer;


    buffer = *(localtime(&raw));
    strftime(time, 30, "%H:%M:%S, %b %d %Y\n", &buffer); // Καταγραφή της ώρας και της ημερομηνίας στο logfile
    fprintf(fp, "\n%s\n", time);
    loop();        // Εκκίνηση του βρόγχου του φλοιού
}
