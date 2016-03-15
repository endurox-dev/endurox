/*	see copyright notice in pscript.h */
#ifndef _PSSTD_BLOBIMPL_H_
#define _PSSTD_BLOBIMPL_H_

struct PSBlob : public PSStream
{
	PSBlob(PSInteger size) {
		_size = size;
		_allocated = size;
		_buf = (unsigned char *)ps_malloc(size);
		memset(_buf, 0, _size);
		_ptr = 0;
		_owns = true;
	}
	virtual ~PSBlob() {
		ps_free(_buf, _allocated);
	}
	PSInteger Write(void *buffer, PSInteger size) {
		if(!CanAdvance(size)) {
			GrowBufOf(_ptr + size - _size);
		}
		memcpy(&_buf[_ptr], buffer, size);
		_ptr += size;
		return size;
	}
	PSInteger Read(void *buffer,PSInteger size) {
		PSInteger n = size;
		if(!CanAdvance(size)) {
			if((_size - _ptr) > 0)
				n = _size - _ptr;
			else return 0;
		}
		memcpy(buffer, &_buf[_ptr], n);
		_ptr += n;
		return n;
	}
	bool Resize(PSInteger n) {
		if(!_owns) return false;
		if(n != _allocated) {
			unsigned char *newbuf = (unsigned char *)ps_malloc(n);
			memset(newbuf,0,n);
			if(_size > n)
				memcpy(newbuf,_buf,n);
			else
				memcpy(newbuf,_buf,_size);
			ps_free(_buf,_allocated);
			_buf=newbuf;
			_allocated = n;
			if(_size > _allocated)
				_size = _allocated;
			if(_ptr > _allocated)
				_ptr = _allocated;
		}
		return true;
	}
	bool GrowBufOf(PSInteger n)
	{
		bool ret = true;
		if(_size + n > _allocated) {
			if(_size + n > _size * 2)
				ret = Resize(_size + n);
			else
				ret = Resize(_size * 2);
		}
		_size = _size + n;
		return ret;
	}
	bool CanAdvance(PSInteger n) {
		if(_ptr+n>_size)return false;
		return true;
	}
	PSInteger Seek(PSInteger offset, PSInteger origin) {
		switch(origin) {
			case PS_SEEK_SET:
				if(offset > _size || offset < 0) return -1;
				_ptr = offset;
				break;
			case PS_SEEK_CUR:
				if(_ptr + offset > _size || _ptr + offset < 0) return -1;
				_ptr += offset;
				break;
			case PS_SEEK_END:
				if(_size + offset > _size || _size + offset < 0) return -1;
				_ptr = _size + offset;
				break;
			default: return -1;
		}
		return 0;
	}
	bool IsValid() {
		return _buf?true:false;
	}
	bool EOS() {
		return _ptr == _size;
	}
	PSInteger Flush() { return 0; }
	PSInteger Tell() { return _ptr; }
	PSInteger Len() { return _size; }
	PSUserPointer GetBuf(){ return _buf; }
private:
	PSInteger _size;
	PSInteger _allocated;
	PSInteger _ptr;
	unsigned char *_buf;
	bool _owns;
};

#endif //_PSSTD_BLOBIMPL_H_
