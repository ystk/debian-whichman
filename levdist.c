/* vim: set sw=8 ts=8 si et: */
/*
 * levdist.c, String distance calculation.
 * This program is distributed under the terms 
 * of the Gnu Public License (GPL). 
 */
/* 
 * Weighted Levenshtein Distance (WLD) calculation.
 * The Levenshtein Distance is defined as the number
 * of replacements, insertions and deletions that are
 * necessary to transform a word into the search pattern.
 *
 * The Levenshtein Distance between  SOmestr and 'SO?e*r'
 * is:
 *   |SOmestr
 *   J1234567  |I
 *   |---------|
 * S |0123456  |1
 * O |1012345  |2
 * ? |2101234  |3
 * e |3210123  |4
 * * |3210000  |5
 * r |4321110  |6 The min distance is always in the lower right
 *                corner.
 *
 * here the number at (J,I) = (4,2) = 2 indicates that the distance
 * between 'SOme' and 'SO' is 2. I.e you must insert 2 characters
 * to transform the substrings into each other.
 *
 * The algorithm below knows about the wildcards '?' and '*'. 
 * ? = any single character (exactly one)
 * * = any character (including no character)
 *
 * The minimum distance is alwasy at least as big as the
 * minimum in each row. This allows to terminate the search
 * already at an early stage if a limit is given. The minimum 
 * distance can only increase from top to bottom.
 * An second example:
 *
 * distance between SOmestr and '*mxest*r'
 *   |SOmestr
 *   |--------
 * * |0000000  min:0
 * m |1101111  min:0
 * x |2211222  min:1
 * e |3321233  min:1
 * s |3432123  min:1
 * t |4443212  min:1
 * * |4443211  min:1
 * r |5554321 The total min distance is 1
 * 
 */

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "levdist.h"


/*
 * compute a standard tolerance value based on search string lenght
 * minus number of wildchards in searchsting
 */
int stdtolerance(const char *searchpat)
{
	int len = 0;
	while (*searchpat && len < 200) {
                if (*searchpat == '?' || *searchpat == '*'){
                        searchpat++;
                        continue;
                }
                searchpat++;
                len++;
        }
        len=(len/6)+1;
	return (len);
}
/*
 * Translate string to lower-case and return word lenght
 * With casesensitive=1 only the word lenght is returned and the
 * string is copied unmodified from word to lowword.
 */
int lowercaseword(char *lowword,const char *word, int maxlen,int casesensitive)
{
	int len = 0;
	while (*word && len < maxlen -1) {
                if(casesensitive){
                        *lowword=*word;
                }else{
                        *lowword=tolower(*word);
                }
                word++;lowword++;
                len++;
        }
        *lowword=0; /*in case maxlen was reached*/
	return len;
}
/*
 *calculate the minimum of a,b,c and return it
 */
inline int minimum(int a,int b, int c)
{
        if (a < b) b = a;
        if (b < c) c = b;
        return(c);
}
/*
* Calculate the Levenshtein distance between iword and ipattern
* Return 'the real distance' if the distance is less or equal 
* to a given limit.
* return -1 if the distance between iword and ipattern is higher 
* than the given limit.
* For casesensitive=1 the algorithm is case sensitive.
*/
int levdist(const char *iword, const char *ipattern, int limit,int casesensitive)
{
	int dmin, p,pp,q,lpat,lwrd,d1,d2,i,j;
        char c,word[MAXLEN],pattern[MAXLEN];
	int dstprof[MAXLEN];
        if (! *iword) return(-1); /*no empty word*/
        if (! *ipattern) return(-1); /*no empty pattern*/

        lwrd = lowercaseword(word,iword,MAXLEN,casesensitive);
        lpat = lowercaseword(pattern,ipattern,MAXLEN,casesensitive);

        /*calculate the first row, that is: distance against
         *the first character of the pattern
         */
	if (*pattern == '*') {
                /*the first row has distance 0 if pattern starts with a star*/
		for (j = 0; j <= lwrd; j++) {
			dstprof[j] = 0;
		}
#ifdef DEBUG
                        printf("0..0");
#endif //DEBUG
	} else {
		dstprof[0] =1;
		i = (*pattern == '?') ? 0 : 1;
		for (j = 0; j < lwrd; j++) {
			if (*pattern == *(word + j)) {
				i = 0;
			}
			dstprof[j+1] = j + i;
#ifdef DEBUG
                        printf("%d",dstprof[j+1]);
#endif //DEBUG
		}
	}
#ifdef DEBUG
        printf("\n");
#endif //DEBUG
        i=1;
        dmin=dstprof[1];
        while(i < lpat && dmin <= limit){
		c = *(pattern + i );
                pp=1;q=1; /*default*/
                if (c == '*' || c == '?') pp = 0;
                if (c == '*') q = 0;
		d2 = dstprof[0];
                dmin = d2 + q;
		dstprof[0] = dmin;
                i++;
		for (j = 1; j <= lwrd; j++) {
			d1 = d2;
			d2 = dstprof[j];
                        p = pp; /*default unless c == word character at pos.*/
                        if (c == *(word + j -1)) p =0;
                        dstprof[j] = minimum(d1 + p,d2 + q,dstprof[j-1] + q);
			if (dstprof[j] < dmin) {
				dmin = dstprof[j];
			}
#ifdef DEBUG
                        printf("%d",dstprof[j]);
#endif //DEBUG
		}
#ifdef DEBUG
                printf("\n");
#endif //DEBUG
	}
        if (dstprof[lwrd] <= limit){
                /*we have a match between word and pattern, they 
                 * are within the distance limit */
                return(dstprof[lwrd]);
        }
	return (-1);
}


#ifdef RETURNVALUE

int main(int argc, char **argv)
{
        int d,limit;
        char *word;
        char *pattern;
	if (argc == 5) {
		word = argv[1];
		pattern = argv[2];
		limit = atoi(argv[3]);
                if (*argv[4] == '1'){
                        printf("not case sensitive\n");
                        d=levdist(word, pattern, limit,1);
                }else{
                        printf("match case sensitive\n");
                        d=levdist(word, pattern, limit,0);
                }
#ifdef DEBUG
                printf("distance: %d\n",d);
#endif //DEBUG
                return(d);
	} else {
		fprintf(stderr,"Usage: %s word pattern dist_limit case_sensitive\n", argv[0]);
		fprintf(stderr,"case_sensitive: 1|0 dist_limit:0-255\n");
		fprintf(stderr,"Example: %s hello hallo 5 1\n", argv[0]);
	}
        return(0);
}

#endif //RETURNVALUE

