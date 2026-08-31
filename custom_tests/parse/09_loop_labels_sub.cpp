

int rand() {
    static int x = 5;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}


struct Stream {
    int size = 10;
    bool hasItems() { return size--; }
    int getNext() { return rand(); }
};

int main(int argc, char** candidates) {

    Stream stream = {};
    bool found = false;

    while(stream.hasItems(); LoopLabel1){
        int val = stream.getNext();
        for(int i = 0; i < argc; i++){
            if(val == stoi(candidates[i])){
                found = true;
                break LoopLabel1;
            }
        }
    }
    if(found){
        // An item in the stream is in the array `candidates`
    }
}
