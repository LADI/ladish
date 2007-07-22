#include <string.h>
#include <stdio.h>
#include <lo/lo.h>
#include "lv2_osc.h"

int
main()
{
	lo_message lo_msg = lo_message_new();
	//lo_message_add_symbol(lo_msg, "a_sym");
	lo_message_add_string(lo_msg, "Hello World");
	lo_message_add_char(lo_msg, 'a');
	lo_message_add_int32(lo_msg, 1234);
	lo_message_add_float(lo_msg, 0.1234);
	lo_message_add_int64(lo_msg, 5678);
	lo_message_add_double(lo_msg, 0.5678);


	/*unsigned char blob_data[] = { 0,1,2,3,4,5,6,7,8,9 };
	lo_blob blob = lo_blob_new(10, blob_data);
	lo_message_add_blob(lo_msg, blob);*/

	size_t raw_msg_size = 0;
	void* raw_msg = lo_message_serialise(lo_msg, "/foo/bar", NULL, &raw_msg_size);

	LV2OSCBuffer* buf = lv2_osc_buffer_new(1024);
	int ret = lv2_osc_buffer_append_message(buf, 0.0, raw_msg_size, raw_msg);
	if (ret)
		fprintf(stderr, "Message append failed: %s", strerror(ret));

	return 0;
}
