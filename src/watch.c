/* TODO
 *
 * Double Buffering. Look at
 * https://gist.github.com/enghqii/c8711c5f04d3f3d2f8f2
 *
 * "watch type ..\hello.txt" works
 * "watch type ../hello.txt" doesn't
 * figure out why
 */

#include <stdbool.h>
#include <stdio.h>
#include <strsafe.h>
#include <tchar.h>
#include <windows.h>

#define CHUNK_SIZE (256 * sizeof(char))

// Command version.
#define VERSION "0.1.0"

// Default interval in seconds.
#define DEFAULT_INTERVAL 2

// Output command usage.
void ExitWithUsage() {
  printf(
      "\n"
      "Usage:\n"
      " watch [options] command\n"
      "\n"
      "Options:\n"
      "  -b, --beep             beep if command has a non-zero exit\n"
      //     "  -c, --color       interpret ANSI color and style sequences\n"
      //     "  -d, --differences[=<permanent>]\n" "
      //     "                    highlight changes between updates\n"
      "  -e, --errexit          exit if command has a non-zero exit\n"
      //"  -g, --chgexit          exit when output from command changes\n"
      "  -n, --interval <secs>  seconds to wait between updates\n"
      //    "  -p, --precise      attempt run command in precise intervals\n"
      "  -t, --no-title         turn off header\n"
      //    "  -x,
      "  --cmd                  pass command to \"cmd /C\" instead of \"PowerShell\" -Command\"\n"
      "\n"
      " -h, --help     display this help and exit\n"
      " -v, --version  output version information and exit\n");

  exit(1);
}

// Check if `arg` is the given short-opt or long-opt.
bool IsOption(char* short_name, char* long_name, char* arg) {
  if (short_name != NULL && !strcmp(short_name, arg)) {
    return true;
  }

  if (long_name != NULL && !strcmp(long_name, arg)) {
    return true;
  }

  return false;
}

// Return the total string-length consumed by `strings`.
int Length(char** strings) {
  int length = 0;
  char* string;
  while ((string = *strings++)) {
    length += strlen(string);
  }
  return length + 1;
}

char* BuildCommandLine(char* prefix, int prefix_length, char** strings,
                       int string_count) {
  size_t count = prefix_length + Length(strings) + string_count;
  size_t size = sizeof(char);
  char* buffer = (char*)calloc(count, size);
  char* string;

  strcat(buffer, prefix);
  while ((string = *strings++)) {
    strcat(buffer, " ");
    strcat(buffer, string);
  }
  return buffer;
}

// Format a readable error message, print it to stderr
// and exit from the application.
void ExitWithError(char* function_name) {
  unsigned long message_id = GetLastError();
  char* message_buffer;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, message_id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)&message_buffer, 0, NULL);

  fprintf(stderr, "%s failed with error %d: %s", function_name, message_id,
          message_buffer);
  LocalFree(message_buffer);
  ExitProcess(1);
}

// Create a child process with redirected output.
// The returned HANDLE can be used to read this output.
HANDLE
ExecuteCommand(char* command_line, PROCESS_INFORMATION* process_information) {
  // Determines whether the returned handle can be inherited by child processes
  SECURITY_ATTRIBUTES security_attributes;

  // The size, in bytes, of this structure.
  security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);

  // Assign the default security descriptor associated with the access token of
  // the calling process.
  security_attributes.lpSecurityDescriptor = NULL;

  // The new process inherits the handle.
  security_attributes.bInheritHandle = TRUE;

  // The read handle for the pipe.
  HANDLE read_handle = NULL;

  // The write handle for the pipe.
  HANDLE write_handle = NULL;

  if (!CreatePipe(&read_handle, &write_handle, &security_attributes, 0)) {
    ExitWithError("CreatePipe");
  }

  // Ensure the read handle to the pipe for STDOUT is not inherited.
  if (!SetHandleInformation(read_handle, HANDLE_FLAG_INHERIT, 0)) {
    ExitWithError("SetHandleInformation");
  }

  // Set up members of the STARTUPINFO structure.
  // This structure specifies the STDOUT and STDERR handles for redirection.
  STARTUPINFO startup_info;
  ZeroMemory(&startup_info, sizeof(STARTUPINFO));
  startup_info.cb = sizeof(STARTUPINFO);
  startup_info.hStdError = write_handle;
  startup_info.hStdOutput = write_handle;
  startup_info.dwFlags |= STARTF_USESTDHANDLES;

  // Set up members of the PROCESS_INFORMATION structure.
  ZeroMemory(process_information, sizeof(PROCESS_INFORMATION));

  // Create the child process.
  bool success = FALSE;

  if (!CreateProcess(NULL,
                     command_line,        // command line
                     NULL,                // process security attributes
                     NULL,                // primary thread security attributes
                     TRUE,                // handles are inherited
                     0,                   // creation flags
                     NULL,                // use parent's environment
                     NULL,                // use parent's current directory
                     &startup_info,       // STARTUPINFO pointer
                     process_information  // receives PROCESS_INFORMATION
                     )) {
    // If an error occurs, exit the application.
    ExitWithError("CreateProcess");
  }

  CloseHandle(write_handle);

  return read_handle;
}

/* Standard error macro for reporting API errors */
#define PERR(bSuccess, api)                                                \
  {                                                                        \
    if (!(bSuccess))                                                       \
      printf("%s:Error %d from %s on line %d\n", __FILE__, GetLastError(), \
             api, __LINE__);                                               \
  }

// https://support.microsoft.com/en-ca/help/99261/how-to-performing-clear-screen-cls-in-a-console-application
void cls() {
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  COORD coordScreen = {0, 0}; /* here's where we'll home the cursor */
  BOOL bSuccess;
  DWORD cCharsWritten;
  CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
  DWORD dwConSize;                 /* number of character cells in
                                                    the current buffer */

  /* get the number of character cells in the current buffer */

  bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
  PERR(bSuccess, "GetConsoleScreenBufferInfo");
  dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

  /* fill the entire screen with blanks */

  bSuccess = FillConsoleOutputCharacter(hConsole, (TCHAR)' ', dwConSize,
                                        coordScreen, &cCharsWritten);
  PERR(bSuccess, "FillConsoleOutputCharacter");

  /* get the current text attribute */

  bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
  PERR(bSuccess, "ConsoleScreenBufferInfo");

  /* now set the buffer's attributes accordingly */

  bSuccess = FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize,
                                        coordScreen, &cCharsWritten);
  PERR(bSuccess, "FillConsoleOutputAttribute");

  /* put the cursor at (0, 0) */

  bSuccess = SetConsoleCursorPosition(hConsole, coordScreen);
  PERR(bSuccess, "SetConsoleCursorPosition");
  return;
}

typedef struct {
  char* text;
  size_t size;
  size_t used;
} text_buffer;

void ReadText(HANDLE read_handle, text_buffer* buffer) {
  if (buffer->text == NULL) {
    buffer->size = CHUNK_SIZE;
    buffer->text = (char*)malloc(buffer->size);
  }

  unsigned long bytes_read;
  buffer->used = 0;

  bool success = FALSE;

  for (;;) {
    if (buffer->used == buffer->size) {
      buffer->size += CHUNK_SIZE;
      buffer->text = (char*)realloc(buffer->text, buffer->size);
    }

    size_t bytes_to_read = buffer->size - buffer->used;
    success = ReadFile(read_handle, buffer->text + buffer->used, bytes_to_read,
                       &bytes_read, NULL);

    buffer->used += bytes_read;

    if (!success || bytes_read == 0) {
      return;
    }
  }
}

void WriteText(HANDLE write_handle, text_buffer* buffer) {
  WriteFile(write_handle, buffer->text, buffer->used, NULL, NULL);
}

int main(int argc, char** argv) {
  unsigned int interval = DEFAULT_INTERVAL;
  bool halt = false;
  bool use_cmd = false;
  bool beep = false;
  bool show_title = true;

  for (int i = 1; i < argc; ++i) {
    char* arg = argv[i];

    // -h, --help
    if (IsOption("-h", "--help", arg)) {
      ExitWithUsage();
    }

    // --cmd
    if (IsOption(NULL, "--cmd", arg)) {
      use_cmd = true;
      continue;
    }

    // -t, --no-title
    if (IsOption("-t", "--no-title", arg)) {
      show_title = false;
      continue;
    }

    // -b, --beep
    if (IsOption("-b", "--beep", arg)) {
      beep = true;
      continue;
    }

    // -e, --errexit
    if (IsOption("-e", "--errexit", arg)) {
      halt = true;
      continue;
    }

    // -v, --version
    if (IsOption("-v", "--version", arg)) {
      printf("%s\n", VERSION);
      exit(1);
    }

    // -n, --interval <secs>
    if (IsOption("-n", "--interval", arg)) {
      if (++i == argc) {
        fprintf(stderr, "\n  --interval requires an argument\n\n");
        exit(1);
      }

      arg = argv[i];
      // seconds
      interval = strtoul(arg, NULL, 10);
      continue;
    }

    // not a flag
    argc -= i;
    // <cmd>
    if (!argc) {
      fprintf(stderr, "\n  <cmd> required\n\n");
      exit(1);
    }

    argv += i;
    break;
  }

  char* prefix_cmd = "C:\\Windows\\System32\\cmd.exe /C";
  char* prefix_powershell =
      "C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe -Command";
  char* prefix = use_cmd ? prefix_cmd : prefix_powershell;

  int prefix_length = strlen(prefix);

  char* command_line = BuildCommandLine(prefix, prefix_length, argv, argc);
  char* command = command_line + prefix_length + 1;

  int sleepMillis = interval * 1000;

  text_buffer buffer;
  buffer.text = NULL;
  buffer.size = 0;
  buffer.used = 0;

  PROCESS_INFORMATION process_information;
  HANDLE read_handle, write_handle;

  unsigned long exit_code;

  for (;;) {
    read_handle = ExecuteCommand(command_line, &process_information);
    write_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    ReadText(read_handle, &buffer);

    system("cls");
    if (show_title) {
      printf("Every %ds: %s\n\n", interval, command);
    }

    WriteText(write_handle, &buffer);

    GetExitCodeProcess(process_information.hProcess, &exit_code);

    // Close handles to the child process and its primary thread.
    // Some applications might keep these handles to monitor the status
    // of the child process, for example.
    CloseHandle(process_information.hProcess);
    CloseHandle(process_information.hThread);

    if (exit_code != 0) {
      if (beep) {
        printf("\a");
      }

      if (halt) {
        printf("command exit with a non-zero status, press a key to exit");
        getch();
        return 0;
      }
    }

    Sleep(sleepMillis);
  }

  return 0;
}
