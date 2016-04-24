#include <ext/string_interpolation.hpp>

const bool ext::interpolate_internal::key_allowed_ascii_chars[128] =
{
/*   0 */ false, /*   1 */ false, /*   2 */ false, /*   3 */ false, /*   4 */ false, /*   5 */ false, /*   6 */ false, /*   7 */ false,
/*   8 */ false, /*   9 */ false, /*  10 */ false, /*  11 */ false, /*  12 */ false, /*  13 */ false, /*  14 */ false, /*  15 */ false,
/*  16 */ false, /*  17 */ false, /*  18 */ false, /*  19 */ false, /*  20 */ false, /*  21 */ false, /*  22 */ false, /*  23 */ false,
/*  24 */ false, /*  25 */ false, /*  26 */ false, /*  27 */ false, /*  28 */ false, /*  29 */ false, /*  30 */ false, /*  31 */ false,
/*  32 */ false, /*  33 */ false, /*  34 */ false, /*  35 */ false, /* '$' */ false, /*  37 */ false, /*  38 */ false, /*  39 */ false,
/*  40 */ false, /*  41 */ false, /*  42 */ false, /*  43 */ false, /*  44 */ false, /*  45 */ false, /*  46 */ false, /*  47 */ false,
/* '0' */  true, /* '1' */  true, /* '2' */  true, /* '3' */  true, /* '4' */  true, /* '5' */  true, /* '6' */  true, /* '7' */  true,
/* '8' */  true, /* '9' */  true, /*  58 */ false, /*  59 */ false, /*  60 */ false, /*  61 */ false, /*  62 */ false, /*  63 */ false,
/*  64 */ false, /* 'A' */  true, /* 'B' */  true, /* 'C' */  true, /* 'D' */  true, /* 'E' */  true, /* 'F' */  true, /* 'G' */  true,
/* 'H' */  true, /* 'I' */  true, /* 'J' */  true, /* 'K' */  true, /* 'L' */  true, /* 'M' */  true, /* 'N' */  true, /* 'O' */  true,
/* 'P' */  true, /* 'Q' */  true, /* 'R' */  true, /* 'S' */  true, /* 'T' */  true, /* 'U' */  true, /* 'V' */  true, /* 'W' */  true,
/* 'X' */  true, /* 'Y' */  true, /* 'Z' */  true, /*  91 */ false, /*  92 */ false, /*  93 */ false, /*  94 */ false, /* '_' */  true,
/*  96 */ false, /* 'a' */  true, /* 'b' */  true, /* 'c' */  true, /* 'd' */  true, /* 'e' */  true, /* 'f' */  true, /* 'g' */  true,
/* 'h' */  true, /* 'i' */  true, /* 'j' */  true, /* 'k' */  true, /* 'l' */  true, /* 'm' */  true, /* 'n' */  true, /* 'o' */  true,
/* 'p' */  true, /* 'q' */  true, /* 'r' */  true, /* 's' */  true, /* 't' */  true, /* 'u' */  true, /* 'v' */  true, /* 'w' */  true,
/* 'x' */  true, /* 'y' */  true, /* 'z' */  true, /* 123 */ false, /* 124 */ false, /* 125 */ false, /* 126 */ false, /* 127 */ false,
};
