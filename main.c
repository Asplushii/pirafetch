#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#define PURPLE "\033[0;35m"
#define RESET "\033[0m"
#define MAX_PATH_LEN 256
#define CONFIG_DIR_PATH "/home/%s/.config/pira"
#define CONFIG_FILE_PATH "/home/%s/.config/pira/config.conf"

void executeCommand(const char * command, char ** output);
void createConfigFile();
void parseConfigFile();

int count_installed_packages();

void executeCommand(const char * command, char ** output) {
  FILE * fp = popen(command, "r");
  if (fp) {
    char buffer[MAX_PATH_LEN];
    size_t len = 0, output_size = MAX_PATH_LEN;
    * output = malloc(output_size);
    if (! * output) {
      perror("malloc");
      exit(EXIT_FAILURE);
    }
    while (fgets(buffer, sizeof(buffer), fp)) {
      size_t buffer_len = strlen(buffer);
      if (len + buffer_len + 1 >= output_size && ( * output = realloc( * output, output_size *= 2)) == NULL) {
        perror("realloc");
        exit(EXIT_FAILURE);
      }
      strcpy( * output + len, buffer);
      len += buffer_len;
    }
  }
}

void createConfigFile() {
  const char * config_content = "info \"OS:\" distro\ninfo \"Kernel:\" kernel\ninfo \"Hostname:\" hostname\ninfo \"Model:\" model\ninfo \"CPU:\" cpu\ninfo \"GPU:\" gpu\ninfo \"Packages:\" packages\ninfo \"Battery:\" battery\n";
  char config_dir[MAX_PATH_LEN], config_file[MAX_PATH_LEN];
  sprintf(config_dir, CONFIG_DIR_PATH, getlogin());
  sprintf(config_file, CONFIG_FILE_PATH, getlogin());
  FILE * fp = fopen(config_file, "w");
  fp && fputs(config_content, fp) && fclose(fp);
}

void parseConfigFile() {
  char config_file[MAX_PATH_LEN];
  sprintf(config_file, CONFIG_FILE_PATH, getlogin());
  FILE * fp = fopen(config_file, "r");
  if (fp) {
    char line[MAX_PATH_LEN];
    while (fgets(line, sizeof(line), fp)) {
      char * info_type = strtok(line, " ");
      if (info_type) {
        char * info_label = strtok(NULL, "\"\n");
        char * info_function = strtok(NULL, "\n");

        if (info_label) {
          int len = strlen(info_label);
          if (info_label[0] == '"' && info_label[len - 1] == '"') {
            printf("%s%s%s\n", PURPLE, info_label + 1, RESET);
          } else {
            printf("%s ", info_label);
          }
        }
        if (info_function) {
          char * trimmed_function = info_function;
          while ( * trimmed_function && isspace( * trimmed_function)) trimmed_function++;
          char * end = trimmed_function + strlen(trimmed_function) - 1;
          while (end > trimmed_function && isspace( * end)) end--;
          end[1] = '\0';

          char * output = NULL;
          if (strcmp(info_type, "info") == 0 || strlen(info_type) == 0) {
            if (strcmp(trimmed_function, "distro") == 0) {
              executeCommand("cat /etc/os-release | grep '^PRETTY_NAME=' | awk -F '\"' '{print $2}' || echo $(grep '^NAME=' /etc/os-release | awk -F '\"' '{print $2}')", & output);
            } else if (strcmp(trimmed_function, "kernel") == 0) {
              executeCommand("uname -r", & output);
            } else if (strcmp(trimmed_function, "hostname") == 0) {
              executeCommand("hostname", & output);
            } else if (strcmp(trimmed_function, "model") == 0) {
              executeCommand("cat /sys/devices/virtual/dmi/id/product_name", & output);
            } else if (strcmp(trimmed_function, "cpu") == 0) {
              executeCommand("lscpu | grep 'Model name' | cut -d':' -f2 | sed 's/^ *//'", & output);
            } else if (strcmp(trimmed_function, "gpu") == 0) {
              executeCommand("lspci | grep -i 'vga compatible controller' | sed -n 's/.*\\[\\(.*\\)\\].*/\\1/p'", & output);
            } else if (strcmp(trimmed_function, "packages") == 0) {
              char package_count[10];
              snprintf(package_count, sizeof(package_count), "%d\n", count_installed_packages());
              output = strdup(package_count);
            } else if (strcmp(trimmed_function, "battery") == 0) {
              executeCommand("upower -i $(upower -e | grep BAT) | awk '/percentage:/ {print $2}'", & output);
            }
            if (output != NULL) {
              printf("%s%s%s\n", PURPLE, output, RESET);
              free(output);
            }
          }
        }
      }
    }
    fclose(fp);
  }
}

int count_installed_packages() {
  FILE * fp = popen("pacman -Qq | wc -l", "r");
  int count = fp ? (fscanf(fp, "%d", & count), pclose(fp), count) : 0;
  return count;
}

int main() {
  char config_dir[MAX_PATH_LEN], config_file[MAX_PATH_LEN];
  sprintf(config_dir, CONFIG_DIR_PATH, getlogin());
  struct stat st;
  if (stat(config_dir, & st) == -1 && mkdir(config_dir, 0777) == -1) {
    perror("mkdir");
    exit(EXIT_FAILURE);
  }
  sprintf(config_file, CONFIG_FILE_PATH, getlogin());
  if (access(config_file, F_OK) == -1) createConfigFile();
  parseConfigFile();
  return 0;
}
