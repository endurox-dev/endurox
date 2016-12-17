/*  see copyright notice in pscript.h */
#ifndef _PSSTRING_H_
#define _PSSTRING_H_

inline PSHash _hashstr (const PSChar *s, size_t l)
{
        PSHash h = (PSHash)l;  /* seed */
        size_t step = (l>>5)|1;  /* if string is too long, don't hash all its chars */
        for (; l>=step; l-=step)
            h = h ^ ((h<<5)+(h>>2)+(unsigned short)*(s++));
        return h;
}

struct PSString : public PSRefCounted
{
    PSString(){}
    ~PSString(){}
public:
    static PSString *Create(PSSharedState *ss, const PSChar *, PSInteger len = -1 );
    PSInteger Next(const PSObjectPtr &refpos, PSObjectPtr &outkey, PSObjectPtr &outval);
    void Release();
    PSSharedState *_sharedstate;
    PSString *_next; //chain for the string table
    PSInteger _len;
    PSHash _hash;
    PSChar _val[1];
};



#endif //_PSSTRING_H_
