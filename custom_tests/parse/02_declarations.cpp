static int global_variable = 100;
const int max_value = 1000;

typedef int Number;

int main() {
    int a, b, c;
    a = 1;
    b = 2;
    c = a + b;

    int x = 5, y = 6;
    int z = x * y;

    int arr[3] = {1, 2, 3};
    int multi[2][3];
    multi[0][0] = 1;
    multi[1][2] = 2;

    int* ptr = &x;
    *ptr = 10;

    char name[8] = "hello";
    char letter = 'A';
    float ratio = 0.75;
    double precise = 3.14159;

    Number alias = 42;
    const int fixed = alias / 2;

    return a + b + c + z + arr[0] + multi[1][2] + *ptr + name[0] + letter + ratio + precise + alias + fixed;
}