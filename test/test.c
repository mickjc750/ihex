	#include <stdint.h>
	#include <string.h>

	#include "greatest.h"
	#include "ihex.h"

//********************************************************************************************************
// Configurable defines
//********************************************************************************************************

//********************************************************************************************************
// Local defines
//********************************************************************************************************

	GREATEST_MAIN_DEFS();

//********************************************************************************************************
// Public variables
//********************************************************************************************************

//********************************************************************************************************
// Private variables
//********************************************************************************************************

//********************************************************************************************************
// Private prototypes
//********************************************************************************************************

	SUITE(ihex_suite);
	TEST test_empty_input_accepts_all(void);
	TEST test_data_record_basic(void);
	TEST test_crlf_is_accepted(void);
	TEST test_ext_linear_address_applies_to_next_data(void);
	TEST test_bad_checksum_latches_error(void);
	TEST test_eof_blocks_further_parsing(void);

	static int feed_bytes(ihex_ctx_t *ctx, const char *s);

//********************************************************************************************************
// Public functions
//********************************************************************************************************

int main(int argc, char **argv)
{
	GREATEST_MAIN_BEGIN();
	RUN_SUITE(ihex_suite);
	GREATEST_MAIN_END();
}

//********************************************************************************************************
// Suites
//********************************************************************************************************

SUITE(ihex_suite)
{
	RUN_TEST(test_empty_input_accepts_all);
	RUN_TEST(test_data_record_basic);
	RUN_TEST(test_crlf_is_accepted);
	RUN_TEST(test_ext_linear_address_applies_to_next_data);
	RUN_TEST(test_bad_checksum_latches_error);
	RUN_TEST(test_eof_blocks_further_parsing);
}

//********************************************************************************************************
// Tests
//********************************************************************************************************

TEST test_empty_input_accepts_all(void)
{
	ihex_ctx_t ctx;
	ihex_init(&ctx);

	int r = ihex_write(&ctx, "", 0);
	ASSERT_EQ(0, r);
	ASSERT_EQ(0, ctx.err);
	ASSERT_EQ(0, ctx.data_size);
	ASSERT_EQ(false, ctx.eof);
	PASS();
}

TEST test_data_record_basic(void)
{
	const char *line = ":100000000102030405060708090A0B0C0D0E0F1068\n";
	const uint8_t test_data[0x10] = {
		0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
		0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10
	};

	ihex_ctx_t ctx;
	ihex_init(&ctx);

	int a = feed_bytes(&ctx, line);
	ASSERT(a >= 0);

	ASSERT_EQ(0, ctx.err);
	ASSERT_EQ(false, ctx.eof);
	ASSERT_EQ(0x00000000u, ctx.data_address);
	ASSERT_EQ(0x10, ctx.data_size);
	ASSERT_MEM_EQ(test_data, &ctx.data_buffer[4], 0x10);

	// Should block until proceed
	int r = ihex_write(&ctx, line, (int)strlen(line));
	ASSERT_EQ(0, r);

	ihex_proceed(&ctx);
	ASSERT_EQ(0, ctx.data_size);

	PASS();
}

TEST test_crlf_is_accepted(void)
{
	const char *line = ":0100000001FE\r\n"; // LL=1, addr 0, type 0, data=01, checksum=FE

	ihex_ctx_t ctx;
	ihex_init(&ctx);

	int a = feed_bytes(&ctx, line);
	ASSERT(a >= 0);

	ASSERT_EQ(0, ctx.err);
	ASSERT_EQ(false, ctx.eof);
	ASSERT_EQ(0u, ctx.data_address);
	ASSERT_EQ(1, ctx.data_size);
	ASSERT_EQ(0x01, ctx.data_buffer[4]);
	PASS();
}

TEST test_ext_linear_address_applies_to_next_data(void)
{
	const char *ela  = ":020000040800F2\n";
	const char *data = ":01000000AA55\n"; // at offset 0, data=AA (checksum 0x55)

	ihex_ctx_t ctx;
	ihex_init(&ctx);

	// ELA record should not surface data
	int a = feed_bytes(&ctx, ela);
	ASSERT(a >= 0);

	ASSERT_EQ(0, ctx.err);
	ASSERT_EQ(0, ctx.data_size);
	ASSERT_EQ(false, ctx.eof);

	// Now feed a data record, should come out at 0x08000000
	a = feed_bytes(&ctx, data);
	ASSERT(a >= 0);

	ASSERT_EQ(0, ctx.err);
	ASSERT_EQ(0x08000000u, ctx.data_address);
	ASSERT_EQ(1, ctx.data_size);
	ASSERT_EQ(0xAA, ctx.data_buffer[4]);
	PASS();
}

TEST test_bad_checksum_latches_error(void)
{
	const char *bad = ":0100000001FF\n";

	ihex_ctx_t ctx;
	ihex_init(&ctx);

	int a = feed_bytes(&ctx, bad);
	(void)a;

	ASSERT_EQ(IHEX_ERR_CHECKSUM, ctx.err);

	// Further writes should return the latched error
	int r2 = ihex_write(&ctx, bad, (int)strlen(bad));
	ASSERT_EQ(IHEX_ERR_CHECKSUM, r2);
	PASS();
}

TEST test_eof_blocks_further_parsing(void)
{
	const char *eof  = ":00000001FF\n";
	const char *data = ":0100000001FE\n";

	ihex_ctx_t ctx;
	ihex_init(&ctx);

	int a = feed_bytes(&ctx, eof);
	ASSERT(a >= 0);

	ASSERT_EQ(0, ctx.err);
	ASSERT_EQ(true, ctx.eof);
	ASSERT_EQ(0, ctx.data_size);

	// Your contract: when EOF set, further writes return 0 (or error)
	int r = ihex_write(&ctx, data, (int)strlen(data));
	ASSERT_EQ(0, r);
	PASS();
}

//********************************************************************************************************
// Private functions
//********************************************************************************************************

// Helper: feed a C string into ihex_write() in chunks of 1 byte (stream-like)
// Returns total accepted bytes, or <0 on parser error, or -999 on unexpected accept count.
static int feed_bytes(ihex_ctx_t *ctx, const char *s)
{
	int accepted_total = 0;

	for(int i = 0; s[i] != '\0'; i++)
	{
		int r = ihex_write(ctx, &s[i], 1);
		if(r < 0)
			return r;
		if(r != 1)
			return -999;
		accepted_total += r;

		if(ctx->data_size != 0 || ctx->eof || ctx->err != 0)
			break;
	};

	return accepted_total;
}
