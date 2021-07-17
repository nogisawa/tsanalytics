#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>

uint64_t getBit(unsigned char *buf, uint64_t *boff, uint64_t gbit) {
	
	// おおまかな動き
	// 下記の例は最初の4ビットから後ろの2ビットを取り出す操作。
	// - まずはtnumにサイズ上限まで突っ込む(ここでは2バイト/16bitの例)
	//                 1byte                1byte
	//  char     |-------------------|-------------------|
	//            1111 1111 1111 1111 1111 1111 1111 1111
	//  uint64_t  1111 1111 1111 1111 1111 1111 1111 1111
	//           |----------------2bytes-----------------|
	//           
	// - 先頭な余分なビットをカット
	//  uint64_t  11 1111 1111 1111 1111 1111 1111 1111   << 2bit shift
	//  uint64_t  0011 1111 1111 1111 1111 1111 1111 1111 >> 2bit shift
	//
	// - 最後に余分な後半ビットをシフト
	//  uint64_t  0011 1111 1111 1111 1111 1111 1111 1111 
	//  uint64_t  0000 0000 0000 0000 0000 0000 0000 0011 >> 56bit shift

	uint64_t tmp,tnum = 0; // 結果代入用変数
	uint64_t pbyte = *boff / 8;
	unsigned char *fbyte;
        fbyte = buf + pbyte;

	// まずはサイズ上限までデータを突っ込む
	int size = sizeof(tnum);
	int i;
	for(i=1; i<=size; i++){
		tmp  = (uint64_t)*(fbyte+i-1);
		tmp  = tmp << ((size-i)*8);
		tnum = tnum | tmp;
	}

	// 前半の部分をシフトして消す
	uint64_t fcutbit = *boff - (pbyte*8);
	tnum = tnum <<  fcutbit;
	tnum = tnum >>  fcutbit;

	// 後半部分をシフト
	tnum = tnum >> ( (sizeof(tnum)*8) - fcutbit - gbit );

	// boff(次の読み込みビット数)を更新
	*boff = *boff + gbit;

	//printf("%X %d %016lX\n", pbyte, pbyte, tnum);


	return tnum;

}

// ファイルサイズを取得する関数
// ファイルサイズを取得する関数
// ファイルが読み込めない場合は-1を返す
int64_t getFileSize( char *target ){

        struct stat st;

        if( stat(target, &st) != 0 ){
                return -1;
        }

        // ファイルかどうかチェック
        if(( st.st_mode & S_IFMT) != S_IFREG){
                return -1;
        }

        return st.st_size;
}

// デバッグ用
// パケットをsize分dumpして表示する
void packetdump(unsigned char *p, int size){
        int i=0;

        printf("--------------------------------------------\n");
        printf("START:0x%X SIZE:%d\n", p, size);
        printf("00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0E 0F\n");
        printf("--------------------------------------------\n");
        for(i=0; i<size; i++){
                printf("%02X ", *(p+i) );
                if( (i+1)%15 == 0 ){ printf("\n"); }
        }
        printf("\nDUMP DONE\n");
}

// 16進数を10進数にそのまま見た目で変換する
// 例：0x20 → 20
uint8_t hex_to_digi(uint8_t x){
	uint8_t result;
	char str[3];
	sprintf(str, "%X", x);
	return(atoi(str));

}


// ---------------------------------------------------------------------------
//               時間関係ツール
// ---------------------------------------------------------------------------
// MJDをtm構造体に突っ込む関数
// 上のtime_to_tmとだいたいセットで使う
void mjd_to_tm( struct tm *tm, uint32_t mjd ){
        int a, b, jd, dtmp;
        jd = mjd + 2400001;
        a = (4*jd - 17918) / 146097;
        b = (3*a + 2) / 4;
        jd = 4 * (jd + b - 37);
        dtmp = 10 * (((jd - 237) % 1461) / 4) + 5;
        // 最終的な結果の書き出し
        tm->tm_year = jd / 1461 - 4712 - 1900;
        tm->tm_mon  = (dtmp / 306 + 2) % 12 ;
        tm->tm_mday  = (dtmp % 306) / 10 + 1;

}
// TSで使われている40バイトの日付時間情報からtm構造体に変換する
void time_to_tm( struct tm *tm, char *target ){
        char tmp[4];
        uint64_t boff;
        boff=0;
        uint16_t mjd;
        mjd       = getBit( target, &boff, 16);
        mjd_to_tm( tm, mjd);
        tm->tm_hour  = hex_to_digi( getBit( target, &boff, 8)  );
        tm->tm_min   = hex_to_digi( getBit( target, &boff, 8)  );
        tm->tm_sec   = hex_to_digi( getBit( target, &boff, 8)  );
        // デバッグ用
        //dump_tm(tm);
}

// tm構造体を文字列に直す
// '2020-01-01 00:00:00'形式(19文字)にする。返ってきたポインタは文字列として使える
char *tm_to_text(struct tm *tm){
	char *result = (char *)calloc(1,20);
        strftime(result, 20, "%Y-%m-%d %H:%M:%S", tm );
	return(result);
}
