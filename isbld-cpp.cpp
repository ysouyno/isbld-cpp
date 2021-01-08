// isbld-cpp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <string>
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

#define MAX_BUFFER 1024
#define SEC0 L"ALLIN1"
#define SEC0_IS L"installshield"
#define SEC0_PROJ_HOME L"project_home"
#define SEC0_PROJ_NAME L"project_name"
#define SEC0_RAR L"winrar"

struct PARAM
{
    std::wstring compiler;
    std::wstring rulfiles;
    std::wstring libraries;
    std::wstring linkpaths;
    std::wstring includeifx;
    std::wstring includeisrt;
    std::wstring includescript;
    std::wstring definitions;
    std::wstring switches;
    std::wstring builder;
    std::wstring installproject;
    std::wstring disk1;
    std::wstring winrar;

    void print_member()
    {
        printf("%S\n%S\n%S\n%S\n%S\n%S\n%S\n%S\n%S\n%S\n%S\n%S\n%S\n",
            compiler.c_str(),
            rulfiles.c_str(),
            libraries.c_str(),
            linkpaths.c_str(),
            includeifx.c_str(),
            includeisrt.c_str(),
            includescript.c_str(),
            definitions.c_str(),
            switches.c_str(),
            builder.c_str(),
            installproject.c_str(),
            disk1.c_str(),
            winrar.c_str()
        );
    }
};

void remove_last_back_slash(std::wstring& path)
{
    std::size_t pos = path.find_last_of(L"\\");
    if (path.length() - 1 == pos)
        path = path.substr(0, pos);
}

void get_current_dir(std::wstring& out)
{
    WCHAR path[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, path, MAX_PATH);

    out = path;
    out = out.substr(0, out.find_last_of(L"\\"));
}

bool query_app_path(const std::wstring& id, const std::wstring& valuename, REGSAM sam_desired, std::wstring& out)
{
    bool ret = false;
    HKEY hkey;
    DWORD dwtype = REG_SZ;
    DWORD dwsize = 0;
    WCHAR sz[MAX_PATH] = { 0 };
    std::wstring subkey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + id;

    LSTATUS lstatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE, subkey.c_str(), 0, KEY_READ | sam_desired, &hkey);
    if (ERROR_SUCCESS != lstatus)
    {
        printf("RegOpenKeyExW failed\n");
        return ret;
    }

    lstatus = RegQueryValueEx(hkey, valuename.c_str(), 0, &dwtype, NULL, &dwsize);
    if (ERROR_SUCCESS == lstatus)
    {
        lstatus = RegQueryValueEx(hkey, valuename.c_str(), 0, &dwtype, (LPBYTE)&sz, &dwsize);
        if (ERROR_SUCCESS == lstatus)
        {
            out = sz;
            remove_last_back_slash(out);
            ret = true;
        }
    }

    RegCloseKey(hkey);
    return ret;
}

bool gen_winrar_script(const std::wstring& curr)
{
    printf("gen_winrar_script()\n");

    std::wstring script_txt = curr + L"\\Script.txt";
    if (PathFileExistsW(script_txt.c_str()))
        return true;

    // Create Script.txt for WinRAR
    FILE* f = NULL;
    errno_t err = _wfopen_s(&f, script_txt.c_str(), L"w+");
    if (NULL == f)
    {
        printf("_wfopen_s failed\n");
        return false;
    }

    std::string script = "Title=Install\nSilent=1\nOverwrite=1\nSetup=Setup.exe\nTempMode";

    fwrite(script.c_str(), script.length(), 1, f);
    fclose(f);

    return true;
}

bool config_ok(std::wstring& config, std::wstring& is, std::wstring& winrar)
{
    WCHAR path[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, path, MAX_PATH);

    config = path;
    config = config.substr(0, config.find_last_of(L".")) + L".ini";

    std::wstring curr;
    get_current_dir(curr);

    if (PathFileExistsW(config.c_str()))
    {
        // Do some checks on the validity of the file content
        WCHAR installshield[MAX_BUFFER] = { 0 };
        WCHAR project_home[MAX_BUFFER] = { 0 };
        WCHAR project_name[MAX_BUFFER] = { 0 };
        WCHAR winrar[MAX_BUFFER] = { 0 };

        GetPrivateProfileStringW(SEC0, SEC0_IS, L"", installshield, MAX_BUFFER, config.c_str());
        if (!PathFileExistsW(installshield))
        {
            printf("%S cannot be found\n", installshield);
            return false;
        }

        GetPrivateProfileStringW(SEC0, SEC0_PROJ_NAME, L"", project_name, MAX_BUFFER, config.c_str());
        if (!wcslen(project_name))
        {
            printf("%S cannot be empty\n", SEC0_PROJ_NAME);
            return false;
        }

        GetPrivateProfileStringW(SEC0, SEC0_PROJ_HOME, L"", project_home, MAX_BUFFER, config.c_str());
        if (!wcslen(project_home))
        {
            // Use current directory and check *.ism if exists
            std::wstring project_ism = curr + L"\\" + project_name;
            if (!PathFileExistsW(project_ism.c_str()))
            {
                printf("%S cannot be found in %S\n", project_ism.c_str(), curr.c_str());
                return false;
            }
        }

        GetPrivateProfileStringW(SEC0, SEC0_RAR, L"", winrar, MAX_BUFFER, config.c_str());
        if (PathFileExistsW(winrar))
        {
            gen_winrar_script(curr);
        }

        return true;
    }

    bool ret = query_app_path(L"{BE32FC13-21DA-4BE0-9E8F-25685F2DC6B5}",
        L"InstallLocation", KEY_WOW64_32KEY, is);
    if (!ret)
    {
        printf("InstallShield cannot be found\n");
        return false;
    }
    printf("InstallShield found in: \"%S\"\n", is.c_str());

    ret = query_app_path(L"WinRAR archiver",
        L"DisplayIcon", KEY_WOW64_32KEY, winrar);
    if (!ret)
    {
        ret = query_app_path(L"WinRAR archiver",
            L"DisplayIcon", KEY_WOW64_64KEY, winrar);
        if (!ret)
        {
            printf("WinRAR cannot be found\n");
            // return false;
        }
    }

    if (!winrar.empty())
        gen_winrar_script(curr);

    // Create a blank file
    FILE* f = NULL;
    errno_t err = _wfopen_s(&f, config.c_str(), L"w+");
    if (NULL == f)
    {
        printf("_wfopen_s failed\n");
        return false;
    }
    fclose(f);

    WritePrivateProfileStringW(SEC0, SEC0_IS, is.c_str(), config.c_str());
    WritePrivateProfileStringW(SEC0, SEC0_PROJ_HOME, L"", config.c_str());
    WritePrivateProfileStringW(SEC0, SEC0_PROJ_NAME, L"Your Project Name.ism", config.c_str());
    WritePrivateProfileStringW(SEC0, SEC0_RAR, winrar.c_str(), config.c_str());

    return true;
}

bool fill_param(const std::wstring& config, PARAM& param)
{
    bool ret = false;
    WCHAR installshield[MAX_BUFFER] = { 0 };
    WCHAR project_home[MAX_BUFFER] = { 0 };
    WCHAR project_name[MAX_BUFFER] = { 0 };
    WCHAR winrar[MAX_BUFFER] = { 0 };

    GetPrivateProfileStringW(SEC0, SEC0_IS, L"", installshield, MAX_BUFFER, config.c_str());
    GetPrivateProfileStringW(SEC0, SEC0_PROJ_HOME, L"", project_home, MAX_BUFFER, config.c_str());
    GetPrivateProfileStringW(SEC0, SEC0_PROJ_NAME, L"", project_name, MAX_BUFFER, config.c_str());
    GetPrivateProfileStringW(SEC0, SEC0_RAR, L"", winrar, MAX_BUFFER, config.c_str());

    param.compiler = installshield;
    param.compiler += L"\\System\\Compile.exe";

    std::wstring curr;
    get_current_dir(curr);
    printf("%S\n", curr.c_str());

    param.rulfiles = curr;
    param.rulfiles += L"\\Script Files\\Setup.rul";
    if (!PathFileExistsW(param.rulfiles.c_str()))
    {
        printf("Setup.rul cannot be found\n");
        return false;
    }

    param.libraries = L"\"isrt.obl\" \"ifx.obl\"";

    param.linkpaths = L"-LibPath\"";
    param.linkpaths += installshield;
    param.linkpaths += L"\\Script\\Ifx\\Lib\" -LibPath\"";
    param.linkpaths += installshield;
    param.linkpaths += L"\\Script\\Isrt\\Lib\"";

    param.includeifx = installshield;
    param.includeifx += L"\\Script\\Ifx\\Include";

    param.includeisrt = installshield;
    param.includeisrt += L"\\Script\\Isrt\\Include";

    param.includescript = curr;
    param.includescript += L"\\Script Files";

    param.definitions = L"";

    param.switches = L"-w50 -e50 -v3 -g";

    param.builder = installshield;
    param.builder += L"\\System\\ISCmdBld.exe";

    param.installproject = curr;
    param.installproject += L"\\";
    param.installproject += project_name;
    if (!PathFileExistsW(param.installproject.c_str()))
    {
        printf("%S cannot be found\n", param.installproject.c_str());
        return false;
    }

    param.disk1 = curr;
    param.disk1 += L"\\Media\\EIOSetup_SCH\\Disk Images\\Disk1";

    param.winrar = winrar;

    return true;
}

bool run_cmd(const WCHAR* command)
{
    PROCESS_INFORMATION pi = { 0 };
    STARTUPINFO si = { 0 };
    si.cb = sizeof(si);
    DWORD dwexit = 0;

    BOOL result = CreateProcessW(NULL, (LPWSTR)command, NULL, NULL, FALSE,
        NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
    if (result)
    {
        WaitForSingleObject(pi.hProcess, INFINITE);

        GetExitCodeProcess(pi.hProcess, &dwexit);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    return (0 == dwexit ? true : false);
}

bool compile(const PARAM& param)
{
    std::wstring cmd = L"\"";
    cmd += param.compiler;
    cmd += L"\" \"";
    cmd += param.rulfiles;
    cmd += L"\" ";
    cmd += param.libraries;
    cmd += L" ";
    cmd += param.linkpaths;
    cmd += L" -I\"";
    cmd += param.includeifx;
    cmd += L"\" -I\"";
    cmd += param.includeisrt;
    cmd += L"\" -I\"";
    cmd += param.includescript;
    cmd += L"\" ";
    cmd += param.definitions;
    cmd += L" ";
    cmd += param.switches;
    cmd += L" -dISINCLUDE_NO_WINAPI_H";

    return run_cmd(cmd.c_str());
}

bool build(const PARAM& param)
{
    std::wstring cmd = L"\"";
    cmd += param.builder;
    cmd += L"\" -p \"";
    cmd += param.installproject;
    cmd += L"\"";

    return run_cmd(cmd.c_str());
}

bool compress_allin1(const PARAM& param)
{
    std::wstring curr;
    get_current_dir(curr);

    std::wstring script_txt = curr + L"\\Script.txt";
    if (!PathFileExistsW(param.winrar.c_str()))
        return false;

    std::wstring cmd = L"\"";
    cmd += param.winrar;
    cmd += L"\" a \"";
    cmd += param.disk1;
    cmd += L".exe\" \"";
    cmd += param.disk1;
    cmd += L"\\*.*\" -ep -sfx";
    if (PathFileExistsW(script_txt.c_str()))
    {
        cmd += L" -z\"";
        cmd += script_txt;
        cmd += L"\"";
    }

    return run_cmd(cmd.c_str());
}

int main()
{
    std::wstring config, is, winrar;
    if (!config_ok(config, is, winrar))
    {
        printf("NO config file found\n");
        return -1;
    }

    PARAM param;
    if (!fill_param(config, param))
    {
        printf("fill param failed\n");
        return -1;
    }
    param.print_member();

    if (!compile(param))
    {
        printf("compile failed\n");
        return -1;
    }

    if (!build(param))
    {
        printf("build failed\n");
        return -1;
    }

    if (!compress_allin1(param))
    {
        printf("compress_allin1 failed\n");
        return -1;
    }

    return 0;
}
