/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
  return ~(~(~x & y) & ~(x & ~y));
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  return 1<<31;

}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x) {
  return (!~((x+1)^x)) & (!(!(x+1)));
  // Tmax = 0111, Tmax+1 = 1000, therefore (Tmax+1)^(Tmax) = 1111
  // second half is to exlude the case 1111
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
  int mask = 0xAA | 0xAA << 8 | 0xAA << 16 | 0xAA << 24;
  return !((x & mask) ^ mask);
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  return ~x+1;
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {
  // return ((!(((0x39+~x+2)>>31)^0))^(!(((0x30+~x)>>31)^0))); //will have the overflow
  return (!((x>>4)^(0x30>>4)))&(!((0x39+~x+1)>>31)^0); 
  // (check the first 28 digits)&(if the same, then check last 4 digtis)
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
  int torf = !!x; // can only be 0000 or 0001
  int all_0_1 = ~torf+1; // 0000 or 1111
  return (~all_0_1&z)|(all_0_1&y);
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  int x_sign = x>>31; //positive --> 0000
  int y_sign = y>>31; //negative --> 1111
  int same_sign = y_sign^x_sign; // same-->0000; not same-->1111
  int y_x = (y+~x+1)>>31; // y>=x --> 0000; y<x -->1111

  return  ((same_sign^0)&(!(y_sign^0)))|(!(same_sign^0)&(!(y_x^0)));
  //(not same sign & y is positive)| (the same sign and y-x is larger and equal 0)
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
  x = x & (~x + 1);
  x = ~x + 1; 
  x = ~x >> 31;
  return x & 0x01;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
  int b16, b8, b4, b2, b1, b0;
  int sign = x >> 31;
  x = (sign & ~x) | (~sign & x);
  // the trick to deal with negative and positive 
  // for negative, find out where is the first place occuring 0
  // for positive, find out where is the first place occuring 1

  b16 = !!(x >> 16) << 4; // for left most significant 16 bits whether it contains 1
  // if contains 1, then record the b16 and right shift b16
  // if not contain, then try smaller shift, removing less number of the right most bits.
  x = x >> b16;
  b8 = !!(x >> 8) << 3; 
  //for left most significant 8 bits (doesn't count the left 16 bits because it's tested by the previous two lines) whether it contains 1, 
  // if contains 1, then record the b8 and right shift b8
  x = x >> b8;
  b4 = !!(x >> 4) << 2;
  x = x >> b4;
  b2 = !!(x >> 2) << 1;
  x = x >> b2;
  b1 = !!(x >> 1);
  x =x >> b1;
  b0 = x;
  return b16 + b8 + b4 + b2 + b1 + b0 + 1; 

}
//float
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf) {
  int sign = uf & 0x80000000;
  int exponent = uf & 0x7f800000;
  int frac = uf & 0x7fffff;

  if(exponent == 0x7f800000) // not a number or infinite
    return uf;

  if(exponent == 0){ //denorm
    frac = frac << 1;
    return frac | sign;
  }

  // norm

  // exponent = exponent<<1;
  // frac = frac << 1;
  // not true, because it's 2**E * 1.frac where E = sum of 2**i
  // therefore if we want to multiple 2, just add one in exp

  exponent += 0x800000;
  return exponent | sign | frac;
}
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf) {

  int sign = uf >> 31;
  int exponent = (0x7f800000 & uf)>>23;
  int frac = 0x7fffff & uf;
  int E = exponent - 127; // exp-bias

  if (E>30){
    return 0x80000000;
  }
  // because for two-components 32bits, Tmax = 2**31-1
  // transform it to IEEE float, it becomes 2**E (where E=30) times 1.1111...1 --> 2, or it is 011...110000000 represented using int
  // when jump to next bit, 011...111000000 is overflow, it is also a case for larger interval when larger E.

  // 2**nonpositive times 1.frac, round to zero or one or 2
  if(E < 0){
    return 0;
  }

  frac = frac | 0x800000; // 1.frac
  
  if(E > 23)
    frac = frac << (E - 23); //left shift --> multiply 2**(E-23)
  else
    frac = frac >> (23 - E); //right shift --> divide by 2**(23-E)

  if(!sign)
  // if sign is 0, or representing positive number
    return frac;
  else
    return ~frac + 1;
  // no round to even?

}
/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
unsigned floatPower2(int x) {
  // x is the same as E in the IEEE float formula
  // x+bias is the same as exp, bias = 127
  int bitShift;

  // x+bias>2**8-1
  if(x >= 128)
    return 0x7f800000; //positive infinity

  // x+bias>0
  if(x > -127){
    int exponent = x + 127;
    return exponent << 23;
  }

  // x+bias <= 0
  bitShift = x + 149; 
  // denorm
  // smallest E is 1-127, smallest interval is 2*(-126-23)
  if(bitShift < 0)
    return 0;
  else
    return 1 << bitShift; 
}
