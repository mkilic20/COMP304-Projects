#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h> // termios, TCSANOW, ECHO, ICANON
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <sys/utsname.h>

const char *sysname = "Shellect";

enum return_codes {
	SUCCESS = 0,
	EXIT = 1,
	UNKNOWN = 2,
};

struct command_t {
	char *name;
	bool background;
	bool auto_complete;
	int arg_count;
	char **args;
	char *redirects[3]; // in/out redirection
	struct command_t *next; // for piping
};

// New alias struct
typedef struct alias_t {
	char *name;
	char *command;
	struct alias_t *next;
} alias_t;

/**
 * Prints a command struct
 * @param struct command_t *
 */
void print_command(struct command_t *command) {
	int i = 0;
	printf("Command: <%s>\n", command->name);
	printf("\tIs Background: %s\n", command->background ? "yes" : "no");
	printf("\tNeeds Auto-complete: %s\n",
		   command->auto_complete ? "yes" : "no");
	printf("\tRedirects:\n");

	for (i = 0; i < 3; i++) {
		printf("\t\t%d: %s\n", i,
			   command->redirects[i] ? command->redirects[i] : "N/A");
	}

	printf("\tArguments (%d):\n", command->arg_count);

	for (i = 0; i < command->arg_count; ++i) {
		printf("\t\tArg %d: %s\n", i, command->args[i]);
	}

	if (command->next) {
		printf("\tPiped to:\n");
		print_command(command->next);
	}
}

/**
 * Release allocated memory of a command
 * @param  command [description]
 * @return         [description]
 */
int free_command(struct command_t *command) {
	if (command->arg_count) {
		for (int i = 0; i < command->arg_count; ++i)
			free(command->args[i]);
		free(command->args);
	}

	for (int i = 0; i < 3; ++i) {
		if (command->redirects[i])
			free(command->redirects[i]);
	}

	if (command->next) {
		free_command(command->next);
		command->next = NULL;
	}

	free(command->name);
	free(command);
	return 0;
}

/**
 * Show the command prompt
 * @return [description]
 */
int show_prompt() {
	char cwd[1024], hostname[1024];
	gethostname(hostname, sizeof(hostname));
	getcwd(cwd, sizeof(cwd));
	printf("%s@%s:%s %s$ ", getenv("USER"), hostname, cwd, sysname);
	return 0;
}

/**
 * Parse a command string into a command struct
 * @param  buf     [description]
 * @param  command [description]
 * @return         0
 */
int parse_command(char *buf, struct command_t *command) {
	const char *splitters = " \t"; // split at whitespace
	int index, len;
	len = strlen(buf);

	// trim left whitespace
	while (len > 0 && strchr(splitters, buf[0]) != NULL) {
		buf++;
		len--;
	}

	while (len > 0 && strchr(splitters, buf[len - 1]) != NULL) {
		// trim right whitespace
		buf[--len] = 0;
	}

	// auto-complete
	if (len > 0 && buf[len - 1] == '?') {
		command->auto_complete = true;
	}

	// background
	if (len > 0 && buf[len - 1] == '&') {
		command->background = true;
	}

	char *pch = strtok(buf, splitters);
	if (pch == NULL) {
		command->name = (char *)malloc(1);
		command->name[0] = 0;
	} else {
		command->name = (char *)malloc(strlen(pch) + 1);
		strcpy(command->name, pch);
	}

	command->args = (char **)malloc(sizeof(char *));

	int redirect_index;
	int arg_index = 0;
	char temp_buf[1024], *arg;

	while (1) {
		// tokenize input on splitters
		pch = strtok(NULL, splitters);
		if (!pch)
			break;
		arg = temp_buf;
		strcpy(arg, pch);
		len = strlen(arg);

		// empty arg, go for next
		if (len == 0) {
			continue;
		}

		// trim left whitespace
		while (len > 0 && strchr(splitters, arg[0]) != NULL) {
			arg++;
			len--;
		}

		// trim right whitespace
		while (len > 0 && strchr(splitters, arg[len - 1]) != NULL) {
			arg[--len] = 0;
		}

		// empty arg, go for next
		if (len == 0) {
			continue;
		}

		// piping to another command
		if (strcmp(arg, "|") == 0) {
			struct command_t *c = malloc(sizeof(struct command_t));
			int l = strlen(pch);
			pch[l] = splitters[0]; // restore strtok termination
			index = 1;
			while (pch[index] == ' ' || pch[index] == '\t')
				index++; // skip whitespaces

			parse_command(pch + index, c);
			pch[l] = 0; // put back strtok termination
			command->next = c;
			continue;
		}

		// background process
		if (strcmp(arg, "&") == 0) {
			// handled before
			continue;
		}

		// handle input redirection
		redirect_index = -1;
		if (arg[0] == '<') {
			redirect_index = 0;
		}

		if (arg[0] == '>') {
			if (len > 1 && arg[1] == '>') {
				redirect_index = 2;
				arg++;
				len--;
			} else {
				redirect_index = 1;
			}
		}

		if (redirect_index != -1) {
			command->redirects[redirect_index] = malloc(len);
			strcpy(command->redirects[redirect_index], arg + 1);
			continue;
		}

		// normal arguments
		if (len > 2 &&
			((arg[0] == '"' && arg[len - 1] == '"') ||
			 (arg[0] == '\'' && arg[len - 1] == '\''))) // quote wrapped arg
		{
			arg[--len] = 0;
			arg++;
		}

		command->args =
			(char **)realloc(command->args, sizeof(char *) * (arg_index + 1));

		command->args[arg_index] = (char *)malloc(len + 1);
		strcpy(command->args[arg_index++], arg);
	}
	command->arg_count = arg_index;

	// increase args size by 2
	command->args = (char **)realloc(
		command->args, sizeof(char *) * (command->arg_count += 2));

	// shift everything forward by 1
	for (int i = command->arg_count - 2; i > 0; --i) {
		command->args[i] = command->args[i - 1];
	}

	// set args[0] as a copy of name
	command->args[0] = strdup(command->name);

	// set args[arg_count-1] (last) to NULL
	command->args[command->arg_count - 1] = NULL;

	return 0;
}

void prompt_backspace() {
	putchar(8); // go back 1
	putchar(' '); // write empty over
	putchar(8); // go back 1 again
}

/**
 * Prompt a command from the user
 * @param  buf      [description]
 * @param  buf_size [description]
 * @return          [description]
 */
int prompt(struct command_t *command) {
	size_t index = 0;
	char c;
	char buf[4096];
	static char oldbuf[4096];

	// tcgetattr gets the parameters of the current terminal
	// STDIN_FILENO will tell tcgetattr that it should write the settings
	// of stdin to oldt
	static struct termios backup_termios, new_termios;
	tcgetattr(STDIN_FILENO, &backup_termios);
	new_termios = backup_termios;
	// ICANON normally takes care that one line at a time will be processed
	// that means it will return if it sees a "\n" or an EOF or an EOL
	new_termios.c_lflag &=
		~(ICANON |
		  ECHO); // Also disable automatic echo. We manually echo each char.
	// Those new settings will be set to STDIN
	// TCSANOW tells tcsetattr to change attributes immediately.
	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

	show_prompt();
	buf[0] = 0;

	while (1) {
		c = getchar();
		// printf("Keycode: %u\n", c); // DEBUG: uncomment for debugging

		// handle tab
		if (c == 9) {
			buf[index++] = '?'; // autocomplete
			break;
		}

		// handle backspace
		if (c == 127) {
			if (index > 0) {
				prompt_backspace();
				index--;
			}
			continue;
		}

		if (c == 27 || c == 91 || c == 66 || c == 67 || c == 68) {
			continue;
		}

		// up arrow
		if (c == 65) {
			while (index > 0) {
				prompt_backspace();
				index--;
			}

			char tmpbuf[4096];
			printf("%s", oldbuf);
			strcpy(tmpbuf, buf);
			strcpy(buf, oldbuf);
			strcpy(oldbuf, tmpbuf);
			index += strlen(buf);
			continue;
		}

		putchar(c); // echo the character
		buf[index++] = c;
		if (index >= sizeof(buf) - 1)
			break;
		if (c == '\n') // enter key
			break;
		if (c == 4) // Ctrl+D
			return EXIT;
	}

	// trim newline from the end
	if (index > 0 && buf[index - 1] == '\n') {
		index--;
	}

	// null terminate string
	buf[index++] = '\0';

	strcpy(oldbuf, buf);

	parse_command(buf, command);

	// print_command(command); // DEBUG: uncomment for debugging

	// restore the old settings
	tcsetattr(STDIN_FILENO, TCSANOW, &backup_termios);
	return SUCCESS;
}

int process_command(struct command_t *command);
// new functions
char *find_command_location(const char *command);
void handle_redirections(struct command_t *command);
void create_aliases_from_text();
void append_alias(const char *name, const char *command);
char *find_alias_command(const char *name);
alias_t *alias_list = NULL;
void print_hex_line(const unsigned char *buffer, size_t len, size_t offset);
int hexdump(FILE *input, int groupsize);
int schedule_alarm(int minutes, const char *audio_path);
int fetch();
void get_user_ip();

int main() {
	// read and appends the alias_list
	create_aliases_from_text();
	while (1) {
		struct command_t *command = malloc(sizeof(struct command_t));

		// set all bytes to 0
		memset(command, 0, sizeof(struct command_t));

		int code;
		code = prompt(command);
		if (code == EXIT) {
			break;
		}

		code = process_command(command);
		if (code == EXIT) {
			break;
		}

		free_command(command);
	}

	printf("\n");
	return 0;
}

int process_command(struct command_t *command) {
	int r;
	//check if it is alias command
	if (strcmp(command->name, "alias") == 0) {
		if (command->arg_count > 2) {
			int length = 0;
			int arg_count = command->arg_count;
			for (int i = 2; i < arg_count; ++i) {
				if (command->args[i]) {
					length += strlen(command->args[i]) + 1;
				}
			}
			char *command_str = malloc(length);
			strcpy(command_str, command->args[2]);
			for (int i = 3; i < command->arg_count; ++i) {
				if (command->args[i]) {
					strcat(command_str, " ");
					strcat(command_str, command->args[i]);
				}
			}
			// try to add the new alias command, if it is a new command it will be added else will return already exists output
			append_alias(command->args[1], command_str);
			free(command_str);
			return SUCCESS;
		} else {
			fprintf(stderr, "Usage: alias <name> <command>\n");
			return UNKNOWN;
		}
	}

	char *alias_command = find_alias_command(command->name);
	if (alias_command) {
		free(command->name);
		command->name = strdup(alias_command);
		char *temp_buf = strdup(alias_command);
		parse_command(temp_buf, command);
		free(temp_buf);
	}

	// case for "hexdump"
	if (strcmp(command->name, "hexdump") == 0) {
		int groupsize = 1; // Default group size
		const char *filename = NULL;

		// Parse arguments
		for (int i = 0; i < command->arg_count; i++) {
			if (strcmp(command->args[i], "-g") == 0 &&
				i + 1 < command->arg_count) {
				groupsize = atoi(command->args[i + 1]);
				if (groupsize <= 0 || groupsize > 16 ||
					(groupsize & (groupsize - 1)) != 0) {
					fprintf(
						stderr,
						"Group size must be a power of 2 and no larger than 16.\n");
					return UNKNOWN;
				}
				i++;
			} else {
				filename = command->args[i];
			}
		}

		// Read filename or read from stdin
		FILE *fp;
		if (filename != NULL) {
			fp = fopen(filename, "rb");
		} else {
			fp = stdin;
		}

		// Call the hexdump function
		int status = hexdump(fp, groupsize);

		if (fp != stdin) {
			fclose(fp);
		}

		return status;
	}

	if (strcmp(command->name, "good_morning") == 0) {
		printf("%d", command->arg_count);
		if (command->arg_count > 4) {
			fprintf(stderr, "Usage: good_morning <minutes> <path/to/audio>\n");
			return -1;
		}

		int minutes = atoi(command->args[1]);
		const char *audio_path = command->args[2];
		return schedule_alarm(minutes, audio_path);
	}

	// add fetch

	if (strcmp(command->name, "fetch") == 0) {
		return fetch();
	}

	// add ip
	if (strcmp(command->name, "getip") == 0) {
		get_user_ip();
		return SUCCESS;
	}

	if (strcmp(command->name, "") == 0) {
		return SUCCESS;
	}

	if (strcmp(command->name, "exit") == 0) {
		return EXIT;
	}

	if (strcmp(command->name, "cd") == 0) {
		if (command->arg_count > 0) {
			r = chdir(command->args[0]);
			if (r == -1) {
				printf("-%s: %s: %s\n", sysname, command->name,
					   strerror(errno));
			}

			return SUCCESS;
		}
	}

	pid_t pid = fork();
	// child
	if (pid == 0) {
		// check for redirects
		handle_redirections(command);

		// new exec
		char *cmd_path =
			find_command_location(command->name); //get command location
		if (cmd_path) {
			execv(cmd_path, command->args);
			free(cmd_path);
		} else {
			fprintf(stderr, "command path not found");
			exit(1);
		}
		exit(1);
	} // parent
	//background
	else if (pid > 0) {
		if (!command->background) {
			waitpid(pid, NULL, 0);
		}
	} // fork error
	else if (pid < 0) {
		fprintf(stderr, "fork failed");
		exit(1);
	}

	return SUCCESS;
}

// P1: finding command locations
char *find_command_location(const char *command) {
	// No command
	if (!command) {
		return NULL;
	}

	// If command is path
	if (command[0] == '/' || command[0] == '.') {
		if (access(command, X_OK) == 0) {
			return strdup(command);
		}
		return NULL;
	}

	// Checking directories for the command
	char *system_path = getenv("PATH");
	char *path_copy = strdup(system_path);
	char *directory = strtok(path_copy, ":");
	while (directory) {
		char *full_path = malloc(strlen(directory) + strlen(command) + 2);
		sprintf(full_path, "%s/%s", directory, command);
		// if path is found
		if (access(full_path, X_OK) == 0) {
			free(path_copy);
			return full_path;
		} // if not found
		else {
			free(full_path);
			directory = strtok(NULL, ":");
		}
	}

	// if path is not found anywhere
	free(path_copy);
	return NULL;
}

// P2: handling redirection commands
void handle_redirections(struct command_t *command) {
	// redirect[0] <
	if (command->redirects[0]) {
		int file_fd = open(command->redirects[0], O_RDONLY);
		if (file_fd < 0) {
			fprintf(stderr, "input not found");
			exit(1);
		}
		dup2(file_fd, 0);
		close(file_fd);
	}

	// redirect[1] >
	if (command->redirects[1]) {
		int file_fd =
			open(command->redirects[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (file_fd < 0) {
			fprintf(stderr, "output not found");
			exit(1);
		}
		dup2(file_fd, 1);
		close(file_fd);
	}

	// redirect[2] >>
	if (command->redirects[2]) {
		int file_fd =
			open(command->redirects[2], O_WRONLY | O_CREAT | O_APPEND, 0666);
		if (file_fd < 0) {
			fprintf(stderr, "output not found");
			exit(1);
		}
		dup2(file_fd, 1);
		close(file_fd);
	}
}

// reads the aliases from alias.txt in beginning and appends to alias_list
void create_aliases_from_text() {
	FILE *file = fopen("alias.txt", "a+");
	if (!file) {
		return;
	}

	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	while ((read = getline(&line, &len, file)) != -1) {
		char *name = strtok(line, " ");
		char *command = strtok(NULL, "\n");
		if (name && command) {
			// Directly add to alias list without writing back to file
			alias_t *new_alias = malloc(sizeof(alias_t));
			new_alias->name = strdup(name);
			new_alias->command = strdup(command);
			new_alias->next = alias_list;
			alias_list = new_alias;
		}
	}
	if (line) {
		free(line);
	}
	fclose(file);
}

void append_alias(const char *name, const char *command) {
	// Check if alias already exists
	for (alias_t *alias = alias_list; alias != NULL; alias = alias->next) {
		if (strcmp(alias->name, name) == 0) {
			free(alias->command);
			alias->command = strdup(command);
			printf("alias already exists.");
			return;
		}
	}

	// If alias does not exist, create a new one
	alias_t *new_alias = malloc(sizeof(alias_t));
	new_alias->name = strdup(name);
	new_alias->command = strdup(command);
	new_alias->next = alias_list;
	alias_list = new_alias;

	// Append the new alias to alias.txt
	FILE *file = fopen("alias.txt", "a");
	if (file) {
		fprintf(file, "\n%s %s", name, command);
		fclose(file);
	}
}
//  finds the alias command from the name in the linkedlist
char *find_alias_command(const char *name) {
	for (alias_t *alias_i = alias_list; alias_i != NULL;
		 alias_i = alias_i->next) {
		if (strcmp(alias_i->name, name) == 0) {
			return alias_i->command;
		}
	}
	return NULL;
}

// P3: Hexdump

int hexdump(FILE *input, int groupsize) {
	// Implement the actual hex dump logic here
	unsigned char buffer[16];
	size_t bytes_read;
	unsigned int offset = 0;

	while ((bytes_read = fread(buffer, 1, sizeof(buffer), input)) > 0) {
		printf("%08x  ", offset);

		// Print the hex codes
		for (size_t i = 0; i < bytes_read; i += groupsize) {
			for (int j = 0; j < groupsize && (i + j < bytes_read); j++) {
				printf("%02x", buffer[i + j]);
			}
			printf(" ");
		}

		// Pput buffer if less than 16 bytes
		if (bytes_read < sizeof(buffer)) {
			for (size_t i = bytes_read; i < sizeof(buffer); i += groupsize) {
				for (int j = 0; j < groupsize; j++) {
					printf("  ");
				}
				printf(" ");
			}
		}

		// Print the ASCII representation
		printf(" |");
		for (size_t i = 0; i < bytes_read; i++) {
			if (isprint(buffer[i])) {
				printf("%c", buffer[i]);
			} else {
				printf(".");
			}
		}
		printf("|\n");

		offset += bytes_read;
	}

	if (ferror(input)) {
		perror("Error reading from input");
		return -1;
	}

	return 0;
}

// Function to print hex dump

void print_hex_line(const unsigned char *buffer, size_t len, size_t offset) {
	printf("%06zx  ", offset); // Print the offset in the file
	for (size_t i = 0; i < len; i++) {
		printf("%02x ", buffer[i]); // Print each byte
		if (i == 7)
			printf(" ");
	}
	printf(" |");
	for (size_t i = 0; i < len; i++) {
		if (buffer[i] < 32 || buffer[i] > 126) //non printables
			printf(".");
		else
			printf("%c", buffer[i]); // ASCII characters
	}
	printf("|\n");
}

// P3: Good Morning

int schedule_alarm(int minutes, const char *audio_path) {
	// Calculate the time to schedule the alarm
	time_t now = time(NULL);
	struct tm *time_info = localtime(&now);
	time_info->tm_min += minutes;
	mktime(time_info);

	// Scheduling
	char cmd[256];
	snprintf(cmd, sizeof(cmd), "echo 'mpg123 \"%s\"' | at %02d:%02d",
			 audio_path, time_info->tm_hour, time_info->tm_min);

	system(cmd);

	printf("Alarm set for %02d:%02d\n", time_info->tm_hour, time_info->tm_min);
	return 0;
}

// P3: Custom Command: Fetch

int fetch() {
	char hostname[256];
	struct utsname uts;
	time_t now;
	struct tm *now_tm;
	int hours, minutes;

	// Get the hostname
	gethostname(hostname, sizeof(hostname));

	// Get system information
	uname(&uts);

	// Get the current time
	now = time(NULL);
	now_tm = localtime(&now);

	// Calculate the system's uptime
	// long uptime = sysconf(_SC_CLK_TCK);
	FILE *uptime_file = fopen("/proc/uptime", "r");
	if (uptime_file == NULL) {
		perror("Error opening uptime file");
		return 1;
	}
	fscanf(uptime_file, "%d.%d", &hours, &minutes);
	fclose(uptime_file);

	// Get the number of running processes
	int process_count = 0;
	system("ps -e --no-headers | wc -l > /tmp/process_count.txt");
	FILE *proc_file = fopen("/tmp/process_count.txt", "r");
	if (proc_file == NULL) {
		perror("Error opening process count file");
		return 1;
	}
	fscanf(proc_file, "%d", &process_count);
	fclose(proc_file);

	// Print the fetched information
	printf("User: %s\n", getenv("USER"));
	printf("Hostname: %s\n", hostname);
	printf("OS: %s\n", uts.sysname);
	printf("Kernel: %s\n", uts.release);
	printf("Uptime: %dh %dm\n", hours, minutes);
	printf("Date: %02d-%02d-%02d\n", now_tm->tm_year + 1900, now_tm->tm_mon + 1,
		   now_tm->tm_mday);
	printf("Time: %02d:%02d\n", now_tm->tm_hour, now_tm->tm_min);
	printf("Running Processes: %d\n", process_count);
	printf("Shell: %s\n", getenv("SHELL"));

	return 0;
}

// Custom Command: getip
void get_user_ip() {
	FILE *fp;
	char path[1000];

	/* Open the command for reading. */
	fp = popen("curl -s http://ipinfo.io/ip", "r");

	/* Read the output a line at a time - output it. */
	while (fgets(path, sizeof(path), fp) != NULL) {
		printf("IP Address is: %s \n", path);
	}

	/* Close the file pointer */
	pclose(fp);
}
