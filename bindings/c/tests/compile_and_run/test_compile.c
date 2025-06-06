#include <copendaq.h>

int main() {
  daqString *str = NULL;
  daqErrCode err = daqString_createString(&str, "Hello, C bindings!");
  int refc = daqReleaseRef(str);
  return err | refc;
}