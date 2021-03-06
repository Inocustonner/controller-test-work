#define _CRT_SECURE_NO_WARNINGS

#include "PipeQueueSystem.hpp"
#include "SafeQueue.hpp"

#include <string_view>
#include <thread>

#include <windows.h>

static SafeQueue<std::wstring> *global_queue;

extern HMODULE g_module;

static std::thread runner;
static ULONG wait_timeout = 5000;
static bool finish = false;

extern "C" 
volatile unsigned long created_process_pid;

static bool* logger_enabled;
static SLogger* logger;

static void _log(const char* format, ...) {
  if (*logger_enabled) {
    va_list args;
    va_start(args, format);

    int buf_size = vsnprintf(nullptr, 0, format, args);

    std::string line;
    line.resize(buf_size, 0);
    vsnprintf(line.data(), line.size() + 1, format, args);

    logger->log(line);
    va_end(args);
  }
}

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

static DWORD createProc(std::wstring_view cmd) {
  STARTUPINFOW si = {};
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;

  PROCESS_INFORMATION pi = {};

  BOOL succ = CreateProcessW(NULL, (LPWSTR)cmd.data(), NULL, NULL, FALSE,
                             CREATE_SUSPENDED, NULL, NULL, &si, &pi);
  created_process_pid = pi.dwProcessId;
  ResumeThread(pi.hThread);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  if (!succ) {
    return -1;
  }

  return pi.dwProcessId;
}

static void _run() {
  while (true) {
    std::wstring cmd;
    bool not_empty = global_queue->ConsumeSync(cmd);

    if (finish)
      break;

    if (!not_empty)
      continue;

    created_process_pid = -1;
    HHOOK h = SetWindowsHookExA(WH_CBT, CBTProc, g_module, 0);
    _log("Attempting to start proc from %ls", cmd.c_str());
    DWORD proc_id= createProc(cmd);
    _log("id(%d) Started proc from %ls", proc_id, cmd.c_str());
    if (proc_id == -1)
      continue;
		HANDLE proc_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, proc_id);
    DWORD r = WaitForSingleObject(proc_handle, wait_timeout);
    UnhookWindowsHookEx(h);
    if (r == WAIT_TIMEOUT) {
      _log("id(%d) Manually shutting down proc ", proc_id);
      //char buffer[50] = {};
      //sprintf(buffer, "taskkill /F /PID %d", created_process_pid);
      //system(buffer);
      TerminateProcess(proc_handle, 0x0);
      WaitForSingleObject(proc_handle, wait_timeout); // wait for termination
    }
    _log("id(%d) proc has finished", proc_id);
    CloseHandle(proc_handle);
  }
}

void start_pipe_queue(bool& le, SLogger& lg) {
  if (global_queue) stop_pipe_queue();

  logger_enabled = &le;
  logger = &lg;

  global_queue = new SafeQueue<std::wstring>{};
  finish = false;
  runner = std::thread(_run); 
}

void stop_pipe_queue() {
  // may hang if at the time of finish thread is not wating
  finish = true;
  global_queue->Finish();
  runner.join();
  delete global_queue;
  global_queue = nullptr;
}

void pipe_push_cmd(std::wstring cmd) { 
  _log("Pushing to queue %ls", cmd.c_str());
  global_queue->Provide(std::move(cmd)); 
}

void pipe_set_timeout(unsigned long ms_timeout) { wait_timeout = ms_timeout; }
