typedef int Sid;

enum Weekday {
    MONDAY,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY,
    SUNDAY
};

union Value {
    int int_value;
    float float_value;
    char char_value;
};

union Tagged {
    int tag;
    Value payload;
};

int main() {
    Sid id = 99;
    Sid next = id + 1;

    Weekday day = FRIDAY;

    Value v;
    v.int_value = 42;
    v.float_value = 1.5;

    Tagged t;
    t.tag = 1;
    t.payload.int_value = 7;

    return id + next + day + v.int_value + t.tag + t.payload.int_value;
}