#include <cjson.h>
#include <ec/ec.h>

int
main()
{
  struct cjson *node = cjson_root_fscan(stdin, CJSON_ALL_E, 1);
  ec_with(node, (ec_unwind_f)cjson_free) {
    cjson_fprint(stdout, node);
  }

  return 0;
}
