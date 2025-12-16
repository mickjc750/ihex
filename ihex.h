#ifndef _IHEX_H_
#define _IHEX_H_

	#include <stdint.h>
	#include <stdbool.h>

//********************************************************************************************************
// Public defines
//********************************************************************************************************

	#ifndef IHEX_LINE_LEN_MAX
		#define IHEX_LINE_LEN_MAX	521
		#warning "Using default IHEX_LINE_LEN_MAX of 521, define IHEX_LINE_LEN_MAX to remove this warning"
	#endif


//	Errors are latching, and prevent further decode until the context is re-initialised with ihex_init()
	#define IHEX_OK						 0
	#define IHEX_ERR_HEX				-1
	#define IHEX_ERR_LEN				-2
	#define IHEX_ERR_CHECKSUM			-3
	#define IHEX_ERR_UNSUPPORTED_RECORD	-4
	#define IHEX_ERR_EXT_ADDR			-5
	#define IHEX_ERR_EOF				-6
	#define IHEX_ERR_START				-7

//********************************************************************************************************
// Public variables
//********************************************************************************************************

	typedef struct ihex_ctx_t
	{
//		Host use:
		int data_size;			//	data size, non0 when a data record is ready
		uint32_t data_address;	//  (includes extended linear address from 0x04 records)
		bool eof;				//	indicates end of file record was parsed.
		int err;				//	parsing error, also returned by ihex_write if non0
		union
		{
			uint8_t data_buffer[(IHEX_LINE_LEN_MAX-11)/2];	// host reads data from data records here
//		Internal use:
			char text_buffer[IHEX_LINE_LEN_MAX];
		};
		int text_size;
		uint32_t ext_lin_addr;
	} ihex_ctx_t;

//********************************************************************************************************
// Public prototypes
//********************************************************************************************************

	void ihex_init(ihex_ctx_t *ctx);

//	Attempt to pass src_len characters to the parser.
//	The parser will accept characters up to and including LF (CR characters are ignored).
//	The number of accepted characters is returned, or < 0 if an error has occurred.
//	If a data record becomes available, ctx.data_size will be non0
//	When a data record is available, parsing will not proceed until ihex_proceed() is called
//	 and the return value will be 0 or < 0 if an error has occured.
	int ihex_write(ihex_ctx_t *ctx, const char *src, int src_len);

//	Provide a C string describing an IHEX_ERR_# code
	const char* ihex_strerr(int err);

//	Once a data record has been read, call this to continue parsing. 
	void ihex_proceed(ihex_ctx_t *ctx);
#endif
