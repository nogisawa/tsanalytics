/* tsanalytics本体 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "common.h"

void indent(FILE *fp_out, int in){
	int i;
	for(i=0;i<in;i++){
		fprintf(fp_out," ");
	}
}

// PIDごとのdrop状況を表示する
void conv_json_drop_pid(FILE *fp_out, TS_DROP_COUNT *p , int in ){

 	indent(fp_out,in); 
	fprintf(fp_out, "\"drop_pid\":{\n");

	int i;
	for(i=0;i<8192;i++){
               if( p[i].total > 0){
                        // 表示
			indent(fp_out,in+3);
                        fprintf(fp_out,"\"0x%04X\" : {\"total\":\"%d\", \"drop\":\"%d\", \"error\":\"%d\", \"scrambling\":\"%d\"}" ,
                         i, p[i].total, p[i].drop, p[i].error, p[i].scrambling );

                        // そのPIDは途中？最後？
                        if( p[i].last == 0 ){ fprintf(fp_out, ",\n");  }
                                       else { fprintf(fp_out, "\n");   }
                }
	}

 	indent(fp_out,in); 
	fprintf(fp_out, "}");

}

// キーごとにdrop状況を表示する
void conv_json_drop_key(FILE *fp_out, DROP_LIST *p , char *name, int in ){

 	indent(fp_out,in); 
	fprintf(fp_out, "\"drop_%s\":{\n", name);

	// 【設定】この件数を超えたら表示を諦める
	int max_count = 20000;

	int i;
        for(i=0;i<max_count;i++){
                if( p==NULL ){ break;}

                // 表示部
		indent(fp_out,in+3);
                fprintf(fp_out,"\"%s\" : {\"drop\":\"%d\", \"error\":\"%d\", \"scrambling\":\"%d\"}",
                                p->key, p->drop, p->error, p->scrambling );

		// 表示の限界に達したらそこで終了
		if(i == max_count-1){  fprintf(fp_out,"\n");
				       break;                  }

                // 続く場合,を入れるが、続かない場合は何も入れない
                if( p->next != NULL ){ fprintf(fp_out,",\n");  }
                                else { fprintf(fp_out,"\n");   }
                p = p->next;
        }

 	indent(fp_out,in); 
	fprintf(fp_out, "}");

}

