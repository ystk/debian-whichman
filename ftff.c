/* vim: set sw=8 ts=8 si et: */
/*
 * ftff, fault tolerant file-find.
 * This program is distributed under the terms 
 * of the Gnu Public License (GPL). 
 */
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "levdist.h"

#ifndef S_IXUSR
#define S_IXUSR S_IEXEC
#endif
#ifndef S_IXGRP
#define S_IXGRP (S_IEXEC >> 3)
#endif
#ifndef S_IXOTH
#define S_IXOTH (S_IEXEC >> 6)
#endif
#ifndef S_IXUGO
#define S_IXUGO (S_IXUSR | S_IXGRP | S_IXOTH)
#endif

static void help();

/* global for efficiency reasons: */
static char *search;
static char *fullpath;
static int tolerance=-1; /*-1 indicates tolerance is not set*/
static int opt_p=0;
static int opt_q=0;
static int opt_F=0;
static int opt_I=0;
static int opt_f=0;



#define PATH_MAX_LEN 1000

char *allocpath(void)
{
	char *ptr;
	if ((ptr = (char *) malloc(PATH_MAX_LEN + 2)) == NULL) {
		perror("Error: malloc() failed");
		exit(1);
	}
	return ptr;
}
/* classify the file type an produce a charater like ls -F */
char filetype(struct stat buffer){
#ifdef linux
        /*umode_t mode;*/
        /*__mode_t mode;*/
        mode_t mode;  /* these days it's all mode_t */
#else /* linux */
#ifdef sun
        /* solaris: */
        mode_t mode;
#else /* sun */
        /* other unix os lands here: */
        /*unsigned long mode;*/
        mode_t mode;
#endif /* sun */
#endif /* linux */


        mode=buffer.st_mode;
        switch(mode & S_IFMT) {
#ifdef S_IFIFO
        case S_IFIFO:
                return '|';
#endif
#ifdef S_IFDIR
        case S_IFDIR:
                return '/';
#endif
#ifdef S_ISLNK
        case S_IFLNK:
                return '@';
#endif
#ifdef S_IFSOCK
        case S_IFSOCK:
                return '=';
#endif
        case S_IFREG:
                if (mode & S_IXUGO) {
                        return '*';
                }
                return ' ';
        /* no default */
        }
        return ' ';
}

/*
 *check for the file names ".." and "."
 *return 0 if dir is exactly ".." or "." otherwise return 1
 */ 
int isnotdotdir(char *dir)
{
        int cnt=0;
        while(*dir){
                if(*dir != '.'){
                        return(1);
                }
                if (cnt > 1){
                        return(1);
                }
                cnt++;
                dir++;
        }
        return(0);
}

/* Traverse directory tree  but we don't follow symbolic links
 * unless opt_f is set. The link-names are processed in any case.
 *
 * Return 0 on sucessful file find.
 * 
 * object = "name of object in FS"
 * fullpath = "path"+object  (this is a global variable)
 * This is a recursive function.
 * Uses also the following global var: opt_p,opt_f,opt_I,opt_q
 */

int traversetree(char *object)
{
	/* for efficiency reasons: */
	static struct stat buffer;
        /*these variables must be local:*/
	DIR *dp;
	struct dirent *dirp;
	char *ptr;
        /* must be static:*/
        static int result =1;

        int d,len;

        if (lstat(fullpath, &buffer) < 0) {
                if (opt_q==0){
                        fprintf(stderr, "Warning: %s, ", fullpath);
                        perror("");
                }
                return(result);
        }
#ifdef DEBUG
	fprintf(stderr, "debug,f:%s o:%s s:%s\n", fullpath,object,search);
#endif
        /*the first time that we are called obj is ".", skip that:*/
        if (isnotdotdir(object)){
                d=levdist(object, search, tolerance,opt_I);
                if (d != -1) {
                        if (opt_p) printf("%03d ",d);
                        printf("%s", fullpath);
                        if (opt_F){
                                printf("%c\n",filetype(buffer));
                        }else{
                                printf("\n");
                        }
                        result = 0;
                }
        }
        if (opt_f && S_ISLNK(buffer.st_mode)){
                if (stat(fullpath, &buffer) < 0) {
                        if (opt_q==0){
                                fprintf(stderr, "Warning: %s, ", fullpath);
                                perror("");
                        }
                        return(result);
                }
        }
	if (S_ISDIR(buffer.st_mode)) {
		if (NULL == (dp = opendir(fullpath))) {
			if (opt_q==0){
                                fprintf(stderr, "Warning: %s, ", fullpath);
                                perror("");
                        }
			return(result);
		}
                len=strlen(fullpath);
		ptr = fullpath + len;
                /* protection against endless loops due to symbolic links
                 * to .. */
                if (len < PATH_MAX_LEN -100){
                        if (*(ptr -1) != '/'){
                                /* do not append a "/" if the user did already 
                                 * provide one in the start_dirictory name 
                                 */
                                *ptr = '/';
                                ptr++;
                        }
                        *ptr = '\0';
                        while (NULL != (dirp = readdir(dp))) {
                                if (isnotdotdir(dirp->d_name)){
                                        /* Append to fullpath without changing 
                                         * ptr this will always succeed since 
                                         * we allocated enough memory.*/
                                        strcpy(ptr, dirp->d_name);
                                        traversetree(dirp->d_name);
                                }
                        }
                        closedir(dp);
                        /*shorten fullpath again to current directory:*/
                        ptr--;
                }
                *ptr='\0';
	}
        return (result);
}


int main(int argc,char *argv[])
{
        int result,optnum;
	struct stat buf;
	char *c,*oarg;

        if (argc < 2 ) help();
        /*
         * After this while loop. argv[0] is the next argument if available
         *
         * options that take an argument write *c=0;break; others write
         * "c++;break;"
         *
         * This parser looks a bit complicated but it can take options
         * like -23 and -ac is the same as "-a -c" Because we number options
         * are allowed we can not use on of the standard option parsers.
         */
        optnum=argc;
        while (--optnum>0) {
                argv++;
                c = *argv; /* c is pointer to one argument/option */
                if (*c != '-'){
                        argv--;
                        break; /*stop at the first non option argument*/
                }
                c++;
                if (*c == '-' && *(c + 1)=='\0'){
                        /* you can write a double -- to stop option
                         * parsing. This is useful if you need to search
                         * for a string that is called "-something" */
                         --optnum;
                         break; /*stop option parsing*/
                }
                while (*c) {
                        switch (*c) {
                        case 'f':
                                opt_f=1;
                                c++;break;
                        case 'F':
                                opt_F=1;
                                c++;break;
                        case 'h':
                                help();
                                /*no break, help does not return*/
                        case 'p':
                                opt_p=1;
                                c++;break;
                        case 'q':
                                opt_q=1;
                                c++;break;
                        case 'I':
                                opt_I=1;
                                c++;break;
                        case 't':
                                /* you can write -t11 or -t 11*/
                                /* first the normal -t11 case: */
                                oarg=c+1;
                                /* then the -t 11 case: */
                                if(*(c + 1)=='\0'){
                                        if(--optnum) oarg=*++argv;
                                }
                                tolerance = atoi(oarg);
                                if (tolerance > 255 || tolerance <0) tolerance = 255;
                                *c=0;break;
                        default:
                                if (isdigit((int)*c)){
                                        tolerance = atoi(c);
                                        if (tolerance > 255 || tolerance <0) tolerance = 255;
                                }else{
                                        fprintf(stderr,"ERROR: No such option. Use -h to get help.\n");
                                        exit(1);
                                }
                                *c=0;
                        }
                }
        }
        if (optnum<0) optnum=0; /* can be less then zero due to a option
                                 * with arg at last pos and missing arg*/
        /*
         *optnum  is now the number of argmunts left after
         *option parsing. Example: option a,c take no arg, option b has arg:
         *                         thisprogram -a -b xx -c d e
         *                         This results in:
         *                         optnum==2 and argv[1] points to d
         *                         argv[2] points to e
         */ 
        fullpath = allocpath();
	if (1 == optnum) {
		search = argv[1];
                /*start directory is current directory*/
                strcpy(fullpath, ".");
	} else if (2 == optnum) {
		search = argv[2];
                strncpy(fullpath, argv[1],PATH_MAX_LEN);
                fullpath[PATH_MAX_LEN]=0;
                if (stat(fullpath, &buf) < 0) {
                        fprintf(stderr, "ERROR: no such directory, %s\n", fullpath);
                        exit(1);
                }
                if (!S_ISDIR(buf.st_mode)){
                       fprintf(stderr, "ERROR: %s is not a directory -h for help.\n",fullpath);
                        exit(1);
                }
	} else {
                help();
	}
        if (tolerance == -1) tolerance=stdtolerance(search);
	result=traversetree(".");
	return result;
}
/*
 * help
 */
void help()
{
printf("ftff -- fault tolerant file find utility\n\
\n\
USAGE: ftff [-#fFhIpq][-t#] [--] [start_dir] filename \n\
\n\
ftff uses an approximate string matching algorithm to match the file names.\n\
\n\
Supported wildcards: * : any arbitrary number of character\n\
                     ? : one character\n\
\n\
OPTIONS: -f  follow symbolic links on directories.\n\
         -F  classify the file type like \"ls -F\".\n\
         -h  this help.\n\
         -I  search case sensitive (default is case in-sensitive)\n\
         -p  print the actual distance value in front of the file name.\n\
         -q  Quiet. Do not print a warning when a directory can not be read.\n\
         -#  set fault tolerance level to # (integer in the range 0-255) \n\
             This is the maximum number of permitted errors.\n\
         -t# same as -# for backward compatibility\n\
\n\
The default start_dir is the current directory. The default fault tolerance\n\
is (string length of searchpattern - number of wildcards)/6 +1.\n");
printf("%s\n",MYVERSION);
exit(0);
}
/*__END__*/
