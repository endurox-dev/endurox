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

char     tchar1     T_CHAR_FLD          1       -       -       '\0'
char     tchar2     T_CHAR_2_FLD        5       C       -       'A'
char     tchar3     T_CHAR_3_FLD        2       CF      -       -

float    tfloat1    T_FLOAT_FLD         1       -       -       1.1
float    tfloat2    T_FLOAT_2_FLD       2       -       -       -

double   tdouble1   T_DOUBLE_FLD        1       -       -       0.0
double   tdouble2   T_DOUBLE_2_FLD      2       -       -       0.0

string   tstring1   T_STRING_FLD        1       P       15      'HELLO WORLDB'
string   tstring2   T_STRING_2_FLD      3       CL      20      'TESTEST'
string   tstring3   T_STRING_3_FLD      4       C       20      'TESTEST'

carray   tcarray1   T_CARRAY_FLD        1       -      10      '-'
carray   tcarray2   T_CARRAY_2_FLD      10       CL      10     '\0\\\nABC\t\f\'\vHELLO'

END


VIEW MYVIEW2
#type    cname      fbname              count   flag    size    null
short    tshort1    -                   1       -       -       2000
long     tlong1     -                   1       -       -       0
char     tchar1     -                   1       -       -       '\0'
float    tfloat1    -                   1       -       -       1.1
double   tdouble1   -                   1       -       -       0.0
string   tstring1   -                   1       P       15      'HELLO WORLDB'
carray   tcarray1   -                   1       -      10      '-'
END
