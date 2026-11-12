/* SPDX-License-Identifier: MIT
 * COPR-only test of the actual unmodified Fedora shared library. No TEE.
 * C is required for the upstream C11 atomic interface. No private values logged.
 */
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <cbor.h>
#include <qcomtee_object_types.h>

static uint64_t milliseconds(void)
{
    struct timeval now;
    assert(gettimeofday(&now, NULL) == 0);
    return (uint64_t)now.tv_sec * 1000 + (uint64_t)now.tv_usec / 1000;
}

int main(void)
{
    /* Unexported callback: a public root shell needs no secure-world handle. */
    struct qcomtee_object root = {0}, *object = NULL;
    root.object_type = QCOMTEE_OBJECT_TYPE_ROOT;
    atomic_init(&root.refs, 1);
    const uint64_t before = milliseconds();
    assert(qcomtee_object_credentials_init(&root, &object) == 0);
    const uint64_t after = milliseconds();
    assert(object && object->root == &root && atomic_load(&root.refs) == 2);
    struct qcomtee_param length = {.attr = QCOMTEE_UBUF_OUTPUT};
    assert(object->ops->dispatch(object, 0, &length, 1) == QCOMTEE_OK);
    assert(length.ubuf.size == sizeof(size_t));
    const size_t size = *(size_t *)length.ubuf.addr;
    assert(size > 4 && size <= 4096);
    uint64_t offset = 0;
    struct qcomtee_param parameters[2] = {
        {.attr = QCOMTEE_UBUF_INPUT, .ubuf = {&offset, sizeof(offset)}},
        {.attr = QCOMTEE_UBUF_OUTPUT, .ubuf = {NULL, size}},
    };
    assert(object->ops->dispatch(object, 1, parameters, 2) == QCOMTEE_OK);
    assert(parameters[1].ubuf.size == size);
    struct cbor_load_result result;
    cbor_item_t *map = cbor_load(parameters[1].ubuf.addr, size, &result);
    assert(map && result.error.code == CBOR_ERR_NONE && result.read == size);
    assert(cbor_isa_map(map) && cbor_map_size(map) == 2);
    struct cbor_pair *pairs = cbor_map_handle(map);
    int uid_seen = 0, time_seen = 0;
    for (size_t i = 0; i < 2; ++i) {
        assert(cbor_isa_uint(pairs[i].key) && cbor_isa_uint(pairs[i].value));
        const uint64_t key = cbor_get_int(pairs[i].key);
        const uint64_t value = cbor_get_int(pairs[i].value);
        if (key == 1) { assert(value == getuid()); ++uid_seen; }
        else { assert(key == 6 && value >= before && value <= after); ++time_seen; }
    }
    assert(uid_seen == 1 && time_seen == 1);
    cbor_decref(&map);
    offset = size;
    assert(object->ops->dispatch(object, 1, parameters, 2) == QCOMTEE_ERROR_INVALID);
    assert(object->ops->dispatch(object, 0, &length, 0) == QCOMTEE_ERROR_INVALID);
    assert(object->ops->dispatch(object, 2, parameters, 2) == QCOMTEE_ERROR_INVALID);
    /* Never exported: release directly, without a kernel namespace operation. */
    object->ops->release(object);
    assert(atomic_fetch_sub(&root.refs, 1) == 2);
    return 0;
}
