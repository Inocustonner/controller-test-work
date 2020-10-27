#include "PipeQueueSystem.hpp"
#include "SafeQueue.hpp"

#include <string_view>
#include <thread>

#include <windows.h>

static SafeQueue<std::wstring> global_queue{};

extern HMODULE g_module;

static std::thread runner;
static ULONG wait_timeout = 5000;
static bool finish = false;

extern "C" 
volatile unsigned long created_process_pid;

LRESULT CALLBACK CBTProc(_In_ int nCode, _In_ WPARAM wParam,
                         _In_ LPARAM lParam) {
  if (nCode < 0) {
    return CallNextHookEx(NULL, nCode, wParam, lParam);
  }
  DWORD pid;
  GetWindowThreadProcessId((HWND)wParam, &pid);
  if (created_process_pid == pid)
    if (nCode == HCBT_CREATEWND || nCode == HCBT_ACTIVATE) {
      ShowWindow((HWND)wParam, SW_HIDE);
		}
  return 0;
}

static HANDLE createProc(std::wstring_view cmd) {
  STARTUPINFOW si = {};
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;

  PROCESS_INFORMATION pi = {};

  BOOL succ = CreateProcessW(NULL, (LPWSTR)cmd.data(), NULL, NULL, FALSE,
                             CREATE_SUSPENDED, NULL, NULL, &si, &pi);
  created_process_pid = pi.dwProcessId;
  ResumeThread(pi.hThread);

  if (!succ) {
    return INVALID_HANDLE_VALUE;
  }
  CloseHandle(pi.hThread);

  return pi.hProcess;
}

static void _run() {
  while (true) {
    std::wstring cmd;
    bool not_empty = global_queue.ConsumeSync(cmd);

    if (finish)
      break;

    if (!not_empty)
      continue;

    created_process_pid = -1;
    HHOOK h = SetWindowsHookExA(WH_CBT, CBTProc, g_module, 0);
		
    HANDLE proc_handle = createProc(cmd);
    if (proc_handle == INVALID_HANDLE_VALUE)
      continue;
		
    DWORD r = WaitForSingleObject(proc_handle, wait_timeout);
    UnhookWindowsHookEx(h);
    if (r == WAIT_TIMEOUT) {
      TerminateProcess(proc_handle, 0x0);
    }
    CloseHandle(proc_handle);
  }
}

void start_pipe_queue() { runner = std::thread(_run); }

void stop_pipe_queue() {
  // may hang if at the time of finish thread is not wating
  finish = true;
  global_queue.Finish();
  runner.join();
}

void pipe_push_cmd(std::wstring cmd) { global_queue.Provide(std::move(cmd)); }

void pipe_set_timeout(unsigned long ms_timeout) { wait_timeout = ms_timeout; }
