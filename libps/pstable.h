/*  see copyright notice in pscript.h */
#ifndef _PSTABLE_H_
#define _PSTABLE_H_
/*
* The following code is based on Lua 4.0 (Copyright 1994-2002 Tecgraf, PUC-Rio.)
* http://www.lua.org/copyright.html#4
* http://www.lua.org/source/4.0.1/src_ltable.c.html
*/

#include "psstring.h"


#define hashptr(p)  ((PSHash)(((PSInteger)p) >> 3))

inline PSHash HashObj(const PSObjectPtr &key)
{
    switch(type(key)) {
        case OT_STRING:     return _string(key)->_hash;
        case OT_FLOAT:      return (PSHash)((PSInteger)_float(key));
        case OT_BOOL: case OT_INTEGER:  return (PSHash)((PSInteger)_integer(key));
        default:            return hashptr(key._unVal.pRefCounted);
    }
}

struct PSTable : public PSDelegable
{
private:
    struct _HashNode
    {
        _HashNode() { next = NULL; }
        PSObjectPtr val;
        PSObjectPtr key;
        _HashNode *next;
    };
    _HashNode *_firstfree;
    _HashNode *_nodes;
    PSInteger _numofnodes;
    PSInteger _usednodes;

///////////////////////////
    void AllocNodes(PSInteger nSize);
    void Rehash(bool force);
    PSTable(PSSharedState *ss, PSInteger nInitialSize);
    void _ClearNodes();
public:
    static PSTable* Create(PSSharedState *ss,PSInteger nInitialSize)
    {
        PSTable *newtable = (PSTable*)PS_MALLOC(sizeof(PSTable));
        new (newtable) PSTable(ss, nInitialSize);
        newtable->_delegate = NULL;
        return newtable;
    }
    void Finalize();
    PSTable *Clone();
    ~PSTable()
    {
        SetDelegate(NULL);
        REMOVE_FROM_CHAIN(&_sharedstate->_gc_chain, this);
        for (PSInteger i = 0; i < _numofnodes; i++) _nodes[i].~_HashNode();
        PS_FREE(_nodes, _numofnodes * sizeof(_HashNode));
    }
#ifndef NO_GARBAGE_COLLECTOR
    void Mark(PSCollectable **chain);
    PSObjectType GetType() {return OT_TABLE;}
#endif
    inline _HashNode *_Get(const PSObjectPtr &key,PSHash hash)
    {
        _HashNode *n = &_nodes[hash];
        do{
            if(_rawval(n->key) == _rawval(key) && type(n->key) == type(key)){
                return n;
            }
        }while((n = n->next));
        return NULL;
    }
    //for compiler use
    inline bool GetStr(const PSChar* key,PSInteger keylen,PSObjectPtr &val)
    {
        PSHash hash = _hashstr(key,keylen);
        _HashNode *n = &_nodes[hash & (_numofnodes - 1)];
        _HashNode *res = NULL;
        do{
            if(type(n->key) == OT_STRING && (scstrcmp(_stringval(n->key),key) == 0)){
                res = n;
                break;
            }
        }while((n = n->next));
        if (res) {
            val = _realval(res->val);
            return true;
        }
        return false;
    }
    bool Get(const PSObjectPtr &key,PSObjectPtr &val);
    void Remove(const PSObjectPtr &key);
    bool Set(const PSObjectPtr &key, const PSObjectPtr &val);
    //returns true if a new slot has been created false if it was already present
    bool NewSlot(const PSObjectPtr &key,const PSObjectPtr &val);
    PSInteger Next(bool getweakrefs,const PSObjectPtr &refpos, PSObjectPtr &outkey, PSObjectPtr &outval);

    PSInteger CountUsed(){ return _usednodes;}
    void Clear();
    void Release()
    {
        ps_delete(this, PSTable);
    }

};

#endif //_PSTABLE_H_
