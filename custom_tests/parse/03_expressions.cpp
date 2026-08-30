int main() {
    int a = 42;
    int b = 7;

    int arith = a + b * 2 - (a / b);
    int rem = a % b;
    int bit = (a & b) | (a ^ b);
    int shift = (a << 2) >> 1;
    int unary = ~a;
    int neg = -a;
    int lnot = !a;
    int cmp1 = (a > b);
    int cmp2 = (a == b);
    int logic = (a > 0) && (b < 10);
    int lor = (a > 100) || (b < 10);
    int ternary = (a > b) ? a : b;

    int inc = ++a;
    int dec = a--;
    int postinc = a++;

    a += 2;
    a -= 1;
    a *= 3;
    a /= 2;
    a %= 5;
    a <<= 1;
    a >>= 1;
    a &= 15;
    a |= 8;
    a ^= 3;

    int commas = (a, b);
    int casted = (int)(double)a;
    int sized1 = sizeof(int);
    int sized2 = sizeof(a);

    return arith + rem + bit + shift + unary + neg + lnot + cmp1 + cmp2 + logic + lor + ternary + inc + dec + postinc + commas + casted + sized1 + sized2;
}