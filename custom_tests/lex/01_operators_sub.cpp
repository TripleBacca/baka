
struct Inner {
  int value;
};

struct Outer {
  Inner *inner_ptr;
  Inner inner_val;
};

int variadic_sum(int count, ...) { return count; }

namespace MathOps {
int global_multiplier = 2;
}

int main() {
  int a = 10;
  int b = 20;
  int c = 0;

  int bitwise_not = ~a;

  int neg = -a;

  ++a;
  a++;

  --b;
  b--;

  bool flag = !false;

  int positive = +a;

  c = a * b;

  c = b / a;

  c = b % a;

  c = a & b;

  c = a | b;

  c = a ^ b;

  c = a << 2;

  c = b >> 1;

  bool cond_and = (a < b) && (b > 0);

  bool cond_or = (a == 10) || (b != 20);

  bool is_equal = (a == b);

  bool is_not_equal = (a != b);

  bool is_less = (a < b);

  bool is_greater = (a > b);

  bool is_less_or_equal = (a <= b);

  bool is_greater_or_equal = (a >= b);

  c = a;

  c += 5;

  c -= 3;

  c *= 2;

  c /= 2;

  c %= 4;

  c &= 0xFF;

  c |= 0x01;

  c ^= 0x0F;

  c <<= 1;

  c >>= 1;

  int scoped_val = MathOps::global_multiplier;

  int ternary_res = (a > b) ? a : b;

  int x = 1, y = 2, z = 3;

  Outer outer_obj;
  outer_obj.inner_val.value = 42;

  outer_obj.inner_ptr = &outer_obj.inner_val;
  int accessed_val = outer_obj.inner_ptr->value;

  return 0;
}
