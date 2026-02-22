// SaveFilePatcher.c by RedTuna (@CannedRedTuna on GitHub)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>

#define Int32Size (sizeof(uint32_t))
#define Int64Size (sizeof(uint64_t))
#define SAFE_FREE(ptr) do { free(ptr); (ptr) = NULL; } while (0)
typedef unsigned char uChar;

void ArrFree(char** Pointer, int Capacity){
    for (int i = Capacity - 1; i >= 0; i--){
        free(Pointer[i]);
        Pointer[i] = NULL;
    }
}

void BuildPath(char* out, size_t outSize, const char* dir, const char* fileName) {
    snprintf(out, outSize, "%s\\%s", dir, fileName);
}

int CreateBackup(const uChar* SaveFile, const size_t Size, const char* basePath){

    char filePath[MAX_PATH];
    BuildPath(filePath, sizeof(filePath), basePath, "MAINSAVE.sav.bak");

    FILE* backup = fopen(filePath, "rb");
    if (backup){
        fclose(backup);
        return 0;
    }

    FILE* Backup = fopen(filePath, "wb");
    if (!Backup){
        perror("Failed to create a backup file");
        return -1;
    }

    fwrite(SaveFile, 1, Size, Backup);
    fclose(Backup);

    return 0;
}

int WriteSaveFile(const uChar* PatchedSaveFile, const size_t PatchedSize, const char* basePath){
    char filePath[MAX_PATH];
    BuildPath(filePath, sizeof(filePath), basePath, "MAINSAVE.sav");

    FILE* Patch = fopen(filePath, "wb");
    if (!Patch){
        perror("Failed to write the patch to file");
        return -1;
    }

    fwrite(PatchedSaveFile, 1, PatchedSize, Patch);
    fclose(Patch);

    return 0;
}

int isValid(const uChar* SaveFile, const size_t Size) {
    if (Size < 0x4C7){
        fprintf(stderr, "File too small to be a savefile.");
        return -1;
    }

    if (memcmp(SaveFile, "GVAS", 4) != 0) {
        fprintf(stderr, "Invalid File Header\n");
        return -1;
    }

    else if (Size == 0x4C7){
        printf("Outdated Savefile.\nUpdating savefile.\n");
        return 1; 
    }

    else if (Size < 0xA000) {
        fprintf(stderr, "Unexpected FileSize. File may be corrupted.\n");
        return -1;
    }

    else if (memcmp(&SaveFile[0x4BF], "SingleplayerSaves", 18) != 0 && Size > 0x4BB) {
        printf("Save Array is misssing.\nCreating new Array.\n");
        return 1; 
    }

    else {
        return 0;
    }
}

int RecoverArray(uChar** SaveFile, size_t* Size) {
    const size_t RepairBlockSize = 69;
    const size_t RepairStart = 0x4BB;
    const size_t bufferSize = *Size - RepairStart;
    const uChar RepairBlock[69] = {
        0x12, 0x00, 0x00, 0x00, 0x53, 0x69, 0x6E, 0x67, 0x6C, 0x65, 0x70, 0x6C,
        0x61, 0x79, 0x65, 0x72, 0x53, 0x61, 0x76, 0x65, 0x73, 0x00, 0x0E, 0x00,
        0x00, 0x00, 0x41, 0x72, 0x72, 0x61, 0x79, 0x50, 0x72, 0x6F, 0x70, 0x65,
        0x72, 0x74, 0x79, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x0C, 0x00, 0x00, 0x00, 0x53, 0x74, 0x72, 0x50, 0x72, 0x6F, 0x70, 0x65,
        0x72, 0x74, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    uChar *buffer = malloc(bufferSize);
    if (!buffer) {
        perror("Failed to allocate memory to recover array");
        return -1;
    }

    memcpy(buffer, &(*SaveFile)[RepairStart], bufferSize);

    uChar *temp = realloc(*SaveFile, *Size + RepairBlockSize);
    if (!temp) {
        perror("Failed to reallocate memory to recover array");
        SAFE_FREE(buffer);
        return -1;
    }

    *SaveFile = temp;
    memcpy(&(*SaveFile)[RepairStart], RepairBlock, RepairBlockSize);
    memcpy(&(*SaveFile)[RepairStart + RepairBlockSize], buffer, bufferSize);
    SAFE_FREE(buffer);

    *Size = *Size + RepairBlockSize;

    return 0;
}

int GetSaveFile(uChar** SaveFile, size_t* Size, const char* basePath){

    char filePath[MAX_PATH];
    BuildPath(filePath, sizeof(filePath), basePath, "MAINSAVE.sav");

    FILE* file = fopen(filePath, "rb");
    if (!file){
        perror("Failed to open MAINSAVE.sav");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    __int64 fileSize = _ftelli64(file);
    if (fileSize < 0) {
        perror("ftell failed");
        fclose(file);
        return -1;
    }
    *Size = (size_t)fileSize;
    fseek(file, 0, SEEK_SET);

    *SaveFile = malloc(*Size);
    if (!*SaveFile){
        perror("Failed to allocate memory for the save file");
        fclose(file);
        return -1;
    }

    fread(*SaveFile, 1, *Size, file);
    fclose(file);

    if (CreateBackup(*SaveFile, *Size, basePath) != 0){
        SAFE_FREE(*SaveFile);
        return -1;
    }

    return 0;
}

int UpdateArray(const char* basePath){
    size_t ArraySizePos = 0x4E3;
    size_t ArrayCountPos = 0x4FC;
    size_t ArrayStartPos = 0x500;

    uint64_t ArraySize = 0;
    uint64_t PatchedArraySize = Int32Size;
    int PatchedArrayCount = 0;
    
    uChar* SaveFile = NULL;
    size_t Size = 0;
    uint64_t PatchedSize = 0;

    if (GetSaveFile(&SaveFile, &Size, basePath) != 0){
        return -1;
    }

    switch (isValid(SaveFile, Size)){
        case -1: {
            SAFE_FREE(SaveFile);
            return -1;
        }

        case 1:{
            if (RecoverArray(&SaveFile, &Size) !=0){
                SAFE_FREE(SaveFile);
                return -1;
            }
            break;
        }
        case 0: break;
    }

    memcpy(&ArraySize, &SaveFile[ArraySizePos], Int64Size);

    size_t ArrayEndPos = ArrayCountPos + ArraySize;

    if (ArrayEndPos >= Size) {
        fprintf(stderr, "Array Size exceeds file size. File likely corrupt.\n");
        SAFE_FREE(SaveFile);
        return -1;
    }

    uChar* buffer = malloc(Size - ArrayEndPos);
    if (!buffer){
        perror("Failed to allocate memory to update the array");
        SAFE_FREE(SaveFile);
        return -1;
    }

    memcpy(buffer, &SaveFile[ArrayEndPos], Size - ArrayEndPos);

    int DynArraySize = 10;
    int DynArrayElementCount = 0;

    char** fileNames = malloc(DynArraySize * sizeof(char*));
    if (!fileNames){
        perror("Failed to allocate memory for the Dynamic Array");
        SAFE_FREE(buffer);
        SAFE_FREE(SaveFile);
        return -1;
    }

    PatchedSize = (Size - ArrayEndPos) + ArrayStartPos;

    WIN32_FIND_DATA findData;
    char searchPath[MAX_PATH];
    BuildPath(searchPath, sizeof(searchPath), basePath, "MULTIPLAYER*.sav");
    HANDLE hFind = FindFirstFile(searchPath, &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                const char* fileName = findData.cFileName;
                const int StrLen = strlen(fileName) - 4;
                if (StrLen < 1) continue;

                if (DynArrayElementCount >= DynArraySize) {
                    DynArraySize *= 2;
                    char** temp = realloc(fileNames, DynArraySize * sizeof(char*));
                    if (!temp) {
                        perror("Failed to reallocate memory when updating the array");
                        ArrFree(fileNames, DynArrayElementCount);
                        free(fileNames);
                        SAFE_FREE(buffer);
                        SAFE_FREE(SaveFile);
                        FindClose(hFind);
                        return -1;
                    }
                    fileNames = temp;
                }

                fileNames[DynArrayElementCount] = malloc(StrLen + 1);
                if (!fileNames[DynArrayElementCount]){
                    perror("Failed to allocate memory when updating the array");
                    ArrFree(fileNames, DynArrayElementCount);
                    free(fileNames);
                    SAFE_FREE(buffer);
                    SAFE_FREE(SaveFile);
                    FindClose(hFind);
                    return -1;
                }
                snprintf(fileNames[DynArrayElementCount], StrLen + 1, "%.*s", StrLen, fileName);
                fileNames[DynArrayElementCount][StrLen] = '\0';

                PatchedSize += (Int32Size + StrLen + 1);
                PatchedArraySize += (Int32Size + StrLen + 1);
                DynArrayElementCount++;
            }
        } while (FindNextFile(hFind, &findData));
        FindClose(hFind);
    }

    PatchedArrayCount = DynArrayElementCount;

    uChar* PatchedSaveFile = malloc(PatchedSize);
    if (!PatchedSaveFile){
        perror("Failed to allocate memory for the patched save");
        ArrFree(fileNames, DynArrayElementCount);
        free(fileNames);
        SAFE_FREE(buffer);
        SAFE_FREE(SaveFile);
        return -1;
    }
    memcpy(PatchedSaveFile, SaveFile, ArrayStartPos);
    SAFE_FREE(SaveFile);

    memcpy(&PatchedSaveFile[ArraySizePos], &PatchedArraySize, Int64Size);
    memcpy(&PatchedSaveFile[ArrayCountPos], &PatchedArrayCount, Int32Size);

    size_t ptr = ArrayStartPos;
    uint32_t StrLen = 0;
    for (int i = 0; i < DynArrayElementCount; i++){
        StrLen = strlen(fileNames[i]) + 1;
        memcpy(&PatchedSaveFile[ptr], &StrLen, Int32Size);
        ptr += Int32Size;
        memcpy(&PatchedSaveFile[ptr], fileNames[i], StrLen);
        ptr += StrLen;
    }
    ArrFree(fileNames, DynArrayElementCount);
    free(fileNames);

    memcpy(&PatchedSaveFile[ptr], buffer, Size - ArrayEndPos);
    SAFE_FREE(buffer);

    if(WriteSaveFile(PatchedSaveFile, PatchedSize, basePath) != 0){
        SAFE_FREE(PatchedSaveFile);
        return -1;
    }
    SAFE_FREE(PatchedSaveFile);
    return 0;
}

void MonitorDirectory(const char* basePath) {
    char buffer[16384];
    DWORD bytesReturned;
    HANDLE hDir = CreateFile(
        basePath,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (hDir == INVALID_HANDLE_VALUE) {
        perror("Failed to open save directory for automatic updates");
        getchar();
        return;
    }

    printf("Waiting for files.\n");
    while (1) {
        if (ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), FALSE, 
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesReturned, NULL, NULL)) {
            
            FILE_NOTIFY_INFORMATION* event = (FILE_NOTIFY_INFORMATION*)buffer;
            int needsUpdate = 0;

            do {
                const WCHAR* filename = event->FileName;
                const int len = event->FileNameLength / sizeof(WCHAR);

                if (len > 15 && wcsncmp(filename, L"MULTIPLAYER", 11) == 0) {
                        WCHAR ext[5];
                        wcsncpy(ext, &filename[len - 4], 4);
                        ext[4] = L'\0';
                    if (wcscmp(ext, L".sav") == 0) {
                        needsUpdate = 1;
                    }
                }

                if (event->NextEntryOffset == 0) break;
                event = (FILE_NOTIFY_INFORMATION*)((BYTE*)event + event->NextEntryOffset);

            } while (1);

            if (needsUpdate) {
                printf("\nChange detected. Updating save array...\n");
                Sleep(100);

                if (UpdateArray(basePath) == 0) {
                    printf("MAINSAVE.sav successfully updated\n");
                }
            }
        }
    }
}

int main(){
    printf("Starting...\n");

    char basePath[MAX_PATH];
    const char *localappdata = getenv("LOCALAPPDATA");
    if (!localappdata) {
        fprintf(stderr, "Failed to get LOCALAPPDATA path.\n");
        return -1;
    }
    snprintf(basePath, sizeof(basePath), "%s\\EscapeTheBackrooms\\Saved\\SaveGames", localappdata);
    printf("Found Save Folder.\n");

    UpdateArray(basePath);

    printf("Initial rebuild to remove old files complete.\n");

    MonitorDirectory(basePath);

    return 0;
}
