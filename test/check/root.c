/* Copyright 2013 Caleb Case
 *
 * This file is part of the CJSON Library.
 *
 * The CJSON Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * The CJSON Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the CJSON Library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <ccstreams/ecx_ccstreams.h>
#include <check.h>
#include <ec/ec.h>
#include <ecx_stdio.h>
#include <ecx_stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <cjson.h>

START_TEST(fscan)
{
#define IN ""
  char *buf = IN;
  FILE *stream = ecx_ccstreams_fstropen(&buf, "r");
  struct cjson *node = cjson_root_fscan(stream, CJSON_ALL_S, 1, NULL);

  fail_unless(node != NULL);
  fail_unless(node->value.root.data == NULL);

  fclose(stream);
  cjson_free(node);
#undef IN
}
END_TEST

START_TEST(fprint)
{
#define IN ""
#define EXP ""
  char *buf = NULL;
  FILE *stream = ecx_ccstreams_fstropen(&buf, "w+");

  char *in = IN;
  FILE *in_stream = ecx_ccstreams_fstropen(&in, "r");
  struct cjson *node = cjson_root_fscan(in_stream, CJSON_ALL_S, 1, NULL);
  fclose(in_stream);

  cjson_root_fprint(stream, node);
  fclose(stream);

  fail_unless(buf != NULL);

  const char fmt[] = "Failed to print array to stream. Got: %s Exp: %s";
  fail_unless(strcmp(buf, EXP) == 0, fmt, buf, EXP);

  cjson_free(node);
  free(buf);
#undef EXP
#undef IN
}
END_TEST

START_TEST(gambit_leaf)
{
#define IN "3.14\n\"\"\ntrue\nnull\n"
#define EXP "3.14\n\"\"\ntrue\nnull"
  char *buf = NULL;
  FILE *stream = ecx_ccstreams_fstropen(&buf, "w+");

  char *in = IN;
  FILE *in_stream = ecx_ccstreams_fstropen(&in, "r");
  struct cjson *node = cjson_root_fscan(in_stream, CJSON_ALL_E, 1, NULL);
  fclose(in_stream);

  cjson_root_fprint(stream, node);
  fclose(stream);

  fail_unless(buf != NULL);

  const char fmt[] = "Failed to print root to stream. Got:\n%s\nExp:\n%s";
  fail_unless(strcmp(buf, EXP) == 0, fmt, buf, EXP);

  cjson_free(node);
  free(buf);
#undef EXP
#undef IN
}
END_TEST

START_TEST(gambit_nested_leaf)
{
#define IN "\n0\n[0,\n\"\"]\n[[0, \"\", true, null]]\n[false]"
#define EXP "0\n" \
            "[\n" \
            "  0,\n" \
            "  \"\"\n" \
            "]\n" \
            "[\n" \
            "  [\n" \
            "    0,\n" \
            "    \"\",\n" \
            "    true,\n" \
            "    null\n" \
            "  ]\n" \
            "]\n" \
            "[\n" \
            "  false\n" \
            "]"
  char *buf = NULL;
  FILE *stream = ecx_ccstreams_fstropen(&buf, "w+");

  char *in = IN;
  FILE *in_stream = ecx_ccstreams_fstropen(&in, "r");
  struct cjson *node = cjson_root_fscan(in_stream, CJSON_ALL_E, 1, NULL);
  fclose(in_stream);

  cjson_root_fprint(stream, node);
  fclose(stream);

  fail_unless(buf != NULL);

  const char fmt[] = "Failed to print root to stream. Got:\n%s\nExp:\n%s";
  fail_unless(strcmp(buf, EXP) == 0, fmt, buf, EXP);

  cjson_free(node);
  free(buf);
#undef EXP
#undef IN
}
END_TEST

START_TEST(manipulate)
{
  char *r_str = "true";
  FILE *r_stream = ecx_ccstreams_fstropen(&r_str, "r");
  struct cjson *r = cjson_root_fscan(r_stream, CJSON_ALL_E, 0, NULL);
  fclose(r_stream);

  fail_unless(cjson_array_length(r) == 1);
  fail_unless(cjson_array_get(r, 0)->type == CJSON_BOOLEAN);
  fail_unless(cjson_array_get(r, 0)->value.boolean == 1);

  char *tmp_str = NULL;
  FILE *tmp_stream = NULL;
  struct cjson *tmp = NULL;
  struct cjson *old = NULL;

  tmp_str = "null";
  tmp_stream = ecx_ccstreams_fstropen(&tmp_str, "r");
  tmp = cjson_null_fscan(tmp_stream, NULL);
  fclose(tmp_stream);

  old = cjson_array_set(r, 0, tmp);
  fail_unless(cjson_array_length(r) == 1);
  fail_unless(cjson_array_get(r, 0) == tmp);
  fail_unless(old->type == CJSON_BOOLEAN);
  cjson_free(old);

  tmp_str = "1";
  tmp_stream = ecx_ccstreams_fstropen(&tmp_str, "r");
  tmp = cjson_number_fscan(tmp_stream, NULL);
  fclose(tmp_stream);

  cjson_array_append(r, tmp);
  fail_unless(cjson_array_length(r) == 2);
  fail_unless(cjson_array_get(r, 1) == tmp);

  tmp_str = "2";
  tmp_stream = ecx_ccstreams_fstropen(&tmp_str, "r");
  tmp = cjson_number_fscan(tmp_stream, NULL);
  fclose(tmp_stream);

  cjson_array_append(r, tmp);
  fail_unless(cjson_array_length(r) == 3);
  fail_unless(cjson_array_get(r, 2) == tmp);

  old = cjson_array_truncate(r, 1);
  fail_unless(cjson_array_length(r) == 1);
  fail_unless(cjson_array_length(old) == 2);
  fail_unless(cjson_array_get(r, 0)->type == CJSON_NULL);
  fail_unless(cjson_array_get(old, 0)->type == CJSON_NUMBER);
  fail_unless(strcmp(cjson_array_get(old, 0)->value.number, "1") == 0);
  fail_unless(cjson_array_get(old, 1)->type == CJSON_NUMBER);
  fail_unless(strcmp(cjson_array_get(old, 1)->value.number, "2") == 0);
  cjson_free(old);

  old = cjson_array_truncate(r, 0);
  fail_unless(cjson_array_length(r) == 0);
  fail_unless(cjson_array_length(old) == 1);
  fail_unless(cjson_array_get(old, 0)->type == CJSON_NULL);
  cjson_free(old);

  cjson_free(r);
}
END_TEST

#define container_of(ptr, type, member) \
({ \
  const typeof(((type *)0)->member)*__mptr = (ptr); \
  (type *)((char *)__mptr - offsetof(type,member)); \
})

struct embed {
  size_t depth;
  struct cjson json;
};

static struct cjson_hook embed_hook;

static
struct cjson *
embed_malloc(enum cjson_type type, struct cjson *parent)
{
  struct embed *node = ecx_malloc(sizeof(*node));
  cjson_init(&(node->json), type, parent);

  if (parent == NULL) {
    node->depth = 0;
  }
  else {
    node->depth = container_of(parent, struct embed, json)->depth + 1;
  }

  return &(node->json);
}

static
void
embed_free(struct cjson *self)
{
  struct embed *node = container_of(self, struct embed, json);
  free(node);
}

static struct cjson_hook embed_hook = {
  .cjson_malloc = embed_malloc,
  .cjson_free = embed_free,
  .valid = NULL,
};

START_TEST(embed)
{
  char *r_str = "true\nfalse\n[[]]";
  FILE *r_stream = ecx_ccstreams_fstropen(&r_str, "r");
  struct cjson *r = cjson_root_fscan(r_stream, CJSON_ALL_E, 1, &embed_hook);
  fclose(r_stream);

  fail_unless(r->hook != NULL);
  fail_unless(r->hook == &embed_hook);

  struct embed *node = NULL;

  node = container_of(r, struct embed, json);
  fail_unless(node->depth == 0);

  node = container_of(cjson_array_get(r, 0), struct embed, json);
  fail_unless(node->depth == 1);

  node = container_of(cjson_array_get(r, 1), struct embed, json);
  fail_unless(node->depth == 1);

  node = container_of(cjson_array_get(r, 2), struct embed, json);
  fail_unless(node->depth == 1);

  node = container_of(cjson_array_get(cjson_array_get(r, 2), 0), struct embed, json);
  fail_unless(node->depth == 2);

  cjson_free(r);
}
END_TEST

const char INVALID[] = "INVALID";

struct validate {
  int valid;
  struct cjson json;
};

static
struct cjson *
validate_malloc(enum cjson_type type, struct cjson *parent)
{
  struct validate *node = ecx_malloc(sizeof(*node));
  cjson_init(&(node->json), type, parent);
  node->valid = 1;
  return &(node->json);
}

static
void
validate_free(struct cjson *self)
{
  struct validate *node = container_of(self, struct validate, json);
  free(node);
}

static
void
validate_valid(struct cjson *self)
{
  struct validate *node = container_of(self, struct validate, json);
  if (!node->valid) {
    ec_throw_str_static(INVALID, "INVALID");
  }
}

static struct cjson_hook validate_hook = {
  .cjson_malloc = validate_malloc,
  .cjson_free = validate_free,
  .valid = validate_valid,
};

START_TEST(validate)
{
  char *r_str = "true\nfalse\n[[]]";
  FILE *r_stream = ecx_ccstreams_fstropen(&r_str, "r");
  struct cjson *r = cjson_root_fscan(r_stream, CJSON_ALL_E, 1, &validate_hook);
  fclose(r_stream);

  fail_unless(r->hook != NULL);
  fail_unless(r->hook == &validate_hook);

  struct validate *node = NULL;
  struct cjson * volatile old = NULL;
  const char *msg = NULL;

  char *tmp_str = NULL;
  FILE *tmp_stream = NULL;
  struct cjson *tmp = NULL;

  tmp_str = "null";
  tmp_stream = ecx_ccstreams_fstropen(&tmp_str, "r");
  tmp = cjson_null_fscan(tmp_stream, r);
  fclose(tmp_stream);

  node = container_of(r, struct validate, json);
  node->valid = 0;

#define UNCHANGED \
  fail_unless(msg != NULL, "Invalid operation was permitted."); \
  fail_unless(old == NULL); \
  fail_unless(cjson_array_length(r) == 3, "Length: %zu (Expecting 3).", cjson_array_length(r)); \
  fail_unless(cjson_array_get(r, 0)->type == CJSON_BOOLEAN); \
  fail_unless(cjson_array_get(r, 0)->value.boolean == 1); \
  fail_unless(cjson_array_get(r, 1)->type == CJSON_BOOLEAN); \
  fail_unless(cjson_array_get(r, 1)->value.boolean == 0); \
  fail_unless(cjson_array_get(r, 2)->type == CJSON_ARRAY); \
  fail_unless(cjson_array_length(cjson_array_get(r, 2)) == 1); \
  fail_unless(cjson_array_get(cjson_array_get(r, 2), 0)->type == CJSON_ARRAY); \
  fail_unless(cjson_array_length(cjson_array_get(cjson_array_get(r, 2), 0)) == 0); \

  msg = NULL;
  ec_try { old = cjson_array_set(r, 0, tmp); } ec_catch_a(INVALID, msg) { } ec_catch { }
  UNCHANGED;

  msg = NULL;
  ec_try { old = cjson_array_truncate(r, 0); } ec_catch_a(INVALID, msg) { } ec_catch { }
  UNCHANGED;

  msg = NULL;
  ec_try { cjson_array_append(r, tmp); } ec_catch_a(INVALID, msg) { } ec_catch { }
  UNCHANGED;

  cjson_free(tmp);
  tmp_str = "[null]";
  tmp_stream = ecx_ccstreams_fstropen(&tmp_str, "r");
  tmp = cjson_array_fscan(tmp_stream, r);
  fclose(tmp_stream);

  msg = NULL;
  ec_try { cjson_array_extend(r, tmp); } ec_catch_a(INVALID, msg) { } ec_catch { }
  UNCHANGED;
#undef UNCHANGED

  /* Test that changes are possible. */
  node->valid = 1;
  cjson_array_extend(r, tmp);
  fail_unless(cjson_array_length(tmp) == 0);
  fail_unless(cjson_array_length(r) == 4);
  fail_unless(cjson_array_get(r, 3)->type == CJSON_NULL);
  cjson_free(tmp);

  tmp_str = "{\"test?\": true}";
  tmp_stream = ecx_ccstreams_fstropen(&tmp_str, "r");
  tmp = cjson_object_fscan(tmp_stream, r);
  fclose(tmp_stream);

  cjson_array_append(r, tmp);
  fail_unless(cjson_array_length(r) == 5);
  fail_unless(cjson_array_get(r, 4)->type == CJSON_OBJECT);
  fail_unless(cjson_object_get(cjson_array_get(r, 4), "test?")->type == CJSON_PAIR);

  old = cjson_array_truncate(r, 2);
  fail_unless(cjson_array_length(r) == 2);
  fail_unless(cjson_array_length(old) == 3);
  cjson_free(old);

  tmp_str = "0.42";
  tmp_stream = ecx_ccstreams_fstropen(&tmp_str, "r");
  tmp = cjson_number_fscan(tmp_stream, r);
  fclose(tmp_stream);

  old = cjson_array_set(r, 0, tmp);
  fail_unless(cjson_array_get(r, 0) == tmp);
  fail_unless(old->type == CJSON_BOOLEAN);
  fail_unless(old->value.boolean == 1);
  cjson_free(old);

  /* Add an object and make it immutable. */
  tmp_str = "{\"fruit\": [\"banana\"]}";
  tmp_stream = ecx_ccstreams_fstropen(&tmp_str, "r");
  tmp = cjson_object_fscan(tmp_stream, r);
  fclose(tmp_stream);

  cjson_array_append(r, tmp);
  fail_unless(cjson_array_get(r, 2) == tmp);

  node = container_of(tmp, struct validate, json);
  node->valid = 0;
  struct cjson *o = cjson_array_get(r, 2);
  fail_unless(cjson_object_count(o) == 1);

  tmp_str = "\"waffle\": [\"blueberry\"]";
  tmp_stream = ecx_ccstreams_fstropen(&tmp_str, "r");
  tmp = cjson_pair_fscan(tmp_stream, r);
  fclose(tmp_stream);

#define UNCHANGED \
  fail_unless(msg != NULL, "Invalid operation was permitted."); \
  fail_unless(old == NULL); \
  fail_unless(cjson_object_count(o) == 1); \
  fail_unless(cjson_object_get(o, "fruit") != NULL); \
  fail_unless(cjson_object_get(o, "fruit")->type == CJSON_PAIR); \
  fail_unless(cjson_object_get(o, "waffle") == NULL); \

  old = NULL;
  msg = NULL;
  ec_try { old = cjson_object_set(o, tmp); } ec_catch_a(INVALID, msg) { } ec_catch { }
  UNCHANGED;

  msg = NULL;
  ec_try { old = cjson_object_remove(o, cjson_object_get(o, "fruit")); } ec_catch_a(INVALID, msg) { } ec_catch { }
  UNCHANGED;

  cjson_free(tmp);
#undef UNCHANGED

  cjson_free(r);
}
END_TEST

static
Suite *
suite(void)
{
  Suite *suite = suite_create("root");

  TCase *tcase_fscan = tcase_create("fscan");
  tcase_add_test(tcase_fscan, fscan);
  suite_add_tcase(suite, tcase_fscan);

  TCase *tcase_fprint = tcase_create("fprint");
  tcase_add_test(tcase_fprint, fprint);
  suite_add_tcase(suite, tcase_fprint);

  TCase *tcase_gambit = tcase_create("gambit");
  tcase_add_test(tcase_gambit, gambit_leaf);
  tcase_add_test(tcase_gambit, gambit_nested_leaf);
  suite_add_tcase(suite, tcase_gambit);

  TCase *tcase_manipulate = tcase_create("manipulate");
  tcase_add_test(tcase_manipulate, manipulate);
  suite_add_tcase(suite, tcase_manipulate);

  TCase *tcase_embed = tcase_create("embed");
  tcase_add_test(tcase_embed, embed);
  suite_add_tcase(suite, tcase_embed);

  TCase *tcase_validate = tcase_create("validate");
  tcase_add_test(tcase_validate, validate);
  suite_add_tcase(suite, tcase_validate);

 return suite;
}

int
main(void)
{
  int failed = 0;

  SRunner *srunner = srunner_create(suite());

  srunner_run_all(srunner, CK_NORMAL);
  failed = srunner_ntests_failed(srunner);

  srunner_free(srunner);

  return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
