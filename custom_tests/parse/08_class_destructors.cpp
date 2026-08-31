class Buffer {
    int capacity;
    int used;

    Buffer() {
        capacity = 16;
        used = 0;
    }

    Buffer(int cap) {
        capacity = cap;
        used = 0;
    }

    ~Buffer() {
        capacity = 0;
        used = 0;
    }

    void fill(int amount) {
        used = used + amount;
    }

    int free_space() {
        return capacity - used;
    }
};

class TrackedBuffer : Buffer {
    int serial;

    TrackedBuffer() {
        serial = 0;
    }

    ~TrackedBuffer() {
        serial = -1;
    }
};

int main() {
    Buffer b;
    b.fill(4);
    b.fill(2);
    int remaining = b.free_space();
    TrackedBuffer t;
    return remaining + t.free_space();
}