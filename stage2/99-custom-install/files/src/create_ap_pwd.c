#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define STRING_LENGTH 16

int main()
{
    srand(time(NULL));//seed

    const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    
    bool      hasLowerAlphabet, hasUpperAlphabet, hasNumeric;
    const int charset_len = sizeof(charset) - 1;
    char      random_string[STRING_LENGTH + 1];
    int       i;

    while (true) {
	hasLowerAlphabet = hasUpperAlphabet = hasNumeric = false;
	for (i = 0; i < STRING_LENGTH; i++) {
	    random_string[i] = charset[rand() % charset_len];
	    if ('a' <= random_string[i] && random_string[i] <= 'z')
	        hasLowerAlphabet = true;
	    else if ('A' <= random_string[i] && random_string[i] <= 'Z')
	         hasUpperAlphabet = true;
	    else hasNumeric = true;
	}
	if (hasLowerAlphabet && hasUpperAlphabet && hasNumeric) break;
    }

    random_string[STRING_LENGTH] = '\0';

    printf("random ap password ： %s\n", random_string);

    return 0;
}
