class BrokenMissingParen {
    ~BrokenMissingParen {
    }
};

class BrokenEmptyParen {
    ~BrokenEmptyParen() ;
};

class BrokenNoName {
    ~ {
    }
};