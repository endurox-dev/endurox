/*  see copyright notice in pscript.h */
#ifndef _PSARRAY_H_
#define _PSARRAY_H_

struct PSArray : public CHAINABLE_OBJ
{
private:
    PSArray(PSSharedState *ss,PSInteger nsize){_values.resize(nsize); INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);}
    ~PSArray()
    {
        REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
    }
public:
    static PSArray* Create(PSSharedState *ss,PSInteger nInitialSize){
        PSArray *newarray=(PSArray*)PS_MALLOC(sizeof(PSArray));
        new (newarray) PSArray(ss,nInitialSize);
        return newarray;
    }
#ifndef NO_GARBAGE_COLLECTOR
    void Mark(PSCollectable **chain);
    PSObjectType GetType() {return OT_ARRAY;}
#endif
    void Finalize(){
        _values.resize(0);
    }
    bool Get(const PSInteger nidx,PSObjectPtr &val)
    {
        if(nidx>=0 && nidx<(PSInteger)_values.size()){
            PSObjectPtr &o = _values[nidx];
            val = _realval(o);
            return true;
        }
        else return false;
    }
    bool Set(const PSInteger nidx,const PSObjectPtr &val)
    {
        if(nidx>=0 && nidx<(PSInteger)_values.size()){
            _values[nidx]=val;
            return true;
        }
        else return false;
    }
    PSInteger Next(const PSObjectPtr &refpos,PSObjectPtr &outkey,PSObjectPtr &outval)
    {
        PSUnsignedInteger idx=TranslateIndex(refpos);
        while(idx<_values.size()){
            //first found
            outkey=(PSInteger)idx;
            PSObjectPtr &o = _values[idx];
            outval = _realval(o);
            //return idx for the next iteration
            return ++idx;
        }
        //nothing to iterate anymore
        return -1;
    }
    PSArray *Clone(){PSArray *anew=Create(_opt_ss(this),0); anew->_values.copy(_values); return anew; }
    PSInteger Size() const {return _values.size();}
    void Resize(PSInteger size)
    {
        PSObjectPtr _null;
        Resize(size,_null);
    }
    void Resize(PSInteger size,PSObjectPtr &fill) { _values.resize(size,fill); ShrinkIfNeeded(); }
    void Reserve(PSInteger size) { _values.reserve(size); }
    void Append(const PSObject &o){_values.push_back(o);}
    void Extend(const PSArray *a);
    PSObjectPtr &Top(){return _values.top();}
    void Pop(){_values.pop_back(); ShrinkIfNeeded(); }
    bool Insert(PSInteger idx,const PSObject &val){
        if(idx < 0 || idx > (PSInteger)_values.size())
            return false;
        _values.insert(idx,val);
        return true;
    }
    void ShrinkIfNeeded() {
        if(_values.size() <= _values.capacity()>>2) //shrink the array
            _values.shrinktofit();
    }
    bool Remove(PSInteger idx){
        if(idx < 0 || idx >= (PSInteger)_values.size())
            return false;
        _values.remove(idx);
        ShrinkIfNeeded();
        return true;
    }
    void Release()
    {
        ps_delete(this,PSArray);
    }

    PSObjectPtrVec _values;
};
#endif //_PSARRAY_H_
