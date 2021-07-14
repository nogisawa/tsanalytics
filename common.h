#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "m2tlib/m2tlib.h"
#include "version.h"

// TOTやpercentと連動させるための
// ドロップカウンタ
typedef struct DROP_LIST DROP_LIST;
struct DROP_LIST {
	char		key[32];
	uint64_t	drop;
	uint64_t	error;
	uint64_t	scrambling;
	DROP_LIST	*next;
};

// tsanalytics引数リスト
typedef struct ARGS_TSANALYTICS ARGS_TSANALYTICS;
struct ARGS_TSANALYTICS {
	uint16_t	service_id;
	char		*from_path;
	char		*out_path;
};

// 関数宣言
// tsanalytics.c
void tsanalytics(ARGS_TSANALYTICS *p);
void add_drop_list( DROP_LIST **drop_list, char *key, TS_DROP_COUNT *ts_drop_count );
void dump_drop_list( DROP_LIST *drop_list );
void free_drop_list( DROP_LIST *ptr );

void conv_json_drop_pid(FILE *fp_out, TS_DROP_COUNT *p , int in );
void conv_json_drop_key(FILE *fp_out, DROP_LIST *p , char *name, int in );
