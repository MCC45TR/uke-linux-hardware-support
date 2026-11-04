/* SPDX-License-Identifier: MIT
 * Exercise the actual upstream C encoder, including its failed Finish paths.
 * This fixture does not open a TEE or run on the tablet.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <qcbor/qcbor.h>

static unsigned finish_calls;
static unsigned finish_mode;

static QCBORError test_finish(QCBOREncodeContext *context, UsefulBufC *output)
{
    ++finish_calls;
    if (finish_mode == 1)
        return QCBOR_ERR_TOO_MANY_CLOSES;
    if (finish_mode == 2) {
        if (finish_calls == 1)
            return QCBOR_ERR_BUFFER_TOO_SMALL;
        return QCBOR_ERR_TOO_MANY_CLOSES;
    }
    if (finish_mode == 3)
        return QCBOR_SUCCESS; /* Invalid success with no output. */
    return QCBOREncode_Finish(context, output);
}

#define USE_QCBOR
#define QCBOREncode_Finish test_finish
#include "objects/credentials_obj.c"
#undef QCBOREncode_Finish

int main(void)
{
    struct qcomtee_ubuf output = {0};
    assert(credentials_init(&output) == 0);
    assert(finish_calls == 1 && output.addr != NULL && output.size > 4);
    /* The real QCBOR path emitted a two-field definite map. */
    assert(((const unsigned char *)output.addr)[0] == 0xa2);
    free(output.addr);

    for (finish_mode = 1; finish_mode <= 3; ++finish_mode) {
        output.addr = (void *)(uintptr_t)1;
        output.size = SIZE_MAX;
        finish_calls = 0;
        assert(credentials_init(&output) != 0);
        assert(output.addr == NULL && output.size == 0);
        assert(finish_calls == (finish_mode == 2 ? 2u : 1u));
    }
    puts("actual QCOMTEE credential encoder: success, error, retry/error and empty success passed");
    return 0;
}
