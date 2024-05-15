#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <pthread.h>

#define MAX_LENGTH 128
#define NUM_COMMANDS 19
#define THREAD_POOL_SIZE 12

typedef struct {
    const char *key;
    char value[MAX_LENGTH];
} CacheEntry;

CacheEntry cache[5]; 
int cache_count = 0;

typedef struct {
    const char *command;
    char output[MAX_LENGTH]; 
} CommandOutput;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int command_index = 0;

char* find_in_cache(const char *key) {
    for (int i = 0; i < cache_count; i++) {
        if (strcmp(cache[i].key, key) == 0) {
            return cache[i].value;
        }
    }
    return NULL;
}

void add_to_cache(const char *key, const char *value) {
    if (cache_count < sizeof(cache)/sizeof(CacheEntry)) {
        cache[cache_count].key = strdup(key); 
        strncpy(cache[cache_count].value, value, MAX_LENGTH);
        cache_count++;
    }
}

void *execute_command(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        int index = command_index++;
        pthread_mutex_unlock(&mutex);

        if (index >= NUM_COMMANDS) {
            break; 
        }

        CommandOutput *cmd = &((CommandOutput *)arg)[index];
        const char *cached_output = find_in_cache(cmd->command);
        if (cached_output != NULL) {
            strcpy(cmd->output, cached_output);
            continue;
        }

        FILE *fp = popen(cmd->command, "r");
        if (fp == NULL) {
            perror("Error executing command");
            pthread_exit(NULL);
        }

        char *fgets_ret;
        char *output = cmd->output;

        do {
            fgets_ret = fgets(output, MAX_LENGTH, fp);
            if (fgets_ret != NULL) {
                size_t len = strlen(output);
                if (len > 0 && output[len - 1] == '\n') {
                    output[len - 1] = '\0';
                }
            }
        } while (fgets_ret != NULL && (!feof(fp)));

        add_to_cache(cmd->command, cmd->output);

        pclose(fp); 
    }

    pthread_exit(NULL);
}

void print_info(const char *label, const char *output) {
    printf("\033[1;32m%s\033[0m: %s\n", label, output);
}

int main() {

    const char *commands[NUM_COMMANDS] = {
        "sed -n 's/^NAME=\"\\(.*\\)\"/\\1/p' /etc/os-release", 
        "hostnamectl | awk '/Static hostname/{print $3}'", 
        "uname -r", 
        "uptime -p | sed 's/up //'", 
        "pacman -Qq | wc -l",
        "basename $SHELL", 
        "xdpyinfo | awk '/dimensions:/{print $2}'", 
        "echo $XDG_CURRENT_DESKTOP", 
        "gsettings get org.gnome.desktop.interface gtk-theme | tr -d \"'\"", 
        "gsettings get org.gnome.desktop.interface icon-theme | tr -d \"'\"", 
        "gsettings get org.gnome.desktop.interface cursor-theme | tr -d \"'\"", 
        "fc-match :family | awk -F '\"' '{print $2}'", 
        "echo $TERM", 
        "lscpu | awk -F': +' '/Model name/{print $2}'", 
        "lspci | awk '/VGA compatible controller/{gsub(/.*\\[|\\].*/, \"\", $0); print}'", 
        "free -h | awk '/^Mem/{print $3\" / \"$2\" (\"int(($3/$2)*100)\"%)\"}'", 
        "df -h / | awk 'NR==2{print $3\" / \"$2\" (\"$5\")\"}'", 
        "ip addr show wlan0 | awk '/inet /{print $2}'", 
        "acpi -b | grep -Eo '[0-9]+%'" 
    };

    const char *labels[NUM_COMMANDS] = {
        "OS", "Host", "Kernel", "Uptime", "Packages (pacman)", "Shell", "Display", 
        "WM/DE", "Theme (GTK)", "Icons (GTK)", "Cursor (GTK)", "Font", "Terminal", 
        "CPU", "GPU", "Memory", "Disk (/)", "Local IP", "Battery"
    };

    pthread_t threads[THREAD_POOL_SIZE];
    CommandOutput outputs[NUM_COMMANDS];

    for (int i = 0; i < NUM_COMMANDS; i++) {
        outputs[i].command = commands[i];
    }

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&threads[i], NULL, execute_command, (void *)outputs);
    }

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < NUM_COMMANDS; i++) {
        print_info(labels[i], outputs[i].output);
    }

    for (int i = 0; i < cache_count; i++) {
        free((void*)cache[i].key);
    }

    return 0;
}
