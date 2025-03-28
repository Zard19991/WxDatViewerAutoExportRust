﻿#include "pch.h"
#include "./main.hpp"
#include <Psapi.h>
#include "./tray.hpp"
#include "./registr.hpp"
#include "./text.hpp"
#include "./usb.hpp"
#include "./window.hpp"
#include "./process_lib.hpp"
#include "./json.hpp"
#include <thread>
using json = nlohmann::json;

#define HMC_CHECK_CATCH catch (char *err){};
HWND winmian = NULL;

namespace Mutex
{
    map<string, HANDLE> AllMutexHandles;
    /**
     * @brief 创建互斥体
     *
     * @param MutexName
     * @return true
     * @return false
     */
    bool create(string MutexName)
    {
        bool has_mut_exist = false;

        HANDLE hMutex = CreateMutexA(NULL, FALSE, MutexName.c_str());

        AllMutexHandles.insert(pair<string, HANDLE>(MutexName, hMutex));

        if (hMutex == NULL)
        {
            has_mut_exist = true;
        }
        // 检查是否已经存在同名的互斥体
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            has_mut_exist = true;
            CloseHandle(hMutex);
        }

        return !has_mut_exist;
    }

    /**
     * @brief 判断是否有这个互斥体
     *
     * @param MutexName
     * @return true
     * @return false
     */
    bool has(string MutexName)
    {
        bool has_mut_exist = true;

        HANDLE hMutex;

        hMutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, MutexName.c_str());
        if (NULL == hMutex)
        {
            has_mut_exist = false;
        }

        CloseHandle(hMutex);
        return has_mut_exist;
    }

    /**
     * @brief 删除通过此方法创建的互斥体
     *
     * @param MutexName
     * @return true
     * @return false
     */
    bool remove(string MutexName)
    {

        auto it = AllMutexHandles.find(MutexName);
        if (it == AllMutexHandles.end())
        {
            return false;
        }
        while (it != AllMutexHandles.end())
        {
            CloseHandle(it->second);
            it++;
        }

        if (!has(MutexName))
        {
            AllMutexHandles.erase(MutexName);
            return true;
        }
        return false;
    }

    /**
     * @brief 获取当前已经创建的互斥体内容
     *
     * @return vector<string>
     */
    vector<string> list()
    {
        vector<string> list;
        map<string, HANDLE>::iterator it = AllMutexHandles.begin();

        while (it != AllMutexHandles.end())
        {
            list.push_back(it->first);
            it++;
        }
        return list;
    }
}

// 获取进程可执行文件路径
string getProcessidFilePath(int ProcessID)
{
    string Run_lpFilename = "";
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessID);
    char lpFilename[1024];
    if (hProcess == nullptr)
    {
        CloseHandle(hProcess);
        return Run_lpFilename;
    }
    GetModuleFileNameExA(hProcess, NULL, (LPSTR)lpFilename, 1024);
    CloseHandle(hProcess);
    return string(lpFilename);
}



void _setWinIconMain(long hwnds)
{
    winmian = (HWND)hwnds;

    if (!IsWindow(winmian))
    {
        return;
    }
    // setWindowTop(winmian,true);
    string execPath = getProcessidFilePath(_getpid());
    hmc_window::setWindowIcon(winmian, execPath, 0);
}

void _setWinIcon(long hwnds)
{
    HWND hwnd = (HWND)hwnds;
    if (!IsWindow(hwnd))
    {
        return;
    }
    // setWindowTop(winmian,true);
    string execPath = getProcessidFilePath(_getpid());
    hmc_window::setWindowIcon(hwnd, execPath, 0);
}

bool _isWindow(long hwnds)
{
    HWND hwnd = (HWND)hwnds;
    return ::IsWindow(hwnd);
}

void _setWindowShake(long hwnds) {
    HWND hwnd = (HWND)hwnds;
    hmc_window::setWindowShake(hwnd);
}

void _setTaskbarWin(long hwnds) {
    HWND main = HWND(hwnds);
    //hmc_window::removeWindowFrame(main);
    //hmc_window::setMoveWindow(main, -66666, -666666, 1, 1);
    MoveWindow(main, -66666, -666666, 1, 1,true);
    UpdateWindow(main);
    hmc_window::setWindowTransparent(main, 0);
    string execPath = getProcessidFilePath(_getpid());
    hmc_window::setWindowIcon(main, execPath, 0);
}

bool ikun_user_auto_disable_sync = false;
bool sync_token = false;
bool ikun_app_startup = false;
bool initializationWindowIsDisplayed = false;

bool _hasInitWindowIsDisplayed() {
    return initializationWindowIsDisplayed;
}

bool _setInitWindowIsDisplayed(bool initWindowIsDisplayed) {
    initializationWindowIsDisplayed = initWindowIsDisplayed;
    return initializationWindowIsDisplayed;
}

void _set_tray()
{
    string execPath = getProcessidFilePath(_getpid());
    hmc_window::setWindowIcon(winmian, execPath, 0);
    hmc_tray::start();
    ikun_user_auto_disable_sync = hmc_registr::hasRegistrKey(HKEY_CURRENT_USER, "SOFTWARE\\WxAutoExIm", "auto_sync");
     _hasStartup();
   
     hmc_tray::addMenuItem(hmc_tray::Menu::check("自动同步", "btn::auto_sync", ikun_user_auto_disable_sync));
     hmc_tray::addMenuItem(hmc_tray::Menu::check("开机启动", "btn::app_startup", ikun_app_startup));
     hmc_tray::addMenuItem(hmc_tray::Menu::menu("立即同步", "btn::auto_sync_token"));
     hmc_tray::addMenuItem(hmc_tray::Menu::separator("btn::separator::01"));
     hmc_tray::setMenuItmeEnable("btn::app_startup",true);
     hmc_tray::setMenuItmeEnable("btn::auto_sync", true);

    hmc_tray::addMenuItem(hmc_tray::Menu::menu("退出程序", "btn::quit_app"));

    hmc_tray::on("click", []()
                 {
            //_putenv("K9V7OKIIMR1E1_theInitializationWindowIsDisplayed=true");
            //_putenv_s("K9V7OKIIMR1E1_theInitializationWindowIsDisplayed", "true");
            initializationWindowIsDisplayed = true;

                     if (IsWindowVisible(winmian))
                     {
                         ShowWindow(winmian, 0);
                     }
                     else
                     {
                         ShowWindow(winmian, SW_RESTORE);
                         SetFocus(winmian);
                         SetActiveWindow(winmian);
                         SetForegroundWindow(winmian);
                     } });

    hmc_tray::on("btn::auto_sync", []()
                 {
            bool select = hmc_tray::getMenuItme("btn::auto_sync").select;
            ikun_user_auto_disable_sync = select;

            if (select) {
                hmc_registr::setRegistrValue(HKEY_CURRENT_USER, "SOFTWARE\\WxAutoExIm", "auto_sync", string("true"));
            }
            else {
                hmc_registr::removeRegistrValue(HKEY_CURRENT_USER, "SOFTWARE\\WxAutoExIm", "auto_sync");
            }
                 });
    hmc_tray::on("btn::auto_sync_token", []()
        {
            sync_token = true;
        });
    
    hmc_tray::on("btn::app_startup", []()
        {
            bool select = hmc_tray::getMenuItme("btn::app_startup").select;
            ikun_app_startup = select;
        });

    hmc_tray::once("btn::quit_app", []()
                   {
                       hmc_tray::close();

                       exit(0);
                   });

    hmc_tray::setTrayIcon(execPath, 0);
    Sleep(5);
    hmc_tray::setTrayIcon(execPath, 0);
}

bool _setCloseWindow(long hwnds, bool force)
{
    if (force)
    {
        CloseHandle((HWND)hwnds);
        DestroyWindow((HWND)hwnds);
    }
    return CloseWindow((HWND)hwnds);
}

bool _setShowWindows(long hwnds, bool visible)
{
    HWND hwnd = (HWND)hwnds;
    ShowWindow(hwnd, visible ? SW_RESTORE : 0);
    if (visible)
    {
        SetActiveWindow(hwnd);
        SetForegroundWindow(hwnd);
    }

    return true;
}

bool _setMinWindows(long hwnds)
{
    HWND hwnd = (HWND)hwnds;
    return ShowWindow(hwnd, SW_MINIMIZE);
}


bool _setWindowsTop(long hwnds, bool visible)
{
    return hmc_window::setWindowTop((HWND)hwnds, visible);
}

bool _createMutex(const char* MutexName)
{
    string copy_MutexName = (MutexName);
    return Mutex::create(copy_MutexName);
}

bool _hasMutex(const char* MutexName)
{
    string copy_MutexName = (MutexName);
    return Mutex::has(copy_MutexName);
}

bool _removeMutex(const char* MutexName)
{
    string copy_MutexName = (MutexName);

    return Mutex::remove(copy_MutexName);
}

bool _Alert(const char* title ,const char* info) {
    string copy_title = (title);
    string copy_info = (info);

    int To_MessageBoxA = MessageBoxA(NULL, copy_info.c_str() , copy_title.c_str(),MB_OK);
    if (To_MessageBoxA == 1 || To_MessageBoxA == 6)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool _Confirm(const char* title, const char* info) {
    string copy_title = (title);
    string copy_info = (info);

    int To_MessageBoxA = MessageBoxA(NULL, copy_info.c_str(), copy_title.c_str(), MB_OKCANCEL);
    if (To_MessageBoxA == 1 || To_MessageBoxA == 6)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void _Stop(const char* title, const char* info) {
    string copy_title = (title);
    string copy_info = (info);

    int To_MessageBoxA = MessageBoxA(NULL, copy_info.c_str(), copy_title.c_str(), MB_ICONERROR);
}

void _Error(const char* title, const char* info) {
    string copy_title = (title);
    string copy_info = (info);

    int To_MessageBoxA = MessageBoxA(NULL, copy_info.c_str(), copy_title.c_str(), MB_ICONEXCLAMATION);
}

bool _setStartup()
{
    string path = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    string key = "IkunWxExportDat";
    string execPath = "\"";
    execPath += getProcessidFilePath(_getpid());
    execPath.append("\" -startup");

    if (hmc_registr::hasRegistrKey(HKEY_LOCAL_MACHINE, path, key))
    {
        ikun_app_startup = hmc_registr::removeRegistrValue(HKEY_LOCAL_MACHINE, path, key) ? false : true;
        hmc_tray::setMenuItmeSelect("btn::app_startup",ikun_app_startup);
        return ikun_app_startup;
    }
    else
    {
        ikun_app_startup = hmc_registr::setRegistrValue(HKEY_LOCAL_MACHINE, path, key, execPath) ? true : false;
        hmc_tray::setMenuItmeSelect("btn::app_startup", ikun_app_startup);

        return ikun_app_startup;
    }
    ikun_app_startup = false;
    hmc_tray::setMenuItmeSelect("btn::app_startup", ikun_app_startup);

    return ikun_app_startup;
}

bool _hasStartup()
{
    string path = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    string key = "IkunWxExportDat";
    ikun_app_startup = hmc_registr::hasRegistrKey(HKEY_LOCAL_MACHINE, path, key);
    hmc_tray::setMenuItmeSelect("btn::app_startup", ikun_app_startup);

    return ikun_app_startup;
}

bool _hasStartupGlobalVar()
{
    return ikun_app_startup;
}

std::string removeNullCharacters(std::string input) {
    string data = string();
    for (size_t i = 0; i < input.size(); i++)
    {
        auto it = input[i];

        if (it == '\0')continue;

        data.push_back(it);
    }
    return data;
}


std::wstring removeNullCharacters(std::wstring input) {
    wstring data = wstring();
    for (size_t i = 0; i < input.size(); i++)
    {
        auto it = input[i];

        if (it == '\0')continue;

        data.push_back(it);
    }
    return data;
}

void _openSelectFolder()
{
    // setWindowTop(winmian, false);

    CoInitialize(NULL);

    BROWSEINFOA browseInfo = {0};
    char folderPath[MAX_PATH];

    browseInfo.hwndOwner = NULL;
    browseInfo.pidlRoot = NULL;
    browseInfo.pszDisplayName = folderPath;
    browseInfo.lpszTitle = "选择文件夹";
    browseInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderA(&browseInfo);
    _putenv_s("IKUN@SelectedFolderPath", "\0");

    if (pidl != NULL)
    {
        SHGetPathFromIDListA(pidl, folderPath);
        std::cout << "Selected folder path: " << folderPath << std::endl;
        _putenv_s("IKUN@SelectedFolderPath", folderPath);
        CoTaskMemFree(pidl);
    }
    else
    {
        std::cout << "Folder selection canceled." << std::endl;
    }

    // 释放 COM 库
    CoUninitialize();
    // setWindowTop(winmian, true);

    // return string(folderPath);
}

/**
 * @brief 选择文件夹（单选）
 *
 * @param folderPath
 * @return true
 * @return false
 */
bool SelectFolder(wstring &folderPath)
{
    bool result = false;
    try
    {
        HRESULT hr;
        IFileOpenDialog *pOpenFolderDialog;
        HWND owner = NULL;

        hr = ::CoCreateInstance(CLSID_FileOpenDialog,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&pOpenFolderDialog));

        if (SUCCEEDED(hr))
        {
            // 获取用户与对话框交互的结果
            pOpenFolderDialog->SetOptions(FOS_PICKFOLDERS);

            // 显示选择文件夹窗口
            hr = pOpenFolderDialog->Show(owner);

            if (SUCCEEDED(hr))
            {

                IShellItem *psiResult;
                hr = pOpenFolderDialog->GetResult(&psiResult);

                LPWSTR folderW = NULL;
                psiResult->GetDisplayName(SIGDN_FILESYSPATH, &folderW);
                if (folderW != NULL) {
                    folderPath.clear();
                    folderPath.append(folderW);
                    wcout << L"folderPath -> " << folderPath << endl;
                }
                
                result = true;
                ::CoTaskMemFree(folderW);
                psiResult->Release();
            }
        }
        pOpenFolderDialog->Release();
    }
    HMC_CHECK_CATCH;
    return result;
}

const char* _openSelectFolder2()
{
    string result = string();
    try
    {
        wstring temp_buf = wstring();
        if (SelectFolder(temp_buf)) {
            result.append(hmc_text_util::W2U8(temp_buf));

        }
    }
    HMC_CHECK_CATCH;
    if (result.empty()) {
        return "\0";
    }
    else {
        char* pUTF8 = new char[result.size() + 1];

        for (size_t i = 0; i < result.size(); i++)
        {
            char data = result[i];
            pUTF8[i] = data;
        }
        const int end = result.size() ;

        pUTF8[end] = *"\0";

        return pUTF8;
    }
    
}

const char* _getRegistrValue(long hKey, const char* _subKey, const char* _key)
{
    string subKey = (_subKey);
    string key = (_key);

    string result = hmc_registr::getRegistrValue<string>((HKEY)hKey, subKey, key);
  
    char* pUTF8 = new char[result.size() + 1];

    for (size_t i = 0; i < result.size(); i++)
    {
        char data = result[i];
        pUTF8[i] = data;
    }
    const int end = result.size();

    pUTF8[end] = *"\0";

    return pUTF8;

}

struct ProcessEnumDetailsCont
{
    DWORD pid;
    string baseName;
    string path;
};


void getProcessList(vector<ProcessEnumDetailsCont>& resultsData)
{
    DWORD processList[1024], cbNeeded;
    if (!EnumProcesses(processList, sizeof(processList), &cbNeeded))
    {
    }
    int numProcesses = cbNeeded / sizeof(DWORD);
    for (int i = 0; i < numProcesses; ++i)
    {
        DWORD processID = processList[i];
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
        if (hProcess)
        {
            char processName[MAX_PATH];
            char Filename[1024];
            GetModuleBaseNameA(hProcess, NULL, processName, MAX_PATH);
            GetModuleFileNameExA(hProcess, NULL, Filename, 1024);
            ProcessEnumDetailsCont processEnumCont;
            processEnumCont.pid = processID;
            processEnumCont.baseName = processName;
            processEnumCont.path = Filename;
            resultsData.push_back(processEnumCont);
            CloseHandle(hProcess);
        }
    }
}


bool _hasWeChat() {
    return hmc_process::hasBaseNameProcess(string("WeChat.exe"));
}

vector<DWORD> getWeChatPidList() {
    auto app_base_name = string("WeChat.exe");
    return hmc_process:: getBaseNameProcessIDList(app_base_name);
}

const char* _enum_file_open_path () {
    string result = "";
    vector<DWORD> pid_list = getWeChatPidList();
    
    for (size_t i = 0; i < pid_list.size(); i++)
    {
        auto pid = pid_list[i];
        HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (hProcess == NULL)
        {
           
        }

    }
    return result.c_str();
}

string get_utf8_str(const char* input, int inputLen = 0) {
    string ouput = string();
    if (inputLen > 0) {
        ouput.resize(inputLen);
        for (size_t i = 0; i < inputLen; i++)
        {
            char data = input[i];
            if (data == (char)"\0") {
                break;
            }
            ouput[i] = input[i];
        }
        //ouput.append("\0");
    }
    else {
        ouput.append(input);
    }

    ouput = hmc_text_util::U82A(ouput);

    return ouput;
}

long _findWindowU8(const char* className, const char* title) {

    string copy_className = hmc_text_util::U82A(className);
    string copy_title = hmc_text_util::U82A(title);
    return 0L;
    //return (long)hmc_window::findWindow(copy_className, copy_title);

}

bool contains(const std::wstring& text, const std::wstring& substring)
{
    return text.find(substring) != text.npos;
}

bool contains(const std::string& text, const std::string& substring)
{
    return text.find(substring) != text.npos;
}

long _findWindowW(const wchar_t* className, const wchar_t* title) {
    HWND hwnd = ::FindWindowW(removeNullCharacters(std::wstring(className)).c_str(), nullptr);
    return long(hwnd);

}

long _findWindow(const char* className, const char* title) {

    // 查找第一个符合条件的窗口
    HWND hwnd = ::FindWindowA(removeNullCharacters(std::string(className)).c_str(), nullptr);
    return long(hwnd);

}

bool _has_auto_sync() {
    return ikun_user_auto_disable_sync;
}

void _set_auto_sync(bool value) {
   ikun_user_auto_disable_sync = value;
   try
   {
       if (value) {
           hmc_registr::setRegistrValue(HKEY_CURRENT_USER, "SOFTWARE\\WxAutoExIm", "auto_sync", string("true"));
       }
       else {
           hmc_registr::removeRegistrValue(HKEY_CURRENT_USER, "SOFTWARE\\WxAutoExIm", "auto_sync");
       }

       hmc_tray::setMenuItmeSelect("btn::auto_sync", ikun_user_auto_disable_sync);

   }HMC_CHECK_CATCH;
}

bool _has_sync_token() {
    if (sync_token) {
        sync_token = false;
        //cout << "_has_sync_token" << endl;
        return true;
    }
    return false;
}

long _getFocusWindow() {
   //return (long) hmc_window::getFocusWindow();
    return (long)::GetForegroundWindow();
}

long _getFocusTopWindow() {
    return (long)hmc_window::getParentWindow(::GetForegroundWindow())|| _getFocusWindow();
}

template <typename T>
const char* hwnd_list2_long_list(vector<T> &hwnd_list) {

    string _hwnd_list = string();
    for (size_t i = 0; i < hwnd_list.size(); i++)
    {
        T hwnd = hwnd_list[i];
        _hwnd_list.append(to_string((long long)hwnd));
        _hwnd_list.append(",");

    }

    if (!_hwnd_list.empty()) {
        _hwnd_list.pop_back();
    }

    //cout << "_hwnd_list->" << _hwnd_list << endl;

    char* pUTF8 = new char[_hwnd_list.size() + 1];

    for (size_t i = 0; i < _hwnd_list.size(); i++)
    {
        char data = _hwnd_list[i];
        pUTF8[i] = data;
    }
    const int end = _hwnd_list.size();

    pUTF8[end] = *"\0";

    //cout << "pUTF8->" << pUTF8 << endl;

    return pUTF8;
}


//std::string removeNullCharacters(std::string str) {
//
//    string data = string();
//    data.append(str);
//
//    // 移除开头的空字符
//    while (!data.empty() && data.front() == '\0') {
//        data.erase(0, 1);
//    }
//
//    // 移除末尾的空字符
//    while (!data.empty() && data.back() == '\0') {
//        data.pop_back();
//    }
//
//    return data;
//}



const char* _findAllWindow(const char* className, const char* title) {
    vector<HWND> hwnd_list ;

    string _hwnd_list = string();
    string _className = removeNullCharacters(string(className));
    string _titleName = removeNullCharacters(string(title));

    HWND winEnumerable = GetTopWindow(0);
   
    while (winEnumerable)
    {
        if (::IsWindow(winEnumerable)) {

        string the_class = string();
        string the_titleName = string();

        if (!_className.empty()) {
            the_class = hmc_window::getClassName(winEnumerable);
            
            if (the_class == _className) {

                if (_titleName.empty()) {

                    hwnd_list.push_back(winEnumerable);

                }
            }
        }


        if (!_titleName.empty()) {
            
            the_titleName = hmc_window::getWindowText(winEnumerable);
            if (the_titleName == _titleName) {

                if (_className.empty()) {
                    hwnd_list.push_back(winEnumerable);
                }

            }
        }

        if (!_className.empty()&& !the_titleName.empty()) {
            if (the_titleName == _titleName&& the_class == _className) {
                hwnd_list.push_back(winEnumerable);
            }
        }

        }

        winEnumerable = GetNextWindow(winEnumerable, GW_HWNDNEXT);
    }

    for (size_t i = 0; i < hwnd_list.size(); i++)
    {
        HWND hwnd = hwnd_list[i];
        _hwnd_list.append(to_string((int)hwnd));
        _hwnd_list.append(",");

    }

    if (!_hwnd_list.empty()) {
        _hwnd_list.pop_back();
    }

    //cout << "_hwnd_list->" << _hwnd_list << endl;

    char* pUTF8 = new char[_hwnd_list.size() + 1];

    for (size_t i = 0; i < _hwnd_list.size(); i++)
    {
        char data = _hwnd_list[i];
        pUTF8[i] = data;
    }
    const int end = _hwnd_list.size();

    pUTF8[end] = *"\0";

    //cout << "pUTF8->" << pUTF8 << endl;

    return pUTF8;

}

const char* _getWindowRect(long hwnds){
    RECT rect;
    ::GetWindowRect(HWND(hwnds), &rect);

    int width = rect.right - rect.left; // 计算窗口宽度
    int height = rect.bottom - rect.top; // 计算窗口高度


    string res_json = string();
    res_json.append("{");
    res_json.append("\"left\":");
    res_json.append(to_string(rect.left));
    res_json.append(",\"top\":");
    res_json.append(to_string(rect.top));
    res_json.append(",\"bottom\":");
    res_json.append(to_string(rect.bottom));
    res_json.append(",\"right\":");
    res_json.append(to_string(rect.right));
    res_json.append(",\"width\":");
    res_json.append(to_string(width));
    res_json.append(",\"height\":");
    res_json.append(to_string(height));
    res_json.append("}");


    char* pUTF8 = new char[res_json.size() + 1];

    for (size_t i = 0; i < res_json.size(); i++)
    {
        char data = res_json[i];
        pUTF8[i] = data;
    }
    const int end = res_json.size();

    pUTF8[end] = *"\0";


    return pUTF8;

}

long long _randomNum() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<long long> dis(-8446744073709551617i64, 8446744073709551617i64);

    int randomNum = dis(gen);

    return randomNum;
}

void _setWindowTransparent(long hwnds,int transparent) {
    HWND hwnd = (HWND)hwnds;
    hmc_window::setHandleTransparent(hwnd, transparent);
}

const char* _getfilePathSingle () {
    string filePath = "";

    if (OpenClipboard(NULL))
    {
        HDROP hDrop = HDROP(::GetClipboardData(CF_HDROP));
        if (hDrop != NULL)
        {
            char szFilePathName[MAX_PATH + 1] = { 0 };
            UINT UintAllFiles = DragQueryFileA(hDrop, 0xFFFFFFFF, NULL, 0);

            for (UINT index = 0; index < UintAllFiles; index++)
            {
                memset(szFilePathName, 0, MAX_PATH + 1);
                // get path
                DragQueryFileA(hDrop, index, szFilePathName, MAX_PATH);

                filePath.append(szFilePathName);
            }
        }
        CloseClipboard();
    }

    char* pUTF8 = new char[filePath.size() + 1];

    for (size_t i = 0; i < filePath.size(); i++)
    {
        char data = filePath[i];
        pUTF8[i] = data;
    }
    const int end = filePath.size();

    pUTF8[end] = *"\0";

    return pUTF8;
}

bool _setWindowEnabled(long hwnds, bool enabled) {
    HWND hwnd = (HWND)hwnds;

    if (enabled) {
        ::SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) | (WS_DISABLED));
        
    }
    else {
        ::SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~(WS_DISABLED));

    }

    LONG windowLong = ::GetWindowLong(hwnd, GWL_STYLE);

    return !(windowLong & WS_DISABLED);
}



// 获取所有屏幕的位置
vector<RECT> GetDeviceCapsAll()
{
    vector<RECT> CrectList;
    DISPLAY_DEVICE displayDevice;
    ZeroMemory(&displayDevice, sizeof(displayDevice));
    displayDevice.cb = sizeof(displayDevice);
    DEVMODE devMode;
    ZeroMemory(&devMode, sizeof(devMode));
    devMode.dmSize = sizeof(devMode);

    for (int i = 0; EnumDisplayDevices(NULL, i, &displayDevice, 0); ++i)
    {
        if (EnumDisplaySettings(displayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &devMode))
        {
            int left = devMode.dmPosition.x;
            int top = devMode.dmPosition.y;
            int right = devMode.dmPosition.x + devMode.dmPelsWidth;
            int bottom = devMode.dmPosition.y + devMode.dmPelsHeight;
            RECT rect;
            rect.bottom = bottom;
            rect.left = left;
            rect.top = top;
            rect.right = right;
            CrectList.push_back(rect);
        }
    }
    return CrectList;
}

bool isInside(int x1, int y1, int x2, int y2, int x, int y)
{
    if (x > x1 && x < x2 && y > y1 && y < y2)
        return true;
    return false;
}

struct GetColorInfo
{
    int r;
    int g;
    int b;
    string hex;
};

// 获取屏幕上指定位置的颜色
GetColorInfo GetColor(int x, int y)
{
    GetColorInfo _ColorInfo;
    _ColorInfo.b = 0;
    _ColorInfo.g = 0;
    _ColorInfo.r = 0;
    _ColorInfo.hex = "#000000";
    // 获取屏幕DC
    HDC hScreenDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
    // 获取屏幕分辨率
    int nScopeWidth = GetDeviceCaps(hScreenDC, HORZRES);
    int nScopeHeight = GetDeviceCaps(hScreenDC, VERTRES);
    int nScopeX = 0;
    int nScopeY = 0;
    // nScopeWidth = 3840;
    //  坐标小于0肯定不对
    if (x < 0 || y < 0)
    {
        return _ColorInfo;
    }
    // 坐标超过了第一个屏幕  则应该开始判断是在第二个屏幕还是参数过界了
    if (nScopeHeight < y || nScopeWidth < x)
    {
        int is_ok = false;
        vector<RECT> deviceCapsAll = GetDeviceCapsAll();
        for (size_t i = 0; i < deviceCapsAll.size(); i++)
        {
            RECT deviceCaps = deviceCapsAll[i];
            if (isInside(deviceCaps.left, deviceCaps.top, deviceCaps.right, deviceCaps.bottom, x, y))
            {

                nScopeWidth = deviceCaps.right;
                nScopeHeight = deviceCaps.bottom;
                nScopeX = deviceCaps.left;
                nScopeY = deviceCaps.top;
                is_ok = true;
            }
        }
        if (!is_ok)
            return _ColorInfo;
    }
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, nScopeWidth, nScopeHeight);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);
    BitBlt(hMemoryDC, 0, 0, nScopeWidth, nScopeHeight, hScreenDC, 0, 0, SRCCOPY);
    COLORREF color = GetPixel(hMemoryDC, x, y);
    SelectObject(hMemoryDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    DeleteDC(hScreenDC);

    int r = GetRValue(color);
    int g = GetGValue(color);
    int b = GetBValue(color);
    char hex[8];
    sprintf_s(hex, "#%02X%02X%02X", r, g, b);
    _ColorInfo.b = b;
    _ColorInfo.g = g;
    _ColorInfo.r = r;
    _ColorInfo.hex = hex;
    transform(_ColorInfo.hex.begin(), _ColorInfo.hex.end(), _ColorInfo.hex.begin(), ::tolower);

    return _ColorInfo;
}

const char* _getColor_json (int x, int y) {

    GetColorInfo data = GetColor(x, y);

    string res_json = string();
    res_json.append("{");
    res_json.append("\"r\":");
    res_json.append(to_string(data.r));
    res_json.append(",\"g\":");
    res_json.append(to_string(data.g));
    res_json.append(",\"b\":");
    res_json.append(to_string(data.b));
    res_json.append(",\"hex\":\"");
    res_json.append(data.hex);
    res_json.append("\"}");


    char* pUTF8 = new char[res_json.size() + 1];

    for (size_t i = 0; i < res_json.size(); i++)
    {
        char data = res_json[i];
        pUTF8[i] = data;
    }
    const int end = res_json.size();

    pUTF8[end] = *"\0";


    return pUTF8;

}

const char* json2pcstr(json jsondata) {

    try {

    std::string quoted = jsondata.dump();

    char* pUTF8 = new char[quoted.size() + 1];

    for (size_t i = 0; i < quoted.size(); i++)
    {
        char data = quoted[i];
        pUTF8[i] = data;
    }
    const int end = quoted.size();

    pUTF8[end] = *"\0";
        return pUTF8;

    }HMC_CHECK_CATCH;
    return "\0";
}

// 判断是否按下三大金刚
const char* _getBasicKeys()
{
    json dataBasicKeys;

    try
    {
        dataBasicKeys.object();
        dataBasicKeys["shift"] = bool(GetKeyState(VK_SHIFT) & 0x8000) || false;
        dataBasicKeys["alt"] = bool(GetKeyState(VK_MENU) & 0x8000) || false;
        dataBasicKeys["ctrl"] = bool(GetKeyState(VK_CONTROL) & 0x8000) || false;
        dataBasicKeys["win"] = bool(GetKeyState(VK_RWIN) & 0x8000 || GetKeyState(VK_LWIN) & 0x8000) || false;
    }HMC_CHECK_CATCH;

    return json2pcstr(dataBasicKeys);
}

/**
 * @brief 选择文件 （单个文件）
 *
 * @param FilePath
 * @return true
 * @return false
 */
bool SelectFile(wstring &FilePath)
{
    bool result = false;
    try
    {
        IFileOpenDialog* pFileOpen;
        PWSTR pszFilePath = NULL;

        HRESULT hr = ::CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
        if (SUCCEEDED(hr))
        {
            hr = pFileOpen->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr))
            {
                IShellItem* pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                    FilePath.append(pszFilePath);

                    result = true;
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
    }
    HMC_CHECK_CATCH;
    return result;
    // return pszFilePath;
}

const char* _selectFile() {
    wstring FilePath;
    SelectFile(FilePath);
    if (!FilePath.empty())
    {
        try {

            std::string quoted = hmc_text_util::W2U8(FilePath);

            char* pUTF8 = new char[quoted.size() + 1];

            for (size_t i = 0; i < quoted.size(); i++)
            {
                char data = quoted[i];
                pUTF8[i] = data;
            }
            const int end = quoted.size();
            pUTF8[end] = *"\0";
            return pUTF8;

        }HMC_CHECK_CATCH;
    }
    return "\0";
}



bool _isKeyDown(int key)
{
    bool res =  (GetKeyState(key) & 0x8000) != 0;
    
    return res;
}

bool _openUrl(const char* Url) {
    HINSTANCE hResult = ShellExecuteA(NULL, "open", Url, NULL, NULL, SW_SHOWNORMAL);
    // return  _create_int64_Number(env,long(hResult));
    return (long long(hResult) >= 31)?true:false;
}