/* Parallel token classification; each worker owns its input stream. */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
typedef struct { const char *input; int id, threads, failed; } Work;
static FILE *outputs[3];
static pthread_mutex_t locks[3] = {PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER};
static void json_string(FILE *out, const unsigned char *s) {
    fputc('"',out);
    for (;*s;++s) {
        if (*s=='"' || *s=='\\') fputc('\\',out);
        if (*s<32 || *s>=127) fprintf(out,"\\u%04x",*s); else fputc(*s,out);
    }
    fputs("\"\n",out);
}
static void *worker(void *argument) {
    Work *work=argument;
    FILE *file=fopen(work->input,"r");
    if (!file) { work->failed=1; return NULL; }
    char *line=NULL;size_t capacity=0,index=0;
    while (getline(&line,&capacity,file)>=0) {
        if (index++ % work->threads != (size_t)work->id) continue;
        char *save=NULL;
        for (char *token=strtok_r(line," \t\r\n",&save);token;token=strtok_r(NULL," \t\r\n",&save)) {
            int alpha=0,digit=0,other=0;
            for (const unsigned char *s=(unsigned char *)token;*s;++s) {
                if (isalpha(*s)) alpha=1;else if (isdigit(*s)) digit=1;else other=1;
            }
            int type=!other&&digit&&!alpha?0:!other&&alpha&&!digit?1:alpha&&digit?2:-1;
            if (type<0) continue;
            pthread_mutex_lock(&locks[type]);json_string(outputs[type],(unsigned char *)token);pthread_mutex_unlock(&locks[type]);
        }
    }
    if (ferror(file)) work->failed=1;
    free(line);fclose(file);return NULL;
}
int main(int argc,char **argv) {
    if (argc<2 || argc>3) { fprintf(stderr,"Usage: %s input.txt [threads]\n",argv[0]); return 1; }
    char *end=NULL;long threads=argc==3?strtol(argv[2],&end,10):12;
    if (threads<1 || threads>256 || (argc==3 && (!*argv[2] || *end))) return 1;
    FILE *check=fopen(argv[1],"r");if (!check) { perror(argv[1]);return 1; }fclose(check);
    const char *names[]={"numbers.jsonl","letters.jsonl","mixed.jsonl"};
    for (int i=0;i<3;++i) {
        outputs[i]=fopen(names[i],"w");
        if (!outputs[i]) { for(int j=0;j<i;++j) fclose(outputs[j]);return 1; }
    }
    Work work[256];pthread_t ids[256];int launched=0,status=0;
    for (int i=0;i<threads;++i) {
        work[i]=(Work){argv[1],i,(int)threads,0};
        if (pthread_create(&ids[i],NULL,worker,&work[i])) { status=1;break; }++launched;
    }
    for (int i=0;i<launched;++i) { pthread_join(ids[i],NULL);status|=work[i].failed; }
    for (int i=0;i<3;++i) { if(ferror(outputs[i])) status=1;if(fclose(outputs[i])) status=1; }
    return status;
}
