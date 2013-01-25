//
//  main.c
//  FileSystem
//
//  Created by Aha on 13-1-21.
//  Copyright (c) 2013年 Aha. All rights reserved.
//

#include<fcntl.h>
#include<unistd.h>
#include<sys/stat.h>
#include<dirent.h>
#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TRUE    1
#define FALSE   0

#define COMMAND_NAME_MAX_LENGTH     20
#define COMMAND_CONTENT_MAX_LENGTH  256
#define WELCOME_COMMAND_NUM         3
#define MENU_COMMAND_NUM            7
#define USER_NAME_LENGTH            32
#define USER_PASWD_LENGTH           32
#define USER_MAX_NUM                64
#define INODE_DIRECTORY_RECORD_NAME_LENGTH      16
#define INODE_DIRECTORY_RECORD_TYPE_LENGTH      1
#define INODE_DIRECTORY_RECORD_INDEX_LENGTH     15
#define INODE_FILE_RECORD_INDEX_LENGTH          16
#define INODE_RECORD_MAX_NUM        128
#define FILE_BLOCK_SIZE             4096
#define INODE_DIRECTORY_NAME        "inode"
#define DISK_DIRECTORY_NAME         "disk"
#define INODE_PREFIX                "inode"
#define FILE_PREFIX                 "file"
#define RECYCLED_INODEINDEX_FILE_NAME       "recycledInodeIndex"
#define RECYCLED_FILEINDEX_FILE_NAME        "recycledFileIndex"
#define SYSTEM_INFORMATION_FILE_NAME        "systemInformation"


typedef enum _SystemState{
    ExitState,
    InitState,
    LoginingState,
    RegistingState,
    MenuState,
    ExitingState,
    DiringState,
    CreatingState,
    DeletingState,
    ReadingState,
    WritingState
} SystemState;

typedef struct _User{
    int isValid;
    int homeInode;
    int homeDirFileVirNum;
    char name[USER_NAME_LENGTH];
    char passwd[USER_PASWD_LENGTH];
} User;

typedef enum _RecordType{
    DirectoryRecord,
    FileRecord,
    UserRecord,
    SystemInformationRecord,
    RecycledFileRecord,
    RecycledInodeRecord
} RecordType;

typedef struct _UserFileRecord {
    char name[USER_NAME_LENGTH];
    char passwd[USER_PASWD_LENGTH];
} UserFileRecord;

typedef enum _InodeDirectoryRecordType {
    Directory,
    UnRWFile,
    RFile,
    RWFile
} InodeDirectoryRecordType;

typedef struct _InodeDirectoryRecord {
    char type;
    char name[INODE_DIRECTORY_RECORD_NAME_LENGTH];
    char inodeIndex[INODE_DIRECTORY_RECORD_INDEX_LENGTH];
} InodeDirectoryRecord;

typedef struct _InodeFileRecord {
    char fileIndex[INODE_FILE_RECORD_INDEX_LENGTH];
} InodeFileRecord;

//recycled record
typedef struct _RecycledInodeIndexRecord {
    char inodeIndex[INODE_DIRECTORY_RECORD_INDEX_LENGTH];
} RecycledInodeIndexRecord;

typedef struct _RecycledFileIndexRecord {
    char fileIndex[INODE_FILE_RECORD_INDEX_LENGTH];
} RecycledFileIndexRecord;

typedef struct _SystemInformation {
    int inodeMaxIndex;
    int fileMaxIndex;
} SystemInformation;



//global variable

SystemState gCurrentSystemState;
User gCurrentUser;
UserFileRecord gUserTable[USER_MAX_NUM];
int gUserNum;
char gCommandBuffer[COMMAND_CONTENT_MAX_LENGTH];
char gWelcomeCommand[WELCOME_COMMAND_NUM][COMMAND_NAME_MAX_LENGTH];
char gMenuCommand[MENU_COMMAND_NUM][COMMAND_NAME_MAX_LENGTH];
int willDoNothing = FALSE;
SystemInformation gSystemInformation;
InodeDirectoryRecord gDirectoryInodeBuffer[FILE_BLOCK_SIZE/(INODE_DIRECTORY_RECORD_NAME_LENGTH+INODE_DIRECTORY_RECORD_TYPE_LENGTH+INODE_DIRECTORY_RECORD_INDEX_LENGTH)];
InodeFileRecord gFileInodeBuffer[FILE_BLOCK_SIZE/INODE_FILE_RECORD_INDEX_LENGTH];
char gCommandParameter[5][32];
char gFileBockBuffer[FILE_BLOCK_SIZE];

//the function name
void initSomething();
void showSomething();
void getSomething();
void doSomething();
void releaseSomething();
int createFile(const char *filePath);
int insertRecordIntoFile(void *record, const char *path, RecordType recordType);
int deleteRecordFromFile(const char *path, RecordType recordType, int index);
int getInodeIndex();
int getFileIndex();
void initUserTable();
void initSystemInfo();
void loadHomeDirInode();

void loadHomeDirInode(){
    char path[256];
    memset(path, 0, 256);
    sprintf(path, "%s/%s%d", INODE_DIRECTORY_NAME, INODE_PREFIX, 2);
    FILE *fp;
	if ((fp = fopen(path, "r")) == NULL)
	{
		printf("Error!Can't load user directory!\n");
		gCurrentSystemState = InitState;
        willDoNothing = TRUE;
	}
    else{
        memset(gDirectoryInodeBuffer, 0, sizeof(gDirectoryInodeBuffer));
        fread(gDirectoryInodeBuffer, sizeof(InodeDirectoryRecord), gUserNum, fp);
        fclose(fp);
        int index;
        for (index = 0; index < gUserNum; index++) {
            if (!strcmp(gDirectoryInodeBuffer[index].name, gCurrentUser.name)) {
                break;
            }
        }
        //the home inode path
        memset(path, 0, 256);
        sprintf(path, "%s/%s%s", INODE_DIRECTORY_NAME, INODE_PREFIX, gDirectoryInodeBuffer[index].inodeIndex);
        //update gUser
        gCurrentUser.homeInode = atoi(gDirectoryInodeBuffer[index].inodeIndex);
        struct stat statBuff;
        stat(path, &statBuff);
        gCurrentUser.homeDirFileVirNum = statBuff.st_size/sizeof(InodeDirectoryRecord);
        //load home directory inode
        if ((fp = fopen(path, "r")) == NULL)
        {
            printf("Error!Can't load home directory!\n");
            gCurrentSystemState = InitState;
            willDoNothing = TRUE;
        }
        else{
            memset(gDirectoryInodeBuffer, 0, sizeof(gDirectoryInodeBuffer));
            fread(gDirectoryInodeBuffer, sizeof(InodeDirectoryRecord), gCurrentUser.homeDirFileVirNum, fp);
            fclose(fp);
        }
    }
}

void initUserTable(){
    memset(gCommandBuffer, 0, sizeof(gCommandBuffer));
    sprintf(gCommandBuffer, "%s/%s%d", DISK_DIRECTORY_NAME, FILE_PREFIX, 0);
    struct stat fileStat;
    stat(gCommandBuffer, &fileStat);
    gUserNum = (fileStat.st_size)/(USER_NAME_LENGTH+USER_PASWD_LENGTH);
	FILE *fp;
	if ((fp = fopen(gCommandBuffer, "r")) == NULL)
	{
		printf("Error!Can't open user file!\n");
		gCurrentSystemState = ExitState;
	}
    else{
        memset(gUserTable, 0, sizeof(gUserTable));
        fread(gUserTable, sizeof(User), gUserNum, fp);
        fclose(fp);
    }
}

int getInodeIndex(){
    FILE *file;
    int index;
    struct stat statBuff;
    stat(RECYCLED_INODEINDEX_FILE_NAME, &statBuff);
    long size = statBuff.st_size;
    if ((file = fopen(RECYCLED_INODEINDEX_FILE_NAME, "r")) != NULL){
        int recordNum = size/sizeof(RecycledInodeIndexRecord);
        int readIndex;
        RecycledInodeIndexRecord *recordBuffer = malloc(sizeof(RecycledInodeIndexRecord)*recordNum);
        fread(recordBuffer, sizeof(RecycledInodeIndexRecord), recordNum, file);
        fclose(file);
        for (readIndex = 0; readIndex < recordNum; readIndex++) {
            if (strcmp(recordBuffer[readIndex].inodeIndex, "-1")) {
                break;
            }
        }
        if (readIndex < recordNum) {
            index = atoi(recordBuffer[readIndex].inodeIndex);
            deleteRecordFromFile(RECYCLED_INODEINDEX_FILE_NAME, RecycledInodeRecord, readIndex);
        }
        else{
            gSystemInformation.inodeMaxIndex++;
            index = gSystemInformation.inodeMaxIndex;
        }
        free(recordBuffer);
    }
    return index;
}

int getFileIndex(){
    FILE *file;
    int index;
    struct stat statBuff;
    stat(RECYCLED_FILEINDEX_FILE_NAME, &statBuff);
    long size = statBuff.st_size;
    if ((file = fopen(RECYCLED_FILEINDEX_FILE_NAME, "r")) != NULL){
        int recordNum = size/sizeof(RecycledFileIndexRecord);
        int readIndex;
        RecycledFileIndexRecord *recordBuffer = malloc(sizeof(RecycledFileIndexRecord)*recordNum);
        fread(recordBuffer, sizeof(RecycledFileIndexRecord), recordNum, file);
        fclose(file);
        for (readIndex = 0; readIndex < recordNum; readIndex++) {
            if (strcmp(recordBuffer[readIndex].fileIndex, "-1")) {
                break;
            }
        }
        if (readIndex < recordNum) {
            index = atoi(recordBuffer[readIndex].fileIndex);
            deleteRecordFromFile(RECYCLED_FILEINDEX_FILE_NAME, RecycledFileRecord, readIndex);
        }
        else{
            gSystemInformation.fileMaxIndex++;
            index = gSystemInformation.fileMaxIndex;
        }
        free(recordBuffer);
    }
    return index;
}

int createFile(const char *filePath){
    FILE *file;
    if ((file = fopen(filePath, "w")) != NULL){
        fclose(file);
        return TRUE;
    }
    else{
        return FALSE;
    }
}

void initSystemInfo(){
    FILE *file;
    struct stat statBuff;
    stat(SYSTEM_INFORMATION_FILE_NAME, &statBuff);
    if (statBuff.st_size > 0 && (file = fopen(SYSTEM_INFORMATION_FILE_NAME, "r")) != NULL){
        fread(&gSystemInformation, sizeof(SystemInformation), 1, file);
        fclose(file);
    }
}

int deleteRecordFromFile(const char *path, RecordType recordType, int index){
    struct stat statBuff;
    stat(path, &statBuff);
    long size = statBuff.st_size;
    FILE *file;
    if ((file = fopen(path, "r")) != NULL){
        switch (recordType) {
            case DirectoryRecord:
            {
                //read the block
                int recordNum = size/sizeof(InodeDirectoryRecord);
                InodeDirectoryRecord *recordBuffer = malloc(sizeof(InodeDirectoryRecord)*recordNum);
                fread(recordBuffer, sizeof(InodeDirectoryRecord), recordNum, file);
                fclose(file);
                //mark that record will be deleted
                strcpy(recordBuffer[index].inodeIndex, "-1");
                //write back
                if ((file = fopen(path, "w")) != NULL){
                    fwrite(recordBuffer, sizeof(InodeDirectoryRecord), recordNum, file);
                }
                free(recordBuffer);
            }
                break;
            case FileRecord:
            {
                //read the block
                int recordNum = size/sizeof(InodeFileRecord);
                InodeFileRecord *recordBuffer = malloc(sizeof(InodeFileRecord)*recordNum);
                fread(recordBuffer, sizeof(InodeFileRecord), recordNum, file);
                fclose(file);
                //mark that record will be deleted
                strcpy(recordBuffer[index].fileIndex, "-1");
                //write back
                if ((file = fopen(path, "w")) != NULL){
                    fwrite(recordBuffer, sizeof(InodeFileRecord), recordNum, file);
                }
                free(recordBuffer);
            }
                break;
            case UserRecord:
            {
                //read the block
                int recordNum = size/sizeof(UserFileRecord);
                UserFileRecord *recordBuffer = malloc(sizeof(UserFileRecord)*recordNum);
                fread(recordBuffer, sizeof(UserFileRecord), recordNum, file);
                fclose(file);
                //mark that record will be deleted
                strcpy(recordBuffer[index].name, "back");
                //write back
                if ((file = fopen(path, "w")) != NULL){
                    fwrite(recordBuffer, sizeof(UserFileRecord), recordNum, file);
                }
                free(recordBuffer);
            }
                break;
            case RecycledFileRecord:
            {
                //read the block
                int recordNum = size/sizeof(RecycledFileIndexRecord);
                RecycledFileIndexRecord *recordBuffer = malloc(sizeof(RecycledFileIndexRecord)*recordNum);
                fread(recordBuffer, sizeof(RecycledFileIndexRecord), recordNum, file);
                fclose(file);
                //mark that record will be deleted
                strcpy(recordBuffer[index].fileIndex, "-1");
                //write back
                if ((file = fopen(path, "w")) != NULL){
                    fwrite(recordBuffer, sizeof(RecycledFileIndexRecord), recordNum, file);
                }
                free(recordBuffer);
            }
                break;
            case RecycledInodeRecord:
            {
                //read the block
                int recordNum = size/sizeof(RecycledInodeIndexRecord);
                RecycledInodeIndexRecord *recordBuffer = malloc(sizeof(RecycledInodeIndexRecord)*recordNum);
                fread(recordBuffer, sizeof(RecycledInodeIndexRecord), recordNum, file);
                fclose(file);
                //mark that record will be deleted
                strcpy(recordBuffer[index].inodeIndex, "-1");
                //write back
                if ((file = fopen(path, "w")) != NULL){
                    fwrite(recordBuffer, sizeof(RecycledInodeIndexRecord), recordNum, file);
                }
                free(recordBuffer);
            }
                break;
            default:
                return FALSE;
        }
        fclose(file);
        return TRUE;
    }
    return FALSE;
}

int insertRecordIntoFile(void *record, const char *path, RecordType recordType){
    FILE *file;
    struct stat statBuff;
    stat(path, &statBuff);
    long size = statBuff.st_size;
    if ((file = fopen(path, "r")) != NULL){
        switch (recordType) {
            case DirectoryRecord:
            {
                int maxRecordNum = FILE_BLOCK_SIZE/sizeof(InodeDirectoryRecord);
                int recordNum = size/sizeof(InodeDirectoryRecord);
                int insertRecordIndex;
                InodeDirectoryRecord *recordBuffer = malloc(sizeof(InodeDirectoryRecord)*(recordNum+1));
                memset(recordBuffer, 0, sizeof(InodeDirectoryRecord)*(recordNum+1));
                fread(recordBuffer, sizeof(InodeDirectoryRecord), recordNum, file);
                for (insertRecordIndex = 0; insertRecordIndex < recordNum; insertRecordIndex++) {
                    if (!strcmp(recordBuffer[insertRecordIndex].inodeIndex, "-1")) {
                        break;
                    }
                }
                if (insertRecordIndex < maxRecordNum) {
                    recordBuffer[insertRecordIndex].type = ((InodeDirectoryRecord *)record)->type;
                    strcpy(recordBuffer[insertRecordIndex].inodeIndex, ((InodeDirectoryRecord *)record)->inodeIndex);
                    strcpy(recordBuffer[insertRecordIndex].name, ((InodeDirectoryRecord *)record)->name);
                    fclose(file);
                    if ((file = fopen(path, "w")) != NULL) {
                        if (insertRecordIndex < recordNum) {
                            fwrite(recordBuffer, sizeof(InodeDirectoryRecord), recordNum, file);
                        } else {
                            fwrite(recordBuffer, sizeof(InodeDirectoryRecord), recordNum+1, file);
                        }
                    }
                    
                }
                else{
                    return FALSE;
                }
                free(recordBuffer);
                
            }
                break;
            case FileRecord:
            {
                int maxRecordNum = FILE_BLOCK_SIZE/sizeof(InodeFileRecord);
                int recordNum = size/sizeof(InodeFileRecord);
                int insertRecordIndex;
                InodeFileRecord *recordBuffer = malloc(sizeof(InodeFileRecord)*(recordNum+1));
                memset(recordBuffer, 0, sizeof(InodeFileRecord)*(recordNum+1));
                fread(recordBuffer, sizeof(InodeFileRecord), recordNum, file);
                for (insertRecordIndex = 0; insertRecordIndex < recordNum; insertRecordIndex++) {
                    if (!strcmp(recordBuffer[insertRecordIndex].fileIndex, "-1")) {
                        break;
                    }
                }
                if (insertRecordIndex < maxRecordNum) {
                    memset(&recordBuffer[insertRecordIndex], 0, sizeof(InodeFileRecord));
                    strcpy(recordBuffer[insertRecordIndex].fileIndex, ((InodeFileRecord *)record)->fileIndex);
                    fclose(file);
                    if ((file = fopen(path, "w")) != NULL) {
                        if (insertRecordIndex < recordNum) {
                            fwrite(recordBuffer, sizeof(InodeFileRecord), recordNum, file);
                        } else {
                            fwrite(recordBuffer, sizeof(InodeFileRecord), recordNum+1, file);
                        }
                    }
                }
                else{
                    return FALSE;
                }
                free(recordBuffer);
            }
                break;
            case UserRecord:
            {
                int maxRecordNum = FILE_BLOCK_SIZE/sizeof(UserFileRecord);
                int recordNum = size/sizeof(UserFileRecord);
                int insertRecordIndex;
                UserFileRecord *recordBuffer = malloc(sizeof(UserFileRecord)*(recordNum+1));
                memset(recordBuffer, 0, sizeof(UserFileRecord)*(recordNum+1));
                fread(recordBuffer, sizeof(UserFileRecord), recordNum, file);
                for (insertRecordIndex = 0; insertRecordIndex < recordNum; insertRecordIndex++) {
                    if (!strcmp(recordBuffer[insertRecordIndex].name, "back")) {
                        break;
                    }
                }
                if (insertRecordIndex < maxRecordNum) {
                    memset(&recordBuffer[insertRecordIndex], 0, sizeof(UserFileRecord));
                    strcpy(recordBuffer[insertRecordIndex].name, ((UserFileRecord *)record)->name);
                    strcpy(recordBuffer[insertRecordIndex].passwd, ((UserFileRecord *)record)->passwd);
                    fclose(file);
                    if ((file = fopen(path, "w")) != NULL) {
                        if (insertRecordIndex < recordNum) {
                            fwrite(recordBuffer, sizeof(UserFileRecord), recordNum, file);
                        } else {
                            fwrite(recordBuffer, sizeof(UserFileRecord), recordNum+1, file);
                        }
                    }
                }
                else{
                    return FALSE;
                }
                free(recordBuffer);
            }
                break;
            case RecycledFileRecord:
            {
                int recordNum = size/sizeof(RecycledFileIndexRecord);
                int insertRecordIndex;
                RecycledFileIndexRecord *recordBuffer = malloc(sizeof(RecycledFileIndexRecord)*(recordNum+1));
                memset(recordBuffer, 0, sizeof(RecycledFileIndexRecord)*(recordNum+1));
                fread(recordBuffer, sizeof(RecycledFileIndexRecord), recordNum, file);
                for (insertRecordIndex = 0; insertRecordIndex < recordNum; insertRecordIndex++) {
                    if (!strcmp(recordBuffer[insertRecordIndex].fileIndex, "-1")) {
                        break;
                    }
                }
                strcpy(recordBuffer[insertRecordIndex].fileIndex, ((RecycledFileIndexRecord *)record)->fileIndex);
                fclose(file);
                if ((file = fopen(path, "w")) != NULL) {
                    if (insertRecordIndex < recordNum) {
                        fwrite(recordBuffer, sizeof(RecycledFileIndexRecord), recordNum, file);
                    } else {
                        fwrite(recordBuffer, sizeof(RecycledFileIndexRecord), recordNum+1, file);
                    }
                }
                free(recordBuffer);
            }
                break;
            case RecycledInodeRecord:
            {
                int recordNum = size/sizeof(RecycledInodeIndexRecord);
                int insertRecordIndex;
                RecycledInodeIndexRecord *recordBuffer = malloc(sizeof(RecycledInodeIndexRecord)*(recordNum+1));
                memset(recordBuffer, 0, sizeof(RecycledInodeIndexRecord)*(recordNum+1));
                fread(recordBuffer, sizeof(RecycledInodeIndexRecord), recordNum, file); 
                for (insertRecordIndex = 0; insertRecordIndex < recordNum; insertRecordIndex++) {
                    if (!strcmp(recordBuffer[insertRecordIndex].inodeIndex, "-1")) {
                        break;
                    }
                }
                strcpy(recordBuffer[insertRecordIndex].inodeIndex, ((RecycledInodeIndexRecord *)record)->inodeIndex);
                fclose(file);
                if ((file = fopen(path, "w")) != NULL) {
                    if (insertRecordIndex < recordNum) {
                        fwrite(recordBuffer, sizeof(RecycledInodeIndexRecord), recordNum, file);
                    } else {
                        fwrite(recordBuffer, sizeof(RecycledInodeIndexRecord), recordNum+1, file);
                    }
                }
                free(recordBuffer);
            }
                break;
            case SystemInformationRecord:
                fwrite(record, sizeof(SystemInformation), 1, file);
                break;
            default:
                return FALSE;
        }
        fclose(file);
        return TRUE;
    }
    return FALSE;
}

void initSomething(){
    //init the user
    gCurrentUser.isValid = FALSE;
    //init the state
    gCurrentSystemState = InitState;
    //create directory
    mkdir(INODE_DIRECTORY_NAME, S_IRWXU);
    mkdir(DISK_DIRECTORY_NAME, S_IRWXU);
    createFile(RECYCLED_INODEINDEX_FILE_NAME);
    createFile(RECYCLED_FILEINDEX_FILE_NAME);
    //create inode0 if no exist, the / directory
    sprintf(gCommandBuffer, "%s/%s%d", INODE_DIRECTORY_NAME, INODE_PREFIX, 0);
    struct stat statBuff;
    if(stat(gCommandBuffer, &statBuff) < 0){
        //if not exist then create it
        createFile(gCommandBuffer);
        InodeDirectoryRecord directoryRecord;
        //insert etc directory
        directoryRecord.type = Directory;
        strcpy(directoryRecord.name, "etc");
        strcpy(directoryRecord.inodeIndex, "1");
        insertRecordIntoFile(&directoryRecord, gCommandBuffer, DirectoryRecord);
        //insert user directory
        memset(directoryRecord.name, 0, sizeof(directoryRecord.name));
        memset(directoryRecord.inodeIndex, 0, sizeof(directoryRecord.inodeIndex));
        strcpy(directoryRecord.name, "user");
        strcpy(directoryRecord.inodeIndex, "2");
        insertRecordIntoFile(&directoryRecord, gCommandBuffer, DirectoryRecord);
        //create etc directory inode
        memset(gCommandBuffer, 0, sizeof(gCommandBuffer));
        sprintf(gCommandBuffer, "%s/%s%d", INODE_DIRECTORY_NAME, INODE_PREFIX, 1);
        createFile(gCommandBuffer);
        directoryRecord.type = UnRWFile;
        memset(directoryRecord.name, 0, sizeof(directoryRecord.name));
        memset(directoryRecord.inodeIndex, 0, sizeof(directoryRecord.inodeIndex));
        strcpy(directoryRecord.name, "passwd");
        strcpy(directoryRecord.inodeIndex, "3");
        insertRecordIntoFile(&directoryRecord, gCommandBuffer, DirectoryRecord);
        //create user directory inode
        memset(gCommandBuffer, 0, sizeof(gCommandBuffer));
        sprintf(gCommandBuffer, "%s/%s%d", INODE_DIRECTORY_NAME, INODE_PREFIX, 2);
        createFile(gCommandBuffer);
        //create user file inode named inode 3
        memset(gCommandBuffer, 0, sizeof(gCommandBuffer));
        sprintf(gCommandBuffer, "%s/%s%d", INODE_DIRECTORY_NAME, INODE_PREFIX, 3);
        createFile(gCommandBuffer);
        InodeFileRecord fileRecord;
        strcpy(fileRecord.fileIndex, "0");
        insertRecordIntoFile(&fileRecord, gCommandBuffer, FileRecord);
        //create user file named file0
        memset(gCommandBuffer, 0, sizeof(gCommandBuffer));
        sprintf(gCommandBuffer, "%s/%s%d", DISK_DIRECTORY_NAME, FILE_PREFIX, 0);
        createFile(gCommandBuffer);
    }
    
    //init command content
    strcpy(gWelcomeCommand[0], "login");
    strcpy(gWelcomeCommand[1], "regist");
    strcpy(gWelcomeCommand[2], "exit");
    strcpy(gMenuCommand[0], "dir");
    strcpy(gMenuCommand[1], "create");
    strcpy(gMenuCommand[2], "delete");
    strcpy(gMenuCommand[3], "read");
    strcpy(gMenuCommand[4], "write");
    strcpy(gMenuCommand[5], "logout");
    strcpy(gMenuCommand[6], "exit");
    //init user table
    initUserTable();
    //init system information
    if(stat(SYSTEM_INFORMATION_FILE_NAME, &statBuff) < 0){
        gSystemInformation.fileMaxIndex = 0;
        gSystemInformation.inodeMaxIndex = 3;
        createFile(SYSTEM_INFORMATION_FILE_NAME);
        insertRecordIntoFile(&gSystemInformation, SYSTEM_INFORMATION_FILE_NAME, SystemInformationRecord);
    }
    else{
        initSystemInfo();
    }
}

void releaseSomething(){
    //free memory
    free(gUserTable);
    //update system information
    memset(gCommandBuffer, 0, sizeof(gCommandBuffer));
    sprintf(gCommandBuffer, "%s", SYSTEM_INFORMATION_FILE_NAME);
    FILE *file;
    if ((file = fopen(gCommandBuffer, "w")) != NULL){
        fwrite(&gSystemInformation, sizeof(SystemInformationRecord), 1, file);
        fclose(file);
    }
}

//definition
void showSomething(){
    system("clear");
    switch (gCurrentSystemState) {
        case InitState:
        {
            memset(&gCurrentUser, 0, sizeof(User));
            gCurrentUser.isValid = FALSE;
            printf("*****************Welcome********************\n");
            printf(">login          (login the system)\n");
            printf(">regist         (regist a user)\n");
            printf(">exit           (exit the system)\n");
            printf("********************************************\n");
            printf("Please input the command:>");
        }
            break;
        case LoginingState:
        {
            memset(&gCurrentUser, 0, sizeof(User));
            gCurrentUser.isValid = FALSE;
            printf("**************Login Windows*****************\n");
            printf("You can back to the previous window when input 'back' behind Name or Password!\n");
            printf("Name:>");
        }
            break;
        case RegistingState:
        {
            memset(&gCurrentUser, 0, sizeof(User));
            gCurrentUser.isValid = FALSE;
            printf("**************Regist Windows****************\n");
            printf("The user name and password must be less than %i characters!\n", USER_MAX_NUM);
            printf("You can back to the previous window when input 'back' behind Name or Password!\n");
            printf("Name:>");
        }
            break;
        case MenuState:
        {
            loadHomeDirInode();
            printf("********************Menu********************\n");
            printf("Hellow,%s! You hava the choice below:\n", gCurrentUser.name);
            printf(">dir                        (show all files)\n");
            printf(">create [-rw] #filename#    (you can create a file, defaulted -r)\n");
            printf(">delete #filename#          (you can delete a file)\n");
            printf(">read #filename#            (you can read a file)\n");
            printf(">wirte [-a-w]#filename#     (you can append something or cover original content in a file, defaulted -a)\n");
            printf(">logout                     (login from this user)\n");
            printf(">exit                       (exit the system)\n");
            printf("********************************************\n");
            printf("Please input the command:>");
        }
            break;
        case ExitingState:
            printf("The system is exiting.\n");
            break;
        case DiringState:
        {
            printf("********************Directory Content********************\n");
            printf("Hellow,%s! You can go back by pressing key 'q' and then press enter!\n", gCurrentUser.name);
            printf("FileName\tPhysicalAddr\tProtectionCode\tFileLength\n");
        }
            break;
        case CreatingState:
        {
            memset(gCommandParameter, 0, sizeof(gCommandParameter));
            scanf("%s", gCommandParameter[1]);
            if (gCommandParameter[1][0] == '-') {
                strcpy(gCommandParameter[0], "PandF");
                scanf("%s", gCommandParameter[2]);
                if (!strcmp(gCommandParameter[1], "-rw") || !strcmp(gCommandParameter[1], "-w")) {
                    printf("Are you sure to create a file which can be read and written?(y/n):>\n");
                }
                else{
                    printf("Are you sure to create a file which just can be read?(y/n):>\n");
                }
            }
            else{
                strcpy(gCommandParameter[0], "JustF");
                printf("Are you sure to create a file which just can be read?(y/n):>\n");
            }
        }
            break;
        case DeletingState:
        {
            memset(gCommandParameter, 0, sizeof(gCommandParameter));
            scanf("%s", gCommandParameter[0]);
            printf("Would you like to delete the file name '%s'?(y/n):>\n", gCommandParameter[0]);
        }
            break;
        case ReadingState:
        {
            memset(gCommandParameter, 0, sizeof(gCommandParameter));
            scanf("%s", gCommandParameter[0]);
        }
            break;
        case WritingState:
        {
            //flush the block buffer
            memset(gFileBockBuffer, 0, sizeof(gFileBockBuffer));
            //read the command parameters
            memset(gCommandParameter, 0, sizeof(gCommandParameter));
            scanf("%s", gCommandParameter[1]);
            if (gCommandParameter[1][0] == '-') {
                strcpy(gCommandParameter[0], "PandF");
                scanf("%s", gCommandParameter[2]);
                if (!strcmp(gCommandParameter[1], "-a")) {
                    printf("Would you like to append content below in file '%s'?(y/n):>\n", gCommandParameter[2]);
                }
                else{
                    printf("Are you sure to over the content in file '%s'? The original content will be lost!(y/n):>\n", gCommandParameter[2]);
                }
            }
            else{
                strcpy(gCommandParameter[0], "JustF");
                printf("Would you like to append content below in file '%s'?(y/n):>\n", gCommandParameter[1]);
            }
        }
            break;
        default:
            break;
    }
}

void getSomething(){
    setbuf(stdin, NULL);
    memset(gCommandBuffer, 0, sizeof(gCommandBuffer));
    switch (gCurrentSystemState) {
        case InitState:
            scanf("%s", gCommandBuffer);
            break;
        case LoginingState:
        {
            scanf("%s", gCommandBuffer);
            if (!strcmp(gCommandBuffer, "back")) {
                gCurrentSystemState = InitState;
                willDoNothing = TRUE;
                break;
            }
            if (strlen(gCommandBuffer) > USER_NAME_LENGTH-1) {
                printf("The name is too long.\nPress enter to continue...");
                getchar();
            }
            strcpy(gCurrentUser.name, gCommandBuffer);
            setbuf(stdin, NULL);
            printf("Password:>");
            memset(gCommandBuffer, 0, sizeof(gCommandBuffer));
            scanf("%s", gCommandBuffer);
            if (!strcmp(gCommandBuffer, "back")) {
                gCurrentSystemState = InitState;
                willDoNothing = TRUE;
                break;
            }
            strcpy(gCurrentUser.passwd, gCommandBuffer);
            gCurrentUser.isValid = TRUE;
        }
            break;
        case RegistingState:
        {
            scanf("%s", gCommandBuffer);
            if (!strcmp(gCommandBuffer, "back")) {
                gCurrentSystemState = InitState;
                willDoNothing = TRUE;
                break;
            }
            if (strlen(gCommandBuffer) > USER_NAME_LENGTH-1) {
                printf("The name is too long.\nPress enter to continue...");
                getchar();
            }
            else{
                strcpy(gCurrentUser.name, gCommandBuffer);
                setbuf(stdin, NULL);
                printf("Password:>");
                memset(gCommandBuffer, 0, sizeof(gCommandBuffer));
                scanf("%s", gCommandBuffer);
                if (!strcmp(gCommandBuffer, "back")) {
                    gCurrentSystemState = InitState;
                    willDoNothing = TRUE;
                    break;
                }
                if (strlen(gCommandBuffer) > USER_PASWD_LENGTH-1) {
                    printf("The password is too long.\nPress enter to continue...");
                    getchar();
                }
                else{
                    strcpy(gCurrentUser.passwd, gCommandBuffer);
                    gCurrentUser.isValid = TRUE;
                }
            }
        }
            break;
        case MenuState:
            scanf("%s", gCommandBuffer);
            break;
        case ExitingState:
        case DiringState:
            break;
        case CreatingState:
            scanf("%s", gCommandBuffer);
            break;
        case DeletingState:
            scanf("%s", gCommandBuffer);
            break;
        case ReadingState:
            break;
        case WritingState:
            scanf("%s", gCommandBuffer);
            break;
        default:
            break;
    }
    
}

void doSomething(){
    if (willDoNothing) {
        willDoNothing = FALSE;
    } else {
        switch (gCurrentSystemState) {
            case InitState:
            {
                int select;
                for (select = 0; select < WELCOME_COMMAND_NUM; select++) {
                    if (!strcmp(gCommandBuffer, gWelcomeCommand[select])) {
                        break;
                    }
                }
                switch (select) {
                    case 0:
                        gCurrentSystemState = LoginingState;
                        break;
                    case 1:
                        gCurrentSystemState = RegistingState;
                        break;
                    case 2:
                        gCurrentSystemState = ExitingState;
                        break;
                    default:
                    {
                        printf("The command is wrong!!!\nPress enter to continue...");
                        setbuf(stdin, NULL);
                        getchar();
                    }
                        break;
                }
            }
                break;
            case LoginingState:
            {
                if (gCurrentUser.isValid) {
                    int i;
                    for (i = 0; i < gUserNum; i++) {
                        if (!strcmp(gUserTable[i].name, gCurrentUser.name) && !strcmp(gUserTable[i].passwd, gCurrentUser.passwd)) {
                            gCurrentSystemState = MenuState;
                            return;
                        }
                    }
                    printf("The user name or password is wrong. \n");
                    printf("Please retry. Press enter to continue. \n");
                    setbuf(stdin, NULL);
                    getchar();
                }
            }
                break;
            case RegistingState:
            {
                if (gCurrentUser.isValid) {
                    gCurrentUser.isValid = FALSE;
                    if (gUserNum > USER_MAX_NUM) {
                        printf("I'm sorry about that the number of users have reached max value.");
                        gCurrentSystemState = InitState;
                        break;
                    }
                    int i;
                    for (i = 0; i < gUserNum; i++) {
                        if (!strcmp(gUserTable[i].name, gCurrentUser.name)) {
                            printf("The user name has existed. \n");
                            printf("Please retry. Press enter to continue. \n");
                            setbuf(stdin, NULL);
                            getchar();
                            return;
                        }
                    }
                    //increase user num
                    gUserNum++;
                    //insert user record
                    memset(gCommandBuffer, 0, sizeof(gCommandBuffer));
                    sprintf(gCommandBuffer, "%s/%s%d", DISK_DIRECTORY_NAME, FILE_PREFIX, 0);
                    UserFileRecord newRecord;
                    strcpy(newRecord.name, gCurrentUser.name);
                    strcpy(newRecord.passwd, gCurrentUser.passwd);
                    insertRecordIntoFile(&newRecord, gCommandBuffer, UserRecord);
                    //create user directory
                    memset(gCommandBuffer, 0, sizeof(gCommandBuffer));
                    sprintf(gCommandBuffer, "%s/%s%d", INODE_DIRECTORY_NAME, INODE_PREFIX, 2);
                    InodeDirectoryRecord newDirectoryRecord;
                    memset(&newDirectoryRecord, 0, sizeof(InodeDirectoryRecord));
                    newDirectoryRecord.type = Directory;
                    strcpy(newDirectoryRecord.name, gCurrentUser.name);
                    sprintf(newDirectoryRecord.inodeIndex, "%d", getInodeIndex());
                    insertRecordIntoFile(&newDirectoryRecord, gCommandBuffer, DirectoryRecord);
                    
                    memset(gCommandBuffer, 0, sizeof(gCommandBuffer));
                    sprintf(gCommandBuffer, "%s/%s%s", INODE_DIRECTORY_NAME, INODE_PREFIX, newDirectoryRecord.inodeIndex);
                    createFile(gCommandBuffer);
                    //reinit user table
                    initUserTable();
                    gCurrentSystemState = InitState;
                }
            }
                break;
            case MenuState:
            {
                int select;
                for (select = 0; select < MENU_COMMAND_NUM; select++) {
                    if (!strcmp(gCommandBuffer, gMenuCommand[select])) {
                        break;
                    }
                }
                switch (select) {
                    case 0:
                        gCurrentSystemState = DiringState;
                        break;
                    case 1:
                        gCurrentSystemState = CreatingState;
                        break;
                    case 2:
                        gCurrentSystemState = DeletingState;
                        break;
                    case 3:
                        gCurrentSystemState = ReadingState;
                        break;
                    case 4:
                        gCurrentSystemState = WritingState;
                        break;
                    case 5:
                        gCurrentSystemState = InitState;
                        break;
                    case 6:
                        gCurrentSystemState = ExitingState;
                        break;
                    default:
                    {
                        printf("The command is wrong!!!\nPress enter to continue...");
                        setbuf(stdin, NULL);
                        getchar();
                    }
                        break;
                }
            }
                break;
            case ExitingState:
            {
                printf("The system has exited.");
                gCurrentSystemState = ExitState;
            }
                break;
            case DiringState:
            {
                char path[256];
                FILE *fp;
                struct stat statBuff;
                int index;
                for (index = 0; index < gCurrentUser.homeDirFileVirNum; index++) {
                    if (strcmp(gDirectoryInodeBuffer[index].inodeIndex, "-1")) {
                        char protectionCode[10];
                        memset(protectionCode, 0, 10);
                        switch (gDirectoryInodeBuffer[index].type) {
                            case UnRWFile:
                                strcpy(protectionCode, "--");
                                break;
                            case RFile:
                                strcpy(protectionCode, "r-");
                                break;
                            case RWFile:
                                strcpy(protectionCode, "rw");
                                break;
                            default:
                                break;
                        }
                        int length = 0;
                        memset(path, 0, 256);
                        sprintf(path, "%s/%s%s", INODE_DIRECTORY_NAME, INODE_PREFIX, gDirectoryInodeBuffer[index].inodeIndex);
                        stat(path, &statBuff);
                        int fileBlockNum = statBuff.st_size/sizeof(InodeFileRecord);
                        if ((fp = fopen(path, "r")) != NULL) {
                            fread(gFileInodeBuffer, sizeof(InodeFileRecord), fileBlockNum, fp);
                            fclose(fp);
                        }
                        int i;
                        for (i = 0; i < fileBlockNum; i++) {
                            if (strcmp(gFileInodeBuffer[i].fileIndex, "-1")) {
                                memset(path, 0, 256);
                                sprintf(path, "%s/%s%s", DISK_DIRECTORY_NAME, FILE_PREFIX, gFileInodeBuffer[i].fileIndex);
                                
                                stat(path, &statBuff);
                                length += statBuff.st_size;
                            }
                        }
                        printf("%s\tfile%s\t%s\t%d\n", gDirectoryInodeBuffer[index].name, gFileInodeBuffer[0].fileIndex, protectionCode, length);
                    }
                }
                while (getchar() != 'q') ;
                gCurrentSystemState = MenuState;
            }
                break;
            case CreatingState:
            {
                if (!strcmp(gCommandBuffer, "y")) {
                    int newInodeIndex, newFileIndex;
                    //insert directory entry
                    InodeDirectoryRecord newRecord;
                    if (!strcmp(gCommandParameter[0], "PandF")) {
                        if (!strcmp(gCommandParameter[1], "-r")) {
                            newRecord.type = RFile;
                            strcpy(newRecord.name, gCommandParameter[2]);
                        } else {
                            newRecord.type = RWFile;
                            strcpy(newRecord.name, gCommandParameter[2]);
                        }
                    }
                    else{
                        newRecord.type = RFile;
                        strcpy(newRecord.name, gCommandParameter[1]);
                    }
                    int j;
                    for (j = 0; j < gCurrentUser.homeDirFileVirNum; j++) {
                        if (!strcmp(gDirectoryInodeBuffer[j].name, newRecord.name) && strcmp(gDirectoryInodeBuffer[j].inodeIndex, "-1")) {
                            break;
                        }
                    }
                    if (j >= gCurrentUser.homeDirFileVirNum) {
                        newInodeIndex = getInodeIndex();
                        sprintf(newRecord.inodeIndex, "%d", newInodeIndex);
                        char path[256];
                        memset(path, 0, 256);
                        sprintf(path, "%s/%s%d", INODE_DIRECTORY_NAME, INODE_PREFIX, gCurrentUser.homeInode);
                        insertRecordIntoFile(&newRecord, path, DirectoryRecord);
                        //create file inode
                        memset(path, 0, 256);
                        sprintf(path, "%s/%s%d", INODE_DIRECTORY_NAME, INODE_PREFIX, newInodeIndex);
                        createFile(path);
                        //insert file block record
                        InodeFileRecord newFileRecord;
                        newFileIndex = getFileIndex();
                        sprintf(newFileRecord.fileIndex, "%d", newFileIndex);
                        insertRecordIntoFile(&newFileRecord, path, FileRecord);
                        //create file block
                        memset(path, 0, 256);
                        sprintf(path, "%s/%s%d", DISK_DIRECTORY_NAME, FILE_PREFIX, newFileIndex);
                        createFile(path);
                    }
                    else{
                        printf("The file name is repeated. \n");
                        printf("Please retry. Press enter to continue. \n");
                        setbuf(stdin, NULL);
                        getchar();
                    }
                }
                gCurrentSystemState = MenuState;
            }
                break;
            case DeletingState:
            {
                if (!strcmp(gCommandBuffer, "y")) {
                    int i = 0;
                    for (i = 0; i < gCurrentUser.homeDirFileVirNum; i++) {
                        if (!strcmp(gDirectoryInodeBuffer[i].name, gCommandParameter[0]) && strcmp(gDirectoryInodeBuffer[i].inodeIndex, "-1")) {
                            break;
                        }
                    }
                    if (i < gCurrentUser.homeDirFileVirNum) {
                        char path[256];
                        //delete directory entry
                        memset(path, 0, 256);
                        sprintf(path, "%s/%s%d", INODE_DIRECTORY_NAME, INODE_PREFIX, gCurrentUser.homeInode);
                        deleteRecordFromFile(path, DirectoryRecord, i);
                        //get the file block indexes
                        RecycledInodeIndexRecord recycledInodeIndex;
                        strcpy(recycledInodeIndex.inodeIndex, gDirectoryInodeBuffer[i].inodeIndex);
                        memset(path, 0, 256);
                        sprintf(path, "%s/%s%s", INODE_DIRECTORY_NAME, INODE_PREFIX, recycledInodeIndex.inodeIndex);
                        struct stat statBuff;
                        stat(path, &statBuff);
                        int blockNum = statBuff.st_size/sizeof(InodeFileRecord);
                        FILE *fp;
                        InodeFileRecord *recordBuffer;
                        if ((fp = fopen(path, "r")) != NULL) {
                            recordBuffer = malloc(sizeof(InodeFileRecord)*blockNum);
                            fread(recordBuffer, sizeof(InodeFileRecord), blockNum, fp);
                            fclose(fp);
                        }
                        //delete file inode and recycle inode num
                        remove(path);
                        insertRecordIntoFile(&recycledInodeIndex, RECYCLED_INODEINDEX_FILE_NAME, RecycledInodeRecord);
                        //delete file block and recycle file index num
                        for (i = 0; i < blockNum; i++) {
                            if (strcmp(recordBuffer[i].fileIndex, "-1")) {
                                RecycledFileIndexRecord recycledFileIndex;
                                strcpy(recycledFileIndex.fileIndex, recordBuffer[i].fileIndex);
                                memset(path, 0, 256);
                                sprintf(path, "%s/%s%s", DISK_DIRECTORY_NAME, FILE_PREFIX, recycledFileIndex.fileIndex);
                                remove(path);
                                insertRecordIntoFile(&recycledFileIndex, RECYCLED_FILEINDEX_FILE_NAME, RecycledFileRecord);
                            }
                        }
                        //free memory
                        free(recordBuffer);
                    }
                    else{
                        printf("There is not such file named '%s'. \n", gCommandParameter[0]);
                        printf("Please retry. Press enter to continue. \n");
                        setbuf(stdin, NULL);
                        getchar();
                    }
                }
                gCurrentSystemState = MenuState;
            }
                break;
            case ReadingState:
            {
                int i = 0;
                for (i = 0; i < gCurrentUser.homeDirFileVirNum; i++) {
                    if (!strcmp(gDirectoryInodeBuffer[i].name, gCommandParameter[0]) && strcmp(gDirectoryInodeBuffer[i].inodeIndex, "-1")) {
                        break;
                    }
                }
                if (i < gCurrentUser.homeDirFileVirNum) {
                    printf("You can press enter to go back. \n");
                    printf("The content of file '%s' is below:\n", gCommandParameter[0]);
                    //load file inode
                    char path[256];
                    memset(path, 0, 256);
                    sprintf(path, "%s/%s%s", INODE_DIRECTORY_NAME, INODE_PREFIX, gDirectoryInodeBuffer[i].inodeIndex);
                    struct stat statBuff;
                    stat(path, &statBuff);
                    int blockNum = statBuff.st_size/sizeof(InodeFileRecord);
                    FILE *fp;
                    InodeFileRecord *recordBuffer;
                    if ((fp = fopen(path, "r")) != NULL) {
                        recordBuffer = malloc(sizeof(InodeFileRecord)*blockNum);
                        fread(recordBuffer, sizeof(InodeFileRecord), blockNum, fp);
                        fclose(fp);
                    }
                    //count the block num
                    int i, count=0;
                    for (i = 0; i < blockNum; i++) {
                        if (strcmp(recordBuffer[i].fileIndex, "-1")) {
                            count++;
                        }
                    }
                    char *content = malloc(FILE_BLOCK_SIZE*count);
                    memset(content, 0, FILE_BLOCK_SIZE*count);
                    
                    for (i = 0; i < blockNum; i++) {
                        if (strcmp(recordBuffer[i].fileIndex, "-1")) {
                            memset(path, 0, 256);
                            sprintf(path, "%s/%s%s", DISK_DIRECTORY_NAME, FILE_PREFIX, recordBuffer[i].fileIndex);
                            stat(path, &statBuff);
                            if ((fp = fopen(path, "r")) != NULL) {
                                fread(gFileBockBuffer, 1, statBuff.st_size, fp);
                                fclose(fp);
                            }
                            strcat(content, gFileBockBuffer);
                        }
                    }
                    printf("%s", content);
                    free(content);
                }
                else{
                    printf("There is not such file named '%s'. \n", gCommandParameter[0]);
                    printf("Please retry. Press enter to continue. \n");
                }
                setbuf(stdin, NULL);
                getchar();
                gCurrentSystemState = MenuState;
            }
                break;
            case WritingState:
            {
                if (!strcmp(gCommandBuffer, "y")) {
                    char name[16];
                    char aOrw;
                    //determind the write type
                    if (!strcmp(gCommandParameter[0], "PandF")) {
                        strcpy(name, gCommandParameter[2]);
                        if (!strcmp(gCommandParameter[1], "-w")) {
                            aOrw = 'w';
                        }
                        else{
                            aOrw = 'a';
                        }
                    }
                    else{
                        aOrw = 'a';
                        strcpy(name, gCommandParameter[1]);
                    }
                    //find directory entry
                    int j;
                    for (j = 0; j < gCurrentUser.homeDirFileVirNum; j++) {
                        if (!strcmp(gDirectoryInodeBuffer[j].name, name) && strcmp(gDirectoryInodeBuffer[j].inodeIndex, "-1")) {
                            break;
                        }
                    }
                    if (j < gCurrentUser.homeDirFileVirNum) {
                        if (gDirectoryInodeBuffer[j].type == RWFile) {
                            //write content
                            if (aOrw == 'a') {
                                //get the file block indexes
                                char path[256];
                                memset(path, 0, 256);
                                sprintf(path, "%s/%s%s", INODE_DIRECTORY_NAME, INODE_PREFIX, gDirectoryInodeBuffer[j].inodeIndex);
                                struct stat statBuff;
                                stat(path, &statBuff);
                                int blockNum = statBuff.st_size/sizeof(InodeFileRecord);
                                FILE *fp;
                                InodeFileRecord *recordBuffer;
                                if ((fp = fopen(path, "r")) != NULL) {
                                    recordBuffer = malloc(sizeof(InodeFileRecord)*blockNum);
                                    fread(recordBuffer, sizeof(InodeFileRecord), blockNum, fp);
                                    fclose(fp);
                                }
                                //find the last file block
                                int i;
                                for (i = blockNum-1; i >= 0; i--) {
                                    if (strcmp(recordBuffer[i].fileIndex, "-1")) {
                                        break;
                                    }
                                }
                                memset(path, 0, 256);
                                sprintf(path, "%s/%s%s", DISK_DIRECTORY_NAME, FILE_PREFIX, recordBuffer[i].fileIndex);
                                //get the input
                                printf("You can input the content in multiple lines.However, at the last line you should input ':q' to end the input.\n");
                                while (fgets(gCommandBuffer, COMMAND_CONTENT_MAX_LENGTH, stdin)) {
                                    if (gCommandBuffer[0]==':' && gCommandBuffer[1]=='q') {
                                        break;
                                    }
                                    else{
                                        stat(path, &statBuff);
                                        if (strlen(gCommandBuffer) <= FILE_BLOCK_SIZE-statBuff.st_size) {
                                            //the space is enough, intake it
                                            if ((fp = fopen(path, "a")) != NULL) {
                                                fwrite(gCommandBuffer, 1, strlen(gCommandBuffer), fp);
                                                fclose(fp);
                                            }
                                        } else {
                                            //the space is not enough, firstly take part of the string
                                            long holdLength = FILE_BLOCK_SIZE - statBuff.st_size;
                                            long anotherHoldLength = strlen(gCommandBuffer) - holdLength;
                                            if ((fp = fopen(path, "a")) != NULL) {
                                                fwrite(gCommandBuffer, 1, holdLength, fp);
                                                fclose(fp);
                                            }
                                            //if the remaining is large than file block size
                                            while (anotherHoldLength > FILE_BLOCK_SIZE) {
                                                holdLength = FILE_BLOCK_SIZE;
                                                anotherHoldLength -= FILE_BLOCK_SIZE;
                                                InodeFileRecord newRecord;
                                                sprintf(newRecord.fileIndex, "%d", getFileIndex());
                                                memset(path, 0, 256);
                                                sprintf(path, "%s/%s%s", INODE_DIRECTORY_NAME, INODE_PREFIX, gDirectoryInodeBuffer[j].inodeIndex);
                                                insertRecordIntoFile(&newRecord, path, FileRecord);
                                                memset(path, 0, 256);
                                                sprintf(path, "%s/%s%s", DISK_DIRECTORY_NAME, FILE_PREFIX, newRecord.fileIndex);
                                                createFile(path);
                                                if ((fp = fopen(path, "a")) != NULL) {
                                                    fwrite(gCommandBuffer, 1, holdLength, fp);
                                                    fclose(fp);
                                                    
                                                }
                                            }
                                            //the remaining is less than or equal to file block size
                                            InodeFileRecord newRecord;
                                            sprintf(newRecord.fileIndex, "%d", getFileIndex());
                                            memset(path, 0, 256);
                                            sprintf(path, "%s/%s%s", INODE_DIRECTORY_NAME, INODE_PREFIX, gDirectoryInodeBuffer[j].inodeIndex);
                                            insertRecordIntoFile(&newRecord, path, FileRecord);
                                            memset(path, 0, 256);
                                            sprintf(path, "%s/%s%s", DISK_DIRECTORY_NAME, FILE_PREFIX, newRecord.fileIndex);
                                            createFile(path);
                                            if ((fp = fopen(path, "a")) != NULL) {
                                                fwrite(gCommandBuffer, 1, anotherHoldLength, fp);
                                                fclose(fp);
                                            }
                                        }
                                    }
                                }
                                free(recordBuffer);
                            }
                            else{
                                //get the file block indexes
                                char path[256];
                                char inodePath[256];
                                memset(inodePath, 0, 256);
                                sprintf(inodePath, "%s/%s%s", INODE_DIRECTORY_NAME, INODE_PREFIX, gDirectoryInodeBuffer[j].inodeIndex);
                                struct stat statBuff;
                                stat(inodePath, &statBuff);
                                int blockNum = statBuff.st_size/sizeof(InodeFileRecord);
                                FILE *fp;
                                InodeFileRecord *recordBuffer;
                                if ((fp = fopen(inodePath, "r")) != NULL) {
                                    recordBuffer = malloc(sizeof(InodeFileRecord)*blockNum);
                                    fread(recordBuffer, sizeof(InodeFileRecord), blockNum, fp);
                                    fclose(fp);
                                }
                                //clear all content
                                int i;
                                for (i = 0; i < blockNum; i++) {
                                    if (strcmp(recordBuffer[i].fileIndex, "-1")) {
                                        memset(path, 0, 256);
                                        sprintf(path, "%s/%s%s", DISK_DIRECTORY_NAME, FILE_PREFIX, recordBuffer[i].fileIndex);
                                        remove(path);
                                        deleteRecordFromFile(inodePath, FileRecord, i);
                                    }
                                }
                                //insert the first file block
                                InodeFileRecord newRecord;
                                sprintf(newRecord.fileIndex, "%d", getFileIndex());
                                memset(path, 0, 256);
                                sprintf(path, "%s/%s%s", DISK_DIRECTORY_NAME, FILE_PREFIX, newRecord.fileIndex);
                                insertRecordIntoFile(&newRecord, inodePath, FileRecord);
                                //insert content
                                printf("You can input the content in multiple lines.However, at the last line you should input ':q' to end the input.\n");
                                while (fgets(gCommandBuffer, COMMAND_CONTENT_MAX_LENGTH, stdin)) {
                                    if (gCommandBuffer[0]==':' && gCommandBuffer[1]=='q') {
                                        break;
                                    }
                                    else{
                                        stat(path, &statBuff);
                                        if (strlen(gCommandBuffer) <= FILE_BLOCK_SIZE-statBuff.st_size) {
                                            //the space is enough, intake it
                                            if ((fp = fopen(path, "a")) != NULL) {
                                                fwrite(gCommandBuffer, 1, strlen(gCommandBuffer), fp);
                                                fclose(fp);
                                            }
                                        } else {
                                            //the space is not enough, firstly take part of the string
                                            long holdLength = FILE_BLOCK_SIZE - statBuff.st_size;
                                            long anotherHoldLength = strlen(gCommandBuffer) - holdLength;
                                            if ((fp = fopen(path, "a")) != NULL) {
                                                fwrite(gCommandBuffer, 1, holdLength, fp);
                                                fclose(fp);
                                            }
                                            //if the remaining is large than file block size
                                            while (anotherHoldLength > FILE_BLOCK_SIZE) {
                                                holdLength = FILE_BLOCK_SIZE;
                                                anotherHoldLength -= FILE_BLOCK_SIZE;
                                                InodeFileRecord newRecord;
                                                sprintf(newRecord.fileIndex, "%d", getFileIndex());
                                                memset(path, 0, 256);
                                                sprintf(path, "%s/%s%s", INODE_DIRECTORY_NAME, INODE_PREFIX, gDirectoryInodeBuffer[j].inodeIndex);
                                                insertRecordIntoFile(&newRecord, path, FileRecord);
                                                memset(path, 0, 256);
                                                sprintf(path, "%s/%s%s", DISK_DIRECTORY_NAME, FILE_PREFIX, newRecord.fileIndex);
                                                createFile(path);
                                                if ((fp = fopen(path, "a")) != NULL) {
                                                    fwrite(gCommandBuffer, 1, holdLength, fp);
                                                    fclose(fp);
                                                }
                                            }
                                            //the remaining is less than or equal to file block size
                                            InodeFileRecord newRecord;
                                            sprintf(newRecord.fileIndex, "%d", getFileIndex());
                                            memset(path, 0, 256);
                                            sprintf(path, "%s/%s%s", INODE_DIRECTORY_NAME, INODE_PREFIX, gDirectoryInodeBuffer[j].inodeIndex);
                                            insertRecordIntoFile(&newRecord, path, FileRecord);
                                            memset(path, 0, 256);
                                            sprintf(path, "%s/%s%s", DISK_DIRECTORY_NAME, FILE_PREFIX, newRecord.fileIndex);
                                            createFile(path);
                                            if ((fp = fopen(path, "a")) != NULL) {
                                                fwrite(gCommandBuffer, 1, anotherHoldLength, fp);
                                                fclose(fp);
                                            }
                                        }
                                    }
                                }
                                free(recordBuffer);
                            }
                        }
                        else{
                            printf("I'm so sorry about that this file can't be written.\n");
                            printf("Please retry. Press enter to continue. \n");
                            setbuf(stdin, NULL);
                            getchar();
                        }
                    }
                    else{
                        printf("There is not such a file. \n");
                        printf("Please retry. Press enter to continue. \n");
                        setbuf(stdin, NULL);
                        getchar();
                    }
                }
                gCurrentSystemState = MenuState;
            }
                break;
            default:
                break;
        }
    }
    
    //printf("%s\n", gCommandBuffer);
    //printf("%d\n", gCurrentSystemState);
}

int main(){
    // insert code here...
    initSomething();
    while (gCurrentSystemState) {
        showSomething();
        getSomething();
        doSomething();
    }
    return 0;
}

