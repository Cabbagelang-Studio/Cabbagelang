#define BASKET_VERSION "1.0.0"
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<stdbool.h>
#include<stdarg.h>
#include<dirent.h>
#include"miniz.h"
bool quiet=false;
extern char* optarg;
char* str_cat(char *a, char *b){
	char *target = (char*)malloc(strlen(a) + strlen(b) + 1);
	strcpy(target, a);
	strcat(target, b);
	return target;
}
void info(char* format, ...) {
    if (!quiet) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}
void warning(char* format,...){
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
void error(char* format,...){
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    exit(0);
}
#ifdef _WIN32
#include<windows.h>
int remove_directory(const char *path) {
    WIN32_FIND_DATA find_file_data;
    HANDLE hFind;

    // Construct the search path
    char search_path[MAX_PATH];
    sprintf(search_path, "%s\\*", path);

    // Find the first file in the directory
    hFind = FindFirstFile(search_path, &find_file_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Unable to open directory\n");
        return -1;
    }

    // Iterate through directory entries
    do {
        // Skip '.' and '..' entries
        if (strcmp(find_file_data.cFileName, ".") == 0 || strcmp(find_file_data.cFileName, "..") == 0) {
            continue;
        }

        // Construct full path
        char full_path[MAX_PATH];
        sprintf(full_path, "%s\\%s", path, find_file_data.cFileName);

        // Remove file or subdirectory
        if (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (remove_directory(full_path) != 0) {
                error("Unable to delete directory\n");
                FindClose(hFind);
                return -1;
            }
        } else {
            if (!DeleteFile(full_path)) {
                error("Unable to delete file\n");
                FindClose(hFind);
                return -1;
            }
        }
    } while (FindNextFile(hFind, &find_file_data) != 0);

    // Close the search handle
    FindClose(hFind);

    // Remove the empty directory
    if (!RemoveDirectory(path)) {
        error("Unable to delete directory\n");
    }

    return 0;
}
#include<wininet.h>
void download_file(const char *url, const char *destination) {
    HINTERNET hInternet, hConnect;

    hInternet = InternetOpen(L"Download", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        error("Failed to fetch: %s.",url);
    }

    hConnect = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        error("Failed to fetch: %s.",url);
    }

    FILE *file = fopen(destination, "wb");
    if (!file) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        error("Failed to open file %s.",destination);
    }

    char buffer[4096];
    DWORD bytesRead;

    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        fwrite(buffer, 1, bytesRead, file);
    }

    fclose(file);

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

#else

void remove_directory(const char *path) {
    DIR *dir;
    struct dirent *entry;
    char full_path[256];

    // Open the directory
    dir = opendir(path);
    if (dir == NULL) {
        error("Unable to open directory");
    }

    // Iterate through directory entries
    while ((entry = readdir(dir)) != NULL) {
        // Skip '.' and '..' entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct full path
        sprintf(full_path, "%s/%s", path, entry->d_name);

        // Remove file or subdirectory
        if (remove(full_path) != 0) {
            error("Unable to delete file");
        }
    }

    // Close directory
    closedir(dir);

    // Remove the empty directory
    if (rmdir(path) != 0) {
        error("Unable to delete directory");
    }
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

#include <curl/curl.h>

void download_file(const char *url, const char *output_file) {
    CURL *curl;
    FILE *file;
    CURLcode res;

    curl = curl_easy_init();
    if (!curl) {
        error("Failed to fetch: %s.",url);
    }

    file = fopen(output_file, "wb");
    if (!file) {
        curl_easy_cleanup(curl);
        error("Failed to fetch: %s.",url);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

    res = curl_easy_perform(curl);

    curl_easy_cleanup(curl);
    fclose(file);

    if(res!=CURLE_OK) error("Failed to open file %s.",output_file);
}
#endif
void unzip(const char *zip_filename, const char *output_dir) {
    FILE *zip_file = fopen(zip_filename, "rb");
    if (!zip_file) {
        error("Failed to open zip file.\n");
    }

    fseek(zip_file, 0, SEEK_END);
    size_t zip_size = ftell(zip_file);
    rewind(zip_file);

    void *zip_data = malloc(zip_size);
    fread(zip_data, 1, zip_size, zip_file);
    fclose(zip_file);

    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if (!mz_zip_reader_init_mem(&zip_archive, zip_data, zip_size, 0)) {
        free(zip_data);
        error("Failed to initialize zip reader.\n");
    }

    for (int i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
            warning("Failed to read file stat for file %d.\n", i);
            continue;
        }

        void *file_data = malloc(file_stat.m_uncomp_size);
        if (!mz_zip_reader_extract_to_mem(&zip_archive, i, file_data, file_stat.m_uncomp_size, 0)) {
            warning("Failed to extract file %d.\n", i);
            free(file_data);
            continue;
        }

        char *output_filename = malloc(strlen(output_dir) + strlen(file_stat.m_filename) + 2);
        sprintf(output_filename, "%s/%s", output_dir, file_stat.m_filename);

        FILE *output_file = fopen(output_filename, "wb");
        if (!output_file) {
            warning("Failed to create output file %s\n", output_filename);
            free(file_data);
            free(output_filename);
            continue;
        }
        fwrite(file_data, 1, file_stat.m_uncomp_size, output_file);
        fclose(output_file);

        info("Extracted file %s\n", file_stat.m_filename);

        free(file_data);
        free(output_filename);
    }

    mz_zip_reader_end(&zip_archive);
    free(zip_data);
}
void usage(){
    printf("Usage:\n\tbasket <command> [options]\nCommands:\n\tinstall\t\tInstall leaves\n\tuninstall\tUninstall leaves\n\tlist\t\tList installed leaves\nOptions:\n\t-h\t\tShow this message\n\t-v\t\tShow version\n\t-q\t\tGives less output\n\t-s\t\tSpecify download source\n");
    exit(0);
}
void install(char* package,char* source){
    info("Installing package: %s\n",package);
    char* url=str_cat(str_cat(source,"/"),package);
    url=str_cat(url,".zip");
    info("Fetching %s...\n",url);
    download_file(url,".cache.leaf");
    info("Unzipping...\n");
    char* leaves_path=getenv("CABBAGELANG_HOME");
    leaves_path=str_cat(leaves_path,"/leaves/");
    unzip(".cache.leaf",leaves_path);
    info("Installed %s successfully.\n",package);
}
void uninstall(char* package){
    info("Uninstalling package: %s\n",package);
    char* leaves_path=getenv("CABBAGELANG_HOME");
    leaves_path=str_cat(leaves_path,"/leaves/");
    int rm_file=remove(str_cat(leaves_path,package));
    if(rm_file!=0){
        error("Failed to remove package file: %s\n",package);
    }
    char* dir_path=str_cat(leaves_path,"__");
    dir_path=str_cat(dir_path,package);
    dir_path=str_cat(dir_path,"__");
    remove_directory(dir_path);
    info("Uninstalled %s successfully.\n",package);
}
void list(){
    info("List of leaves:\n");
    char* leaves_path=getenv("CABBAGELANG_HOME");
    leaves_path=str_cat(leaves_path,"/leaves/");
    DIR* directory = opendir(leaves_path);
    struct dirent* entry;
    while ((entry = readdir(directory)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || (strlen(entry->d_name)>2 && entry->d_name[0] == '_' && entry->d_name[1] == '_')) {
            continue;
        }
        printf("%s\n", entry->d_name);
    }
    closedir(directory);
}
void show_ver(){
    printf("Basket version: %s",BASKET_VERSION);
    exit(0);
}
int main(int argc,char** argv){
    if(argc==1){
        usage();
    }else if(argc==2){
        if(!strcmp(argv[1],"list")) list();
        else if(!strcmp(argv[1],"-v")) show_ver();
        else if(!strcmp(argv[1],"-h")) usage();
        else usage();
    }else if(argc==3){
        if(!strcmp(argv[1],"install"))
            install(argv[2],"https://leaves.cabbagelang.xyz/leaves");
        else if(!strcmp(argv[1],"uninstall"))
            uninstall(argv[2]);
        else
            usage();
    }else{
        char opt;
        char source[2048]="https://leaves.cabbagelang.xyz/leaves";
        char* nargv[argc-3];
        for(int i=2;i<argc;i++){
            nargv[i-2]=argv[i];
        }
        while((opt=getopt(argc-2,nargv,"qs:"))!=-1){
            if(opt=='q') quiet=true;
            else if(opt=='s') strcpy(source,optarg);
            else usage();
        }
        char* command=argv[1];
        char* package=argv[2];
        if(!strcmp(command,"install")) install(package,source);
        else if(!strcmp(command,"uninstall")) uninstall(package);
        else usage();
    }
    return 0;
}