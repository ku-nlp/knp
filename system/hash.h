#ifndef HASH_H
#define HASH_H

#ifndef NSEED
#define	NSEED	32	/* 乱数表の種類。2の羃乗でなければならない。 */
#endif

#ifndef NSIZE
#define NSIZE	256
#endif

#define	HASH_TBLSIZE	8192

#define	HASH_READ_MODE	1
#define	HASH_WRITE_MODE	2

#define	HASH_NOOVERWRITE	8
#define	HASH_KEYEXIST		(-30996)

/* 実データ (内部表現) */
typedef struct _hash_data_unit_internal {
    unsigned int	key_size;
    unsigned int	value_size;
    char		*key;
    char		*value;
    struct _hash_data_unit_internal	*next;	/* conflict時に格納する */
} HASH_DATA_UNIT_INTERNAL;

typedef struct {
    unsigned int	pos;
} HASH_HEADER_UNIT;

typedef struct {
    unsigned int	key_size;
    unsigned int	value_size;
} HASH_DATA_UNIT;

typedef struct {
    FILE *fp;
    int mode;
    unsigned int seed[NSEED][NSIZE];
    HASH_HEADER_UNIT *hhdata;
    HASH_DATA_UNIT_INTERNAL **hddata;
} HASH_FILE;

#endif
