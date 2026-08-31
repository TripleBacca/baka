

namespace TestNamespace {

        typedef unsigned long long uint64_t;

        using Byte = unsigned char;

        enum Color {
        RED,
        GREEN,
        BLUE
    };

        class BaseClass {
    public:
                static const int default_capacity = 16;

        BaseClass() {}

                BaseClass* get_self() {
            return this;
        }

                void do_nothing() {}
    };

        struct Point {
                int x;
        float y;
        double z;

                short s;
        long l;
        signed int si;
        unsigned int ui;
        char c;
        bool flag;
    };

        union ValueHolder {
        int int_val;
        double dbl_val;
        char char_val;
    };
}

void test_control_flow(int x) {
        if (x > 0) {
        x = x - 1;
    } else {
        x = x + 1;
    }

        while (x < 100) {
        x = x + 10;
        if (x == 50) {
            continue;
        }
        if (x == 80) {
            break;
        }
    }

        do {
        x = x + 1;
    } while (x < 90);

        for (int i = 0; i < 10; i = i + 1) {
        x = x + i;
    }

        switch (x) {
        case 1:
            break;
        case 2:
            break;
        default:             break;
    }

        goto end_label;

end_label:
        return;
}

int test_memory_and_operators() {
        bool is_valid = true;
    bool is_done = false;

        int size_int = sizeof(int);
    int size_struct = sizeof(TestNamespace::Point);

        int* ptr = new int;
    *ptr = 42;
    delete ptr;

    int* arr = new int[10];
    delete[] arr;

        return 0;
}
