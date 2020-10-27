#include <Windows.h>
#include <Retranslator_i.h>
#include <Retranslator_i.c>
#include <thread>
#include <chrono>
int main() {
  IRetranslator *ir = nullptr;
  CoInitialize(nullptr);
  HRESULT hr = CoCreateInstance(CLSID_RetranslatorAX, NULL, CLSCTX_INPROC_SERVER, IID_IRetranslator,
                                reinterpret_cast<void **>(&ir));
  if (SUCCEEDED(hr)) {
    long res;
    ir->setCorr(50);
    ir->Release();
  }
  CoUninitialize();
  return 0;
}