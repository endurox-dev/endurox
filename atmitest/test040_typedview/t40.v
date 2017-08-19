VIEW MYVIEW1
#type    cname      fbname              count   flag    size    null

short    tshort1    T_SHORT_FLD         1       -       -       2000
short    tshort2    T_SHORT_2_FLD       2       C       -       2001
short    tshort3    T_SHORT_3_FLD       3       N       -       -
short    tshort4    -                   1       N       -       NONE

long     tlong1     T_LONG_FLD          1       S       -       0
int      tint2      T_LONG_2_FLD        2       L       -       0
int      tint3      -                   1       -       -       -1
int      tint4      -                   2       -       -       -1

char     tchar1     T_CHAR_FLD          1       -       -       '\n'
char     tchar2     T_CHAR_2_FLD        5       C       -       'A'
char     tchar3     T_CHAR_3_FLD        2       CF      -       -

float    tfloat1    T_FLOAT_FLD         4       -       -       1.1
float    tfloat2    T_FLOAT_2_FLD       2       -       -       -
float    tfloat3    -                   1       -       -       9999.99

double   tdouble1   T_DOUBLE_FLD        2       -       -       55555.99
double   tdouble2   T_DOUBLE_2_FLD      1       -       -       -999.123

string   tstring0   -                   3       -       18      '\n\t\f\\\'\"\vHELLOWORLD'
string   tstring1   T_STRING_FLD        3       P       20      'HELLO WORLDB'
string   tstring2   T_STRING_2_FLD      3       CL      20      -
string   tstring3   T_STRING_3_FLD      4       C       20      'TESTEST'
string   tstring4   -                   1       P       15      'HELLO TEST'

carray   tcarray1   T_CARRAY_FLD        1       -       30      '\0\n\t\f\\\'\"\vHELLOWORLD'
carray   tcarray2   T_CARRAY_2_FLD      1       P       25      '\0\n\t\f\\\'\"\vHELLOWORL\n'
carray   tcarray3   T_CARRAY_3_FLD      10       CLP    30      '\0\\\nABC\t\f\'\vHELLO'
carray   tcarray4   -                   1       -       5       'ABC'
carray   tcarray5   -                   1       -       5       -

END


VIEW MYVIEW2
#type    cname      fbname              count   flag    size    null
short    tshort1    -                   1       -       -       2000
long     tlong1     -                   1       -       -       5
char     tchar1     -                   1       -       -       '\0'
float    tfloat1    -                   1       -       -       1.1
double   tdouble1   -                   1       -       -       0.0
string   tstring1   -                   1       P       15      'HELLO WORLDB'
carray   tcarray1   -                   1       -      10      '-'
END

