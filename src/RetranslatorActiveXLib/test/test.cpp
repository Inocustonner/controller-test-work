#include <Windows.h>
#include <Retranslator_i.h>
#include <Retranslator_i.c>
#include <thread>
#include <chrono>
int main() {
  IRetranslatorComPort *ir = nullptr;
  (CoInitialize(nullptr));
  HRESULT hr = CoCreateInstance(CLSID_RetranslatorComPort, NULL, CLSCTX_INPROC_SERVER, IID_IRetranslatorComPort,
                                reinterpret_cast<void **>(&ir));
  if (SUCCEEDED(hr)) {
      long status;
      ir->openPort(11, 9600, 8, &status);
      BSTR array;
      ir->read(8, &array);
      ir->Release();
  }
  CoUninitialize();
  return 0;
}