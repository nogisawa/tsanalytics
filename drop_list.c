/* tsanalytics本体 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "common.h"

#define DEBUG 0

// これ以上数えない用に抑止しておく。念の為。
#define MAX_COUNT 99999999

void add_drop_list( DROP_LIST **drop_list, char *key, TS_DROP_COUNT *ts_drop_count ){

	// 最終的にドロップを入れていく連結リスト
	DROP_LIST *p;

	if( *drop_list == NULL ){
		// 初回だった場合領域を確保してそこをpとする
		*drop_list = (DROP_LIST *)calloc(1,sizeof(DROP_LIST));
		strcpy((*drop_list)->key, key );
#if DEBUG
		printf("drop_list.c: INITIAL: %s\n",key);
#endif

	}

	p = *drop_list;

	int i;
	for(i=0; i<100000; i++){

		// キーと一致したらそこを採用
		if( strcmp(p->key, key)==0 ){ break; }

		// 最後まで来てしまったら新規領域を作成
		if( p->next == NULL ){
			p->next = (DROP_LIST *)calloc(1,sizeof(DROP_LIST));
                       	p = p->next;
                      	strcpy(p->key, key );
                	break;
		}

		// ポインタを勧める
		p = p->next;

	}

	// 異常な回数ループしたらどうしようね
	

#if DEBUG
	printf("drop_list.c: ADD: %s (drop=%d, error=%d, scrambling=%d)\n",
			p->key, ts_drop_count->now_drop, ts_drop_count->now_error, ts_drop_count->now_scrambling);
#endif

	// 各種値の挿入
	// ただし、MAC_COUNTを超えそうな場合は抑止する
	if( p->drop  < MAX_COUNT ){ p->drop       += ts_drop_count->now_drop;  }
	if( p->error < MAX_COUNT ){ p->error      += ts_drop_count->now_error; }
	if( p->scrambling < MAX_COUNT ){ p->scrambling += ts_drop_count->now_scrambling; }

	return;

}

// ドロップリストをクリアする
void free_drop_list( DROP_LIST *ptr ){
	int i;
	DROP_LIST *tmp;
	for(i=0;i<1000000;i++){
		if( ptr == NULL){ break; }
		tmp = ptr->next;
		free(ptr);
		ptr=tmp;
	}

}

void dump_drop_list( DROP_LIST *drop_list ){

	if( drop_list == NULL ){
		printf("DROP LIST IS EMPTY\n");
		return;
	}

	int i;
	DROP_LIST *p = drop_list;
	for(i=0;i<10000;i++){
		if( p == NULL ){break;}
		printf("%s: drop=%d error=%d scrambling=%d\n",
			p->key, p->drop, p->error, p->scrambling);
		p = p->next;
	}

}
