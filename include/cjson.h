#ifndef CJSON_H
#define CJSON_H 1

#include <stdio.h>
#include <stdint.h>

extern const char CJSONX_PARSE[];

enum cjson_type {
  CJSON_ARRAY   = 0x01,
  CJSON_BOOLEAN = 0x02,
  CJSON_NULL    = 0x04,
  CJSON_NUMBER  = 0x08,
  CJSON_OBJECT  = 0x10,
  CJSON_STRING  = 0x20,

  CJSON_PAIR    = 0x40,       /* A key+value pair suitable for inserting into an Object. */
  CJSON_ROOT    = 0x80,       /* The root type of all documents, similar to an array. */

  CJSON_ALL_S   = 0x11,       /* Convenience type for all valid standard bare json types. */
  CJSON_ALL_E   = 0x3F,       /* Convenience type for all valid extended bare json types. */
};

struct cjson {
  enum cjson_type type;
  struct cjson *parent;

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

struct cjson *
cjson_fscan(FILE *stream, struct cjson *parent);

void
cjson_fprint(FILE *stream, struct cjson *node);

/* Finialize and deallocate a cjson tree. */
void cjson_free(struct cjson *node);

/*** Array ***/

struct cjson *
cjson_array_fscan(FILE *stream, struct cjson *parent);

void
cjson_array_fprint(FILE *stream, struct cjson *node);

void
cjson_array_lappend(struct cjson *self, struct cjson *item);

/*** Boolean ***/

struct cjson *
cjson_boolean_fscan(FILE *stream, struct cjson *parent);

void
cjson_boolean_fprint(FILE *stream, struct cjson *node);

/*** Null ***/

struct cjson *
cjson_null_fscan(FILE *stream, struct cjson *parent);

void
cjson_null_fprint(FILE *stream, struct cjson *node);

/*** Number ***/

struct cjson *
cjson_number_fscan(FILE *stream, struct cjson *parent);

void
cjson_number_fprint(FILE *stream, struct cjson *node);

/*** Object ***/

struct cjson *
cjson_object_fscan(FILE *stream, struct cjson *parent);

void
cjson_object_fprint(FILE *stream, struct cjson *node);

void
cjson_object_insert(struct cjson *self, struct cjson *item);

/*** Pair ***/

struct cjson *
cjson_pair_fscan(FILE *stream, struct cjson *parent);

void
cjson_pair_fprint(FILE *stream, struct cjson *node);

/*** Root ***/

struct cjson *
cjson_root_fscan(FILE *stream, enum cjson_type valid, unsigned int continuous);

void
cjson_root_fprint(FILE *stream, struct cjson *node);

void
cjson_root_lappend(struct cjson *self, struct cjson *item);

/*** UTF-8 ***/

int64_t
cjson_u8_fgetu(FILE *stream);

void
cjson_u8_fputu(const uint32_t u, FILE *stream);

/*** UTF-16 Escape ***/

int64_t
cjson_u16e_fgetu(FILE *stream);

void
cjson_u16e_fputu(const uint32_t u, FILE *stream);

/*** JSON Encoded String ***/

int64_t
cjson_jestr_fgetu(FILE *stream);

void
cjson_jestr_fputu(const uint32_t u, FILE *stream);

char *
cjson_jestr_fscan(FILE *stream);

void
cjson_jestr_fprint(FILE *stream, char *jestr);

/*** String ***/

struct cjson *
cjson_string_fscan(FILE *stream, struct cjson *parent);

void
cjson_string_fprint(FILE *stream, struct cjson *node);

#endif /* CJSON_H */
