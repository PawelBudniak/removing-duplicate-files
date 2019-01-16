#define _FILE_OFFSET_BITS 64 //defined before stdio.h in order to use 64 bit off_t with fseeko and ftello
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdbool.h>

#define UNDEF -3
#define BUF_CMP 512
#define PATH_MAX 4096

struct Node{
    off_t size;
    bool flag;
    bool error;
    struct Node* next;
    char fPath [];
};
typedef struct Node Node;

Node* addNew (Node* head, const char* newFile, const off_t size){
    if (strlen(newFile) >= PATH_MAX){
        fputs ("File path too long\n", stderr);
        return head;                    /* if the path is too long no changes are made to head - the node isn't added */
    }
    Node* newElement = malloc (sizeof(Node)+strlen(newFile)+1);
    if (!newElement){
        fputs ("Memory shortage\n", stderr);
        exit (EXIT_FAILURE);
    }
    newElement->size = size;
    newElement->next = head;
    strcpy(newElement->fPath, newFile);
    newElement->flag = 0;               /* flag and error are 0 by default */
    newElement->error = 0;
    return newElement;
}
/* Removes every node from list starting from current */
void clearList (Node* current){
    Node* prev = NULL;
    while (current){
        prev = current;
        current = current->next;
        free(prev);
    }
}
/* returns file size in bytes.
   fseeko and ftello are used to handle large files */
off_t getFileSize (const char* path){
    FILE* fp = fopen(path,"rb");
    if (!fp)
        return -2;
    /* If fseek fails UNDEF is returned, so that the
     file can be checked again later on by filecmp */
    if (fseeko64(fp, 0, SEEK_END)){
        fclose(fp);
        return UNDEF;
    }
    off_t size = ftello64(fp);
    fclose(fp);
    return size;
}
/* takes 2 file pointers as arguments and compares the files, return values:
 *0 if they are equal
 *1 if they are not
 *(-2) if there was an error with the first file
 *(-3) if there was an error with the second file */
int filecmp2 (FILE* fp1,  FILE* fp2){
    /* set file pointers and the beginning
       and clear error indicators */
    rewind(fp1);
    rewind(fp2);
    /* declare buffers which will store bytes */
    char buf1[BUF_CMP];
    char buf2[BUF_CMP];
    size_t elemsRead1, elemsRead2;

    do {
        elemsRead1 = fread(buf1, sizeof(*buf1), sizeof(buf1)/sizeof(*buf1), fp1);
        if (elemsRead1 == 0 && ferror(fp1))
            return -2;
        elemsRead2 = fread(buf2, sizeof(*buf2), sizeof(buf2)/sizeof(*buf2), fp2);
        if (elemsRead2 == 0 && ferror(fp2))
            return -3;
        if (elemsRead1 != elemsRead2 || memcmp(buf1, buf2, elemsRead1))
            return 1;
    }while(elemsRead1); /* iterate while elements are being read, because of the 1st and 3rd ifs we don't have to also check elemsRead2 here */
    return 0;
}
/* prints a linux script to stdout, which removes duplicate files and replaces them with hard links.
   returns the number of errors that have occured */
int printScript2 (Node* current){
    int errorCount = 0;

    while (current){
        FILE* fp1 = fopen(current->fPath, "rb");
        if (!fp1){
            current->error = 1;
            current->flag = 1;
            current = current->next;
            continue;
        }
        Node* temp = current->next;
        /* The loop only starts if current hasn't been flagged, also stops if it gets flagged due to an error during iteration */
        while (temp && !current->flag ){
            /* Only go further if files are equal in size (or current's size is undefined) and temp hasn't been flagged */
            if ( !((temp->size != current->size && current->size != UNDEF ) || temp->flag )){
                FILE* fp2 = fopen(temp->fPath, "rb");
                if (!fp2){
                    temp->error = 1;
                    temp->flag = 1;
                    continue;
                }
                else {
                    switch (filecmp2(fp1, fp2)){
                    case 0:                 /* if they are equal remove the duplicate and make a hardlink */
                        printf (" rm %s \n ln %s %s \n", temp->fPath, current->fPath, temp->fPath);
                        temp->flag = 1;     /* If the file is removed in the script it gets flagged, so that it doesn't get removed multiple times */
                        break;
                    case -2:                /* (-2) returned by filecmp indicates an error with current */
                        current->error = 1;
                        current->flag = 1;  /* if there's an error with the file it also gets flagged, in order to avoid working on a damaged file */
                        ++errorCount;
                        break;
                    case -3:                /* (-3) returned indicates an error with temp */
                        temp->error = 1;
                        temp->flag = 1;
                        ++errorCount;
                        break;
                    }
                }
            }
            fclose(fp2);
            temp = temp->next;
        }
        fclose(fp1);
        current = current->next;
    }
    return errorCount;
}
/* 0 if dir, 1 if not */
bool isDir (const char* path){
    struct stat statinfo;
    stat(path,&statinfo);      /* fill the statinfo buffer with information about the file */
    if (stat(path, &statinfo)) /* if stat fails 0 is also returned, indicating non-directory file */
        return 0;
    return S_ISDIR(statinfo.st_mode);
}
/* Adds contents of a directory (and it's subdirectories)
 located under path to list pointed to by head
 * Return values:
 * (-1) - error while opening directory
 * (-2) - path too long
 *  0 - operation finished successfully */
int listdir (const char *path, Node** head){
  struct dirent *entry;
  DIR *dp;

  dp = opendir(path);
  if (!dp){
    fprintf(stderr, "Couldn't open directory: %s\n", path);
    return -1;
  }
  if (strlen(path)+3 > PATH_MAX){ /* +3, because at the very least there should be room for '\0', a slash and a one letter filename, e.g: path/f */
    fputs("Path too long\n", stderr);
    closedir(dp);
    return -2;
  }
   /*char buf[PATH_MAX];
  strcpy(buf,path);
  strcat(buf, "/"); */
  while((entry = readdir(dp))){
    char buf[PATH_MAX];
    /* create a valid path */
    strcpy(buf,path);
    strcat(buf, "/");
    strcat(buf, entry->d_name);
    /* ignore ".", ".." directories, and files with path lengths exceeding the limit */
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..") || sizeof(buf)-strlen(buf) <= strlen(entry->d_name) )
        continue;
    /* if it's a directory recursively list it */
    if (isDir(buf)){
        listdir (buf, head);
    }
    /* if it's a normal file add it to list */
    else{
        off_t size = getFileSize(buf);
        /* if there was an error opening file don't add it to list and print error message */
        if (size == -2)
            fprintf(stderr, "Couldn't open file: %s\n", buf);
        else
            *head = addNew(*head, buf, size);
    }
  }
  closedir(dp);
  return 0;
}
void printErrors (Node* current, const int errorCount){
    fputs("While handling those files some errors have occurred:\n", stderr);
    while (current){
        if (current->error) {
            fprintf(stderr, "%s \n", current->fPath);
        }
    current = current->next;
    }
}

int main (int argc, char** argv){
    Node* head = NULL;
    int errorCount;

    if (argc != 2 ){
        fputs ("Incorrect number of arguments, please enter only one directory\n", stderr);
        exit(EXIT_FAILURE);
    }
    /* if the first call of listdir failed, then exit */
    if (listdir (argv[1], &head) < 0 )
        exit(EXIT_FAILURE);
    errorCount = printScript2(head);   /* print the script and save error count */
    if (errorCount)
        printErrors(head, errorCount); /* errors encountered while comparing files are printed later, so that they don't mess up the script */
    fprintf(stderr, "There have been errors with %d files in total", errorCount);
    clearList (head);
    printf("%d",sizeof(long long));
    return (EXIT_SUCCESS);
}
