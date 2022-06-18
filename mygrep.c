#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "mygrep.h"

#define MAX_SIZE 100 // Maximum string size
#define NTHREADS 6

int total; // Καθολική μεταβλητή για απαρίθμηση των εμφανίσεων της λέξης
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct { // Δομή αποθήκευσης για της παραμέτρους του νήματος
    char *string;
    char *word;
    int line;
}thread_arg;

void *search_line(void* args) // Συνάρτηση νήματος για την εύρεση λέξης ανά γραμμή
{
    int i, j, found, count;
    int stringLen, searchLen;
    thread_arg *data = (thread_arg *) args;

    char *str = data->string, *word = data->word;
    stringLen = strlen(str);      // Μήκος γραμμής
    searchLen = strlen(word); // Μήκος λέξης

   // printf("Buffer size: %d\n", stringLen);

    pthread_mutex_lock(&mutex); // Είσοδος στην κρίσιμη περιοχή
    count = 0;

    for(i=0; i <= stringLen-searchLen; i++)
    {
        // Σύγκριση χαρακτήρα-προς-χαρακτήρα για ταύτιση με τη λέξη
        found = 1;
        for(j=0; j<searchLen; j++)
        {
            if(str[i + j] != word[j])
            {
                found = 0;
                break;
            }
        }

        if(found == 1)
        {
            // Αν η ακολουθία χαρακτήρων υπάρχει στη γραμμή, σημειώνεται
            total++;
            count++;

        }
    }
    if (count > 0) printf("%s\tline: %d\n", str, data->line);
    pthread_mutex_unlock(&mutex); // Έξοδος από την κρίσιμη περιοχή
}

int mygrep(char* word, FILE* file)
{
    char *line_buf = NULL;
    size_t line_buf_size = 0;
    int line_count = 0;
    ssize_t line_size;
    total=0;
    thread_arg data;

    pthread_t pool[NTHREADS];   // Συνολικά 6 νήματα πραγματοποιούν τη διαδικασία (ρυθμίζεται με μακροεντολή)

    int thread=0;
    line_size = getline(&line_buf, &line_buf_size, file); // Ανάγνωση της πρώτης γραμμής από το αρχείο
    /*for (int i=0; i<=line_count; i++){
    }*/

    while (line_size >= 0)
    {
        data.string = line_buf;
        data.word = word;
        data.line = line_count;
        if (pthread_create(&pool[thread], NULL, &search_line, &data) != 0) return -1;  // Κλήση ενός από τα νήματα με τη σειρά του
        /* Increment our line numbers */
        line_count++;

        thread++;
        if (thread == NTHREADS) { // Αν ο μετρητής νημάτων φτάσει το προκαθορισμένο όριο, επανέρχεται στο πρώτο
            thread = 0;
        }
        line_size = getline(&line_buf, &line_buf_size, file); // Ανάγνωση επόμενης γραμμής

    }

    for (int i=0; i<=NTHREADS; i++) pthread_join(pool[thread], NULL); // Τα νήματα συνενώνονται στο τέλος

    free(line_buf);
    line_buf = NULL;
    return total; // Επιστροφή του συνολικού αριθμού εμφανίσεων
}
