#include <stdio.h>
#include <stdint.h>
#include <time.h>

// ADAPTATION_FIELD
typedef struct ADAPTATION_FIELD ADAPTATION_FIELD;
struct ADAPTATION_FIELD {
	int           adaptation_field_length;
	int           discontinuity_counter;
	int           random_access_indicator;
	int           elementary_stream_priority_indicator;
	int           pcr_flag;
	int           opcr_flag;
	int           splicing_point_flag;
	int           transport_private_data_flag;
	int           adaptation_field_extension_flag;
	int64_t       program_clock_reference;
	int64_t       original_program_clock_reference;
	int           splice_countdown;
	int           transport_private_data_length;
	int           adaptation_field_extension_length;
	int           ltw_flag;
	int           piecewise_rate_flag;
	int           seamless_splice_flag;
	int           ltw_valid_flag;
	int           ltw_offset;
	int           piecewise_rate;
	int           splice_type;
	int64_t       dts_next_au;
};


// SDT用構造体
typedef struct TS_HEAD TS_HEAD;
struct TS_HEAD {
        // ARIB仕様
        uint8_t         sync_byte;
        uint8_t         transport_error_indicator;
        uint8_t         payload_unit_start_indicator;
        uint8_t         transport_priority;
        uint16_t        pid;
        uint8_t         transport_scrambling_control;
        uint8_t         adaptation_field_control;
        uint8_t         continuity_counter;
	// 独自追加
        uint16_t        payload_start_pos;        // ペイロードのオフセット
        uint16_t        adaptation_field_length;  // アダプテーションフィールドの長さ
	ADAPTATION_FIELD adapt;
};


// TSパケット用構造体
typedef struct TS_PACKET TS_PACKET;
struct TS_PACKET {
	uint16_t	section_length;
	char		*packet;
	uint8_t		crc_check;
	uint8_t		working; // 読み込み中フラグ(0=読み込み前、1=読み込み中)
	uint64_t	loaded;  // 読み込み済みバイト数
};

// PAT関連
typedef struct TS_PAT_PIDS TS_PAT_PIDS;
struct TS_PAT_PIDS {
	uint16_t	program_number;
	uint16_t	network_PID;
	uint16_t	program_map_PID;
	TS_PAT_PIDS	*next;
};


typedef struct TS_PAT TS_PAT;
struct TS_PAT {
	uint8_t		table_id;
	uint8_t		section_syntax_indicator;
	uint16_t	section_length;
	uint16_t	transport_stream_id;
	uint8_t		version_number;
	uint8_t		current_next_indicator;
	uint8_t		section_number;
	uint8_t		last_section_number;

	// 独自追加
	uint16_t	num_of_pids;
	TS_PAT_PIDS	*pids;
	uint16_t	pmt_pid;
};

// descriptor用構造体
// タグとlengthの構造は基本的に同じなので、まずはそれを囲む
typedef struct TS_DESCRIPTOR TS_DESCRIPTOR;
struct TS_DESCRIPTOR {
	uint8_t		descriptor_tag;
	uint8_t		descriptor_length;
	char		*payload;
	TS_DESCRIPTOR	*next;
};

typedef struct DESC_COMPONENT DESC_COMPONENT;
struct DESC_COMPONENT {
	uint8_t		stream_content;
	uint8_t		component_type;
	uint8_t		component_tag;
	char		langcode[24];
	char		*text_char;
};

typedef struct TS_EIT_EVENTS TS_EIT_EVENTS;
struct TS_EIT_EVENTS{
	uint16_t	event_id;
	char		start_time[5];
	struct tm	start_time_tm;
	uint32_t	duration;
	uint64_t	duration_sec;
	uint8_t		running_status;
	uint8_t		free_CA_mode;
	uint16_t	descriotors_loop_length;
	TS_DESCRIPTOR   *desc;
	TS_EIT_EVENTS	*next;
};

typedef struct TS_EIT TS_EIT;
struct TS_EIT {
	uint8_t		table_id;
	uint8_t		section_syntax_indicator;
	uint16_t	section_length;
	uint16_t	service_id;
	uint8_t		version_number;
	uint8_t		current_next_indicator;
	uint8_t		section_number;
	uint8_t		last_section_number;
	uint16_t	transport_stream_id;
	uint16_t	original_network_id;
	uint8_t		segment_last_section_number;
	uint8_t		last_table_id;
	TS_EIT_EVENTS	*events;
};

// PACKET_LIST構造体
// PIDは13bitなので8192個用意すれば良い
typedef struct PACKET_LIST PACKET_LIST;
struct PACKET_LIST {
	uint8_t  enable;    // 有効・無効フラグ
	char     *payload;  // パケットの中身
	uint16_t size;      // パケット全体のサイズ
	uint16_t copybytes; // 現時点で何バイトコピーしたかを格納する
	uint8_t	 crc_check; // CRCのチェック状況
	uint8_t  loading;   // 読み込み中フラグ(0/1)
};

// TOT用構造体
typedef struct TS_TOT TS_TOT;
struct TS_TOT {
	uint8_t 	table_id;
	unsigned char	JST_time[5]; // plainなJST_time
	struct tm	tm;          // 最終的にはtm構造体にする

	// 以下は一時的にしか使わないかもしれない
	uint16_t	MJD;         // MJD
};

// 範囲指定する時に使える
typedef struct POS_SELECT POS_SELECT;
struct POS_SELECT {
	uint64_t  start;
	uint64_t  end;
};

// PMT関係構造体
typedef struct TS_PMT_STREAMS TS_PMT_STREAMS;
struct TS_PMT_STREAMS {
        uint8_t         stream_type;
        uint16_t        elementary_PID;
        uint16_t        ES_info_length;
        //void          *desc;
        uint32_t        CRC_32;
        TS_PMT_STREAMS  *next;
};

typedef struct TS_PMT TS_PMT;
struct TS_PMT {
        uint8_t         table_id;
        uint8_t         section_syntax_indicator;
        uint16_t        section_length;
        uint16_t        program_number;
        uint8_t         version_number;
        uint8_t         current_next_indicator;
        uint8_t         section_number;
        uint8_t         last_section_number;
        uint16_t        PCR_PID;
        uint16_t        program_info_length;
        //void          *desc;
        TS_PMT_STREAMS  *streams;

        //独自
        uint64_t        end_of_streams;
        uint64_t        num_of_streams;

};

typedef struct TS_PMT_CUTLIST TS_PMT_CUTLIST;
struct TS_PMT_CUTLIST {
        uint64_t        pos;
        uint16_t        pid[32];
        TS_PMT_CUTLIST  *next;
};

// ドロップカウンタ
typedef struct TS_DROP_COUNT TS_DROP_COUNT;
struct TS_DROP_COUNT{
	uint64_t      total;
	uint64_t      drop;
	uint64_t      error;
	uint64_t      scrambling;
	uint8_t       now_drop;
	uint8_t       now_error;
	uint8_t       now_scrambling;
	uint64_t      offset;
	uint64_t      repeat;
	uint8_t       last;   // ここが1だとこれ以上後ろに情報がない事を示す
	uint8_t       use;
	char          before_packet[188];
};

// 関数宣言
// util.c
uint64_t getBit(unsigned char *buf, uint64_t *pbit, uint64_t gbit);
void packetdump(unsigned char *p, int size);
uint64_t getFileSize( char *target );
uint8_t hex_to_digi(uint8_t x);
void time_to_tm( struct tm *tm, char *target );
void mjd_to_tm( struct tm *tm, uint32_t mjd );
char *tm_to_text(struct tm *tm);

// crc32.c
uint32_t crc32 (char *data, int len);
int crc32_check(unsigned char *packet);

// head.c
void st_head( TS_HEAD *ptr, unsigned char *buf );
void free_head( TS_HEAD *ptr );

// head_adapt.c
void extract_adaptation_field(ADAPTATION_FIELD *dst, unsigned char *data);

// packet.c
int st_packet( PACKET_LIST *packet_list, char *buf );
void free_packet_list(PACKET_LIST *packet_list);
void free_packet_list_all(PACKET_LIST *ptr);

// pat.c
void st_pat( TS_PAT *out, unsigned char *buf );
uint16_t get_pmt_pid( TS_PAT *ptr, uint16_t service_id );
void free_pat_pids( TS_PAT *out );
void dump_pat( TS_PAT *out );

// pmt.c
void st_pmt( TS_PMT *out, unsigned char *buf );
void dump_pmt(TS_PMT *pmt);

// tot.c
void st_tot( TS_TOT *out, unsigned char *buf );
void free_tot(TS_TOT *tot);

// eit.c
int st_eit( TS_EIT *out, unsigned char *buf );
void free_eit( TS_EIT *out );
void dump_eit( TS_EIT *out );

// dump.c
void dump_eit( TS_EIT *out );
void dump_pat( TS_PAT *out );

// drop_count.c
uint8_t drop_check_and_count( TS_DROP_COUNT *ts_drop_count, char *buf , uint64_t p_cnt );
uint16_t mark_last_drop_count( TS_DROP_COUNT *ptr );
void dump_drop_count( TS_DROP_COUNT *ts_drop_count );

// descriptor.c
DESC_COMPONENT *st_desc_component( unsigned char *buf );
void free_desc_component( DESC_COMPONENT *target );
