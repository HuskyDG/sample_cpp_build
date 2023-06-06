
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>

void write_string(int fd, const char *str) {
	write(fd, str, strlen(str));
}

bool check_valid_name(const char *name) {
	if (name[0] >= '0' && name[0] <= '9')
	    return false;
	for (int i = 0; name[i]; i++) {
		if ((name[i] < 'a' || name[i] > 'z') &&
			  (name[i] < 'A' || name[i] > 'Z') &&
			  name[i] != '_' &&
			  (name[i] < '0' || name[i] > '9'))
			  return false;
	}
	return true;
}


int main(int argc, char *argv[])
{
	char c;
	char buf[100], input[1024], output[1024], name[1024];
	if (argc >= 4) {
		strcpy(input, argv[2]);
		strcpy(output, argv[3]);
		strcpy(name, argv[1]);
	} else {
		printf("Enter source file: ");
		gets(input);
		printf("Enter target file: ");
		gets(output);
		printf("Enter name: ");
		gets(name);
	}
	if (!check_valid_name(name)) {
		printf("Invalid name variable!\n");
		return 1;
	}
	int fd[2] = { open(input, O_RDONLY), open(output, O_WRONLY | O_CREAT, 0644) };
	if (fd[0] < 0 || fd[1] < 0) {
		printf("Unable to read or dump\n");
		return -1;
	}
	printf("Create source code to dump file...\n");
	write_string(fd[1], 
    "#include <unistd.h>\n"
    "#include <fcntl.h>\n"
    "\n"
	"struct ");
	write_string(fd[1], " {\n"
	"    const char *buf = \"");
	ssize_t size = 0, count;
	while ((count = read(fd[0], &c, 1)) > 0) {
			sprintf(buf, "\\x%2x", c);
			if (buf[2] == ' ') buf[2] = '0';
	        write(fd[1], buf, strlen(buf));
		    size += count;
	}
	write_string(fd[1], "\";\n"
	"    const unsigned long size = ");
	sprintf(buf, "%lu", size);
	write_string(fd[1], buf);
	write_string(fd[1], ";\n"
	"    int dump(const char *path) {\n"
	"        int fd = open(path, O_WRONLY | O_CREAT, 0644);\n"
	"        if (fd < 0) return -1;\n"
	"        int ret = write(fd, buf, size);\n"
	"        close(fd);\n"
	"        return ret;\n"
	"    }\n"
	"}");
	write_string(fd[1], name);
	write_string(fd[1], ";\n");
	printf("Well done!\n");
	return 0;
}
