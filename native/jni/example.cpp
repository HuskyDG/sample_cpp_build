#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <libgen.h>


const char *HEADER = "ENCRYPTEDFILE_DEFINED_USER";

#define ADD_HEADER 1

int encrypt_file(const char *src, const char *dest, const char *pw) {
	if (strlen(pw) > strlen(HEADER))
	    return -1;
	int fd_in = open(src, O_RDONLY);
	int fd_out = open(dest, O_CREAT | O_RDWR, 0644);
	char c;
	int n = strlen(pw) + 1;
	if (fd_in < 0 || fd_out < 0) {
		close(fd_in);
		close(fd_out);
		return -1;
	}
	// WRITE HEADER
#if ADD_HEADER
	for (int i = 0; (c = HEADER[i]) != 0; i++) {
    	int enc = c + pw[i % n];
    	int off = (enc > 255)? enc - 255 : 0;
    	enc %= 256;
    	write(fd_out, &off, 1);
    	write(fd_out, &enc, 1);
	}
#endif
	// ENCRYPT BODY
    for (int i = 0; read(fd_in, &c, 1) > 0; i++) {
    	int enc = c + pw[i % n];
    	int off = (enc > 255)? enc - 255 : 0;
    	enc %= 256;
    	write(fd_out, &off, 1);
    	write(fd_out, &enc, 1);
    }
   close(fd_in);
   close(fd_out);
   return 0;
}

int decrypt_file(const char *src, const char *dest, const char *pw) {
	if (strlen(pw) > strlen(HEADER))
	    return -1;
	int fd_in = open(src, O_RDONLY);
	int fd_out = open(dest, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	int off, enc;
	int n = strlen(pw) + 1;
	if (fd_in < 0 || fd_out < 0) {
		close(fd_in);
		close(fd_out);
		return -1;
	}
#if ADD_HEADER
	char HEADER_dec[strlen(HEADER) + 1];
	for (int i = 0; i < strlen(HEADER); i++) {
	   if (read(fd_in, &off, 1) <= 0 || read(fd_in, &enc, 1) <= 0)
	       break;
    	enc += off;
    	enc -= pw[i % n];
    	HEADER_dec[i] = enc;
	}
	HEADER_dec[strlen(HEADER)] = 0;
	if (strcmp(HEADER_dec, HEADER) != 0) {
	   close(fd_in);
	   close(fd_out);
	   return -1;
	}
#endif
    for (int i = 0; read(fd_in, &off, 1) > 0 && read(fd_in, &enc, 1) > 0; i++) {
    	enc += off;
    	enc -= pw[i % n];
    	write(fd_out, &enc, 1);
    }
   close(fd_in);
   close(fd_out);
   return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s [e|d] src dest password\n", basename(argv[0]));
        return -1;
    }
    char *mode = argv[1];
    char *src = argv[2];
    char *dest = argv[3];
    char *pw = argv[4]; 
    if (mode[0] == 'e') {
        // Encrypt the file
        int res = encrypt_file(src, dest, pw);
        if (res == 0) {
            printf("File encrypted successfully.\n");
            return 0;
        } else {
            printf("File encryption failed.\n");
        }
    } else if (mode[0] == 'd') {
        // Decrypt the file
        int res = decrypt_file(src, dest, pw);
        if (res == 0) {
            printf("File decrypted successfully.\n");
            return 0;
        } else {
            printf("File decryption failed.\n");
        }
    } else {
        printf("Invalid mode. Use e for encryption or d for decryption.\n");
    }
    return 1;
}
