#ifndef LEVDIST
#define LEVDIST 1
/*max search pattern length: */
#define MAXLEN 256 
#define MYVERSION "@(#)Version: 2.3" 

extern int levdist(const char *iword, const char *ipattern, int limit,int casesensitive);
extern int stdtolerance(const char *searchpat);
extern int lowercaseword(char *lowword,const char *word, int maxlen,int casesensitive);

#endif //LEVDIST
