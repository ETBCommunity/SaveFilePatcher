// SaveFilePatcher.c by RedTuna (@CannedRedTuna on GitHub)

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>

#define IntSize (sizeof(uint32_t))
typedef unsigned long uLong;
typedef unsigned char uChar;

int Backup(const uChar* Data, const uLong FileSize){
    FILE* backup = fopen("MAINSAVE.sav.bak", "wb");
    if (!backup){
        printf("Backup Failed!");
        getchar();
        return -1;
    }

    fwrite(Data, 1, FileSize, backup);
    fclose(backup);
    return 0;
}

void fullFree(uChar** Pointer){
    free(*Pointer);
    *Pointer = NULL;
}

void ArrFree(char** Pointer, int Capacity){
    for (int i = Capacity - 1; i >= 0; i--){
        free(Pointer[i]);
        Pointer[i] = NULL;
    }
}

uLong EndOfArray(const uChar* Data){

    const uLong ArrayCountOffset = 0x4FC;
    const uLong ArraySizeOffset = 0x4E3;

    uint64_t ArraySize = 0;

    memcpy(&ArraySize, &Data[ArraySizeOffset], sizeof(uint64_t));

    return ArraySize + ArrayCountOffset;
}

int ProcessSave(uChar** Data, uLong* outFileSize){
    const char* filename = "MAINSAVE.sav";
    const uLong HubUnlockedOffset = 0x4BF;

    FILE* save = fopen(filename, "rb");

    if (!save){
        printf("MAINSAVE.sav not found.\n");
        getchar();
        return -1;
    }

    fseek(save, 0, SEEK_END);
    uLong FileSize = ftell(save);
    fseek(save, 0, SEEK_SET);

    if (FileSize < 0xA000){
        fclose(save);
        printf("File Likely corrupted.\n");
        getchar();
        return -1;
    }

    *Data = malloc(FileSize);
    if (!*Data){
        printf("Try downloading more ram.\n");
        getchar();
        fclose(save);
        return -1;
    }

    fread(*Data, 1, FileSize, save);
    fclose(save);
    Backup(*Data, FileSize);

if ((*Data)[0] != 'G'){
    printf("What\n");
    fullFree(Data);
    getchar();
    return -1;
}

if ((*Data)[HubUnlockedOffset] != 'S'){
    printf("Detected missing savelist.\n");
    const uLong RepairDataSize = 69;
    const uLong NewFileSize = FileSize + RepairDataSize;

    const uLong BackupSize = FileSize - HubUnlockedOffset + IntSize;
    uChar* EmergencyRepairs = malloc(BackupSize);
    if (!EmergencyRepairs){
        printf("Try downloading more ram.\n");
        fullFree(Data);
        getchar();
        return -1;
    }

    printf("Generating a new savelist.\n");
    const uChar RepairData[69] = {
        0x12, 0x00, 0x00, 0x00, 0x53, 0x69, 0x6E, 0x67, 0x6C, 0x65, 0x70, 0x6C,
        0x61, 0x79, 0x65, 0x72, 0x53, 0x61, 0x76, 0x65, 0x73, 0x00, 0x0E, 0x00,
        0x00, 0x00, 0x41, 0x72, 0x72, 0x61, 0x79, 0x50, 0x72, 0x6F, 0x70, 0x65,
        0x72, 0x74, 0x79, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x0C, 0x00, 0x00, 0x00, 0x53, 0x74, 0x72, 0x50, 0x72, 0x6F, 0x70, 0x65,
        0x72, 0x74, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    memcpy(EmergencyRepairs, &(*Data)[HubUnlockedOffset - IntSize], BackupSize);

    uChar* TempData = realloc(*Data, NewFileSize);
    if (!TempData){
        printf("Realloc failed\n");
        free(EmergencyRepairs);
        fullFree(Data);
        getchar();
        return -1;
    }

    *Data = TempData;

    memcpy(&(*Data)[HubUnlockedOffset - IntSize], RepairData, RepairDataSize);
    memcpy(&(*Data)[HubUnlockedOffset - IntSize + RepairDataSize], EmergencyRepairs, BackupSize);

    FileSize = NewFileSize;
    if (outFileSize){
        *outFileSize = NewFileSize;
    }

    free(EmergencyRepairs);
    printf("Successfully generated a new savelist.\n");
}

    if (outFileSize){
        *outFileSize = FileSize;
    }
    return 0;
}

int RebuildArray() {
    const uLong ArraySizeOffset = 0x4E3;
    const uLong ArrayCountOffset = 0x4FC;

    uChar* Data = NULL;
    uLong FileSize = 0;

    if (ProcessSave(&Data, &FileSize) != 0){
        return -1;
    }

    uLong NewFileSize = ArrayCountOffset + IntSize;
    uint64_t NewArraySize = IntSize;

    int DynArrSize = 10;
    int ElementCount = 0;
    char** FileNames = malloc(DynArrSize * sizeof(char*));
    if (!FileNames){
        printf("Try downloading more ram.\n");
        fullFree(&Data);
        getchar();
        return -1;
    }

    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile("MULTIPLAYER*.sav", &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                const char* fileName = findData.cFileName;
                const int StrLen = strlen(fileName) - 4;
                if (StrLen < 1) continue;

                if (ElementCount >= DynArrSize) {
                    DynArrSize *= 2;
                    char** temp = realloc(FileNames, DynArrSize * sizeof(char*));
                    if (!temp) {
                        printf("Try downloading even more ram.\n");
                        ArrFree(FileNames, ElementCount);
                        free(FileNames);
                        fullFree(&Data);
                        FindClose(hFind);
                        getchar();
                        return -1;
                    }
                    FileNames = temp;
                }

                FileNames[ElementCount] = malloc(StrLen + 1);
                if (!FileNames[ElementCount]){
                    ArrFree(FileNames, ElementCount);
                    free(FileNames);
                    fullFree(&Data);
                    FindClose(hFind);
                    getchar();
                    return -1;
                }
                strncpy(FileNames[ElementCount], fileName, StrLen);
                FileNames[ElementCount][StrLen] = '\0';

                NewFileSize += (IntSize + StrLen + 1);
                NewArraySize += (IntSize + StrLen + 1);
                ElementCount++;
            }
        } while (FindNextFile(hFind, &findData));
        FindClose(hFind);
    }

    uLong Cursor = ArrayCountOffset;
    uLong DataEnd = EndOfArray(Data);
    uLong SpliceSize = FileSize - DataEnd;
    NewFileSize += SpliceSize;

    uChar* Splice = malloc(SpliceSize);
    if (!Splice){
        printf("Try downloading more ram.\n");
        fullFree(&Data);
        ArrFree(FileNames, ElementCount);
        free(FileNames);
        return -1;
    }
    memcpy(Splice, &Data[DataEnd], SpliceSize);

    uChar* ReWrite = malloc(NewFileSize);
    if (!ReWrite){
        printf("Try downloading more ram.\n");
        fullFree(&Data);
        fullFree(&Splice);
        ArrFree(FileNames, ElementCount);
        free(FileNames);
        return -1;
    }
    memcpy(ReWrite, Data, ArrayCountOffset);
    memcpy(&ReWrite[Cursor], &ElementCount, IntSize);

    Cursor += IntSize;
    uint32_t StrLen = 0;
    for (int i = 0; i < ElementCount; i++){
        StrLen = strlen(FileNames[i]) + 1;
        memcpy(&ReWrite[Cursor], &StrLen, IntSize);
        Cursor += IntSize;
        memcpy(&ReWrite[Cursor], FileNames[i], StrLen);
        Cursor += StrLen;
    }

    memcpy(&ReWrite[Cursor], Splice, SpliceSize);
    memcpy(&ReWrite[ArraySizeOffset], &NewArraySize, sizeof(uint64_t));

    fullFree(&Data);
    fullFree(&Splice);

    FILE* Update = fopen("MAINSAVE.sav", "wb");
    if (!Update){
        printf("The file just didn't open.\n");
        fullFree(&ReWrite);
        ArrFree(FileNames, ElementCount);
        free(FileNames);
        return -1;
    }

    fwrite(ReWrite, 1, NewFileSize, Update);
    fclose(Update);

    fullFree(&ReWrite);
    ArrFree(FileNames, ElementCount);
    free(FileNames);

    printf("Array rebuilt with %d files\n", ElementCount);
    return 0;
}

void MonitorDirectory() {
    char buffer[1024];
    DWORD bytesReturned;
    HANDLE hDir = CreateFile(
        ".",
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (hDir == INVALID_HANDLE_VALUE) {
        printf("Windows throwing\n");
        getchar();
        return;
    }

    printf("Waiting for files.\n");

    while (1) {
        if (ReadDirectoryChangesW(
            hDir,
            buffer,
            sizeof(buffer),
            FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesReturned,
            NULL,
            NULL
        )) {
            FILE_NOTIFY_INFORMATION* event = (FILE_NOTIFY_INFORMATION*)buffer;

            do {
                const WCHAR* filename = event->FileName;
                const int len = event->FileNameLength / sizeof(WCHAR);

                if (len > 15 && wcsncmp(filename, L"MULTIPLAYER", 11) == 0) {
                    WCHAR ext[5];
                    wcsncpy(ext, &filename[len - 4], 4);
                    ext[4] = L'\0';

                    if (wcscmp(ext, L".sav") == 0 &&
                        (event->Action == FILE_ACTION_ADDED ||
                         event->Action == FILE_ACTION_RENAMED_NEW_NAME)) {

                        printf("\nNew save detected.\n");
                        Sleep(100);

                        if (RebuildArray() == 0) {
                            printf("MAINSAVE.sav successfully updated\n");
                        } else {
                            printf("Yeah idfk\n");
                        }
                    }
                }
                
                if (event->NextEntryOffset == 0) break;
                event = (FILE_NOTIFY_INFORMATION*)((BYTE*)event + event->NextEntryOffset);
            } while (1);
        }
    }

    CloseHandle(hDir);
}

int main() {
    printf("Starting...\n");
    
    printf("Performing initial rebuild...\n");
    if (RebuildArray() != 0) {
        printf("Initial rebuild failed\n");
        return -1;
    }
    
    printf("Initial rebuild complete\n\n");
    
    MonitorDirectory();
    
    return 0;
}  