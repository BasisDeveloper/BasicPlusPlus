#include "Basic++/Sys.hpp"
#include "Basic++/Result.hxx"
#include "Basic++/Common.hxx"
#include "Basic++/win32/Win32.hpp"
#include "Basic++/dbg.hxx"

#define STRSAFE_NO_DEPRECATE
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>

#include <cstdlib>
#include <cstring>

static HANDLE global_job_object = nullptr;

struct Win32Pipe
{
    HANDLE READ = nullptr;
    HANDLE WRITE = nullptr;
};

// Some info explaining what this does
//       : https://stackoverflow.com/questions/53208/how-do-i-automatically-destroy-child-processes-in-windows
void Setup_Global_Job_Handle()
{
    static bool has_ran_before = false;

    EXPECT(has_ran_before == false,
        "{} has already been invoked before, and must be only invoked ONCE.", stringify(Setup_Global_Job_Handle));

    global_job_object = CreateJobObjectA(nullptr, nullptr); // GLOBAL

    WIN32_CHECK(global_job_object != INVALID_HANDLE_VALUE);

    WIN32_CHECK(global_job_object != nullptr);

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION extended_job_information = {};

    // Configure all child processes associated with the job to terminate when the
    extended_job_information.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

    if (0 == SetInformationJobObject(global_job_object,
        JobObjectExtendedLimitInformation,
        &extended_job_information,
        sizeof(extended_job_information)))
    {
        Basic::win32::Error_Exit("SetInformationJobObject");
    }

    has_ran_before = true;
}

Basic::Sys::ShellExecuteResult Basic::Sys::Shell_Execute_Write_Then_Read(
    const std::string& command,
    const std::string_view& arguments,
    bool wait_for_process_exit_before_read)
{
    if (global_job_object == nullptr)
    {
        Setup_Global_Job_Handle();
    }

    SECURITY_ATTRIBUTES security_attributes{
            .nLength = sizeof(SECURITY_ATTRIBUTES),
            .lpSecurityDescriptor = nullptr,
            .bInheritHandle = true
    };

    /* This is the pipe that the child process will send its stdout, and in our case, child_pipe_OUT. */
    Win32Pipe child_pipe_OUT{};
    /* This is the pipe that WE use to write to data from the child process. */
    Win32Pipe child_pipe_IN{};
    /* This is the pipe that WE use to get the stderr stream data from the child process. */
    Win32Pipe child_pipe_ERR{};

    // Creating a pipe for the child process stdout
    WIN32_CHECK(CreatePipe(&child_pipe_OUT.READ, &child_pipe_OUT.WRITE, &security_attributes, 0));
    WIN32_CHECK(SetHandleInformation(child_pipe_OUT.READ, HANDLE_FLAG_INHERIT, FALSE));

    // Creating a pipe for the child process stderr
    WIN32_CHECK(CreatePipe(&child_pipe_ERR.READ, &child_pipe_ERR.WRITE, &security_attributes, 0));
    WIN32_CHECK(SetHandleInformation(child_pipe_ERR.READ, HANDLE_FLAG_INHERIT, FALSE));

    // Creating a pipe for the child process stdin
    WIN32_CHECK(CreatePipe(&child_pipe_IN.READ, &child_pipe_IN.WRITE, &security_attributes, 0));
    WIN32_CHECK(SetHandleInformation(child_pipe_IN.WRITE, HANDLE_FLAG_INHERIT, FALSE));

    PROCESS_INFORMATION process_info{};

    STARTUPINFOA startup_info{};
    startup_info.cb = sizeof(STARTUPINFO);
    startup_info.hStdError = child_pipe_ERR.WRITE;
    startup_info.hStdOutput = child_pipe_OUT.WRITE;
    startup_info.hStdInput = child_pipe_IN.READ;
    startup_info.dwFlags |= STARTF_USESTDHANDLES;

    constexpr size_t MAX_COMMAND_LENGTH = 256;

    char szCmdline[MAX_COMMAND_LENGTH]{};

    std::strcpy(szCmdline, command.c_str());

    WIN32_CHECK(CreateProcessA(nullptr,
        szCmdline,     // command line
        nullptr,          // process security attributes
        nullptr,          // primary thread security attributes
        TRUE,          // handles are inherited
        0,             // creation flags
        nullptr,          // use parent's environment
        nullptr,          // use parent's current directory
        &startup_info,  // STARTUPINFO pointer
        &process_info));

    WIN32_CHECK(WaitForInputIdle(process_info.hProcess, WAIT_FAILED));

    /* Close handles to the child process and its primary thread.
      Some applications might keep these handles to monitor the status
      of the child process, for example. But since we don't use them,
      we can close them. */
    WIN32_CHECK(CloseHandle(process_info.hThread));

    // TODO: Adding Processes to job objects should be easier. AKA, the code should be self-explanatory.
    WIN32_CHECK(global_job_object != 0 and AssignProcessToJobObject(global_job_object, process_info.hProcess));

    constexpr size_t BUFFER_SIZE = 1000;
    std::vector<CHAR> buf;
    buf.reserve(BUFFER_SIZE);
    BOOL bSuccess = FALSE;
    DWORD amount_read{}, amount_written{};
    DWORD bytes_available = 0;

    /* WE, OUR process doesn't use the write-end of the child_pipe_OUT pipe, since we've already created the child process
    with the child_pipe_OUT.WRITE open, we can safely close ours because they already have theirs */
    WIN32_CHECK(CloseHandle(child_pipe_OUT.WRITE));
    /* Same reasoning as ↑, this time with child_pipe_ERR */
    WIN32_CHECK(CloseHandle(child_pipe_ERR.WRITE));

    WIN32_CHECK(WriteFile(child_pipe_IN.WRITE, arguments.data(), arguments.length(), &amount_written, NULL));

    /* Since we're done writing data to the child pipe, we MUST close the child_pipe_IN.WRITE to let the child process know what we're done.
        If we don't, the child will indefinitely wait, forever, blocking our process */
    WIN32_CHECK(CloseHandle(child_pipe_IN.WRITE));

    if (wait_for_process_exit_before_read)
    {
        WIN32_CHECK(WaitForSingleObject(process_info.hProcess, INFINITE) != WAIT_FAILED);
    }

    std::string std_out;
    std::string std_err;

    while (PeekNamedPipe(child_pipe_OUT.READ,
        nullptr,
        0,
        nullptr,
        &bytes_available,
        nullptr))
    {
        if (bytes_available > 0)
        {
            /* HACK: This feels, strange, kinda hacky. I'm pretty sure that every time we resize, it'll allocate memory, and
             that's very inefficient to do, because we'd want to allocate each allocation with the algorithm,
             current_size * 2, so we avoid frequent and small relocations. At least in this way, we can read as much memory
             as we want without having to worry about freeing, deleting or manually resizing anything. std::vector
             helps us out. But I feel like we don't actually need it, or at least it needs to be used this way. */
            buf.resize(buf.size() + bytes_available);

            bSuccess = ReadFile(child_pipe_OUT.READ, buf.data(), bytes_available, &amount_read, NULL);

            if (!bSuccess || amount_read <= 0) win32::Error_Exit("ReadFile");
        }
    }

    std_out = std::string(buf.data(), buf.size());

    // Getting read to read stdout
    buf.clear();

    while (PeekNamedPipe(child_pipe_ERR.READ,
        nullptr,
        0,
        nullptr,
        &bytes_available,
        nullptr))
    {
        if (bytes_available > 0)
        {
            buf.resize(buf.size() + bytes_available); // HACK: ⬆️ Look up there for reason.

            bSuccess = ReadFile(child_pipe_ERR.READ, buf.data(), bytes_available, &amount_read, nullptr);

            if (!bSuccess || amount_read <= 0) win32::Error_Exit("ReadFile");
        }
    }

    std_err = std::string(buf.data(), buf.size());

    size_t exit_code{};

    WIN32_CHECK(GetExitCodeProcess(process_info.hProcess, (DWORD*)&exit_code));

    if (exit_code == STILL_ACTIVE)
    {
        exit_code = Sys::INVALID_EXIT_CODE;
    }

    WIN32_CHECK(CloseHandle(process_info.hProcess));

    return ShellExecuteResult{ .out = std_out, .err = std_err, .exit_code = exit_code };
}

Basic::Sys::ShellExecuteResult Basic::Sys::Shell_Execute_Write_Then_Read(
    const std::string& command,
    bool wait_for_process_exit_before_read)
{
    return Basic::Sys::Shell_Execute_Write_Then_Read(
        command,
        "",
        wait_for_process_exit_before_read
    );
}
