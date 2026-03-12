/**
 * NA plugin template: "abc"
 *
 * This file implements a skeleton NA plugin.  Every callback that appears in
 * struct na_class_ops (defined in na.h) is listed here with a brief comment
 * explaining what the callback is expected to do.  Stubs that are required for
 * a minimally functional plugin return an error; optional callbacks that can
 * safely be left NULL are set to NULL in the ops table at the bottom.
 *
 * To turn this into a real plugin:
 *   1. Replace "abc" / "myprotocol" with your class / protocol names.
 *   2. Define private data structures for addresses, memory handles, op IDs,
 *      and plugin-level class state.  Attach class state via
 *      na_class->plugin_class (set during initialize, freed during finalize).
 *   3. Implement each callback according to the comments below.
 *
 * Build:
 *   mkdir build && cd build
 *   cmake .. -Dmercury_DIR=<mercury-install>/lib/cmake/mercury
 *   make
 *
 * Use (dynamic plugin):
 *   NA_PLUGIN_PATH=/path/to/libna_plugin_abc.so-dir  <your application>
 *
 * Use (runtime registration — no dlopen needed):
 *   #include <na.h>
 *   extern const struct na_class_ops na_abc_class_ops_g;
 *   NA_Register_plugin(&na_abc_class_ops_g);
 */

#include <na_plugin.h>

#include <string.h>

/*
 * The NA_PLUGIN macro marks the symbol for export from the shared library so
 * that NA's dynamic plugin loader can find it via dlsym().  The symbol name
 * must follow the pattern  na_<name>_class_ops_g  where <name> matches the
 * file name pattern libna_plugin_<name>.so.
 */

/* ------------------------------------------------------------------ */
/*  Forward declarations of all callbacks                              */
/* ------------------------------------------------------------------ */

/* Returns protocol info entries for protocols this plugin supports.
 * Called by NA_Get_protocol_info().  Allocate entries with
 * na_protocol_info_alloc() and chain them via the ->next pointer. */
static na_return_t
na_abc_get_protocol_info(
    const struct na_info *na_info, struct na_protocol_info **na_protocol_info_p);

/* Return true if this plugin can handle the given protocol name.
 * For example, if your plugin handles "myprotocol", return
 * strcmp(protocol_name, "myprotocol") == 0.
 * REQUIRED — must not be NULL. */
static bool
na_abc_check_protocol(const char *protocol_name);

/* Allocate and initialize plugin-private state.  Store it in
 * na_class->plugin_class.  Parse connection details from na_info.
 * If listen is true, set up to accept incoming connections.
 * REQUIRED — must not be NULL. */
static na_return_t
na_abc_initialize(
    na_class_t *na_class, const struct na_info *na_info, bool listen);

/* Tear down plugin-private state that was set up in initialize().
 * Free na_class->plugin_class.
 * REQUIRED — must not be NULL. */
static na_return_t
na_abc_finalize(na_class_t *na_class);

/* Clean up any global/static resources (temp files, shared memory, etc.)
 * that would otherwise survive process termination.  Called from
 * NA_Cleanup().  Optional — may be NULL. */
static void
na_abc_cleanup(void);

/* Allocate per-context state.  Store it in *plugin_context_p.
 * id is a caller-chosen context identifier (e.g. for multi-context). */
static na_return_t
na_abc_context_create(na_class_t *na_class, na_context_t *na_context,
    void **plugin_context_p, uint8_t id);

/* Free per-context state allocated in context_create(). */
static na_return_t
na_abc_context_destroy(na_class_t *na_class, void *plugin_context);

/* Allocate an operation ID.  flags is reserved for future use.
 * The returned op ID is passed into send/recv/put/get calls and
 * is used to track and cancel in-flight operations.
 * REQUIRED. */
static na_op_id_t *
na_abc_op_create(na_class_t *na_class, unsigned long flags);

/* Free an operation ID allocated by op_create().
 * REQUIRED. */
static void
na_abc_op_destroy(na_class_t *na_class, na_op_id_t *op_id);

/* Resolve a peer address from a string name (e.g. "host:port").
 * Allocate an na_addr_t and return it via *addr_p.
 * REQUIRED. */
static na_return_t
na_abc_addr_lookup(
    na_class_t *na_class, const char *name, na_addr_t **addr_p);

/* Free an address returned by addr_lookup, addr_self, addr_dup, or
 * addr_deserialize.
 * REQUIRED. */
static void
na_abc_addr_free(na_class_t *na_class, na_addr_t *addr);

/* Return the address of this process/endpoint via *addr_p.
 * REQUIRED. */
static na_return_t
na_abc_addr_self(na_class_t *na_class, na_addr_t **addr_p);

/* Duplicate an address (deep copy).
 * REQUIRED. */
static na_return_t
na_abc_addr_dup(
    na_class_t *na_class, na_addr_t *addr, na_addr_t **new_addr_p);

/* Return true if addr1 and addr2 refer to the same endpoint.
 * REQUIRED. */
static bool
na_abc_addr_cmp(na_class_t *na_class, na_addr_t *addr1, na_addr_t *addr2);

/* Return true if addr is the local (self) address.
 * REQUIRED. */
static bool
na_abc_addr_is_self(na_class_t *na_class, na_addr_t *addr);

/* Convert an address to a human-readable string.  Write at most *buf_size
 * bytes into buf and update *buf_size to the total size needed (including
 * the terminating NUL).  If buf is NULL, just return the required size.
 * REQUIRED. */
static na_return_t
na_abc_addr_to_string(
    na_class_t *na_class, char *buf, size_t *buf_size, na_addr_t *addr);

/* Return the number of bytes needed to serialize addr. */
static size_t
na_abc_addr_get_serialize_size(na_class_t *na_class, na_addr_t *addr);

/* Serialize addr into buf (buf_size bytes available). */
static na_return_t
na_abc_addr_serialize(
    na_class_t *na_class, void *buf, size_t buf_size, na_addr_t *addr);

/* Deserialize an address from buf into *addr_p. */
static na_return_t
na_abc_addr_deserialize(na_class_t *na_class, na_addr_t **addr_p,
    const void *buf, size_t buf_size, uint64_t flags);

/* Return the maximum payload size for unexpected (unmatched) messages. */
static size_t
na_abc_msg_get_max_unexpected_size(const na_class_t *na_class);

/* Return the maximum payload size for expected (matched) messages. */
static size_t
na_abc_msg_get_max_expected_size(const na_class_t *na_class);

/* Return the maximum tag value this plugin supports. */
static na_tag_t
na_abc_msg_get_max_tag(const na_class_t *na_class);

/* Post a non-blocking unexpected (unmatched) send.  When the operation
 * completes, call na_cb_completion_add() with the completion data so
 * that NA_Trigger() can invoke callback(arg).
 * REQUIRED. */
static na_return_t
na_abc_msg_send_unexpected(na_class_t *na_class, na_context_t *context,
    na_cb_t callback, void *arg, const void *buf, size_t buf_size,
    void *plugin_data, na_addr_t *dest_addr, uint8_t dest_id, na_tag_t tag,
    na_op_id_t *op_id);

/* Post a non-blocking unexpected receive.  When a matching message
 * arrives, complete the operation via na_cb_completion_add().
 * REQUIRED. */
static na_return_t
na_abc_msg_recv_unexpected(na_class_t *na_class, na_context_t *context,
    na_cb_t callback, void *arg, void *buf, size_t buf_size, void *plugin_data,
    na_op_id_t *op_id);

/* Post a non-blocking expected (matched) send.
 * REQUIRED. */
static na_return_t
na_abc_msg_send_expected(na_class_t *na_class, na_context_t *context,
    na_cb_t callback, void *arg, const void *buf, size_t buf_size,
    void *plugin_data, na_addr_t *dest_addr, uint8_t dest_id, na_tag_t tag,
    na_op_id_t *op_id);

/* Post a non-blocking expected receive.
 * REQUIRED. */
static na_return_t
na_abc_msg_recv_expected(na_class_t *na_class, na_context_t *context,
    na_cb_t callback, void *arg, void *buf, size_t buf_size, void *plugin_data,
    na_addr_t *source_addr, uint8_t source_id, na_tag_t tag,
    na_op_id_t *op_id);

/* Create a memory handle that describes a contiguous region for RMA.
 * flags is a bitmask of NA_MEM_READ_ONLY / NA_MEM_WRITE_ONLY / NA_MEM_READWRITE.
 * REQUIRED. */
static na_return_t
na_abc_mem_handle_create(na_class_t *na_class, void *buf, size_t buf_size,
    unsigned long flags, na_mem_handle_t **mem_handle_p);

/* Free a memory handle. */
static void
na_abc_mem_handle_free(na_class_t *na_class, na_mem_handle_t *mem_handle);

/* Return the serialized size of a memory handle. */
static size_t
na_abc_mem_handle_get_serialize_size(
    na_class_t *na_class, na_mem_handle_t *mem_handle);

/* Serialize a memory handle into buf. */
static na_return_t
na_abc_mem_handle_serialize(
    na_class_t *na_class, void *buf, size_t buf_size, na_mem_handle_t *mem_handle);

/* Deserialize a memory handle from buf. */
static na_return_t
na_abc_mem_handle_deserialize(na_class_t *na_class,
    na_mem_handle_t **mem_handle_p, const void *buf, size_t buf_size);

/* Initiate a non-blocking RMA put (write to remote memory).
 * REQUIRED for RMA support. */
static na_return_t
na_abc_put(na_class_t *na_class, na_context_t *context, na_cb_t callback,
    void *arg, na_mem_handle_t *local_mem_handle, na_offset_t local_offset,
    na_mem_handle_t *remote_mem_handle, na_offset_t remote_offset,
    size_t length, na_addr_t *remote_addr, uint8_t remote_id,
    na_op_id_t *op_id);

/* Initiate a non-blocking RMA get (read from remote memory).
 * REQUIRED for RMA support. */
static na_return_t
na_abc_get(na_class_t *na_class, na_context_t *context, na_cb_t callback,
    void *arg, na_mem_handle_t *local_mem_handle, na_offset_t local_offset,
    na_mem_handle_t *remote_mem_handle, na_offset_t remote_offset,
    size_t length, na_addr_t *remote_addr, uint8_t remote_id,
    na_op_id_t *op_id);

/* Return a file descriptor that becomes readable when progress can be made.
 * Return -1 if not supported (caller will fall back to busy-polling).
 * Optional — may be NULL. */
static int
na_abc_poll_get_fd(na_class_t *na_class, na_context_t *context);

/* Return true if the caller should enter an OS-level wait (epoll/poll/select)
 * on the fd returned by poll_get_fd().  Return false if there is already
 * work pending.
 * Optional — may be NULL. */
static bool
na_abc_poll_try_wait(na_class_t *na_class, na_context_t *context);

/* Poll for completed operations.  Store the number of completions in *count_p.
 * Return NA_SUCCESS if completions were found, NA_TIMEOUT if none.
 * REQUIRED. */
static na_return_t
na_abc_poll(
    na_class_t *na_class, na_context_t *context, unsigned int *count_p);

/* Cancel an in-flight operation.
 * REQUIRED. */
static na_return_t
na_abc_cancel(
    na_class_t *na_class, na_context_t *context, na_op_id_t *op_id);

/* ------------------------------------------------------------------ */
/*  Callback implementations (stubs)                                   */
/* ------------------------------------------------------------------ */

static na_return_t
na_abc_get_protocol_info(const struct na_info *na_info NA_UNUSED,
    struct na_protocol_info **na_protocol_info_p)
{
    /* TODO: Allocate one or more protocol info entries using
     * na_protocol_info_alloc("abc", "myprotocol", "device0") and chain
     * them via ->next.  Return the head of the list via *na_protocol_info_p.
     * If na_info is non-NULL, use it to filter results (e.g. matching a
     * requested protocol name). */
    *na_protocol_info_p = NULL;
    return NA_SUCCESS;
}

static bool
na_abc_check_protocol(const char *protocol_name)
{
    /* TODO: Return true if protocol_name matches a protocol this plugin
     * handles (e.g. "myprotocol"). */
    return (strcmp(protocol_name, "myprotocol") == 0);
}

static na_return_t
na_abc_initialize(
    na_class_t *na_class NA_UNUSED,
    const struct na_info *na_info NA_UNUSED,
    bool listen NA_UNUSED)
{
    /* TODO: Allocate plugin-private class state and store in
     * na_class->plugin_class.  Parse connection info from na_info
     * (protocol_name, host_name, na_init_info).  If listen is true,
     * start accepting incoming connections. */
    return NA_PROTOCOL_ERROR; /* not yet implemented */
}

static na_return_t
na_abc_finalize(na_class_t *na_class NA_UNUSED)
{
    /* TODO: Shut down transport, release resources, free
     * na_class->plugin_class. */
    return NA_SUCCESS;
}

static void
na_abc_cleanup(void)
{
    /* TODO: Remove any global resources (temp files, shared memory segments)
     * that would persist after the process exits.  Called from NA_Cleanup(). */
}

static na_return_t
na_abc_context_create(na_class_t *na_class NA_UNUSED,
    na_context_t *na_context NA_UNUSED, void **plugin_context_p,
    uint8_t id NA_UNUSED)
{
    /* TODO: Allocate per-context state (e.g. completion queues, poll sets)
     * and return it via *plugin_context_p.  The context id can be used to
     * bind to specific hardware resources. */
    *plugin_context_p = NULL;
    return NA_SUCCESS;
}

static na_return_t
na_abc_context_destroy(
    na_class_t *na_class NA_UNUSED, void *plugin_context NA_UNUSED)
{
    /* TODO: Free per-context state allocated in context_create(). */
    return NA_SUCCESS;
}

static na_op_id_t *
na_abc_op_create(na_class_t *na_class NA_UNUSED, unsigned long flags NA_UNUSED)
{
    /* TODO: Allocate and return an operation ID structure.  This is the
     * handle that tracks an in-flight send/recv/put/get/cancel. */
    return NULL;
}

static void
na_abc_op_destroy(na_class_t *na_class NA_UNUSED, na_op_id_t *op_id NA_UNUSED)
{
    /* TODO: Free the operation ID allocated by op_create(). */
}

static na_return_t
na_abc_addr_lookup(na_class_t *na_class NA_UNUSED, const char *name NA_UNUSED,
    na_addr_t **addr_p NA_UNUSED)
{
    /* TODO: Resolve a string address (e.g. "host:port") to an internal
     * address structure.  Allocate and return via *addr_p. */
    return NA_PROTOCOL_ERROR;
}

static void
na_abc_addr_free(na_class_t *na_class NA_UNUSED, na_addr_t *addr NA_UNUSED)
{
    /* TODO: Free an address allocated by addr_lookup/addr_self/addr_dup/
     * addr_deserialize. */
}

static na_return_t
na_abc_addr_self(
    na_class_t *na_class NA_UNUSED, na_addr_t **addr_p NA_UNUSED)
{
    /* TODO: Allocate and return the local endpoint address. */
    return NA_PROTOCOL_ERROR;
}

static na_return_t
na_abc_addr_dup(na_class_t *na_class NA_UNUSED, na_addr_t *addr NA_UNUSED,
    na_addr_t **new_addr_p NA_UNUSED)
{
    /* TODO: Deep-copy addr into a newly allocated address. */
    return NA_PROTOCOL_ERROR;
}

static bool
na_abc_addr_cmp(na_class_t *na_class NA_UNUSED, na_addr_t *addr1 NA_UNUSED,
    na_addr_t *addr2 NA_UNUSED)
{
    /* TODO: Return true if addr1 and addr2 point to the same endpoint. */
    return false;
}

static bool
na_abc_addr_is_self(
    na_class_t *na_class NA_UNUSED, na_addr_t *addr NA_UNUSED)
{
    /* TODO: Return true if addr is the local address. */
    return false;
}

static na_return_t
na_abc_addr_to_string(na_class_t *na_class NA_UNUSED, char *buf NA_UNUSED,
    size_t *buf_size NA_UNUSED, na_addr_t *addr NA_UNUSED)
{
    /* TODO: Write a human-readable representation of addr into buf.
     * Update *buf_size to the total bytes needed (including NUL).
     * If buf is NULL, just report the required size. */
    return NA_PROTOCOL_ERROR;
}

static size_t
na_abc_addr_get_serialize_size(
    na_class_t *na_class NA_UNUSED, na_addr_t *addr NA_UNUSED)
{
    /* TODO: Return the number of bytes needed to serialize addr. */
    return 0;
}

static na_return_t
na_abc_addr_serialize(na_class_t *na_class NA_UNUSED, void *buf NA_UNUSED,
    size_t buf_size NA_UNUSED, na_addr_t *addr NA_UNUSED)
{
    /* TODO: Serialize addr into buf (buf_size bytes available). */
    return NA_PROTOCOL_ERROR;
}

static na_return_t
na_abc_addr_deserialize(na_class_t *na_class NA_UNUSED,
    na_addr_t **addr_p NA_UNUSED, const void *buf NA_UNUSED,
    size_t buf_size NA_UNUSED, uint64_t flags NA_UNUSED)
{
    /* TODO: Reconstruct an address from buf and return via *addr_p. */
    return NA_PROTOCOL_ERROR;
}

static size_t
na_abc_msg_get_max_unexpected_size(const na_class_t *na_class NA_UNUSED)
{
    /* TODO: Return the maximum payload size for an unexpected message.
     * This should reflect transport limits. */
    return 0;
}

static size_t
na_abc_msg_get_max_expected_size(const na_class_t *na_class NA_UNUSED)
{
    /* TODO: Return the maximum payload size for an expected message. */
    return 0;
}

static na_tag_t
na_abc_msg_get_max_tag(const na_class_t *na_class NA_UNUSED)
{
    /* TODO: Return the largest tag value that can be used for matching.
     * Tags are unsigned integers; typical transports support at least 2^30. */
    return 0;
}

static na_return_t
na_abc_msg_send_unexpected(na_class_t *na_class NA_UNUSED,
    na_context_t *context NA_UNUSED, na_cb_t callback NA_UNUSED,
    void *arg NA_UNUSED, const void *buf NA_UNUSED, size_t buf_size NA_UNUSED,
    void *plugin_data NA_UNUSED, na_addr_t *dest_addr NA_UNUSED,
    uint8_t dest_id NA_UNUSED, na_tag_t tag NA_UNUSED,
    na_op_id_t *op_id NA_UNUSED)
{
    /* TODO: Initiate a non-blocking unexpected send to dest_addr.
     * When complete, fill in an na_cb_completion_data and call
     * na_cb_completion_add(context, &completion_data). */
    return NA_PROTOCOL_ERROR;
}

static na_return_t
na_abc_msg_recv_unexpected(na_class_t *na_class NA_UNUSED,
    na_context_t *context NA_UNUSED, na_cb_t callback NA_UNUSED,
    void *arg NA_UNUSED, void *buf NA_UNUSED, size_t buf_size NA_UNUSED,
    void *plugin_data NA_UNUSED, na_op_id_t *op_id NA_UNUSED)
{
    /* TODO: Post a non-blocking unexpected receive.  When a message
     * arrives, complete the operation via na_cb_completion_add(). */
    return NA_PROTOCOL_ERROR;
}

static na_return_t
na_abc_msg_send_expected(na_class_t *na_class NA_UNUSED,
    na_context_t *context NA_UNUSED, na_cb_t callback NA_UNUSED,
    void *arg NA_UNUSED, const void *buf NA_UNUSED, size_t buf_size NA_UNUSED,
    void *plugin_data NA_UNUSED, na_addr_t *dest_addr NA_UNUSED,
    uint8_t dest_id NA_UNUSED, na_tag_t tag NA_UNUSED,
    na_op_id_t *op_id NA_UNUSED)
{
    /* TODO: Initiate a non-blocking expected send. */
    return NA_PROTOCOL_ERROR;
}

static na_return_t
na_abc_msg_recv_expected(na_class_t *na_class NA_UNUSED,
    na_context_t *context NA_UNUSED, na_cb_t callback NA_UNUSED,
    void *arg NA_UNUSED, void *buf NA_UNUSED, size_t buf_size NA_UNUSED,
    void *plugin_data NA_UNUSED, na_addr_t *source_addr NA_UNUSED,
    uint8_t source_id NA_UNUSED, na_tag_t tag NA_UNUSED,
    na_op_id_t *op_id NA_UNUSED)
{
    /* TODO: Post a non-blocking expected receive. */
    return NA_PROTOCOL_ERROR;
}

static na_return_t
na_abc_mem_handle_create(na_class_t *na_class NA_UNUSED, void *buf NA_UNUSED,
    size_t buf_size NA_UNUSED, unsigned long flags NA_UNUSED,
    na_mem_handle_t **mem_handle_p NA_UNUSED)
{
    /* TODO: Create a memory handle describing a contiguous buffer for RMA.
     * flags indicates access mode (NA_MEM_READ_ONLY, etc.). */
    return NA_PROTOCOL_ERROR;
}

static void
na_abc_mem_handle_free(
    na_class_t *na_class NA_UNUSED, na_mem_handle_t *mem_handle NA_UNUSED)
{
    /* TODO: Free a memory handle created by mem_handle_create. */
}

static size_t
na_abc_mem_handle_get_serialize_size(
    na_class_t *na_class NA_UNUSED, na_mem_handle_t *mem_handle NA_UNUSED)
{
    /* TODO: Return the number of bytes needed to serialize the handle. */
    return 0;
}

static na_return_t
na_abc_mem_handle_serialize(na_class_t *na_class NA_UNUSED,
    void *buf NA_UNUSED, size_t buf_size NA_UNUSED,
    na_mem_handle_t *mem_handle NA_UNUSED)
{
    /* TODO: Serialize the memory handle into buf so it can be sent to
     * a remote peer for RMA. */
    return NA_PROTOCOL_ERROR;
}

static na_return_t
na_abc_mem_handle_deserialize(na_class_t *na_class NA_UNUSED,
    na_mem_handle_t **mem_handle_p NA_UNUSED, const void *buf NA_UNUSED,
    size_t buf_size NA_UNUSED)
{
    /* TODO: Reconstruct a remote memory handle from buf. */
    return NA_PROTOCOL_ERROR;
}

static na_return_t
na_abc_put(na_class_t *na_class NA_UNUSED, na_context_t *context NA_UNUSED,
    na_cb_t callback NA_UNUSED, void *arg NA_UNUSED,
    na_mem_handle_t *local_mem_handle NA_UNUSED,
    na_offset_t local_offset NA_UNUSED,
    na_mem_handle_t *remote_mem_handle NA_UNUSED,
    na_offset_t remote_offset NA_UNUSED, size_t length NA_UNUSED,
    na_addr_t *remote_addr NA_UNUSED, uint8_t remote_id NA_UNUSED,
    na_op_id_t *op_id NA_UNUSED)
{
    /* TODO: Write length bytes from (local_mem_handle + local_offset)
     * to (remote_mem_handle + remote_offset) on remote_addr.
     * Complete via na_cb_completion_add() when done. */
    return NA_PROTOCOL_ERROR;
}

static na_return_t
na_abc_get(na_class_t *na_class NA_UNUSED, na_context_t *context NA_UNUSED,
    na_cb_t callback NA_UNUSED, void *arg NA_UNUSED,
    na_mem_handle_t *local_mem_handle NA_UNUSED,
    na_offset_t local_offset NA_UNUSED,
    na_mem_handle_t *remote_mem_handle NA_UNUSED,
    na_offset_t remote_offset NA_UNUSED, size_t length NA_UNUSED,
    na_addr_t *remote_addr NA_UNUSED, uint8_t remote_id NA_UNUSED,
    na_op_id_t *op_id NA_UNUSED)
{
    /* TODO: Read length bytes from (remote_mem_handle + remote_offset)
     * on remote_addr into (local_mem_handle + local_offset).
     * Complete via na_cb_completion_add() when done. */
    return NA_PROTOCOL_ERROR;
}

static int
na_abc_poll_get_fd(
    na_class_t *na_class NA_UNUSED, na_context_t *context NA_UNUSED)
{
    /* TODO: Return a file descriptor that can be polled (epoll/select) for
     * readability to know when progress can be made.  Return -1 if fd-based
     * notification is not supported. */
    return -1;
}

static bool
na_abc_poll_try_wait(
    na_class_t *na_class NA_UNUSED, na_context_t *context NA_UNUSED)
{
    /* TODO: Return true if it is safe to block on poll_get_fd(); return
     * false if there is already pending work (so the caller should call
     * poll() immediately instead of waiting). */
    return false;
}

static na_return_t
na_abc_poll(na_class_t *na_class NA_UNUSED, na_context_t *context NA_UNUSED,
    unsigned int *count_p NA_UNUSED)
{
    /* TODO: Check for completed operations and process them.  For each
     * completion, call na_cb_completion_add().  Store the number of
     * completions processed in *count_p. */
    if (count_p)
        *count_p = 0;
    return NA_TIMEOUT;
}

static na_return_t
na_abc_cancel(na_class_t *na_class NA_UNUSED, na_context_t *context NA_UNUSED,
    na_op_id_t *op_id NA_UNUSED)
{
    /* TODO: Cancel the in-flight operation identified by op_id.  On
     * successful cancellation the operation's callback should still be
     * invoked with NA_CANCELED as the return value. */
    return NA_PROTOCOL_ERROR;
}

/* ------------------------------------------------------------------ */
/*  Plugin ops table                                                   */
/* ------------------------------------------------------------------ */

/*
 * This is the exported symbol that NA's dynamic loader looks up via dlsym()
 * when loading libna_plugin_abc.so.  The name MUST be  na_abc_class_ops_g.
 *
 * For runtime registration (NA_Register_plugin), the same symbol is passed
 * by pointer.
 *
 * Callbacks set to NULL are optional; NA provides default behavior or simply
 * skips the call.  See the comment on each field for details.
 */
NA_PLUGIN const struct na_class_ops NA_PLUGIN_OPS(abc) = {
    "abc",                              /* class_name (REQUIRED) */
    na_abc_get_protocol_info,           /* get_protocol_info */
    na_abc_check_protocol,              /* check_protocol (REQUIRED) */
    na_abc_initialize,                  /* initialize (REQUIRED) */
    na_abc_finalize,                    /* finalize (REQUIRED) */
    na_abc_cleanup,                     /* cleanup (optional) */
    NULL,                               /* has_opt_feature (optional) */
    na_abc_context_create,              /* context_create */
    na_abc_context_destroy,             /* context_destroy */
    na_abc_op_create,                   /* op_create */
    na_abc_op_destroy,                  /* op_destroy */
    na_abc_addr_lookup,                 /* addr_lookup */
    na_abc_addr_free,                   /* addr_free */
    NULL,                               /* addr_set_remove (optional) */
    na_abc_addr_self,                   /* addr_self */
    na_abc_addr_dup,                    /* addr_dup */
    na_abc_addr_cmp,                    /* addr_cmp */
    na_abc_addr_is_self,                /* addr_is_self */
    na_abc_addr_to_string,              /* addr_to_string */
    na_abc_addr_get_serialize_size,     /* addr_get_serialize_size */
    na_abc_addr_serialize,              /* addr_serialize */
    na_abc_addr_deserialize,            /* addr_deserialize */
    na_abc_msg_get_max_unexpected_size, /* msg_get_max_unexpected_size */
    na_abc_msg_get_max_expected_size,   /* msg_get_max_expected_size */
    NULL,                               /* msg_get_unexpected_header_size (opt) */
    NULL,                               /* msg_get_expected_header_size (opt) */
    na_abc_msg_get_max_tag,             /* msg_get_max_tag */
    NULL,                               /* msg_buf_alloc (optional — NA provides default) */
    NULL,                               /* msg_buf_free  (optional — NA provides default) */
    NULL,                               /* msg_init_unexpected (optional) */
    na_abc_msg_send_unexpected,         /* msg_send_unexpected */
    na_abc_msg_recv_unexpected,         /* msg_recv_unexpected */
    NULL,                               /* msg_multi_recv_unexpected (optional) */
    NULL,                               /* msg_init_expected (optional) */
    na_abc_msg_send_expected,           /* msg_send_expected */
    na_abc_msg_recv_expected,           /* msg_recv_expected */
    na_abc_mem_handle_create,           /* mem_handle_create */
    NULL,                               /* mem_handle_create_segments (optional) */
    na_abc_mem_handle_free,             /* mem_handle_free */
    NULL,                               /* mem_handle_get_max_segments (optional) */
    NULL,                               /* mem_register (optional) */
    NULL,                               /* mem_deregister (optional) */
    na_abc_mem_handle_get_serialize_size, /* mem_handle_get_serialize_size */
    na_abc_mem_handle_serialize,        /* mem_handle_serialize */
    na_abc_mem_handle_deserialize,      /* mem_handle_deserialize */
    na_abc_put,                         /* put */
    na_abc_get,                         /* get */
    na_abc_poll_get_fd,                 /* poll_get_fd (optional) */
    na_abc_poll_try_wait,               /* poll_try_wait (optional) */
    na_abc_poll,                        /* poll */
    NULL,                               /* poll_wait (optional — NA busy-polls if NULL) */
    na_abc_cancel                       /* cancel */
};
