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
static char **splitman();
static int findmandir(char *manpathcomponent, char *key);
static void matchfilename(char *path, char *key);
static int stringmatch(char *s1, char *s2);
static int maybemandir(const char *dirname);
static char *removemanextesions(char *manpagefilename);
static int streq(const char *a,const char *b);

/*global for simplicity:*/
static int printdist=0;
static int casesensitive=0;
static int nomanfilefound=1;
/*searchmethod indicates the tolerance level for Lev. Dist search
 *-2 not set, 
 *-1 exact match,
 * 0 = exact match with wildcards,
 * 1-255 max distance value*/
static int searchmethod=-2;


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
                        case 'e':
                                searchmethod=-1;
                                c++;break;
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
                                if (searchmethod !=-1 ){
                                        /*ignore -e -t1 */
                                        searchmethod = atoi(oarg);
                                        if (searchmethod > 255 || searchmethod <0) searchmethod = 255;
                                }
                                *c=0;break;
                        default:
                                if (isdigit((int)*c)){
                                        if (searchmethod !=-1 ){
                                                /*ignore -e -t1 */
                                                searchmethod = atoi(c);
                                                if (searchmethod > 255 || searchmethod <0) searchmethod = 255;
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
        if (casesensitive){
                strncpy(searchkey, argv[1],MAXLEN);
                searchkey[MAXLEN]=0;
        }else{
                /*we translate everything to lower case in order to do case
                 *insensitive match: */ 
                lowercaseword(searchkey, argv[1],MAXLEN,0);
        }

        if (searchmethod == -2){
                /*tolerance for Levenshtein Distance search: */
                searchmethod = stdtolerance(searchkey);
        }

	pathcomp = splitman();
	while (*pathcomp) {
		findmandir(*pathcomp, searchkey);
		pathcomp++;
	}
	return (nomanfilefound);
}
/*__END OF MAIN__*/
/*
 * Try to find all .../manX subdirectories.
 * This function considers also internationalized man-directory 
 * structures of the tpye manpathcomponent/<country code>/manX 
 * and the traditional man structures with manpathcomponent/manX
 * X may be somthing like 1,2,3..l,3s,...
 * This function is recursive and calls it self max once.
 * manpathcomponent is a null pointer if the function calls it self
 * again and the vaiable manpath contains the path.
 * NOTE: this is a recursive function
 * The return value is not significant.
 */
int findmandir(char *manpathcomponent, char *key)
{
        /*1024 should definitly be a conservative limit for one part of 
         *the man-path:*/
	static char manpath[1024]; 

        char *manpathendptr;
        int len=0;
        struct stat stbuff;
	DIR *dp;
        /*note:
         * The pointer returned by readdir() points to data  which  may
         * be  overwritten  by  another  call  to readdir() on the same
         * directory stream.  This data is not overwritten  by  another
         * call to readdir() on a different directory stream. Thus
         * we must be sure to open a directory only once.*/
	struct dirent *entry;
        manpathendptr=manpath; 
        /* manpathcomponent is NULL if this function was recursively called*/
        if (manpathcomponent == NULL){
                /*find the end of the manpath string:*/
                manpathendptr+=strlen(manpath);
        }else{
               /*we were called the first time and manpathcomponent
                *contains the path. Thus copy it into manpath.
                *manpathendptr must point to the end of the path
                *at the end of this loop*/
                while (*manpathcomponent && len < 800) {
                        *manpathendptr=*manpathcomponent;
                        manpathcomponent++;
                        manpathendptr++;
                        len++;
                }
                *manpathendptr='\0';
        }
	if ((dp = opendir(manpath)) == NULL) {
                /* we get here if we have invalid manpath component*/
                return(0);
        }
        if (*(manpathendptr-1) != '/'){
                *manpathendptr='/';
                manpathendptr++;
                *manpathendptr='\0';
        }
        /* manpathendptr points now to the end of:
         * /usr/man/de/ or /usr/man/ */
        while ((entry = readdir(dp)) != NULL) {
                /*ignore . or .. or .file :*/
                if (*(entry->d_name) == '.') continue; 
                /*we have enough memory left,
                 * this should succeed:*/
                strcpy(manpathendptr,entry->d_name);
                if (maybemandir(entry->d_name)){
                        /*manpath is now something like 
                         * /usr/man/man1 or /usr/man/de/man1 
                         * but manpathendptr is still unchanged*/
                        /*now search the file, no more recursion:*/
                        matchfilename(manpath,key);
                }else{
                        /*the <country code> path extension is max
                         *one component of the path. We might have catched
                         *a file not a directory */
                        if (stat(manpath, &stbuff) < 0) continue;
                        if (manpathcomponent != NULL && S_ISDIR(stbuff.st_mode)){
                                strcpy(manpathendptr,entry->d_name);
                                findmandir(NULL,key);
                        }
                }
        }
        closedir(dp);
        return(0);
}

/*
 * match all entries in the directory variable path points to
 * against the key.
 * This function sets the global variable nomanfilefound
 */
void matchfilename(char *path, char *key)
{
        char *manname;
        int d;
	DIR *dp;
	struct dirent *entry;
	if ((dp = opendir(path)) != NULL) {
                while ((entry = readdir(dp)) != NULL) {
                        /*ignore . or .. or .file: */
                        if (*(entry->d_name) == '.') continue; 
                        /* xxxxx.1.gz ->xxxxx :*/
                        manname=removemanextesions(entry->d_name); 
                        d=stringmatch(manname,key);
                        if (d != -1){
                                if (printdist) printf("%03d ",d);
                                printf("%s/%s\n", path, entry->d_name);
                                nomanfilefound = 0;
                        }
                }
                closedir(dp);
        }
}

/*
 * check if this is a man directory. This is faster than a stat
 * system call. Man directories look like: man1, man11, manl, man3s
 * but not: manpage,man.1
 * return 1 on 'may be a man directory' and 0 on 'definitly no man dir'
 */
int maybemandir(const char *dirname)
{
	static char man[] = "man";
	int i = 0;
	while (*dirname && i < 3) {
		if (*dirname != man[i]) return (0);
		dirname++;
		i++;
	}
	if (i != 3) return (0);
	if (isalnum((int)*dirname) == 0) return (0);
	dirname++;
	if (*dirname == '\0') return (1);
	if (isalnum((int)*dirname) == 0) return (0);
	return (1);
}

/*
 * string match
 * For substring matches s2 has to be a substring of s1
 * Parameters:
 * s1=string to search in, s2=search key, 
 * searchmethod=-1  or searchmethod='tolerance for Levenshtein Distance'
 * This function consults the global variables casesensitive in order
 * to see if matching should be casesensitive or not. If matching is
 * not casesensitive (casesensitive=0) then s2 has to be already lower case.
 *
 * Return values:
 * searchmethod == -1: exact match, Return values: 0 on match -1 on no match
 * searchmethod >= 0 : LD match, Return values: distance value or -1 on failure
 */
int stringmatch(char *s1,char *s2)
{
        char c;
	if (searchmethod == -1) {
                /*fast exact match:*/
                while (*s1) {
                        c=*s1;
                        if (!casesensitive) c=tolower(*s1);
                        if (c != *s2) {
                                return (-1);
                        }
                        s1++;s2++;
                }
                /*the length of s2 and s1 must also be equal in order to match:*/
                if (*s2 == '\0') return (0);
        }else{
                /*approximate matching and substr match:*/
		return (levdist(s1, s2, searchmethod,casesensitive));
	}
        return (-1);
}
/*
 * remove the extensions from the man-file name string
 * xxx.n becomes xxx
 * yyy.l.gz becomes yyy
 * passwd.nntp.5 becomes passwd.nntp
 * aaa.zzz.3s.gz becomes aaa.zzz
 * wish.addition.1.gz becomes wish.addition
 * sem_init.3thr becomes sem_init
 * innwatch.ctl.5 becomes innwatch.ctl
 *
 * the string must follow the pattern :
 * something.<anything word>[.gz or .Z or .bz2] becomes something
 */ 
char *removemanextesions(char *manpagefilename)
{
        char *rightdot;
        static char tmp[MAXLEN];
        /* the zero at the end is the terminator:*/
        int i=0;
        /*first copy then try to remove a .gz or .Z*/
        rightdot = NULL;
        while (*manpagefilename && i < MAXLEN -1 ){
                tmp[i]=*manpagefilename;
                if (*manpagefilename == '.'){
                        rightdot=tmp + i;
                }
                i++;
		manpagefilename++;
        }
        tmp[i]='\0';/* in case we reached MAXLEN*/
        if (rightdot == NULL){
                /*now this is rather strange we do not have a dot
                 *so let's return the whole string*/
                return(tmp);
        }
        if (streq(rightdot,".gz")){
                *rightdot='\0';
        }
        if (streq(rightdot,".bz2")){
                *rightdot='\0';
        }
        if (streq(rightdot,".bz")){
                *rightdot='\0';
        }
        if (streq(rightdot,".Z")){
                *rightdot='\0';
        }
        /*now we should have something like xxxx.1 or xxxx.3s
         *and we remove everything from the dot on*/
        manpagefilename = tmp; /*work on the copy*/
        rightdot=NULL;
        while (*manpagefilename ){
                if (*manpagefilename == '.'){
                        rightdot=manpagefilename;
                }
		manpagefilename++;
        }
        if (rightdot){
                /*cut at the right most dot:*/
                *rightdot='\0'; 
        }
        return(tmp);
}

/*
 * split MANPATH into substrings and return a pointer to 
 * a 2 dimmentional array with all the sub-strings of the
 * MANPATH. 
 */
char **splitman()
{
	char *mpath; /* man path */
	char *cmpath;/* copy of man path */
	static char **pathcomp;
        int i=0;
        int notseen=1;
	char *start;
	int compind = 0;	/* index of *pathcomp[] */
	int complen = 1;	/* len of one component in the path + 16 */

	mpath = (char *) getenv("MANPATH");
	if (mpath == NULL || *mpath == '\0') {
		fprintf(stderr,"Warning: MANPATH not defined, using /usr/man:/usr/share/man\n");
                mpath="/usr/man:/usr/share/man";
	}
        /* make a copy that is static */
        cmpath = (char *)malloc(sizeof(char)*(strlen(mpath) + 2));
        strcpy(cmpath, mpath);
	start = cmpath;
        /* count the number of colons in the path to find how many
         * components we have in the path */
	while (*cmpath) {
                if (*cmpath == ':') i++;
		cmpath++;
        }
        i+=3; /*some space in case we have corruped path below*/
        pathcomp=(char **)malloc(sizeof(char *)*i);
        /* split the path by replacing : with \0 */
	cmpath = start;
        pathcomp[compind]=cmpath;
	while (*cmpath) {
		if (*cmpath == ':') {
                        *cmpath = '\0';
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
                        if (complen > 2 && *(cmpath-1) == '/') *(cmpath-1)='\0';
                        pathcomp[compind] = cmpath+1;
                        complen = 0;
		}
		cmpath++;
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
printf("whichman -- fault tolerant approximate search command for man-pages\n\
\n\
USAGE:  whichman [-#hIep][-t#] [--] man-page-name\n\
\n\
Supported wildcards: * -- any arbitrary number of character\n\
                     ? -- one character\n\
\n\
OPTIONS: -h  this help\n\
         -I  search case sensitive (default is case in-sensitive)\n\
         -e  do exact match (disables also the wildcards * and ?)\n\
         -p  print the actual distance (tolerance) value in front of the\n\
             man-page file name.\n\
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
