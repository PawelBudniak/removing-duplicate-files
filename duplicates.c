#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h> // ????
#include <stdbool.h>

#define BUF_CMP 256
#define PATH_MAX 4096
// wyswietlanie bledow, te w trakcie pisania skryptu (sprawdznie fopen) tym bardziej (zapisac w tablicy i printowac pod skryptem?)
// consty w definicjach funkcji
// fclose w ladniejszy sposob


struct Node{
    char fPath [PATH_MAX];
    bool flag; // mozna tak?????
    bool error;
    Node* next;
};
typedef struct Node Node;

Node* addNew (Node* head, char* newFile){
    assert (newFile < PATH_MAX); // ?????
    if (strlen(newFile) >= PATH_MAX){
        fputs (" File path too long ", stderr);
        return head;
    }
    Node* newElement = malloc (sizeof(Node));
    newElement->next = head;
    newElement->fPath = newFile;
    newElement->flag = 0;
    newElement->error = 0;
    return newElement;
}
void clearList ( Node* head ){
    Node* prev = NULL;
    while (head){
        prev = head;
        head = head->next;
        free(prev);
    }
}
void printScript (Node* head){
    while (head){
        Node* current = head->next;
         //if (!head->flag){
        while (current && !head->flag && !head->error){
            switch (filecmp(head->fPath, current->fPath)){
            case 0:
                printf (" rm %s /n ln %s %s /n", current->fPath, head->fPath, current->fPath);
                current->flag = 1;
                break;
            case -2;
                head->error = 1;
                head->flag = 1;
                break;
            case -3;
                current->error=1;
                current->flag=1;
                break;
            }
            if (filecmp(head->fPath, current->fPath) == 0){ //co robic jak juz jest hardlinkiem, zeby go dwukrotnie nie traktowac? // dac flage lol
                printf (" rm %s /n ln %s %s /n", current->fPath, head->fPath, current->fPath);
                current->flag = 1;
            }
            current = current->next;
            }
        }
        head = head->next;
    }
}
 /*bool checkFilePointer (FILE* fp){
    return ferror(fp);
} */
extern inline size_t minSize_t (const size_t a, const size_t b){
    return a<b ? a : b;
}
/* takes paths to 2 files as arguments and compares the files, return values:
*0 if they are equal
*1 if they are not
*(-2) if there was an error with the first file
*(-3) if there was an error with the second file */
int filecmp (const char* path1, const char* path2){
    FILE* fp1 = fopen(path1, "rb");
    if (!fp1)
        return -2;
    FILE* fp2 = fopen(path2, "rb"); // rb?????
    if (!fp2)
        return -3;
    char buf1[BUF_CMP];
    char buf2[BUF_CMP];
    size_t elemsRead1, elemsRead2;
    while (!feof(fp1) || !feof(fp2)){
        elemsRead1 = fread(buf1, sizeof(*buf1), sizeof(buf1)/sizeof(*buf1), fp1);
        if (elemsRead1==0 && ferror(fp1)){
            fclose (fp1);
            fclose (fp2);
            return -2;
        }
        elemsRead2 = fread(buf1, sizeof(*buf2), sizeof(buf2)/sizeof(*buf2), fp2);
        if (elemsRead2==0 && ferror(fp2)){
            fclose (fp1);
            fclose (fp2);
            return -3;
        }
        if (memcmp(buf1, buf2, minSize_t(elemsRead1, elemsRead2))){
            fclose (fp1);
            fclose (fp2);
            return 1;
        }

    }
    fclose (fp1);
    fclose (fp2);
    return 0;
}
int listdir (const char *path, Node** head){ //strcat PATH????
  struct dirent *entry;
  DIR *dp;
  dp = opendir(path);
  if (dp == NULL){
    perror(" Couldn't open directory ");
    closedir(dp); //?????
    return -1;
  }
  if (strlen(path)+3 > PATH_MAX){ /* +3, because at the very least there should be room for '\0', a slash and a one letter filename, e.g: path/f */
    perror("Directory path too long");
    closedir(dp);
    return -2;
  }
  char buf[PATH_MAX];
  strcpy(buf,path);
  strcat(buf, "/");
  while((entry = readdir(dp))){
    if ( sizeof(buf)-strlen(buf) <= strlen(entry->d_name) )
        continue;
    strcat(buf, entry->d_name);
    if (isDir(buf)){
        listdir (buf, head); // &(*head)???
    }
    else{
        *head = addNew(*head, buf);
    }
  }
  closedir(dp);
  return 0;
}
int isDir (const char* path){
    struct stat statinfo;
    if (!stat(path, statinfo)) // blad jakis?
        return 0;
    return S_ISDIR(statinfo.st_mode);
}
void printErrors (Node* head){
    int count=0;
    fputs("While handling those files some errors have occured:", stderr);
    while (head){
        if (head->error) {
            fputs(head->fPath, stderr);
            ++count;
        }
    }
    fprintf(stderr, "There have been errors with %d files in total", count);
}
int main (int argc, char** argv)
{
    Node* head = NULL;
    printf("Hello world!\n");
    if (argc >1 ){
        fputs (" Too many arguments ", stderr);
        exit(EXIT_FAILURE);
    }
    listdir (argv[1], &head);
    printScript (head);
    printErrors(head);
    clearList (head);
    return 0;
}
