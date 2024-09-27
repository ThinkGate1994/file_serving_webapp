#include "web_server.h"
#include "sd_init.h"

static const char *TAG = "webserver";
httpd_handle_t server = NULL;

#define FILE_PATH_MAX 256
#define MAX_FILE_SIZE (1000 * 1024) // 200 KB
#define MAX_FILE_SIZE_STR "1000KB"
#define SCRATCH_BUFSIZE 8192
const char *base_path = "/sdcard";
char scratch[SCRATCH_BUFSIZE];
const char *mode = "";

int is_file(char *filename)
{
    char *ptr = NULL;
    ptr = strrchr(filename, '/');
    ptr = strrchr(ptr, '.');
    if (ptr == NULL)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

int removePath(const char *path)
{
    struct stat pathStat;
    if (stat(path, &pathStat) != 0)
    {
        printf("Unable to access path '%s'.\n", path);
        return -1; // Error occurred while accessing the path
    }

    if (S_ISDIR(pathStat.st_mode))
    {
        DIR *dir = opendir(path);
        if (dir == NULL)
        {
            printf("Unable to open directory '%s'.\n", path);
            return -1; // Error occurred while opening the directory
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue; // Skip current directory (.) and parent directory (..)
            }

            char entryPath[270];
            snprintf(entryPath, sizeof(entryPath), "%s/%s", path, entry->d_name);

            removePath(entryPath); // Recursively remove files and directories
        }

        closedir(dir);

        if (rmdir(path) == 0)
        {
            printf("Directory '%s' removed successfully.\n", path);
            return 0; // Directory removed successfully
        }
        else
        {
            printf("Unable to remove directory '%s'.\n", path);
            return -1; // Error occurred while removing the directory
        }
    }
    else
    {
        if (remove(path) == 0)
        {
            printf("File '%s' removed successfully.\n", path);
            return 0; // File removed successfully
        }
        else
        {
            printf("Unable to remove file '%s'.\n", path);
            return -1; // Error occurred while removing the file
        }
    }
}

int createIntermediateDirs(const char *filePath, const char *basepath)
{

    char *pathCopy = strdup(filePath);                      // Make a copy of the file path
    char *token = strtok(pathCopy + strlen(basepath), "/"); // Split the path by '/'
    char path[256] = "";                                    // Assuming a maximum path length of 256 characters

    sprintf(&path[0], "%s/%s", basepath, token);
    while (token != NULL)
    {

        // Check if the directory already exists
        struct stat st;
        if (is_file(path) == 0)
        {
            if (stat(path, &st) != 0)
            {
                // Directory doesn't exist, create it
                if (mkdir(path, 0777) != 0)
                {
                    printf("Failed to create directory: %s\n", path);
                    return -1;
                }
            }
        }
        token = strtok(NULL, "/"); // Get the next token
        if (token != NULL)
        {
            sprintf(&path[strlen(path)], "/%s", token); // Append the token to create the full path
        }
    }

    free(pathCopy); // Free the allocated memory

    return 0;
}

void remove_and_rename_new_files(const char *dir_path)
{
    DIR *dir;
    struct dirent *ent;
    size_t new_suffix_len = strlen("_new");

    dir = opendir(dir_path);
    if (dir != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            if (ent->d_type == DT_REG)
            { // Check if it's a regular file
                size_t filename_len = strlen(ent->d_name);

                // Check if the filename ends with "_new"
                if (filename_len > new_suffix_len &&
                    strncmp(ent->d_name + filename_len - new_suffix_len, "_new", new_suffix_len) == 0)
                {

                    // Copy the original filename without the "_new" suffix
                    char *old_filename = (char *)malloc(filename_len - new_suffix_len + 1);
                    strncpy(old_filename, ent->d_name, filename_len - new_suffix_len);
                    old_filename[filename_len - new_suffix_len] = '\0';

                    // Check if the original file exists
                    char *old_filepath = (char *)malloc(FILE_PATH_MAX + 10);
                    snprintf(old_filepath, FILE_PATH_MAX + 10, "%s/%s", dir_path, old_filename);
                    FILE *original_file = fopen(old_filepath, "r");
                    if (original_file == NULL)
                    {
                        // Original file does not exist, remove the "_new" suffix
                        // Rename "_new" file to the original filename
                        char *new_filename = (char *)malloc(FILE_PATH_MAX + 10);
                        snprintf(new_filename, FILE_PATH_MAX + 10, "%s/%s", dir_path, ent->d_name);
                        new_filename[strlen(dir_path) + filename_len + 1] = '\0'; // Remove "_new" suffix from the new filename
                        printf("Renaming _new file %s to: %s\n", new_filename, old_filepath);
                        if (rename(new_filename, old_filepath) != 0)
                        { // file left is renamed to file right
                            printf("Error renaming new_version to working_version when original doesn't exist.\r\n");
                        }
                        free(new_filename);
                    }
                    else
                    { // Original file exists
                        fclose(original_file);
                        // Rename the original file to "filename_old"
                        char *old_version = (char *)malloc(FILE_PATH_MAX + 10);
                        snprintf(old_version, FILE_PATH_MAX + 10, "%s/%s_old", dir_path, old_filename);
                        printf("Renaming original file to: %s\n", old_version);
                        if (rename(old_filepath, old_version) != 0)
                        {
                            printf("Error renaming working_version to old_version\r\n");
                        }

                        // Rename "_new" file to the original filename
                        char *new_filename = (char *)malloc(FILE_PATH_MAX + 10);
                        snprintf(new_filename, FILE_PATH_MAX + 10, "%s/%s", dir_path, ent->d_name);
                        new_filename[strlen(dir_path) + filename_len + 1] = '\0'; // Remove "_new" suffix from the new filename
                        printf("Renaming _new file %s to: %s\n", new_filename, old_filepath);
                        if (rename(new_filename, old_filepath) != 0)
                        {
                            printf("Error renaming new_version to working_version\r\n");
                        }
                        free(new_filename);

                        // Remove "filename_old" file
                        printf("Removing original file: %s\n", old_version);
                        if (remove(old_version) != 0)
                        {
                            printf("Error removing old file\r\n");
                        }
                        free(old_version);
                    }
                    free(old_filename);
                    free(old_filepath);
                }
            }
            else if (ent->d_type == DT_DIR && strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0)
            {
                // Recursively process subdirectories
                char subdirectory[FILE_PATH_MAX + 1];
                snprintf(subdirectory, sizeof(subdirectory), "%s/%s", dir_path, ent->d_name);
                remove_and_rename_new_files(subdirectory);
            }
        }
        closedir(dir);
    }
    else
    {
        perror("Error opening directory");
    }
}

void renameFoldersWithNewSuffix(const char *base_path)
{
    DIR *dir = opendir(base_path);
    struct dirent *entry;

    if (dir == NULL)
    {
        ESP_LOGE(TAG, "Failed to open directory");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            char old_folder_name[FILE_PATH_MAX + 20];
            char new_folder_name[FILE_PATH_MAX + 20];
            char old_folder_name_temp[FILE_PATH_MAX + 20];

            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            // Check if the folder name ends with "_new"
            if (strstr(entry->d_name, "_new") == entry->d_name + strlen(entry->d_name) - 4)
            {
                snprintf(old_folder_name, sizeof(old_folder_name), "%s/%.*s", base_path, (int)strlen(entry->d_name) - 4, entry->d_name);
                snprintf(new_folder_name, sizeof(new_folder_name), "%s/%s", base_path, entry->d_name);
                snprintf(old_folder_name_temp, sizeof(old_folder_name_temp), "%s/%.*s_old", base_path, (int)strlen(entry->d_name) - 4, entry->d_name);
                char temp_base_path[sizeof(base_path) + 2];
                sprintf(temp_base_path, "%s/", base_path);
                printf("old folder name %s\r\n", old_folder_name);
                printf("temp_base_path %s\r\n", temp_base_path);
                if (strcmp(old_folder_name, temp_base_path) == 0) // strcmp(old_folder_name,"/sdcard/")==0
                {
                    // Move existing files in the root to _old folder
                    DIR *root_dir = opendir(base_path);
                    struct dirent *root_entry;

                    if (root_dir == NULL)
                    {
                        ESP_LOGE(TAG, "Failed to open root directory");
                        return;
                    }

                    while ((root_entry = readdir(root_dir)) != NULL)
                    {
                        if (root_entry->d_type == DT_REG)
                        {
                            char old_file_path[FILE_PATH_MAX + 20];
                            char new_file_path[FILE_PATH_MAX + 20];

                            snprintf(old_file_path, sizeof(old_file_path), "%s/%s", base_path, root_entry->d_name);
                            snprintf(new_file_path, sizeof(new_file_path), "%s/_old/%s", base_path, root_entry->d_name);
                            createIntermediateDirs(new_file_path, base_path);

                            if (rename(old_file_path, new_file_path) != 0)
                            {
                                ESP_LOGE(TAG, "Failed to move file '%s' to '%s'", old_file_path, new_file_path);
                            }
                        }
                    }
                    closedir(root_dir);

                    // Copy files from _new to the root folder
                    DIR *new_dir = opendir(new_folder_name);
                    struct dirent *new_entry;

                    if (new_dir == NULL)
                    {
                        ESP_LOGE(TAG, "Failed to open _new directory");
                        return;
                    }

                    while ((new_entry = readdir(new_dir)) != NULL)
                    {
                        if (new_entry->d_type == DT_REG)
                        {
                            char old_file_path[(FILE_PATH_MAX * 2) + 20];
                            char new_file_path[FILE_PATH_MAX + 20];

                            snprintf(old_file_path, sizeof(old_file_path), "%s/%s", new_folder_name, new_entry->d_name);
                            snprintf(new_file_path, sizeof(new_file_path), "%s/%s", base_path, new_entry->d_name);

                            if (rename(old_file_path, new_file_path) != 0)
                            {
                                ESP_LOGE(TAG, "Failed to copy file '%s' to '%s'", old_file_path, new_file_path);
                            }
                        }
                    }
                    closedir(new_dir);

                    // Remove _old and _new folders
                    removePath(old_folder_name_temp);
                    removePath(new_folder_name);
                }
                else
                {
                    printf("old_folder_name %s\r\n", old_folder_name);
                    printf("old_folder_name_temp %s\r\n", old_folder_name_temp);
                    if (rename(old_folder_name, old_folder_name_temp) == 0)
                    {
                        ESP_LOGI(TAG, "Renamed folder from '%s' to '%s'", old_folder_name, old_folder_name_temp);

                        if (rename(new_folder_name, old_folder_name) == 0)
                        {
                            ESP_LOGI(TAG, "Renamed folder from '%s' to '%s'", new_folder_name, old_folder_name);

                            if (removePath(old_folder_name_temp) == 0)
                            {
                                ESP_LOGI(TAG, "Removed folder '%s'", old_folder_name_temp);
                            }
                            else
                            {
                                ESP_LOGE(TAG, "Failed to remove folder '%s'", old_folder_name_temp);
                            }
                        }
                        else
                        {
                            ESP_LOGE(TAG, "1Failed to rename folder from '%s' to '%s'", new_folder_name, old_folder_name);
                        }
                    }
                    else
                    {
                        ESP_LOGE(TAG, "2Failed to rename folder from '%s' to '%s'", old_folder_name, old_folder_name_temp);
                        /*new part. handle when sd card was empty from the start*/
                        if (rename(new_folder_name, old_folder_name) == 0)
                        {
                            ESP_LOGI(TAG, "Renamed folder from '%s' to '%s'", new_folder_name, old_folder_name);

                            if (removePath(old_folder_name_temp) == 0)
                            {
                                ESP_LOGI(TAG, "Removed folder '%s'", old_folder_name_temp);
                            }
                            else
                            {
                                ESP_LOGE(TAG, "Failed to remove folder '%s'", old_folder_name_temp);
                            }
                        }
                        else
                        {
                            ESP_LOGE(TAG, "3Failed to rename folder from '%s' to '%s'", new_folder_name, old_folder_name);
                        }
                    }
                }
            }
        }
    }

    closedir(dir);
}

/* Copies the full path into destination buffer and returns
 * pointer to path (skipping the preceding base path) */
static const char *get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{
    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest)
    {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash)
    {
        pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize)
    {
        /* Full path string won't fit into destination buffer */
        return NULL;
    }

    /* Construct full path (base + path) */
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* Return pointer to path, skipping the base */
    return dest + base_pathlen;
}

esp_err_t clean_post_handler(httpd_req_t *req)
{
    printf("file clean requested\r\n");
    const char *subdirectory = "/systems";
    char result[strlen(base_path) + 9];
    char tempMode[20];
    mode = get_path_from_uri(tempMode, base_path, req->uri + sizeof("/clean"), sizeof(tempMode));
    // printf("mode from uri: %s\r\n",&mode[0]);
    if (strcmp(mode, "weba") == 0)
    {
        printf("The string is \"weba\"\n");
        snprintf(result, sizeof(result), "%s%s", base_path, subdirectory);
        remove_and_rename_new_files(result);

        subdirectory = "/include";
        snprintf(result, sizeof(result), "%s%s", base_path, subdirectory);
        remove_and_rename_new_files(result);
    }
    else if (strcmp(mode, "conf") == 0)
    {
        printf("The string is \"conf\"\n");
        subdirectory = "/systems";
        snprintf(result, sizeof(result), "%s%s", base_path, subdirectory);
        remove_and_rename_new_files(result);
    }
    else if (strcmp(mode, "allf") == 0)
    {
        printf("The string is \"allf\"\n");
        renameFoldersWithNewSuffix(base_path);
    }
    else
    {
        printf("The string is not anything\n");
    }
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

esp_err_t upload_post_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
    FILE *fd = NULL;
    struct stat file_stat;

    /* Skip leading "/upload" from URI to get filename */
    /* Note sizeof() counts NULL termination hence the -1 */
    const char *filename = get_path_from_uri(filepath, base_path, req->uri + sizeof("/upload") - 1, sizeof(filepath));
    printf("The filename is: %s\r\n", filename);
    printf("File path is: %s\r\n", filepath);
    if (!filename)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
        return ESP_FAIL;
    }

    /* Filename cannot have a trailing '/' */
    if (filename[strlen(filename) - 1] == '/')
    {
        ESP_LOGE(TAG, "Invalid filename : %s", filename);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid filename");
        return ESP_FAIL;
    }

    if (stat(filepath, &file_stat) == 0)
    {
        ESP_LOGE(TAG, "File already exists : %s", filepath);
        unlink(filepath);
        ESP_LOGE(TAG, "Removing existing file at : %s", filepath);
    }

    /* File cannot be larger than a limit */
    if (req->content_len > MAX_FILE_SIZE)
    {
        ESP_LOGE(TAG, "File too large : %d bytes", req->content_len);
        /* Respond with 400 Bad Request */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "File size must be less than " MAX_FILE_SIZE_STR "!");
        /* Return failure to close underlying connection else the
         * incoming file content will keep the socket busy */
        return ESP_FAIL;
    }

    fd = fopen(filepath, "w");
    if (!fd)
    { // only the file or file and folders both dont exist
        ESP_LOGE(TAG, "Failed to create file : %s", filepath);
        struct stat st;
        // int filefound = 0;
        if (stat(filepath, &st) != 0)
        {
            createIntermediateDirs(filepath, &base_path[0]);

            fd = fopen(filepath, "w");
            if (!fd)
            {
                printf("fail create file\n");
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create file");
                return ESP_FAIL;
            }
        }
    }

    ESP_LOGI(TAG, "Receiving file : %s...", filename);

    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *buf = scratch;
    int received;

    /* Content length of the request gives
     * the size of the file being uploaded */
    int remaining = req->content_len;

    while (remaining > 0)
    {

        ESP_LOGI(TAG, "Remaining size : %d", remaining);
        /* Receive the file part by part into a buffer */
        if ((received = httpd_req_recv(req, buf, MIN(remaining, SCRATCH_BUFSIZE))) <= 0)
        {
            if (received == HTTPD_SOCK_ERR_TIMEOUT)
            {
                /* Retry if timeout occurred */
                continue;
            }
            /* In case of unrecoverable error,
             * close and delete the unfinished file*/
            fclose(fd);
            unlink(filepath);

            ESP_LOGE(TAG, "File reception failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
            return ESP_FAIL;
        }

        /* Write buffer content to file on storage */
        if (received && (received != fwrite(buf, 1, received, fd)))
        {
            /* Couldn't write everything to file!
             * Storage may be full? */
            fclose(fd);
            unlink(filepath);

            ESP_LOGE(TAG, "File write failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to write file to storage");
            return ESP_FAIL;
        }

        /* Keep track of remaining size of
         * the file left to be uploaded */
        remaining -= received;
    }

    /* Close file upon upload completion */
    fclose(fd);
    ESP_LOGI(TAG, "File reception complete");

    /* Redirect onto root to see the updated file list */
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_sendstr(req, "File uploaded successfully");
    return ESP_OK;
}

esp_err_t serve_static_file(httpd_req_t *req, const unsigned char *file_start, const unsigned char *file_end, const char *mime_type)
{
    const size_t file_size = file_end - file_start;

    if (file_size == 0)
    {
        ESP_LOGE(TAG, "File size is zero, cannot send response");
        return ESP_FAIL;
    }

    esp_err_t error = httpd_resp_set_type(req, mime_type);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set response type for %s", mime_type);
        return error;
    }

    error = httpd_resp_send(req, (const char *)file_start, file_size);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send %s file", mime_type);
        return error;
    }

    return ESP_OK;
}

esp_err_t boot_min_css(httpd_req_t *req)
{
    extern const unsigned char bootstrap_css_start[] asm("_binary_bootstrap_min_css_start");
    extern const unsigned char bootstrap_css_end[] asm("_binary_bootstrap_min_css_end");
    return serve_static_file(req, bootstrap_css_start, bootstrap_css_end, "text/css");
}

esp_err_t boot_min_js(httpd_req_t *req)
{
    extern const unsigned char bootstrap_js_start[] asm("_binary_bootstrap_bundle_min_js_start");
    extern const unsigned char bootstrap_js_end[] asm("_binary_bootstrap_bundle_min_js_end");
    return serve_static_file(req, bootstrap_js_start, bootstrap_js_end, "application/javascript");
}

esp_err_t jszip_get_handler(httpd_req_t *req)
{
    extern const unsigned char jszip_js_start[] asm("_binary_jszip_min_js_start");
    extern const unsigned char jszip_js_end[] asm("_binary_jszip_min_js_end");
    return serve_static_file(req, jszip_js_start, jszip_js_end, "application/javascript");
}

esp_err_t index_handler(httpd_req_t *req)
{
    extern const unsigned char index_html_start[] asm("_binary_index_html_start");
    extern const unsigned char index_html_end[] asm("_binary_index_html_end");
    return serve_static_file(req, index_html_start, index_html_end, "text/html");
}

esp_err_t index_css_get_handler(httpd_req_t *req)
{
    extern const unsigned char index_style_start[] asm("_binary_style_index_css_start");
    extern const unsigned char index_style_end[] asm("_binary_style_index_css_end");
    return serve_static_file(req, index_style_start, index_style_end, "text/css");
}

esp_err_t index_js_get_handler(httpd_req_t *req)
{
    extern const unsigned char index_js_start[] asm("_binary_script_index_js_start");
    extern const unsigned char index_js_end[] asm("_binary_script_index_js_end");
    return serve_static_file(req, index_js_start, index_js_end, "text/javascript");
}

esp_err_t flash_clear_handler(httpd_req_t *req)
{
    const char *response = NULL;
    if (esp_vfs_fat_sdcard_format(MOUNT_POINT, card) == ESP_OK)
    {
        response = "{\"status\": \"success\"}";
    }
    else
    {
        response = "{\"status\": \"fail\"}";
    }

    // Send a response back to the client
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));

    return ESP_OK;
}

esp_err_t sdcard_dir_handler(httpd_req_t *req)
{
    // Get the requested directory path from the URI
    const char *base_path = MOUNT_POINT; // Base path to the SD card
    char full_path[256];                 // Buffer to hold the full path
    const char *uri = req->uri;

    // Assuming the URI format is /dir/sdcard/<subdirectory>
    snprintf(full_path, sizeof(full_path), "%s%s", base_path, uri + strlen("/dir/sdcard")); // Append requested path
    // printf("Full path: %s\n", full_path);

    DIR *dir = opendir(full_path);
    if (!dir)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open directory");
        return ESP_FAIL;
    }

    cJSON *json = cJSON_CreateArray();
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        // Create a new JSON object for each file/folder
        cJSON *file = cJSON_CreateObject();
        cJSON_AddStringToObject(file, "name", entry->d_name);

        // Construct full file path for stat() function
        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s/%s", full_path, entry->d_name); // Using 'full_path' here

        struct stat file_stat;
        if (stat(file_path, &file_stat) == 0)
        {
            if (S_ISDIR(file_stat.st_mode))
            {
                cJSON_AddStringToObject(file, "type", "directory");
            }
            else if (S_ISREG(file_stat.st_mode))
            {
                cJSON_AddStringToObject(file, "type", "file");

                double size_kb = file_stat.st_size / 1024.0;
                char size_str[16];
                snprintf(size_str, sizeof(size_str), "%.2f", size_kb);

                cJSON_AddStringToObject(file, "size", size_str);
            }
        }
        else
        {
            // Handle stat() failure, log error and skip this entry
            ESP_LOGE("FLASH", "Error getting file info for %s", file_path);
            cJSON_Delete(file); // Cleanup in case of failure
            continue;
        }

        // Add the file/folder to the array
        cJSON_AddItemToArray(json, file);
    }
    closedir(dir);

    // Send the JSON response
    const char *json_str = cJSON_Print(json);
    // printf("JSON string: %s\n", json_str);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));

    // Clean up
    cJSON_Delete(json);
    free((void *)json_str);

    return ESP_OK;
}

esp_err_t sdcard_file_download_handler(httpd_req_t *req)
{
    const char *base_path = MOUNT_POINT;
    char file_path[256];
    const char *uri = req->uri;

    // Construct the full file path dynamically, considering subdirectories
    snprintf(file_path, sizeof(file_path), "%s%s", base_path, uri + strlen("/download"));

    // Log the constructed file path
    ESP_LOGI("file_download", "File path: %s", file_path);

    // Check if the file exists
    FILE *file = fopen(file_path, "r");
    if (!file)
    {
        ESP_LOGE("file_error", "Failed to open file: %s, errno: %d", file_path, errno);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open file for download");
        return ESP_FAIL;
    }

    // Set the correct Content-Type and headers for the file
    httpd_resp_set_type(req, "application/octet-stream");

    // Send file content in chunks to the client
    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        if (httpd_resp_send_chunk(req, buffer, bytes_read) != ESP_OK)
        {
            fclose(file);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file content");
            return ESP_FAIL;
        }
    }

    // End the HTTP response
    httpd_resp_send_chunk(req, NULL, 0);
    fclose(file);
    return ESP_OK;
}

esp_err_t sdcard_list_files_recursive(char *basePath, httpd_req_t *req, bool *isFirstFile) {
    char path[600];
    struct dirent *entry;
    DIR *dir = opendir(basePath);

    if (!dir) {
        printf("Failed to open directory: %s\n", basePath);  // Debug print
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open directory");
        return ESP_FAIL;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            // Skip hidden directories like . and ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            // Recursively enter subdirectories
            snprintf(path, sizeof(path), "%s/%s", basePath, entry->d_name);
            printf("Entering directory: %s\n", path);  // Debug print
            sdcard_list_files_recursive(path, req, isFirstFile);
        } else {
            // This is a file, send it in the response
            if (*isFirstFile) {
                *isFirstFile = false; // Set the flag to false after the first file
            } else {
                httpd_resp_sendstr_chunk(req, ","); // Add comma separator for subsequent files
            }

            snprintf(path, sizeof(path), "\"%s/%s\"", basePath, entry->d_name);
            printf("Adding file to JSON: %s\n", path);  // Debug print
            httpd_resp_sendstr_chunk(req, path);
        }
    }

    closedir(dir);
    return ESP_OK;
}

esp_err_t sdcard_list_files_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "{\"files\":["); // Start JSON response

    bool isFirstFile = true; // Flag to track the first file
    sdcard_list_files_recursive("/sdcard", req, &isFirstFile); // Pass the flag to recursive function

    httpd_resp_sendstr_chunk(req, "]}"); // Close the JSON array and object
    httpd_resp_sendstr_chunk(req, NULL);  // End the response

    printf("Finished sending the file list.\n");  // Debug print

    return ESP_OK;
}

esp_err_t static_file_handler(httpd_req_t *req)
{
    // Extract the endpoint from the URI
    const char *uri = req->uri;

    // Handle different API endpoints
    if (strcmp(uri, "/plugins/bootstrap/bootstrap.min.css") == 0)
    {
        // Return the bootstrap minified CSS file
        return boot_min_css(req);
    }
    else if (strcmp(uri, "/plugins/bootstrap/bootstrap.bundle.min.js") == 0)
    {
        // Return the bootstrap minified JS file
        return boot_min_js(req);
    }
    else if (strcmp(uri, "/plugins/jszip/jszip.min.js") == 0)
    {
        // Return the JSZip minified JS file
        return jszip_get_handler(req);
    }
    else if (strcmp(uri, "/") == 0)
    {
        // Return the index.html file
        return index_handler(req);
    }
    else if (strcmp(uri, "/styles/style_index.css") == 0)
    {
        // Return the index CSS file
        return index_css_get_handler(req);
    }
    else if (strcmp(uri, "/scripts/script_index.js") == 0)
    {
        // Return the index JS file
        return index_js_get_handler(req);
    }
    else if (strstr(uri, "/dir/") != 0)
    {
        // Return the index JS file
        return sdcard_dir_handler(req);
    }
    else if (strstr(uri, "/download/") != 0)
    {
        // Return the index JS file
        return sdcard_file_download_handler(req);
    }
    else if (strstr(uri, "/list-files") != 0)
    {
        // Return the index JS file
        return sdcard_list_files_handler(req);
    }

    ESP_LOGE(TAG, "URI not found : %s", uri);
    // Unknown endpoint, respond with 404
    httpd_resp_send_404(req);
    return ESP_FAIL;
}

esp_err_t api_handler(httpd_req_t *req)
{
    // Extract the endpoint from the URI
    const char *uri = req->uri;

    // Handle different API endpoints
    if (strcmp(uri, "/api/clear_flash") == 0)
    {
        // Clear the flash storage
        return flash_clear_handler(req);
    }

    ESP_LOGE(TAG, "URI not found : %s", uri);
    // Unknown endpoint, respond with 404
    httpd_resp_send_404(req);
    return ESP_FAIL;
}

// Register a handler for all static files
httpd_uri_t static_files = {
    .uri = "/*",
    .method = HTTP_GET,
    .handler = static_file_handler,
    .user_ctx = NULL};

/* URI handler for clearing main folders and files from server */
httpd_uri_t file_clean = {
    .uri = "/clean/*",
    .method = HTTP_POST,
    .handler = clean_post_handler,
    .user_ctx = NULL};

// Register a handler for all API endpoints
httpd_uri_t upload = {
    .uri = "/upload/*",
    .method = HTTP_POST,
    .handler = upload_post_handler,
    .user_ctx = NULL};

// Register a handler for all API endpoints
httpd_uri_t api_endpoints = {
    .uri = "/api/*",
    .method = HTTP_POST,
    .handler = api_handler,
    .user_ctx = NULL};

httpd_handle_t start_webserver(void)
{
    ESP_LOGI(TAG, "Starting webserver");
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.stack_size = 8192;
    config.max_uri_handlers = 10;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &static_files);
        httpd_register_uri_handler(server, &api_endpoints);
        httpd_register_uri_handler(server, &upload);
        httpd_register_uri_handler(server, &file_clean);

        // ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &connect_handler, &server));
        // ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &disconnect_handler, &server));
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void stop_webserver()
{
    ESP_LOGI(TAG, "Stopping webserver");
    httpd_stop(server);
}

void disconnect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    stop_webserver();
}

void connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    start_webserver();
}