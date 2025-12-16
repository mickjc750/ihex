

	#include <stdint.h>
	#include <string.h>

	#include "ihex.h"

//********************************************************************************************************
// Local defines
//********************************************************************************************************

	#define MIN_VALID_LINE_LEN			((int)sizeof(":LLAAAATTCC")-1)
	#define MIN_VALID_BYTE_COUNT		((MIN_VALID_LINE_LEN-1)/2)


//********************************************************************************************************
// Public variables
//********************************************************************************************************

//********************************************************************************************************
// Private variables
//********************************************************************************************************

//********************************************************************************************************
// Private prototypes
//********************************************************************************************************

	static int write_chunk(ihex_ctx_t *ctx, const char *src, int src_len);

	static int ascii2raw(uint8_t *dst, const char *src, int byte_count);
	static int8_t hex_nibble(uint8_t c);

	static int process_line(ihex_ctx_t *ctx);
	static int process_rec_data(ihex_ctx_t *ctx);
	static int process_rec_eof(ihex_ctx_t *ctx);
	static int process_rec_ext_lin_add(ihex_ctx_t *ctx);

//********************************************************************************************************
// Public functions
//********************************************************************************************************

void ihex_init(ihex_ctx_t *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
}


int ihex_write(ihex_ctx_t *ctx, const char *src, int src_len)
{
	int retval;
	if(ctx->err != IHEX_OK)
		retval = ctx->err;
	else if(ctx->eof == false && ctx->data_size == 0)
		retval = write_chunk(ctx, src, src_len);
	else
		retval = 0;

	return retval;
}

const char* ihex_strerr(int err)
{
	const char *c;
	switch(err)
	{
		case IHEX_OK:						c = "OK"; break;
		case IHEX_ERR_HEX:					c = "HEX"; break;
		case IHEX_ERR_LEN:					c = "LEN"; break;
		case IHEX_ERR_CHECKSUM:				c = "CHECKSUM"; break;
		case IHEX_ERR_UNSUPPORTED_RECORD:	c = "UNSUPPORTED_RECORD"; break;
		case IHEX_ERR_EXT_ADDR:				c = "EXT_ADDR"; break;
		case IHEX_ERR_EOF:					c = "EOF"; break;
		case IHEX_ERR_START:				c = "START"; break;
		default : c = "";
	};
	return c;
}

void ihex_proceed(ihex_ctx_t *ctx)
{
	ctx->data_size = 0;
}

//********************************************************************************************************
// Private functions
//********************************************************************************************************

static int write_chunk(ihex_ctx_t *ctx, const char *src, int src_len)
{
	bool finished = false;
	int accepted = 0;
	char *dst = &ctx->text_buffer[ctx->text_size];
	char c;

	while((accepted < src_len) && !finished)
	{
		c = src[accepted++];
		if(c == '\n')
		{
			if(ctx->text_size)
			{
				ctx->err = process_line(ctx);
				finished = true;
			};
		}
		else if(c != '\r')
		{
			if(ctx->text_size == IHEX_LINE_LEN_MAX)
			{
				ctx->err = IHEX_ERR_LEN;
				finished = true;
			}
			else
			{
				*dst++ = c;
				ctx->text_size++;
			};
		};
	};

	return ctx->err == 0 ? accepted:ctx->err;
}

static int process_line(ihex_ctx_t *ctx)
{
	int err = IHEX_OK;
	int byte_count;
	int data_length;
	int record_type;
	int i;
	uint8_t checksum;

	if(ctx->text_buffer[0] != ':')
		err = IHEX_ERR_START;
	
	if(!err && (ctx->text_size % 2 == 0 || ctx->text_size < MIN_VALID_LINE_LEN))
		err = IHEX_ERR_LEN;

	if(!err)
	{
		byte_count = (ctx->text_size-1)/2;
		ctx->text_size = 0;
		err = ascii2raw(ctx->data_buffer, &ctx->text_buffer[1], byte_count);
	};

	if(!err)
	{
		data_length = ctx->data_buffer[0];
		
		if(data_length != byte_count - MIN_VALID_BYTE_COUNT)
			err = IHEX_ERR_LEN;
	};

	if(!err)
	{
		checksum = 0;
		i = byte_count;
		while(i--)
			checksum += ctx->data_buffer[i];
		if(checksum != 0x00)
			err = IHEX_ERR_CHECKSUM;
	};

	if(!err)
	{
		record_type = ctx->data_buffer[3];
		switch(record_type)
		{
			case 0x00: err = process_rec_data(ctx); break;
			case 0x01: err = process_rec_eof(ctx); break;
			case 0x04: err = process_rec_ext_lin_add(ctx); break;
			case 0x05: break;
			default: err = IHEX_ERR_UNSUPPORTED_RECORD; break;
		};
	};
	
	return err;
}

static int process_rec_data(ihex_ctx_t *ctx)
{
	ctx->data_address = (ctx->data_buffer[1] << 8) + ctx->data_buffer[2];
	ctx->data_address |= ctx->ext_lin_addr;
	ctx->data_size = ctx->data_buffer[0];
	return IHEX_OK;
}

static int process_rec_eof(ihex_ctx_t *ctx)
{
	static const uint8_t expected_bytes[3] = {0x00, 0x00, 0x00};
	int err = (memcmp(expected_bytes, ctx->data_buffer, sizeof(expected_bytes))==0) ? IHEX_OK:IHEX_ERR_EOF;

	if(!err)
		ctx->eof = true;

	return err;
}

static int process_rec_ext_lin_add(ihex_ctx_t *ctx)
{
	static const uint8_t expected_bytes[3] = {0x02, 0x00, 0x00};
	int err = (memcmp(expected_bytes, ctx->data_buffer, sizeof(expected_bytes))==0) ? IHEX_OK:IHEX_ERR_EXT_ADDR;

	if(!err)
		ctx->ext_lin_addr = ((uint32_t)ctx->data_buffer[4] << 24) | ((uint32_t)ctx->data_buffer[5] << 16);

	return err;
}


static int ascii2raw(uint8_t *dst, const char *src, int byte_count)
{
	int err = IHEX_OK;
	int8_t h;

	while(byte_count-- && err == IHEX_OK)
	{
		h = hex_nibble(*src++);
		if(h == -1)
			err = IHEX_ERR_HEX;
		*dst =  h << 4;

		h = hex_nibble(*src++);
		if(h == -1)
			err = IHEX_ERR_HEX;
		*dst |=  h;
		dst++;
	};

	return err;
}

static int8_t hex_nibble(uint8_t c)
{
    uint8_t d = c - '0';
    if (d <= 9) return (int8_t)d;

    c |= 0x20;
    d = c - 'a';
    if (d <= 5) return (int8_t)(d + 10);

    return -1;
}