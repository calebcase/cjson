#ifndef CJSON_H
#define CJSON_H 1

#include <stdio.h>
#include <stdint.h>

/* Exception Types */
extern const char CJSONX_PARSE[];       /* Data: C String. */
extern const char CJSONX_TYPE[];        /* Data: C String. */
extern const char CJSONX_INDEX[];       /* Data: C String. */
extern const char CJSONX_NOT_FOUND[];   /* Data: C String. */

/* cjson Node Types */
enum cjson_type {
  CJSON_ARRAY   = 0x01,       /* [] */
  CJSON_BOOLEAN = 0x02,       /* false */
  CJSON_NULL    = 0x04,       /* null */
  CJSON_NUMBER  = 0x08,       /* 0 */
  CJSON_OBJECT  = 0x10,       /* {} */
  CJSON_STRING  = 0x20,       /* "" */

  CJSON_PAIR    = 0x40,       /* A key+value pair suitable for inserting into an Object. */
  CJSON_ROOT    = 0x80,       /* The root type of all documents, similar to an array. */

  CJSON_ALL_S   = 0x11,       /* Convenience type for all valid standard bare json types. */
  CJSON_ALL_E   = 0x3F,       /* Convenience type for all valid extended bare json types. */
};

/* cjson Node Structure */
struct cjson;

/* cjson node hooks used to manage the lifecycle of the node. */
struct cjson_hook {
  /* If provided, this function will be called when allocating a new node. This
   * does not control how memory is allocated for internal resources. It only
   * affects the memory allocation for the cjson struct itself.
   */
  struct cjson *(*cjson_malloc)(enum cjson_type type, struct cjson *parent);

  /* If provided, this function will be called when freeing node. This does not
   * control how memory is deallocated for internal resources. It only affects
   * the memory deallocation for the cjson struct itself.
   */
  void (*cjson_free)(struct cjson *self);

  /* If provided, this function will be called after a new node has been
   * initialized. This is useful for providing stricter validation requirements
   * on new node creation (e.g. restricting a numeric node to a specific
   * range).
   */
  void (*valid)(struct cjson *self);
};

struct cjson {
  enum cjson_type type;
  struct cjson *parent;       /* The parent/container node for this node (e.g. an object). */
  struct cjson_hook *hook;

  union {
    struct {
      void *data;             /* JudyL (Map index => struct cjson *) */
    } array;

    unsigned int boolean;     /* 0 for false; 1 for true. */

    char *number;             /* C String */

    struct {
      size_t key_length;      /* The length of the longest key (not including null). */
      void *data;             /* JudySL (Map C String => struct cjson *) */
    } object;

    struct {
      char *key;              /* JSON Escaped String (jestr). */
      struct cjson *value;    /* cjson Array, Boolean, Null, Number, Object, or String */
    } pair;

    struct {
      void *data;             /* JudyL (Map index => struct cjson *) */
    } root;

    struct {
      size_t length;
      char *bytes;            /* UTF-8 byte array (no trailing null). */
    } string;
  } value;
};

/*** Generic ***/

/* Initialize a node to be the given type and a child of the provided parent
 * node.
 */
void
cjson_init(
  struct cjson *node,
  enum cjson_type type,
  struct cjson *parent
);

/* Render the node to the provided stream (recusively). */
void
cjson_fprint(
  FILE *stream,
  struct cjson *node
);

/* Given a null separated string of path segments, return the node found at
 * that path.
 * 
 * For example, given this node:
 *
 * {"a": [{"c": true}]}
 *
 * And this path:
 *
 * "a\00\0c\0"
 *
 * It will return the node:
 *
 * true
 */
struct cjson *
cjson_get(
  struct cjson *node,
  const char *segments
);

/* Render path segments from the provide node to the given child. The segments
 * will be null separated.
 *
 * For example, given this node and a reference to the node for the nested
 * 'true' as the child:
 *
 * {"a": [{"c": true}]}
 *
 * It will return this path:
 *
 * "a\00\0c\0"
 */
void
cjson_segments_fprint(
  FILE *stream,
  struct cjson *node,
  struct cjson *child
);

/* Convenience typedef for the cjson_walk callback. */
typedef
int (*cjson_walk_call_f)(
  void *data,
  struct cjson *node
);

/* Walk all the child nodes of this node in a left recursive depth first
 * fashion.
 *
 * The callback will be called:
 *
 *  - Immediately for each leaf node found.
 *  - After all children for container nodes (e.g. array or object).
 *
 * The data pointer is provided so that state may be carried by the callback
 * as the tree is traversed.
 *
 * The callback may terminate execution early by returning a non-zero value.
 * cjson_walk will return the same value.
 */
int
cjson_walk(
  struct cjson *self,
  int (*call)(void *data, struct cjson *node),
  void *data
);

/* Finialize and deallocate a cjson tree. If the cjson_free hook was set, then
 * that callback will be used to deallocate the node, otherwise free is used.
 */
void
cjson_free(
  struct cjson *node
);

/*** Array ***/

/* Read a CJSON_ARRAY from the stream.
 *
 * Throws:
 *
 * CJSONX_PARSE
 *  If the stream does not contain an array.
 */
struct cjson *
cjson_array_fscan(
  FILE *stream,
  struct cjson *parent
);

/* Render a CJSON_ARRAY to the stream.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If the node is not a CJSON_ARRAY.
 */
void
cjson_array_fprint(
  FILE *stream,
  struct cjson *node
);

/* Return the length fo the array.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If the node is not a CJSON_ARRAY or CJSON_ROOT.
 */
size_t
cjson_array_length(
  struct cjson *self
);

/* Return a reference to the value at the given index in the array.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If self is not a CJSON_ARRAY or CJSON_ROOT.
 *
 * CJSONX_INDEX
 *  If the index is outside the bounds of the array.
 */
struct cjson *
cjson_array_get(
  struct cjson *self,
  size_t index
);

/* Set the value at the given index to the provided item. Return a reference to
 * previous value.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If self is not a CJSON_ARRAY or CJSON_ROOT.
 *
 * CJSONX_INDEX
 *  If the index is outside the bounds of the array.
 */
struct cjson *
cjson_array_set(
  struct cjson *self,
  size_t index,
  struct cjson *item
);

/* Truncate the array to the provided length. Return a new array containing the
 * items removed.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If self is not a CJSON_ARRAY or CJSON_ROOT.
 *
 * CJSONX_INDEX
 *  If the index is outside the bounds of the array.
 */
struct cjson *
cjson_array_truncate(
  struct cjson *self,
  size_t length
);

/* Append an item to the end of the array.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If self is not a CJSON_ARRAY or CJSON_ROOT.
 *
 * CJSONX_INDEX
 *  If the index is outside the bounds of the array.
 */
void
cjson_array_append(
  struct cjson *self,
  struct cjson *item
);

/* Move the items in the provided array to this array.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If self or array is not a CJSON_ARRAY or CJSON_ROOT.
 *
 * CJSONX_INDEX
 *  If the index is outside the bounds of the array.
 */
void
cjson_array_extend(
  struct cjson *self,
  struct cjson *array
);

/*** Boolean ***/

/* Read a CJSON_BOOLEAN from the stream.
 *
 * Throws:
 *
 * CJSONX_PARSE
 *  If the stream does not contain a boolean.
 */
struct cjson *
cjson_boolean_fscan(
  FILE *stream,
  struct cjson *parent
);

/* Render a CJSON_BOOLEAN to the stream.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If the node is not a CJSON_BOOLEAN.
 */
void
cjson_boolean_fprint(
  FILE *stream,
  struct cjson *node
);

/*** Null ***/

/* Read a CJSON_NULL from the stream.
 *
 * Throws:
 *
 * CJSONX_PARSE
 *  If the stream does not contain a null.
 */
struct cjson *
cjson_null_fscan(
  FILE *stream,
  struct cjson *parent
);

/* Render a CJSON_NULL to the stream.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If the node is not a CJSON_NULL.
 */
void
cjson_null_fprint(
  FILE *stream,
  struct cjson *node
);

/*** Number ***/

/* Read a CJSON_NUMBER from the stream.
 *
 * Throws:
 *
 * CJSONX_PARSE
 *  If the stream does not contain a number.
 */
struct cjson *
cjson_number_fscan(
  FILE *stream,
  struct cjson *parent
);

/* Render a CJSON_NUMBER to the stream.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If the node is not a CJSON_NUMBER.
 */
void
cjson_number_fprint(
  FILE *stream,
  struct cjson *node
);

/*** Object ***/

/* Read a CJSON_OBJECT from the stream.
 *
 * Throws:
 *
 * CJSONX_PARSE
 *  If the stream does not contain an object.
 */
struct cjson *
cjson_object_fscan(
  FILE *stream,
  struct cjson *parent
);

/* Render a CJSON_OBJECT to the stream.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If the node is not a CJSON_OBJECT.
 */
void
cjson_object_fprint(
  FILE *stream,
  struct cjson *node
);

/* Count and return the number of key value pairs found in the object. This is
 * an O(n) operation.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If self is not a CJSON_OBJECT.
 */
size_t
cjson_object_count(
  struct cjson *self
);

/* Return the CJSON_PAIR with the matching key or NULL if the key is not found.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If self is not a CJSON_OBJECT.
 */
struct cjson *
cjson_object_get(
  struct cjson *self,
  const char *key
);

/* Insert a CJSON_PAIR into the object. Return the previous CJSON_PAIR or NULL
 * if it didn't exist.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If self is not a CJSON_OBJECT.
 *  If pair is not a CJSON_PAIR.
 */
struct cjson *
cjson_object_set(
  struct cjson *self,
  struct cjson *pair
);

/* Remove a CJSON_PAIR from the object. The value in the pair does not need to
 * be set (only the key). Return the removed CJSON_PAIR.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If self is not a CJSON_OBJECT.
 *  If pair is not a CJSON_PAIR.
 *
 * CJSONX_NOT_FOUND
 *  If the pair was not found in the object.
 */
struct cjson *
cjson_object_remove(
  struct cjson *self,
  struct cjson *pair
);

/* Convenience typedef for cjson_object_for_each callback. */
typedef int (*cjson_object_call_f)(void *data, struct cjson *pair);

/* For each CJSON_PAIR in the object, call the callback.
 *
 * The data pointer is passed to the callback on each call and provides a way
 * for the callback to carry state.
 *
 * The callback may terminate execution early by returning a non-zero value.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If self is not a CJSON_OBJECT.
 *
 * CJSONX_NOT_FOUND
 *  If the pair was not found in the object.
 */
int
cjson_object_for_each(
  struct cjson *self,
  int (*call)(void *data, struct cjson *pair),
  void *data
);

/*** Pair ***/

/* Read a CJSON_PAIR from the stream.
 *
 * Throws:
 *
 * CJSONX_PARSE
 *  If the stream does not contain an key value pair.
 */
struct cjson *
cjson_pair_fscan(
  FILE *stream,
  struct cjson *parent
);

/* Render a CJSON_PAIR to the stream.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If the node is not a CJSON_PAIR.
 */
void
cjson_pair_fprint(
  FILE *stream,
  struct cjson *node
);

/*** Root ***/

/* Read a CJSON_ROOT from the stream. The valid set indicates which types are
 * valid root types. If the more than one item is expected, set continuous to
 * true. CJSON_ROOT operates much like an arrya and can be used with most of
 * the array functions.
 *
 * Throws:
 *
 * CJSONX_PARSE
 *  If the stream does not contain a valid root type.
 */
struct cjson *
cjson_root_fscan(
  FILE *stream,
  enum cjson_type valid,
  unsigned int continuous,
  struct cjson_hook *hook
);

/* Render a CJSON_ROOT to the stream.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If the node is not a CJSON_ROOT.
 */
void
cjson_root_fprint(
  FILE *stream,
  struct cjson *node
);

/*** UTF-8 ***/

/* Read a UTF-8 encoded character from the stream and return the Unicode code
 * point.
 *
 * Throws:
 *
 * CJSONX_PARSE
 *  If the stream does not contain a valid UTF-8 encoded character.
 */
int64_t
cjson_u8_fgetu(
  FILE *stream
);

/* Render the Unicode code point to the stream.
 *
 * Throws:
 *
 * CJSONX_PARSE
 *  If the code point is not valid.
 */
void
cjson_u8_fputu(
  const uint32_t u,
  FILE *stream
);

/*** UTF-16 Escape ***/

/* Read the Unicode code point from a UTF-8 encoded stream as a UTF-16 escape
 * sequence. The leading backslash is assumed to be consumed already so a
 * surrogate pair will look like: uD834\uDD1E.
 *
 * Throws:
 *
 * CJSONX_PARSE
 *  If the stream does not contain a valid UTF-16 escape sequence.
 */
int64_t
cjson_u16e_fgetu(
  FILE *stream
);

/* Render the Unicode code point to a UTF-8 encoded stream as a UTF-16 escape
 * sequence.
 *
 * Throws:
 *
 * CJSONX_PARSE
 *  If the code point is not valid.
 */
void
cjson_u16e_fputu(
  const uint32_t u,
  FILE *stream
);

/*** JSON Encoded String ***/

/* Read the Unicode code point from a JSON encoded string stream.
 *
 * Throws:
 *
 * CJSONX_PARSE
 *  If the stream does not contain a valid JSON encoded string.
 */
int64_t
cjson_jestr_fgetu(
  FILE *stream
);

/* Render the Unicode code point to a JSON encoded string stream.
 *
 * Throws:
 *
 * CJSONX_PARSE
 *  If the code point is not valid.
 */
void
cjson_jestr_fputu(
  const uint32_t u,
  FILE *stream
);

/* Read a JSON escaped string from the stream.
 *
 * Throws:
 *
 * CJSONX_PARSE
 *  If the stream does not contain a JSON escaped string.
 */
char *
cjson_jestr_fscan(
  FILE *stream
);

/* Render a JSON escaped string to the stream.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If the jestr is not a valid JSON escaped string.
 */
void
cjson_jestr_fprint(
  FILE *stream,
  char *jestr
);

/* Return a copy of the JSON escaped string normalizing UTF-16 escape sequences
 * and UTF-8 encodings.
 *
 * Throws:
 *
 * CJSONX_PARSE
 *  If jestr is not a valid JSON escaped string.
 */
char *
cjson_jestr_normalize(
  const char *jestr
);

/*** String ***/

/* Read a CJSON_STRING from the stream.
 *
 * Throws:
 *
 * CJSONX_PARSE
 *  If the stream does not contain a string.
 */
struct cjson *
cjson_string_fscan(
  FILE *stream,
  struct cjson *parent
);

/* Render a CJSON_STRING to the stream.
 *
 * Throws:
 *
 * CJSONX_TYPE
 *  If the node is not a valid CJSON_STRING.
 */
void
cjson_string_fprint(
  FILE *stream,
  struct cjson *node
);

#endif /* CJSON_H */
