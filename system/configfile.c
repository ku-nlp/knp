/* $Id$ */

#include "knp.h"
#include <sys/types.h>
#include <sys/stat.h>

#define KNPRCFILE ".knprc"
char *RuleDIR = NULL;

void DelTailSpace(char *buffer)
{
    int i;

    if (!buffer)
	return;

    for (i = strlen(buffer)-1; i >= 0; i--) {
	if (*(buffer+i) == ' ' || *(buffer+i) == '\t')
	    *(buffer+i) = '\0';
	else
	    break;
    }
}

void ScanLine(char *buffer)
{
    char key[DATA_LEN], value[DATA_LEN];
    int status;
    struct stat sb;

    sscanf(buffer, "%[^=]=%[^\n]\n", key, value);

    if (!strcmp(key, "ruledir")) {
	DelTailSpace(value);
	RuleDIR = strdup(value);
	status = stat(RuleDIR, &sb);
	if (status < 0) {
	    fprintf(stderr, "%s ... %s: No such directory.\n", key, RuleDIR);
	    exit(1);
	}
#if 1
    fprintf(stderr, "... <%s> <%s> ... ", key, RuleDIR);
    if (status < 0)
	fprintf(stderr, "not found\n");
    else
	fprintf(stderr, "OK\n");
#endif
    }
}

void ReadConfigFile()
{
    FILE *fp;
    char *home, *file, buffer[DATA_LEN];

    home = getenv("HOME");

    if (!home)
	return;

    file = (char *)malloc_data(strlen(home)+strlen(KNPRCFILE)+2);
    sprintf(file, "%s/%s", home, KNPRCFILE);

#if 1
    fprintf(stderr, "Opening %s ... ", file);
#endif

    if (!(fp = fopen(file, "r"))) {
#if 1
	fprintf(stderr, "failed\n");
#endif
	return;
    }
#if 1
	fprintf(stderr, "OK\n");
#endif

    buffer[DATA_LEN-1] = GUARD;

    while (fgets(buffer, DATA_LEN, fp)) {
	if (buffer[DATA_LEN-1] != GUARD)
	    break;

	ScanLine(buffer);
    }

    fclose(fp);
}

#if 1
int main()
{
    ReadConfigFile();
}
#endif
