/* vim: set sw=8 ts=8 si et: */
/*
 * Author: Guido Socher. This program is distributed
 * under the terms of the Gnu Public License (GPL). 
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <stdlib.h>
#include "levdist.h"

static void help();
static char **splitpath();
static int checkftype(char *path, char *filename);
static void matchfilename(char *path, char *key);
static int streq(const char *a,const char *b);

/*global for simplicity:*/
static int printdist=0;
static int casesensitive=0;
static int nomatchfound=1;
/*matchlimit indicates the tolerance level for Lev. Dist search
 *-2 not set, 
 * 0 = exact match with wildcards,
 * 1-255 max distance value*/
static int matchlimit=-2;


int main(int argc, char *argv[])
{
	char searchkey[MAXLEN +1];
	char **pathcomp;
        int optnum;
	char *c,*oarg;

        if (argc < 2 ) help();
        /*
         * After this while loop. argv[0] is the next argument if available
         *
         * options that take an argument write *c=0;break; others write
         * "c++;break;"
         *
         * This parser looks a bit complicated but it can take options
         * like -23 and -ac is the same as "-a -c"
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
                        case 'h':
                                help();
                                /*no break, help does not return*/
                        case 'p':
                                printdist=1;
                                c++;break;
                        case 'I':
                                casesensitive=1;
                                c++;break;
                        case 't':
                                /* you can write -t11 or -t 11*/
                                /* first the normal -t11 case: */
                                oarg=c+1;
                                /* then the -t 11 case: */
                                if(*(c + 1)=='\0'){
                                        if(--optnum) oarg=*++argv;
                                }
                                matchlimit = atoi(oarg);
                                if (matchlimit > 255 || matchlimit <0) matchlimit = 255;
                                *c=0;break;
                        default:
                                if (isdigit((int)*c)){
                                        if (matchlimit !=-1 ){
                                                /*ignore -e -t1 */
                                                matchlimit = atoi(c);
                                                if (matchlimit > 255 || matchlimit <0) matchlimit = 255;
                                        }
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
	if (optnum != 1) {
		/* you must provide exactly one argument */
		help();
		/*help does not return */
	}
        strncpy(searchkey, argv[1],MAXLEN);
        if (matchlimit == -2){
                /*tolerance for Levenshtein Distance search: */
                matchlimit = stdtolerance(searchkey);
        }

	pathcomp = splitpath();
	while (*pathcomp) {
		matchfilename(*pathcomp, searchkey);
		pathcomp++;
	}
	return (nomatchfound);
}
/*__END OF MAIN__*/

/*
 * check that the file is a normal executable.
 * The abs. path to the file is "path/filename"
 * Return 1 if it is normal executable or a link to an normal executable
 */
#define MAXFULLPATHLEN 500
int checkftype(char *path, char *filename)
{
        struct stat stbuf;
        static char fullpath[MAXFULLPATHLEN];
        int i;
        /*construct the full path to the file:*/
        i=0;
        while(*path && i < MAXFULLPATHLEN-7){ /* -7 so, we have some room */
                fullpath[i]=*path;
                i++;path++;
        }
        if (fullpath[i-1]!='/'){
                fullpath[i]='/';
                i++;
        }
        while(*filename && i < MAXFULLPATHLEN-1){
                fullpath[i]=*filename;
                i++;filename++;
        }
        fullpath[i]='\0';
        /*check if file exists:*/
        if (stat(fullpath,&stbuf)!=0) return(0);
        /*not a regular file*/
        if ((stbuf.st_mode & S_IFREG)==0) return(0);
        /* test if executable for user or grp or others */
        if ((stbuf.st_mode & S_IXUSR) || (stbuf.st_mode & S_IXGRP)\
                ||(stbuf.st_mode & S_IXOTH)) return(1);
        return(0);
}
/*
 * match all entries in the directory variable path points to
 * against the key.
 * This function sets the global variable nomatchfound
 */
void matchfilename(char *path, char *key)
{
        int dist;
	DIR *dp;
	struct dirent *entry;
	if ((dp = opendir(path)) != NULL) {
                while ((entry = readdir(dp)) != NULL) {
                        /*ignore . or .. or .file: */
                        if (*(entry->d_name) == '.') continue; 
                        dist=levdist(entry->d_name, key, matchlimit,casesensitive);
                        if (dist != -1 && checkftype(path,entry->d_name)){
                                if (printdist) printf("%03d ",dist);
                                printf("%s/%s\n", path, entry->d_name);
                                nomatchfound = 0;
                        }
                }
                closedir(dp);
        }
}


/*
 * split PATH into substrings and return a pointer to 
 * a 2 dimmentional array with all the sub-strings of the
 * PATH. 
 */
char **splitpath()
{
	char *spath; /* search path */
	char *cspath;/* copy of search path */
	static char **pathcomp;
        int i=0;
        int notseen=1;
	char *start;
	int compind = 0;	/* index of *pathcomp[] */
	int complen = 1;	/* len of one component in the path + 16 */

	spath = (char *) getenv("PATH");
	if (spath == NULL || *spath == '\0') {
		fprintf(stderr,"Warning: PATH not defined, using /bin:/usr/bin\n");
                spath="/bin:/usr/bin";
	}
        /* make a copy that is static */
        cspath = (char *)malloc(strlen(spath) + 2);
        strcpy(cspath, spath);
	start = cspath;
        /* count the number of colons in the path to find how many
         * components we have in the path */
	while (*cspath) {
                if (*cspath == ':') i++;
		cspath++;
        }
        i+=3; /*some space in case we have corruped path below*/
        pathcomp=(char **)malloc(sizeof(char *)*i);
        /* split the path by replacing : with \0 */
	cspath = start;
        pathcomp[compind]=cspath;
	while (*cspath) {
		if (*cspath == ':') {
                        *cspath = '\0';
			if (complen > 1) { /*ignore zero length comp. e.g :: */
                                /* now check if it is a duplicate path 
                                 * component that we can just ignore: */
                                i=0;notseen=1;
                                while(notseen && i < compind){
                                        if(streq(pathcomp[compind],pathcomp[i])){
                                                notseen=0;
                                        }
                                        i++;
                                }
                                if (notseen) compind++;
			}
                        /* remove any tailing slashes but not if it is 
                         * only the root dir (just a "/") */
                        if (complen > 2 && *(cspath-1) == '/') *(cspath-1)='\0';
                        pathcomp[compind] = cspath+1;
                        complen = 0;
		}
		cspath++;
                complen++;
	}
        if (complen < 2){
		/*we have some corrupted path, this happens when you have a
		 *colon at the end of the path */
                pathcomp[compind] = NULL;   /*terminate */
        }else{
                /* now check if the last one is a duplicate path 
                 * component that we can just ignore: */
                i=0;notseen=1;
                while(notseen && i < compind){
                        if(streq(pathcomp[compind],pathcomp[i])){
                                notseen=0;
                        }
                        i++;
                }
                if (notseen) compind++;
                pathcomp[compind] = NULL;   /*terminate */
        }
	return (&pathcomp[0]);
}
/* return 1 if the 2 strings a and b are equal */
int streq(const char *a,const char *b){
	while(*a && *b){
		if(*a != *b) return(0);
		a++;b++;
	}
	/* both must be at \0 */
	if(*a || *b) return(0);
	return(1);
}
/*
 * help
 */
void help()
{
printf("ftwhich -- fault tolerant approximate search command for programs\n\
\n\
USAGE:  ftwhich [-#hIp][-t#] [--] program-name\n\
\n\
Supported wildcards: * -- any arbitrary number of character\n\
                     ? -- one character\n\
\n\
OPTIONS: -h  this help\n\
         -I  search case sensitive (default is case in-sensitive)\n\
         -p  print the actual distance (tolerance) value in front of the\n\
             file name.\n\
         -#  set fault tolerance level to # (integer in the range 0-255)\n\
             It specifies the maximum distance. This is the number of\n\
             errors permitted for finding the approximate match.\n\
         -t# same as -# for backward compatibility\n\
\n\
With no option specified search is fault tolerant using a tolerance\n\
level of: (string length of searchpattern - number of wildcards)/6 +1\n");
printf("%s\n",MYVERSION);
exit(0);
}
/*__END__*/
