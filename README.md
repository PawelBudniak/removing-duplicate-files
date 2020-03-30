# removing-duplicate-files
Takes directory path as an argument and writes a linux script to ./script.sh, which replaces duplicate files in that folder with hardlinks to the 1 copy remaining.
Usage:
```
$ cd <folder you put linuxdupOff_t.c in>
$ gcc -o name linuxdupOff_t.c
$ ./name <path to target folder>
$ sh script.sh
```
