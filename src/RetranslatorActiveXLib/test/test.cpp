#include <Windows.h>
#include <Retranslator_i.h>
#include <Retranslator_i.c>

int main() {
  IRetranslator *ir = nullptr;
  CoInitialize(nullptr);
  HRESULT hr = CoCreateInstance(CLSID_RetranslatorAX, NULL, CLSCTX_INPROC_SERVER, IID_IRetranslator,
                                reinterpret_cast<void **>(&ir));
  if (SUCCEEDED(hr)) {
    long res;
    ir->get_getWeight(&res);
    ir->Release();
  }
  CoUninitialize();
  return 0;
}