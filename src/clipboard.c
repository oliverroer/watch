#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>
#include <tchar.h>
#include <windows.h>

#define BUFFER_SIZE 1024

int main(int argc, char **argv) {
  int istty = isatty(fileno(stdin));

  if (istty) {
    return paste();
  } else {
    return copy();
  }
}

void ErrorExit(LPTSTR lpszFunction) {
  // Retrieve the system error message for the last-error code

  LPVOID lpMsgBuf;
  LPVOID lpDisplayBuf;
  DWORD dw = GetLastError();

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&lpMsgBuf, 0, NULL);

  // Display the error message and exit the process

  lpDisplayBuf = (LPVOID)LocalAlloc(
      LMEM_ZEROINIT,
      (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) *
          sizeof(TCHAR));
  StringCchPrintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                  TEXT("%s failed with error %d: %s"), lpszFunction, dw,
                  lpMsgBuf);
  MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

  LocalFree(lpMsgBuf);
  LocalFree(lpDisplayBuf);
  ExitProcess(dw);
}

int paste() {
  bool isAvailable = IsClipboardFormatAvailable(CF_BITMAP);
  if (!isAvailable) {
    return 1;
  }

  int opened = OpenClipboard(NULL);
  if (!opened) {
    return 1;
  }

  HANDLE hData = GetClipboardData(CF_BITMAP);
  if (hData == NULL) {
    return 1;
  }

  if (hData != NULL) {
    LPVOID ptr = GlobalLock(hData);
    DWORD err = GetLastError();
    ErrorExit("FOO");
    //if (lptstr != NULL) {
    //  size_t slen = strlen(lptstr);
    //  fwrite(lptstr, sizeof(char), slen, stdout);
    //  GlobalUnlock(hData);
    //}
  }

  int closed = CloseClipboard();

  return 0;
}

int copy() {
  char *text = (char *)malloc(BUFFER_SIZE);

  size_t length = fread(text, sizeof(char), BUFFER_SIZE, stdin);

  int opened = OpenClipboard(NULL);
  if (!opened) {
    return 1;
  }

  int emptied = EmptyClipboard();
  if (!emptied) {
    return 1;
  }

  size_t size = (length + 1) * sizeof(char);

  void *hMem = GlobalAlloc(GMEM_MOVEABLE, size);
  if (hMem == NULL) {
    CloseClipboard();
    return 1;
  }

  char *lptstrCopy = GlobalLock(hMem);
  memcpy(lptstrCopy, text, length * sizeof(char));
  lptstrCopy[length] = 0;
  GlobalUnlock(hMem);

  unsigned int uFormat = CF_TEXT;
  SetClipboardData(CF_TEXT, hMem);
  SetClipboardData(CF_BITMAP, hMem);

  int closed = CloseClipboard();

  return 0;
}
