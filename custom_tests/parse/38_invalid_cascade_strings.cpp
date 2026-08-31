int str_len(char s[10]) {
    int len = 0;
    while (s[len] != 0; counting) {
        len = len + 1;
    }
    return len;
}

int is_digit(char c) {
    if (c >= '0' && c <= '9') {
        return 1;
    }
    return 0;
}

char to_upper(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 32
    }
    return c;
}

int find_char(char s[10], char target) {
    int i = 0;
    for (i = 0; s[i] != 0; i = i + 1; search) {
        if (s[i] == target {
            return i;
        }
    }
    return -1;
}

int count_words(char s[10]) {
    int words = 0;
    int in_word = 0;
    int i = 0;
    while (s[i] != 0) {
        if (is_digit(s[i]) == 0 {
            in_word = 1;
        } else {
            in_word = 0;
        }
        i = i + 1
    }
    return words;
}

int sum_digits(char s[10]) {
    int sum = 0;
    int i = 0;
    do {
        if (is_digit(s[i])) {
            sum = sum + s[i] - 48;
        }
        i = i + 1
    } while (s[i] != 0; tally);
    return sum;
}

int main() {
    char greeting[10];
    greeting[0] = 'h';
    greeting[1] = 'i';
    int length = str_len(greeting);
    int digits = sum_digits(greeting)
    int hits = find_char(greeting, 'i');
    char upper = to_upper('a');
    int words = count_words(greeting);
    return length + digits + hits + upper + words;
}