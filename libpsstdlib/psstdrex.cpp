/* see copyright notice in pscript.h */
#include <pscript.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <psstdstring.h>

#ifdef _DEBUG
#include <stdio.h>

static const PSChar *g_nnames[] =
{
    _SC("NONE"),_SC("OP_GREEDY"),   _SC("OP_OR"),
    _SC("OP_EXPR"),_SC("OP_NOCAPEXPR"),_SC("OP_DOT"),   _SC("OP_CLASS"),
    _SC("OP_CCLASS"),_SC("OP_NCLASS"),_SC("OP_RANGE"),_SC("OP_CHAR"),
    _SC("OP_EOL"),_SC("OP_BOL"),_SC("OP_WB"),_SC("OP_MB")
};

#endif

#define OP_GREEDY       (MAX_CHAR+1) // * + ? {n}
#define OP_OR           (MAX_CHAR+2)
#define OP_EXPR         (MAX_CHAR+3) //parentesis ()
#define OP_NOCAPEXPR    (MAX_CHAR+4) //parentesis (?:)
#define OP_DOT          (MAX_CHAR+5)
#define OP_CLASS        (MAX_CHAR+6)
#define OP_CCLASS       (MAX_CHAR+7)
#define OP_NCLASS       (MAX_CHAR+8) //negates class the [^
#define OP_RANGE        (MAX_CHAR+9)
#define OP_CHAR         (MAX_CHAR+10)
#define OP_EOL          (MAX_CHAR+11)
#define OP_BOL          (MAX_CHAR+12)
#define OP_WB           (MAX_CHAR+13)
#define OP_MB           (MAX_CHAR+14) //match balanced

#define PSREX_SYMBOL_ANY_CHAR ('.')
#define PSREX_SYMBOL_GREEDY_ONE_OR_MORE ('+')
#define PSREX_SYMBOL_GREEDY_ZERO_OR_MORE ('*')
#define PSREX_SYMBOL_GREEDY_ZERO_OR_ONE ('?')
#define PSREX_SYMBOL_BRANCH ('|')
#define PSREX_SYMBOL_END_OF_STRING ('$')
#define PSREX_SYMBOL_BEGINNING_OF_STRING ('^')
#define PSREX_SYMBOL_ESCAPE_CHAR ('\\')


typedef int PSRexNodeType;

typedef struct tagPSRexNode{
    PSRexNodeType type;
    PSInteger left;
    PSInteger right;
    PSInteger next;
}PSRexNode;

struct PSRex{
    const PSChar *_eol;
    const PSChar *_bol;
    const PSChar *_p;
    PSInteger _first;
    PSInteger _op;
    PSRexNode *_nodes;
    PSInteger _nallocated;
    PSInteger _nsize;
    PSInteger _nsubexpr;
    PSRexMatch *_matches;
    PSInteger _currsubexp;
    void *_jmpbuf;
    const PSChar **_error;
};

static PSInteger psstd_rex_list(PSRex *exp);

static PSInteger psstd_rex_newnode(PSRex *exp, PSRexNodeType type)
{
    PSRexNode n;
    n.type = type;
    n.next = n.right = n.left = -1;
    if(type == OP_EXPR)
        n.right = exp->_nsubexpr++;
    if(exp->_nallocated < (exp->_nsize + 1)) {
        PSInteger oldsize = exp->_nallocated;
        exp->_nallocated *= 2;
        exp->_nodes = (PSRexNode *)ps_realloc(exp->_nodes, oldsize * sizeof(PSRexNode) ,exp->_nallocated * sizeof(PSRexNode));
    }
    exp->_nodes[exp->_nsize++] = n;
    PSInteger newid = exp->_nsize - 1;
    return (PSInteger)newid;
}

static void psstd_rex_error(PSRex *exp,const PSChar *error)
{
    if(exp->_error) *exp->_error = error;
    longjmp(*((jmp_buf*)exp->_jmpbuf),-1);
}

static void psstd_rex_expect(PSRex *exp, PSInteger n){
    if((*exp->_p) != n)
        psstd_rex_error(exp, _SC("expected paren"));
    exp->_p++;
}

static PSChar psstd_rex_escapechar(PSRex *exp)
{
    if(*exp->_p == PSREX_SYMBOL_ESCAPE_CHAR){
        exp->_p++;
        switch(*exp->_p) {
        case 'v': exp->_p++; return '\v';
        case 'n': exp->_p++; return '\n';
        case 't': exp->_p++; return '\t';
        case 'r': exp->_p++; return '\r';
        case 'f': exp->_p++; return '\f';
        default: return (*exp->_p++);
        }
    } else if(!scisprint(*exp->_p)) psstd_rex_error(exp,_SC("letter expected"));
    return (*exp->_p++);
}

static PSInteger psstd_rex_charclass(PSRex *exp,PSInteger classid)
{
    PSInteger n = psstd_rex_newnode(exp,OP_CCLASS);
    exp->_nodes[n].left = classid;
    return n;
}

static PSInteger psstd_rex_charnode(PSRex *exp,PSBool isclass)
{
    PSChar t;
    if(*exp->_p == PSREX_SYMBOL_ESCAPE_CHAR) {
        exp->_p++;
        switch(*exp->_p) {
            case 'n': exp->_p++; return psstd_rex_newnode(exp,'\n');
            case 't': exp->_p++; return psstd_rex_newnode(exp,'\t');
            case 'r': exp->_p++; return psstd_rex_newnode(exp,'\r');
            case 'f': exp->_p++; return psstd_rex_newnode(exp,'\f');
            case 'v': exp->_p++; return psstd_rex_newnode(exp,'\v');
            case 'a': case 'A': case 'w': case 'W': case 's': case 'S':
            case 'd': case 'D': case 'x': case 'X': case 'c': case 'C':
            case 'p': case 'P': case 'l': case 'u':
                {
                t = *exp->_p; exp->_p++;
                return psstd_rex_charclass(exp,t);
                }
            case 'm':
                {
                     PSChar cb, ce; //cb = character begin match ce = character end match
                     cb = *++exp->_p; //skip 'm'
                     ce = *++exp->_p;
                     exp->_p++; //points to the next char to be parsed
                     if ((!cb) || (!ce)) psstd_rex_error(exp,_SC("balanced chars expected"));
                     if ( cb == ce ) psstd_rex_error(exp,_SC("open/close char can't be the same"));
                     PSInteger node =  psstd_rex_newnode(exp,OP_MB);
                     exp->_nodes[node].left = cb;
                     exp->_nodes[node].right = ce;
                     return node;
                }
            case 'b':
            case 'B':
                if(!isclass) {
                    PSInteger node = psstd_rex_newnode(exp,OP_WB);
                    exp->_nodes[node].left = *exp->_p;
                    exp->_p++;
                    return node;
                } //else default
            default:
                t = *exp->_p; exp->_p++;
                return psstd_rex_newnode(exp,t);
        }
    }
    else if(!scisprint(*exp->_p)) {

        psstd_rex_error(exp,_SC("letter expected"));
    }
    t = *exp->_p; exp->_p++;
    return psstd_rex_newnode(exp,t);
}
static PSInteger psstd_rex_class(PSRex *exp)
{
    PSInteger ret = -1;
    PSInteger first = -1,chain;
    if(*exp->_p == PSREX_SYMBOL_BEGINNING_OF_STRING){
        ret = psstd_rex_newnode(exp,OP_NCLASS);
        exp->_p++;
    }else ret = psstd_rex_newnode(exp,OP_CLASS);

    if(*exp->_p == ']') psstd_rex_error(exp,_SC("empty class"));
    chain = ret;
    while(*exp->_p != ']' && exp->_p != exp->_eol) {
        if(*exp->_p == '-' && first != -1){
            PSInteger r;
            if(*exp->_p++ == ']') psstd_rex_error(exp,_SC("unfinished range"));
            r = psstd_rex_newnode(exp,OP_RANGE);
            if(exp->_nodes[first].type>*exp->_p) psstd_rex_error(exp,_SC("invalid range"));
            if(exp->_nodes[first].type == OP_CCLASS) psstd_rex_error(exp,_SC("cannot use character classes in ranges"));
            exp->_nodes[r].left = exp->_nodes[first].type;
            PSInteger t = psstd_rex_escapechar(exp);
            exp->_nodes[r].right = t;
            exp->_nodes[chain].next = r;
            chain = r;
            first = -1;
        }
        else{
            if(first!=-1){
                PSInteger c = first;
                exp->_nodes[chain].next = c;
                chain = c;
                first = psstd_rex_charnode(exp,PSTrue);
            }
            else{
                first = psstd_rex_charnode(exp,PSTrue);
            }
        }
    }
    if(first!=-1){
        PSInteger c = first;
        exp->_nodes[chain].next = c;
    }
    /* hack? */
    exp->_nodes[ret].left = exp->_nodes[ret].next;
    exp->_nodes[ret].next = -1;
    return ret;
}

static PSInteger psstd_rex_parsenumber(PSRex *exp)
{
    PSInteger ret = *exp->_p-'0';
    PSInteger positions = 10;
    exp->_p++;
    while(isdigit(*exp->_p)) {
        ret = ret*10+(*exp->_p++-'0');
        if(positions==1000000000) psstd_rex_error(exp,_SC("overflow in numeric constant"));
        positions *= 10;
    };
    return ret;
}

static PSInteger psstd_rex_element(PSRex *exp)
{
    PSInteger ret = -1;
    switch(*exp->_p)
    {
    case '(': {
        PSInteger expr;
        exp->_p++;


        if(*exp->_p =='?') {
            exp->_p++;
            psstd_rex_expect(exp,':');
            expr = psstd_rex_newnode(exp,OP_NOCAPEXPR);
        }
        else
            expr = psstd_rex_newnode(exp,OP_EXPR);
        PSInteger newn = psstd_rex_list(exp);
        exp->_nodes[expr].left = newn;
        ret = expr;
        psstd_rex_expect(exp,')');
              }
              break;
    case '[':
        exp->_p++;
        ret = psstd_rex_class(exp);
        psstd_rex_expect(exp,']');
        break;
    case PSREX_SYMBOL_END_OF_STRING: exp->_p++; ret = psstd_rex_newnode(exp,OP_EOL);break;
    case PSREX_SYMBOL_ANY_CHAR: exp->_p++; ret = psstd_rex_newnode(exp,OP_DOT);break;
    default:
        ret = psstd_rex_charnode(exp,PSFalse);
        break;
    }


    PSBool isgreedy = PSFalse;
    unsigned short p0 = 0, p1 = 0;
    switch(*exp->_p){
        case PSREX_SYMBOL_GREEDY_ZERO_OR_MORE: p0 = 0; p1 = 0xFFFF; exp->_p++; isgreedy = PSTrue; break;
        case PSREX_SYMBOL_GREEDY_ONE_OR_MORE: p0 = 1; p1 = 0xFFFF; exp->_p++; isgreedy = PSTrue; break;
        case PSREX_SYMBOL_GREEDY_ZERO_OR_ONE: p0 = 0; p1 = 1; exp->_p++; isgreedy = PSTrue; break;
        case '{':
            exp->_p++;
            if(!isdigit(*exp->_p)) psstd_rex_error(exp,_SC("number expected"));
            p0 = (unsigned short)psstd_rex_parsenumber(exp);
            /*******************************/
            switch(*exp->_p) {
        case '}':
            p1 = p0; exp->_p++;
            break;
        case ',':
            exp->_p++;
            p1 = 0xFFFF;
            if(isdigit(*exp->_p)){
                p1 = (unsigned short)psstd_rex_parsenumber(exp);
            }
            psstd_rex_expect(exp,'}');
            break;
        default:
            psstd_rex_error(exp,_SC(", or } expected"));
            }
            /*******************************/
            isgreedy = PSTrue;
            break;

    }
    if(isgreedy) {
        PSInteger nnode = psstd_rex_newnode(exp,OP_GREEDY);
        exp->_nodes[nnode].left = ret;
        exp->_nodes[nnode].right = ((p0)<<16)|p1;
        ret = nnode;
    }

    if((*exp->_p != PSREX_SYMBOL_BRANCH) && (*exp->_p != ')') && (*exp->_p != PSREX_SYMBOL_GREEDY_ZERO_OR_MORE) && (*exp->_p != PSREX_SYMBOL_GREEDY_ONE_OR_MORE) && (*exp->_p != '\0')) {
        PSInteger nnode = psstd_rex_element(exp);
        exp->_nodes[ret].next = nnode;
    }

    return ret;
}

static PSInteger psstd_rex_list(PSRex *exp)
{
    PSInteger ret=-1,e;
    if(*exp->_p == PSREX_SYMBOL_BEGINNING_OF_STRING) {
        exp->_p++;
        ret = psstd_rex_newnode(exp,OP_BOL);
    }
    e = psstd_rex_element(exp);
    if(ret != -1) {
        exp->_nodes[ret].next = e;
    }
    else ret = e;

    if(*exp->_p == PSREX_SYMBOL_BRANCH) {
        PSInteger temp,tright;
        exp->_p++;
        temp = psstd_rex_newnode(exp,OP_OR);
        exp->_nodes[temp].left = ret;
        tright = psstd_rex_list(exp);
        exp->_nodes[temp].right = tright;
        ret = temp;
    }
    return ret;
}

static PSBool psstd_rex_matchcclass(PSInteger cclass,PSChar c)
{
    switch(cclass) {
    case 'a': return isalpha(c)?PSTrue:PSFalse;
    case 'A': return !isalpha(c)?PSTrue:PSFalse;
    case 'w': return (isalnum(c) || c == '_')?PSTrue:PSFalse;
    case 'W': return (!isalnum(c) && c != '_')?PSTrue:PSFalse;
    case 's': return isspace(c)?PSTrue:PSFalse;
    case 'S': return !isspace(c)?PSTrue:PSFalse;
    case 'd': return isdigit(c)?PSTrue:PSFalse;
    case 'D': return !isdigit(c)?PSTrue:PSFalse;
    case 'x': return isxdigit(c)?PSTrue:PSFalse;
    case 'X': return !isxdigit(c)?PSTrue:PSFalse;
    case 'c': return iscntrl(c)?PSTrue:PSFalse;
    case 'C': return !iscntrl(c)?PSTrue:PSFalse;
    case 'p': return ispunct(c)?PSTrue:PSFalse;
    case 'P': return !ispunct(c)?PSTrue:PSFalse;
    case 'l': return islower(c)?PSTrue:PSFalse;
    case 'u': return isupper(c)?PSTrue:PSFalse;
    }
    return PSFalse; /*cannot happen*/
}

static PSBool psstd_rex_matchclass(PSRex* exp,PSRexNode *node,PSChar c)
{
    do {
        switch(node->type) {
            case OP_RANGE:
                if(c >= node->left && c <= node->right) return PSTrue;
                break;
            case OP_CCLASS:
                if(psstd_rex_matchcclass(node->left,c)) return PSTrue;
                break;
            default:
                if(c == node->type)return PSTrue;
        }
    } while((node->next != -1) && (node = &exp->_nodes[node->next]));
    return PSFalse;
}

static const PSChar *psstd_rex_matchnode(PSRex* exp,PSRexNode *node,const PSChar *str,PSRexNode *next)
{

    PSRexNodeType type = node->type;
    switch(type) {
    case OP_GREEDY: {
        //PSRexNode *greedystop = (node->next != -1) ? &exp->_nodes[node->next] : NULL;
        PSRexNode *greedystop = NULL;
        PSInteger p0 = (node->right >> 16)&0x0000FFFF, p1 = node->right&0x0000FFFF, nmaches = 0;
        const PSChar *s=str, *good = str;

        if(node->next != -1) {
            greedystop = &exp->_nodes[node->next];
        }
        else {
            greedystop = next;
        }

        while((nmaches == 0xFFFF || nmaches < p1)) {

            const PSChar *stop;
            if(!(s = psstd_rex_matchnode(exp,&exp->_nodes[node->left],s,greedystop)))
                break;
            nmaches++;
            good=s;
            if(greedystop) {
                //checks that 0 matches satisfy the expression(if so skips)
                //if not would always stop(for instance if is a '?')
                if(greedystop->type != OP_GREEDY ||
                (greedystop->type == OP_GREEDY && ((greedystop->right >> 16)&0x0000FFFF) != 0))
                {
                    PSRexNode *gnext = NULL;
                    if(greedystop->next != -1) {
                        gnext = &exp->_nodes[greedystop->next];
                    }else if(next && next->next != -1){
                        gnext = &exp->_nodes[next->next];
                    }
                    stop = psstd_rex_matchnode(exp,greedystop,s,gnext);
                    if(stop) {
                        //if satisfied stop it
                        if(p0 == p1 && p0 == nmaches) break;
                        else if(nmaches >= p0 && p1 == 0xFFFF) break;
                        else if(nmaches >= p0 && nmaches <= p1) break;
                    }
                }
            }

            if(s >= exp->_eol)
                break;
        }
        if(p0 == p1 && p0 == nmaches) return good;
        else if(nmaches >= p0 && p1 == 0xFFFF) return good;
        else if(nmaches >= p0 && nmaches <= p1) return good;
        return NULL;
    }
    case OP_OR: {
            const PSChar *asd = str;
            PSRexNode *temp=&exp->_nodes[node->left];
            while( (asd = psstd_rex_matchnode(exp,temp,asd,NULL)) ) {
                if(temp->next != -1)
                    temp = &exp->_nodes[temp->next];
                else
                    return asd;
            }
            asd = str;
            temp = &exp->_nodes[node->right];
            while( (asd = psstd_rex_matchnode(exp,temp,asd,NULL)) ) {
                if(temp->next != -1)
                    temp = &exp->_nodes[temp->next];
                else
                    return asd;
            }
            return NULL;
            break;
    }
    case OP_EXPR:
    case OP_NOCAPEXPR:{
            PSRexNode *n = &exp->_nodes[node->left];
            const PSChar *cur = str;
            PSInteger capture = -1;
            if(node->type != OP_NOCAPEXPR && node->right == exp->_currsubexp) {
                capture = exp->_currsubexp;
                exp->_matches[capture].begin = cur;
                exp->_currsubexp++;
            }
            PSInteger tempcap = exp->_currsubexp;
            do {
                PSRexNode *subnext = NULL;
                if(n->next != -1) {
                    subnext = &exp->_nodes[n->next];
                }else {
                    subnext = next;
                }
                if(!(cur = psstd_rex_matchnode(exp,n,cur,subnext))) {
                    if(capture != -1){
                        exp->_matches[capture].begin = 0;
                        exp->_matches[capture].len = 0;
                    }
                    return NULL;
                }
            } while((n->next != -1) && (n = &exp->_nodes[n->next]));

            exp->_currsubexp = tempcap;
            if(capture != -1)
                exp->_matches[capture].len = cur - exp->_matches[capture].begin;
            return cur;
    }
    case OP_WB:
        if((str == exp->_bol && !isspace(*str))
         || (str == exp->_eol && !isspace(*(str-1)))
         || (!isspace(*str) && isspace(*(str+1)))
         || (isspace(*str) && !isspace(*(str+1))) ) {
            return (node->left == 'b')?str:NULL;
        }
        return (node->left == 'b')?NULL:str;
    case OP_BOL:
        if(str == exp->_bol) return str;
        return NULL;
    case OP_EOL:
        if(str == exp->_eol) return str;
        return NULL;
    case OP_DOT:{
        if (str == exp->_eol) return NULL;
        str++;
                }
        return str;
    case OP_NCLASS:
    case OP_CLASS:
        if (str == exp->_eol) return NULL;
        if(psstd_rex_matchclass(exp,&exp->_nodes[node->left],*str)?(type == OP_CLASS?PSTrue:PSFalse):(type == OP_NCLASS?PSTrue:PSFalse)) {
            str++;
            return str;
        }
        return NULL;
    case OP_CCLASS:
        if (str == exp->_eol) return NULL;
        if(psstd_rex_matchcclass(node->left,*str)) {
            str++;
            return str;
        }
        return NULL;
    case OP_MB:
        {
            PSInteger cb = node->left; //char that opens a balanced expression
            if(*str != cb) return NULL; // string doesnt start with open char
            PSInteger ce = node->right; //char that closes a balanced expression
            PSInteger cont = 1;
            const PSChar *streol = exp->_eol;
            while (++str < streol) {
              if (*str == ce) {
                if (--cont == 0) {
                    return ++str;
                }
              }
              else if (*str == cb) cont++;
            }
        }
        return NULL; // string ends out of balance
    default: /* char */
        if (str == exp->_eol) return NULL;
        if(*str != node->type) return NULL;
        str++;
        return str;
    }
    return NULL;
}

/* public api */
PSRex *psstd_rex_compile(const PSChar *pattern,const PSChar **error)
{
    PSRex * volatile exp = (PSRex *)ps_malloc(sizeof(PSRex)); // "volatile" is needed for setjmp()
    exp->_eol = exp->_bol = NULL;
    exp->_p = pattern;
    exp->_nallocated = (PSInteger)scstrlen(pattern) * sizeof(PSChar);
    exp->_nodes = (PSRexNode *)ps_malloc(exp->_nallocated * sizeof(PSRexNode));
    exp->_nsize = 0;
    exp->_matches = 0;
    exp->_nsubexpr = 0;
    exp->_first = psstd_rex_newnode(exp,OP_EXPR);
    exp->_error = error;
    exp->_jmpbuf = ps_malloc(sizeof(jmp_buf));
    if(setjmp(*((jmp_buf*)exp->_jmpbuf)) == 0) {
        PSInteger res = psstd_rex_list(exp);
        exp->_nodes[exp->_first].left = res;
        if(*exp->_p!='\0')
            psstd_rex_error(exp,_SC("unexpected character"));
#ifdef _DEBUG
        {
            PSInteger nsize,i;
            PSRexNode *t;
            nsize = exp->_nsize;
            t = &exp->_nodes[0];
            scprintf(_SC("\n"));
            for(i = 0;i < nsize; i++) {
                if(exp->_nodes[i].type>MAX_CHAR)
                    scprintf(_SC("[%02d] %10s "),i,g_nnames[exp->_nodes[i].type-MAX_CHAR]);
                else
                    scprintf(_SC("[%02d] %10c "),i,exp->_nodes[i].type);
                scprintf(_SC("left %02d right %02d next %02d\n"), (PSInt32)exp->_nodes[i].left, (PSInt32)exp->_nodes[i].right, (PSInt32)exp->_nodes[i].next);
            }
            scprintf(_SC("\n"));
        }
#endif
        exp->_matches = (PSRexMatch *) ps_malloc(exp->_nsubexpr * sizeof(PSRexMatch));
        memset(exp->_matches,0,exp->_nsubexpr * sizeof(PSRexMatch));
    }
    else{
        psstd_rex_free(exp);
        return NULL;
    }
    return exp;
}

void psstd_rex_free(PSRex *exp)
{
    if(exp) {
        if(exp->_nodes) ps_free(exp->_nodes,exp->_nallocated * sizeof(PSRexNode));
        if(exp->_jmpbuf) ps_free(exp->_jmpbuf,sizeof(jmp_buf));
        if(exp->_matches) ps_free(exp->_matches,exp->_nsubexpr * sizeof(PSRexMatch));
        ps_free(exp,sizeof(PSRex));
    }
}

PSBool psstd_rex_match(PSRex* exp,const PSChar* text)
{
    const PSChar* res = NULL;
    exp->_bol = text;
    exp->_eol = text + scstrlen(text);
    exp->_currsubexp = 0;
    res = psstd_rex_matchnode(exp,exp->_nodes,text,NULL);
    if(res == NULL || res != exp->_eol)
        return PSFalse;
    return PSTrue;
}

PSBool psstd_rex_searchrange(PSRex* exp,const PSChar* text_begin,const PSChar* text_end,const PSChar** out_begin, const PSChar** out_end)
{
    const PSChar *cur = NULL;
    PSInteger node = exp->_first;
    if(text_begin >= text_end) return PSFalse;
    exp->_bol = text_begin;
    exp->_eol = text_end;
    do {
        cur = text_begin;
        while(node != -1) {
            exp->_currsubexp = 0;
            cur = psstd_rex_matchnode(exp,&exp->_nodes[node],cur,NULL);
            if(!cur)
                break;
            node = exp->_nodes[node].next;
        }
        text_begin++;
    } while(cur == NULL && text_begin != text_end);

    if(cur == NULL)
        return PSFalse;

    --text_begin;

    if(out_begin) *out_begin = text_begin;
    if(out_end) *out_end = cur;
    return PSTrue;
}

PSBool psstd_rex_search(PSRex* exp,const PSChar* text, const PSChar** out_begin, const PSChar** out_end)
{
    return psstd_rex_searchrange(exp,text,text + scstrlen(text),out_begin,out_end);
}

PSInteger psstd_rex_getsubexpcount(PSRex* exp)
{
    return exp->_nsubexpr;
}

PSBool psstd_rex_getsubexp(PSRex* exp, PSInteger n, PSRexMatch *subexp)
{
    if( n<0 || n >= exp->_nsubexpr) return PSFalse;
    *subexp = exp->_matches[n];
    return PSTrue;
}

