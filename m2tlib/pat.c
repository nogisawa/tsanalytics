#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m2tlib.h"

#define DEBUG 0

void st_pat( TS_PAT *out, unsigned char *buf ){

        uint64_t gomi;
        uint64_t boff = 0;
	uint64_t i;
        out->table_id                   = getBit( buf, &boff, 8 );
        out->section_syntax_indicator   = getBit( buf, &boff, 1 );
        gomi                            = getBit( buf, &boff, 1 );
        gomi	                        = getBit( buf, &boff, 2 );
        out->section_length             = getBit( buf, &boff, 12);
        out->transport_stream_id        = getBit( buf, &boff, 16);
        gomi                            = getBit( buf, &boff, 2 );
        out->version_number             = getBit( buf, &boff, 5 );
        out->current_next_indicator     = getBit( buf, &boff, 1 );
        out->section_number             = getBit( buf, &boff, 8 );
        out->last_section_number        = getBit( buf, &boff, 8 );

        // いくつのPIDが入っているか計算する。これはsection_lengthから求められる
        out->num_of_pids        = ( out->section_length -5 -4 ) /4;

        // 初期値はNULL
        out->pids = NULL;
        TS_PAT_PIDS *before_p, *p;
        before_p = NULL;

        // PIDを連結リストに代入していく
        for(i=0; i<out->num_of_pids ; i++){

                // 領域を確保
                p = (TS_PAT_PIDS *)calloc(1, sizeof(TS_PAT_PIDS));

                // 初回だったらout->pidsに記録
                if( out->pids == NULL ){
                        out->pids = p;
                }

                // 情報を代入していく
                p->program_number       = getBit( buf, &boff, 16);
                gomi                    = getBit( buf, &boff, 3 );

                // program_numberが0の時はnetwork_PID
                if( p->program_number == 0x00 ){
                        p->network_PID          = getBit( buf, &boff, 13);
                }else{
                        p->program_map_PID      = getBit( buf, &boff, 13);
                }

                // 1つ前のリストのnextを更新する
                // 現時点でのnextはNULLへ
                if( before_p != NULL ) { before_p->next = p; }
                p->next         = NULL;

                // 次に進める前の下準備
                before_p = p;

        }


#if DEBUG
        // === PIDリストのテスト
        printf("pat.c: TABLE=0x%02X SecLen=%d NumID=%d MemAddress=%X\n",
             out->table_id , out->section_length, out->num_of_pids, out);

	p = out->pids;
        for(int i=0; i<out->num_of_pids ; i++ ){
                printf("pat.c:    PID ProgNum:0x%04X NetPID:0x%04X MapPID:0x%04X\n", p->program_number, p->network_PID, p->program_map_PID );
                p = p->next;
        }

#endif

}

// ====================================== free系
void free_pat_pids( TS_PAT *out ){

#if DEBUG
	printf("pat.c: FREE_PAT_PIDS(0x%X)\n",out);
#endif

	// 実際はTS_PAT_PIDSのみを初期化する。PAT自体は消さない
        TS_PAT_PIDS *p, *tmp;
        p = out->pids;
	int i;
        for(i=0; i<out->num_of_pids ; i++ ){
                if(p == NULL){ break; }
                tmp = p->next;
                free(p);
                p = tmp;
        }
        out->pids = NULL;
}

uint16_t get_pmt_pid( TS_PAT *ptr, uint16_t service_id ){

	int i;
	TS_PAT_PIDS *p = ptr->pids;
        for(i=0; i<ptr->num_of_pids ; i++ ){
		if( p == NULL ){ break; }
		if( p->program_number != 0x0 && service_id == p->program_number ){
			return(p->program_map_PID);
		}
                p = p->next;
        }
	
	return(0);

}

// PATの中身をdumpする
void dump_pat( TS_PAT *out ){

        TS_PAT_PIDS *p;
        p = out->pids;

        for(int i=0; i<out->num_of_pids ; i++ ){
                printf("        PID ProgNum:0x%04X NetPID:0x%04X MapPID:0x%04X\n", p->program_number, p->network_PID, p->program_map_PID );
                p = p->next;
        }
        printf("TABLE=0x%02X SecLen=%d NumID=%d\n",
                out->table_id , out->section_length, out->num_of_pids);

}

