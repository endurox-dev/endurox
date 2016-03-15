/*	see copyright notice in pscript.h */
#ifndef _PSUTILS_H_
#define _PSUTILS_H_

void *ps_vm_malloc(PSUnsignedInteger size);
void *ps_vm_realloc(void *p,PSUnsignedInteger oldsize,PSUnsignedInteger size);
void ps_vm_free(void *p,PSUnsignedInteger size);

#define ps_new(__ptr,__type) {__ptr=(__type *)ps_vm_malloc(sizeof(__type));new (__ptr) __type;}
#define ps_delete(__ptr,__type) {__ptr->~__type();ps_vm_free(__ptr,sizeof(__type));}
#define PS_MALLOC(__size) ps_vm_malloc((__size));
#define PS_FREE(__ptr,__size) ps_vm_free((__ptr),(__size));
#define PS_REALLOC(__ptr,__oldsize,__size) ps_vm_realloc((__ptr),(__oldsize),(__size));

#define ps_aligning(v) (((size_t)(v) + (PS_ALIGNMENT-1)) & (~(PS_ALIGNMENT-1)))

//psvector mini vector class, supports objects by value
template<typename T> class psvector
{
public:
	psvector()
	{
		_vals = NULL;
		_size = 0;
		_allocated = 0;
	}
	psvector(const psvector<T>& v)
	{
		copy(v);
	}
	void copy(const psvector<T>& v)
	{
		if(_size) {
			resize(0); //destroys all previous stuff
		}
		//resize(v._size);
		if(v._size > _allocated) {
			_realloc(v._size);
		}
		for(PSUnsignedInteger i = 0; i < v._size; i++) {
			new ((void *)&_vals[i]) T(v._vals[i]);
		}
		_size = v._size;
	}
	~psvector()
	{
		if(_allocated) {
			for(PSUnsignedInteger i = 0; i < _size; i++)
				_vals[i].~T();
			PS_FREE(_vals, (_allocated * sizeof(T)));
		}
	}
	void reserve(PSUnsignedInteger newsize) { _realloc(newsize); }
	void resize(PSUnsignedInteger newsize, const T& fill = T())
	{
		if(newsize > _allocated)
			_realloc(newsize);
		if(newsize > _size) {
			while(_size < newsize) {
				new ((void *)&_vals[_size]) T(fill);
				_size++;
			}
		}
		else{
			for(PSUnsignedInteger i = newsize; i < _size; i++) {
				_vals[i].~T();
			}
			_size = newsize;
		}
	}
	void shrinktofit() { if(_size > 4) { _realloc(_size); } }
	T& top() const { return _vals[_size - 1]; }
	inline PSUnsignedInteger size() const { return _size; }
	bool empty() const { return (_size <= 0); }
	inline T &push_back(const T& val = T())
	{
		if(_allocated <= _size)
			_realloc(_size * 2);
		return *(new ((void *)&_vals[_size++]) T(val));
	}
	inline void pop_back()
	{
		_size--; _vals[_size].~T();
	}
	void insert(PSUnsignedInteger idx, const T& val)
	{
		resize(_size + 1);
		for(PSUnsignedInteger i = _size - 1; i > idx; i--) {
			_vals[i] = _vals[i - 1];
		}
    	_vals[idx] = val;
	}
	void remove(PSUnsignedInteger idx)
	{
		_vals[idx].~T();
		if(idx < (_size - 1)) {
			memmove(&_vals[idx], &_vals[idx+1], sizeof(T) * (_size - idx - 1));
		}
		_size--;
	}
	PSUnsignedInteger capacity() { return _allocated; }
	inline T &back() const { return _vals[_size - 1]; }
	inline T& operator[](PSUnsignedInteger pos) const{ return _vals[pos]; }
	T* _vals;
private:
	void _realloc(PSUnsignedInteger newsize)
	{
		newsize = (newsize > 0)?newsize:4;
		_vals = (T*)PS_REALLOC(_vals, _allocated * sizeof(T), newsize * sizeof(T));
		_allocated = newsize;
	}
	PSUnsignedInteger _size;
	PSUnsignedInteger _allocated;
};

#endif //_PSUTILS_H_
