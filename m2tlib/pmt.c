#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m2tlib.h"

void st_pmt( TS_PMT *out, unsigned char *buf ){

        uint64_t gomi;
        uint64_t boff = 0;
	uint64_t i;
        out->table_id                   = getBit( buf, &boff, 8 );
        out->section_syntax_indicator   = getBit( buf, &boff, 1 );
        gomi                            = getBit( buf, &boff, 1 );
        gomi                            = getBit( buf, &boff, 2 );
        out->section_length             = getBit( buf, &boff, 12);
        out->program_number             = getBit( buf, &boff, 16);
        gomi                            = getBit( buf, &boff, 2 );
        out->version_number             = getBit( buf, &boff, 5 );
        out->current_next_indicator     = getBit( buf, &boff, 1 );
        out->section_number             = getBit( buf, &boff, 8 );
        out->last_section_number        = getBit( buf, &boff, 8 );
	gomi				= getBit( buf, &boff, 3 );
	out->PCR_PID			= getBit( buf, &boff, 13);
	gomi				= getBit( buf, &boff, 4 );
	out->program_info_length	= getBit( buf, &boff, 12);

	// 初期値はNULL
	out->streams			= NULL;
	TS_PMT_STREAMS	*p, *before_p;
	p	 = NULL;
	before_p = NULL;

	// 今回Descriptorは使わないのでスキップする
	boff += (out->program_info_length)*8;

	// streamsの最後のバイト数を求める
	out->end_of_streams = out->section_length - 4;
	out->num_of_streams = 0;

	// PIDリストを格納する構造体を定義

	// ストリームを連結リストに入れていく
	//printf("stream_type=%X\n", getBit( buf, &boff, 8) );
	while( boff/8 < out->end_of_streams ){

		p = (TS_PMT_STREAMS *)calloc(1, sizeof(TS_PMT_STREAMS));

		// 初回なら元の構造体にアドレスを残す
		if( out->streams == NULL ){ out->streams = p; }


		p->stream_type    = getBit( buf, &boff, 8 );
		gomi		  = getBit( buf, &boff, 3 );
		p->elementary_PID = getBit( buf, &boff, 13);
		gomi		  = getBit( buf, &boff, 4 );
		p->ES_info_length = getBit( buf, &boff, 12);

		// 今回はdescriptorをスキップする
		boff += (p->ES_info_length)*8;

		// printf("stream_type=%X (PID=%X)\n", p->stream_type, p->elementary_PID );

		// ポインタの付け替え
		if( before_p != NULL ){ before_p->next = p; }
		before_p 	= p;

		// streamの個数をカウントしておく
		out->num_of_streams++;

	}

}

void dump_pmt(TS_PMT *pmt){

	printf("ProgNum=0x%04X table_id=0x%02X\n",pmt->program_number, pmt->table_id );

	int i;
	TS_PMT_STREAMS *p = pmt->streams;
	for(i=0;i<pmt->num_of_streams;i++){
		printf("stream_type=0x%02X elementary_PID=0x%04X\n", p->stream_type, p->elementary_PID );
		p=p->next;
	}

}
