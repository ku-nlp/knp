#ifdef INTERNAL_HASH

#include <stdio.h>
#include <string.h>
#include "hash.h"


/*==================================================================*/
		  void read_seed(HASH_FILE *hashdb)
/*==================================================================*/
{
    fread(hashdb->seed, sizeof(unsigned int), NSEED*NSIZE, hashdb->fp);
}

/*==================================================================*/
		 void read_hhdata(HASH_FILE *hashdb)
/*==================================================================*/
{
    hashdb->hhdata = (HASH_HEADER_UNIT *)malloc_data(sizeof(HASH_DATA_UNIT)*HASH_TBLSIZE, 
						     "read_hhdata");
    fread(hashdb->hhdata, sizeof(HASH_HEADER_UNIT), HASH_TBLSIZE, hashdb->fp);
}

/*==================================================================*/
		  void init_hash_seed(HASH_FILE *hashdb)
/*==================================================================*/
{
    int i, j;

    srand(time(NULL));
    for (i = 0; i < NSEED; ++i) {
	for (j = 0; j < NSIZE; ++j) {
	    hashdb->seed[i][j] = rand();
	}
    }
}

/*==================================================================*/
 int internal_hash(HASH_FILE *hashdb, unsigned char *key, int keylen)
/*==================================================================*/
{
    int i = 0, j = 0, hash = 0;

    while (i < keylen) {
	hash ^= hashdb->seed[j++][key[i++]];
	j &= NSEED-1;
    }
    return (hash & (HASH_TBLSIZE-1));
}

/*==================================================================*/
		HASH_FILE *hash_read_open(char *file)
/*==================================================================*/
{
    HASH_FILE *hashdb;

    hashdb = (HASH_FILE *)malloc_data(sizeof(HASH_FILE), "hash_read_open");
    hashdb->mode = HASH_READ_MODE;

    if ((hashdb->fp = fopen(file, "r")) == NULL) {
	fprintf(stderr, "Cannot open file (%s) !!\n", file);
	return NULL;
    }

    read_seed(hashdb);
    read_hhdata(hashdb);

    return hashdb;
}

/*==================================================================*/
		HASH_FILE *hash_write_open(char *file)
/*==================================================================*/
{
    HASH_FILE *hashdb;

    hashdb = (HASH_FILE *)malloc_data(sizeof(HASH_FILE), "hash_write_open");
    hashdb->mode = HASH_WRITE_MODE;

    init_hash_seed(hashdb);

    hashdb->hddata = (HASH_DATA_UNIT_INTERNAL **)malloc_data(sizeof(HASH_DATA_UNIT_INTERNAL *)*HASH_TBLSIZE, 
							    "hash_write_open");
    memset(hashdb->hddata, 0, sizeof(HASH_DATA_UNIT_INTERNAL *)*HASH_TBLSIZE);

    if ((hashdb->fp = fopen(file, "w")) == NULL) {
	fprintf(stderr, ";; Cannot open file (%s) !!\n", file);
	exit(1);
    }

    return hashdb;
}

/*==================================================================*/
	    char *hash_fetch(HASH_FILE *hashdb, char *key)
/*==================================================================*/
{
    char *rkey, *rvalue;
    int h, i, key_size, max_key_size, pos, num, target;
    HASH_DATA_UNIT hdu, *hdp;

    key_size = strlen(key);
    h = internal_hash(hashdb, key, key_size);

    /* 書き込みモードのとき */
    if (hashdb->mode == HASH_WRITE_MODE) {
	HASH_DATA_UNIT_INTERNAL **hdpp;

	hdpp = &(hashdb->hddata[h]);
	while (*hdpp) {
	    if (!strcmp((*hdpp)->key, key)) {
		char *ret;

		ret = strdup((*hdpp)->value);
		return ret;
	    }
	    hdpp = &((*hdpp)->next);
	}
	return NULL;
    }

    /* 読み込みモードのとき */

    if (hashdb->hhdata[h].pos == 0) {
	return NULL;
    }
    else {
	target = -1;
	max_key_size = 0;

	fseek(hashdb->fp, hashdb->hhdata[h].pos, SEEK_SET);
	fread(&num, sizeof(int), 1, hashdb->fp);	/* 同じhash中の個数 */

	/* 個数分のkey, valueのサイズを取得 */
	hdp = (HASH_DATA_UNIT *)malloc_data(sizeof(HASH_DATA_UNIT)*num, "hash_fetch");
	for (i = 0; i < num; i++) {
	    fread(&hdu, sizeof(HASH_DATA_UNIT), 1, hashdb->fp);
	    (hdp+i)->key_size = hdu.key_size;
	    (hdp+i)->value_size = hdu.value_size;
	    if (max_key_size < hdu.key_size) {
		max_key_size = hdu.key_size;
	    }
	}

	rkey = (char *)malloc_data(max_key_size, "hash_fetch");

	/* マッチするkeyを探す */
	for (i = 0; i < num; i++) {
	    if (target < 0) {
		fread(rkey, (hdp+i)->key_size, 1, hashdb->fp);
		if (key_size == (hdp+i)->key_size && 
		    !strncmp(key, rkey, (hdp+i)->key_size)) {
		    target = i;
		}
	    }
	    else {
		/* keyがみつかった後はvalueの頭までseekしておく */
		fseek(hashdb->fp, (hdp+i)->key_size, SEEK_CUR);
	    }
	}

	free(rkey);
	if (target < 0) {
	    return NULL;	/* マッチしなかった */
	}

	rvalue = (char *)malloc_data((hdp+target)->value_size+1, "hash_fetch");

	/* 目的のvalueまでseek */
	for (i = 0; i < target; i++) {
	    fseek(hashdb->fp, (hdp+i)->value_size, SEEK_CUR);
	}

	fread(rvalue, (hdp+target)->value_size, 1, hashdb->fp);
	*(rvalue+(hdp+target)->value_size) = '\0';

	free(hdp);
    }
    return rvalue;
}


/*==================================================================*/
  int hash_put(HASH_FILE *hashdb, char *key, char *value, int mode)
/*==================================================================*/
{
    int h;
    HASH_DATA_UNIT_INTERNAL **hdpp, *next = NULL;

    h = internal_hash(hashdb, key, strlen(key));
    hdpp = &(hashdb->hddata[h]);

    while (*hdpp) {
	/* 同じkeyがすでにあれば上書き */
	if (!strcmp((*hdpp)->key, key)) {
	    if (mode == HASH_NOOVERWRITE) {
		return HASH_KEYEXIST;
	    }
	    next = (*hdpp)->next;
	    free(*hdpp);
	    break;
	}
	hdpp = &((*hdpp)->next);
    }

    *hdpp = (HASH_DATA_UNIT_INTERNAL *)malloc_data(sizeof(HASH_DATA_UNIT_INTERNAL), "hash_put");
    (*hdpp)->key = strdup(key);
    (*hdpp)->key_size = strlen(key);
    (*hdpp)->value = strdup(value);
    (*hdpp)->value_size = strlen(value);
    (*hdpp)->next = next;

    return 0;
}

/*==================================================================*/
		 void hash_finish(HASH_FILE *hashdb)
/*==================================================================*/
{
    int i, j, count, pos;
    HASH_HEADER_UNIT hhu;
    HASH_DATA_UNIT hdu;
    HASH_DATA_UNIT_INTERNAL *hdp;
    int cdata[HASH_TBLSIZE];

    /* SEEDの書き込み */
    for (i = 0; i < NSEED; ++i) {
	for (j = 0; j < NSIZE; ++j) {
	    if (fwrite(&(hashdb->seed[i][j]), sizeof(unsigned int), 1, hashdb->fp) < 1) {
		fprintf(stderr, ";; Error in fwrite.\n");
		exit(1);
	    }
	}
    }

    /* 個数のカウント */
    for (i = 0; i < HASH_TBLSIZE; i++) {
	if (hashdb->hddata[i]) {
	    count = 0;
	    hdp = hashdb->hddata[i];
	    while (hdp) {
		count++;
		hdp = hdp->next;
	    }
	    cdata[i] = count;
	}
	else {
	    cdata[i] = 0;
	}
    }

    /* ハッシュ配列(アドレス)の書き込み */
    pos = sizeof(unsigned int)*NSEED*NSIZE+sizeof(HASH_HEADER_UNIT)*HASH_TBLSIZE;
    for (i = 0; i < HASH_TBLSIZE; i++) {
	if (hashdb->hddata[i]) {
	    hhu.pos = pos;

	    pos += sizeof(int)+sizeof(HASH_DATA_UNIT)*cdata[i];

	    hdp = hashdb->hddata[i];
	    while (hdp) {
		pos += hdp->key_size+hdp->value_size;
		hdp = hdp->next;
	    }
	}
	else {
	    hhu.pos = 0;
	}
	if (fwrite(&hhu, sizeof(HASH_HEADER_UNIT), 1, hashdb->fp) < 1) {
	    fprintf(stderr, ";; Error in fwrite.\n");
	    exit(1);
	}
    }

    /* データの書き込み */
    for (i = 0; i < HASH_TBLSIZE; i++) {
	if (hashdb->hddata[i]) {
	    /* 個数の書き込み */
	    if (fwrite(&cdata[i], sizeof(int), 1, hashdb->fp) < 1) {
		fprintf(stderr, ";; Error in fwrite.\n");
		exit(1);
	    }

	    /* key, value のサイズを書き込み */
	    hdp = hashdb->hddata[i];
	    while (hdp) {
		hdu.key_size = hdp->key_size;
		hdu.value_size = hdp->value_size;

		if (fwrite(&hdu, sizeof(HASH_DATA_UNIT), 1, hashdb->fp) < 1) {
		    fprintf(stderr, ";; Error in fwrite.\n");
		    exit(1);
		}

		hdp = hdp->next;
	    }

	    /* key */
	    hdp = hashdb->hddata[i];
	    while (hdp) {
		if (fwrite(hdp->key, hdp->key_size, 1, hashdb->fp) < 1) {
		    fprintf(stderr, ";; Error in fwrite.\n");
		    exit(1);
		}

		hdp = hdp->next;
	    }

	    /* value */
	    hdp = hashdb->hddata[i];
	    while (hdp) {
		if (fwrite(hdp->value, hdp->value_size, 1, hashdb->fp) < 1) {
		    fprintf(stderr, ";; Error in fwrite.\n");
		    exit(1);
		}

		hdp = hdp->next;
	    }
	}
    }
}

/*==================================================================*/
		  void hash_close(HASH_FILE *hashdb)
/*==================================================================*/
{
    if (hashdb->mode == HASH_READ_MODE) {
	free(hashdb->hhdata);
    }
    else if (hashdb->mode == HASH_WRITE_MODE) {
	int i;
	HASH_DATA_UNIT_INTERNAL *hdp, *next;

	hash_finish(hashdb);

	for (i = 0; i < HASH_TBLSIZE; i++) {
	    if (hashdb->hddata[i]) {
		hdp = hashdb->hddata[i];
		while (hdp) {
		    next = hdp->next;
		    free(hdp->key);
		    free(hdp->value);
		    free(hdp);
		    hdp = next;
		}
	    }
	}
	free(hashdb->hddata);
    }

    fclose(hashdb->fp);
    free(hashdb);
}

#endif
