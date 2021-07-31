/*
Copyright (c) 2007-2010, Troy D. Hanson   http://exhash.sourceforge.net
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef UTLIST_INT_H
#define UTLIST_INT_H

#define UTLIST_VERSION 1.9.1

/* 
 * This file contains macros to manipulate singly and doubly-linked lists.
 *
 * 1. LL_ macros:  singly-linked lists.
 * 2. DL_ macros:  doubly-linked lists.
 * 3. CDL_ macros: circular doubly-linked lists.
 *
 * To use singly-linked lists, your structure must have a "NDRX_NEXT" pointer.
 * To use doubly-linked lists, your structure must "NDRX_PREV" and "NDRX_NEXT" pointers.
 * Either way, the pointer to the head of the list must be initialized to NULL.
 * 
 * ----------------.EXAMPLE -------------------------
 * struct item {
 *      int id;
 *      struct item *NDRX_PREV, *NDRX_NEXT;
 * }
 *
 * struct item *list = NULL:
 *
 * int main() {
 *      struct item *item;
 *      ... allocate and populate item ...
 *      DL_APPEND(list, item);
 * }
 * --------------------------------------------------
 *
 * For doubly-linked lists, the append and delete macros are O(1)
 * For singly-linked lists, append and delete are O(n) but prepend is O(1)
 * The sort macro is O(n log(n)) for all types of single/double/circular lists.
 */

/* These macros use decltype or the earlier __typeof GNU extension.
   As decltype is only available in newer compilers (VS2010 or gcc 4.3+
   when compiling c++ code), this code uses whatever method is needed
   or, for VS2008 where neither is available, uses casting workarounds. */
#ifdef _MSC_VER            /* MS compiler */
#if _MSC_VER >= 1600 && defined(__cplusplus)  /* VS2010 or newer in C++ mode */
#define LDECLTYPE(x) decltype(x)
#else                     /* VS2008 or older (or VS2010 in C mode) */
#define NO_DECLTYPE
#define LDECLTYPE(x) char*
#endif
#else                      /* GNU, Sun and other compilers */
#define LDECLTYPE(x) __typeof(x)
#endif

/* for VS2008 we use some workarounds to get around the lack of decltype,
 * namely, we always reassign our tmp variable to the list head if we need
 * to dereference its NDRX_PREV/NDRX_NEXT pointers, and save/restore the real head.*/
#ifdef NO_DECLTYPE
#define _SV(elt,list) _tmp = (char*)(list); {char **_alias = (char**)&(list); *_alias = (elt); }
#define _NEXT(elt,list) ((char*)((list)->NDRX_NEXT))
#define _NEXTASGN(elt,list,to) { char **_alias = (char**)&((list)->NDRX_NEXT); *_alias=(char*)(to); }
#define _PREV(elt,list) ((char*)((list)->NDRX_PREV))
#define _PREVASGN(elt,list,to) { char **_alias = (char**)&((list)->NDRX_PREV); *_alias=(char*)(to); }
#define _RS(list) { char **_alias = (char**)&(list); *_alias=_tmp; }
#define _CASTASGN(a,b) { char **_alias = (char**)&(a); *_alias=(char*)(b); }
#else 
#define _SV(elt,list)
#define _NEXT(elt,list) ((elt)->NDRX_NEXT)
#define _NEXTASGN(elt,list,to) ((elt)->NDRX_NEXT)=(to)
#define _PREV(elt,list) ((elt)->NDRX_PREV)
#define _PREVASGN(elt,list,to) ((elt)->NDRX_PREV)=(to)
#define _RS(list)
#define _CASTASGN(a,b) (a)=(b)
#endif

/******************************************************************************
 * The sort macro is an adaptation of Simon Tatham's O(n log(n)) mergesort    *
 * Unwieldy variable names used here to avoid shadowing passed-in variables.  *
 *****************************************************************************/
#define LL_SORT(list, cmp)                                                                     \
do {                                                                                           \
  LDECLTYPE(list) _ls_p;                                                                       \
  LDECLTYPE(list) _ls_q;                                                                       \
  LDECLTYPE(list) _ls_e;                                                                       \
  LDECLTYPE(list) _ls_tail;                                                                    \
  LDECLTYPE(list) _ls_oldhead;                                                                 \
  LDECLTYPE(list) _tmp;                                                                        \
  int _ls_insize, _ls_nmerges, _ls_psize, _ls_qsize, _ls_i, _ls_looping;                       \
  if (list) {                                                                                  \
    _ls_insize = 1;                                                                            \
    _ls_looping = 1;                                                                           \
    while (_ls_looping) {                                                                      \
      _CASTASGN(_ls_p,list);                                                                   \
      _CASTASGN(_ls_oldhead,list);                                                             \
      list = NULL;                                                                             \
      _ls_tail = NULL;                                                                         \
      _ls_nmerges = 0;                                                                         \
      while (_ls_p) {                                                                          \
        _ls_nmerges++;                                                                         \
        _ls_q = _ls_p;                                                                         \
        _ls_psize = 0;                                                                         \
        for (_ls_i = 0; _ls_i < _ls_insize; _ls_i++) {                                         \
          _ls_psize++;                                                                         \
          _SV(_ls_q,list); _ls_q = _NEXT(_ls_q,list); _RS(list);                               \
          if (!_ls_q) break;                                                                   \
        }                                                                                      \
        _ls_qsize = _ls_insize;                                                                \
        while (_ls_psize > 0 || (_ls_qsize > 0 && _ls_q)) {                                    \
          if (_ls_psize == 0) {                                                                \
            _ls_e = _ls_q; _SV(_ls_q,list); _ls_q = _NEXT(_ls_q,list); _RS(list); _ls_qsize--; \
          } else if (_ls_qsize == 0 || !_ls_q) {                                               \
            _ls_e = _ls_p; _SV(_ls_p,list); _ls_p = _NEXT(_ls_p,list); _RS(list); _ls_psize--; \
          } else if (cmp(_ls_p,_ls_q) <= 0) {                                                  \
            _ls_e = _ls_p; _SV(_ls_p,list); _ls_p = _NEXT(_ls_p,list); _RS(list); _ls_psize--; \
          } else {                                                                             \
            _ls_e = _ls_q; _SV(_ls_q,list); _ls_q = _NEXT(_ls_q,list); _RS(list); _ls_qsize--; \
          }                                                                                    \
          if (_ls_tail) {                                                                      \
            _SV(_ls_tail,list); _NEXTASGN(_ls_tail,list,_ls_e); _RS(list);                     \
          } else {                                                                             \
            _CASTASGN(list,_ls_e);                                                             \
          }                                                                                    \
          _ls_tail = _ls_e;                                                                    \
        }                                                                                      \
        _ls_p = _ls_q;                                                                         \
      }                                                                                        \
      _SV(_ls_tail,list); _NEXTASGN(_ls_tail,list,NULL); _RS(list);                            \
      if (_ls_nmerges <= 1) {                                                                  \
        _ls_looping=0;                                                                         \
      }                                                                                        \
      _ls_insize *= 2;                                                                         \
    }                                                                                          \
  } else _tmp=NULL; /* quiet gcc unused variable warning */                                    \
} while (0)

#define DL_SORT(list, cmp)                                                                     \
do {                                                                                           \
  LDECLTYPE(list) _ls_p;                                                                       \
  LDECLTYPE(list) _ls_q;                                                                       \
  LDECLTYPE(list) _ls_e;                                                                       \
  LDECLTYPE(list) _ls_tail;                                                                    \
  LDECLTYPE(list) _ls_oldhead;                                                                 \
  LDECLTYPE(list) _tmp;                                                                        \
  int _ls_insize, _ls_nmerges, _ls_psize, _ls_qsize, _ls_i, _ls_looping;                       \
  if (list) {                                                                                  \
    _ls_insize = 1;                                                                            \
    _ls_looping = 1;                                                                           \
    while (_ls_looping) {                                                                      \
      _CASTASGN(_ls_p,list);                                                                   \
      _CASTASGN(_ls_oldhead,list);                                                             \
      list = NULL;                                                                             \
      _ls_tail = NULL;                                                                         \
      _ls_nmerges = 0;                                                                         \
      while (_ls_p) {                                                                          \
        _ls_nmerges++;                                                                         \
        _ls_q = _ls_p;                                                                         \
        _ls_psize = 0;                                                                         \
        for (_ls_i = 0; _ls_i < _ls_insize; _ls_i++) {                                         \
          _ls_psize++;                                                                         \
          _SV(_ls_q,list); _ls_q = _NEXT(_ls_q,list); _RS(list);                               \
          if (!_ls_q) break;                                                                   \
        }                                                                                      \
        _ls_qsize = _ls_insize;                                                                \
        while (_ls_psize > 0 || (_ls_qsize > 0 && _ls_q)) {                                    \
          if (_ls_psize == 0) {                                                                \
            _ls_e = _ls_q; _SV(_ls_q,list); _ls_q = _NEXT(_ls_q,list); _RS(list); _ls_qsize--; \
          } else if (_ls_qsize == 0 || !_ls_q) {                                               \
            _ls_e = _ls_p; _SV(_ls_p,list); _ls_p = _NEXT(_ls_p,list); _RS(list); _ls_psize--; \
          } else if (cmp(_ls_p,_ls_q) <= 0) {                                                  \
            _ls_e = _ls_p; _SV(_ls_p,list); _ls_p = _NEXT(_ls_p,list); _RS(list); _ls_psize--; \
          } else {                                                                             \
            _ls_e = _ls_q; _SV(_ls_q,list); _ls_q = _NEXT(_ls_q,list); _RS(list); _ls_qsize--; \
          }                                                                                    \
          if (_ls_tail) {                                                                      \
            _SV(_ls_tail,list); _NEXTASGN(_ls_tail,list,_ls_e); _RS(list);                     \
          } else {                                                                             \
            _CASTASGN(list,_ls_e);                                                             \
          }                                                                                    \
          _SV(_ls_e,list); _PREVASGN(_ls_e,list,_ls_tail); _RS(list);                          \
          _ls_tail = _ls_e;                                                                    \
        }                                                                                      \
        _ls_p = _ls_q;                                                                         \
      }                                                                                        \
      _CASTASGN(list->NDRX_PREV, _ls_tail);                                                    \
      _SV(_ls_tail,list); _NEXTASGN(_ls_tail,list,NULL); _RS(list);                            \
      if (_ls_nmerges <= 1) {                                                                  \
        _ls_looping=0;                                                                         \
      }                                                                                        \
      _ls_insize *= 2;                                                                         \
    }                                                                                          \
  } else _tmp=NULL; /* quiet gcc unused variable warning */                                    \
} while (0)

#define CDL_SORT(list, cmp)                                                                    \
do {                                                                                           \
  LDECLTYPE(list) _ls_p;                                                                       \
  LDECLTYPE(list) _ls_q;                                                                       \
  LDECLTYPE(list) _ls_e;                                                                       \
  LDECLTYPE(list) _ls_tail;                                                                    \
  LDECLTYPE(list) _ls_oldhead;                                                                 \
  LDECLTYPE(list) _tmp;                                                                        \
  LDECLTYPE(list) _tmp2;                                                                       \
  int _ls_insize, _ls_nmerges, _ls_psize, _ls_qsize, _ls_i, _ls_looping;                       \
  if (list) {                                                                                  \
    _ls_insize = 1;                                                                            \
    _ls_looping = 1;                                                                           \
    while (_ls_looping) {                                                                      \
      _CASTASGN(_ls_p,list);                                                                   \
      _CASTASGN(_ls_oldhead,list);                                                             \
      list = NULL;                                                                             \
      _ls_tail = NULL;                                                                         \
      _ls_nmerges = 0;                                                                         \
      while (_ls_p) {                                                                          \
        _ls_nmerges++;                                                                         \
        _ls_q = _ls_p;                                                                         \
        _ls_psize = 0;                                                                         \
        for (_ls_i = 0; _ls_i < _ls_insize; _ls_i++) {                                         \
          _ls_psize++;                                                                         \
          _SV(_ls_q,list);                                                                     \
          if (_NEXT(_ls_q,list) == _ls_oldhead) {                                              \
            _ls_q = NULL;                                                                      \
          } else {                                                                             \
            _ls_q = _NEXT(_ls_q,list);                                                         \
          }                                                                                    \
          _RS(list);                                                                           \
          if (!_ls_q) break;                                                                   \
        }                                                                                      \
        _ls_qsize = _ls_insize;                                                                \
        while (_ls_psize > 0 || (_ls_qsize > 0 && _ls_q)) {                                    \
          if (_ls_psize == 0) {                                                                \
            _ls_e = _ls_q; _SV(_ls_q,list); _ls_q = _NEXT(_ls_q,list); _RS(list); _ls_qsize--; \
            if (_ls_q == _ls_oldhead) { _ls_q = NULL; }                                        \
          } else if (_ls_qsize == 0 || !_ls_q) {                                               \
            _ls_e = _ls_p; _SV(_ls_p,list); _ls_p = _NEXT(_ls_p,list); _RS(list); _ls_psize--; \
            if (_ls_p == _ls_oldhead) { _ls_p = NULL; }                                        \
          } else if (cmp(_ls_p,_ls_q) <= 0) {                                                  \
            _ls_e = _ls_p; _SV(_ls_p,list); _ls_p = _NEXT(_ls_p,list); _RS(list); _ls_psize--; \
            if (_ls_p == _ls_oldhead) { _ls_p = NULL; }                                        \
          } else {                                                                             \
            _ls_e = _ls_q; _SV(_ls_q,list); _ls_q = _NEXT(_ls_q,list); _RS(list); _ls_qsize--; \
            if (_ls_q == _ls_oldhead) { _ls_q = NULL; }                                        \
          }                                                                                    \
          if (_ls_tail) {                                                                      \
            _SV(_ls_tail,list); _NEXTASGN(_ls_tail,list,_ls_e); _RS(list);                     \
          } else {                                                                             \
            _CASTASGN(list,_ls_e);                                                             \
          }                                                                                    \
          _SV(_ls_e,list); _PREVASGN(_ls_e,list,_ls_tail); _RS(list);                          \
          _ls_tail = _ls_e;                                                                    \
        }                                                                                      \
        _ls_p = _ls_q;                                                                         \
      }                                                                                        \
      _CASTASGN(list->NDRX_PREV,_ls_tail);                                                     \
      _CASTASGN(_tmp2,list);                                                                   \
      _SV(_ls_tail,list); _NEXTASGN(_ls_tail,list,_tmp2); _RS(list);                           \
      if (_ls_nmerges <= 1) {                                                                  \
        _ls_looping=0;                                                                         \
      }                                                                                        \
      _ls_insize *= 2;                                                                         \
    }                                                                                          \
  } else _tmp=NULL; /* quiet gcc unused variable warning */                                    \
} while (0)

/******************************************************************************
 * singly linked list macros (non-circular)                                   *
 *****************************************************************************/
#define LL_PREPEND(head,add)                                                                   \
do {                                                                                           \
  (add)->NDRX_NEXT = head;                                                                     \
  head = add;                                                                                  \
} while (0)

#define LL_APPEND(head,add)                                                                    \
do {                                                                                           \
  LDECLTYPE(head) _tmp;                                                                        \
  (add)->NDRX_NEXT=NULL;                                                                       \
  if (head) {                                                                                  \
    _tmp = head;                                                                               \
    while (_tmp->NDRX_NEXT) { _tmp = _tmp->NDRX_NEXT; }                                        \
    _tmp->NDRX_NEXT=(add);                                                                     \
  } else {                                                                                     \
    (head)=(add);                                                                              \
  }                                                                                            \
} while (0)

#define LL_DELETE(head,del)                                                                    \
do {                                                                                           \
  LDECLTYPE(head) _tmp;                                                                        \
  if ((head) == (del)) {                                                                       \
    (head)=(head)->NDRX_NEXT;                                                                  \
  } else {                                                                                     \
    _tmp = head;                                                                               \
    while (_tmp->NDRX_NEXT && (_tmp->NDRX_NEXT != (del))) {                                    \
      _tmp = _tmp->NDRX_NEXT;                                                                  \
    }                                                                                          \
    if (_tmp->NDRX_NEXT) {                                                                     \
      _tmp->NDRX_NEXT = ((del)->NDRX_NEXT);                                                    \
    }                                                                                          \
  }                                                                                            \
} while (0)

/* Here are VS2008 replacements for LL_APPEND and LL_DELETE */
#define LL_APPEND_VS2008(head,add)                                                             \
do {                                                                                           \
  if (head) {                                                                                  \
    (add)->NDRX_NEXT = head;     /* use add->NDRX_NEXT as a temp variable */                   \
    while ((add)->NDRX_NEXT->NDRX_NEXT) { (add)->NDRX_NEXT = (add)->NDRX_NEXT->NDRX_NEXT; }    \
    (add)->NDRX_NEXT->NDRX_NEXT=(add);                                                         \
  } else {                                                                                     \
    (head)=(add);                                                                              \
  }                                                                                            \
  (add)->NDRX_NEXT=NULL;                                                                       \
} while (0)

#define LL_DELETE_VS2008(head,del)                                                             \
do {                                                                                           \
  if ((head) == (del)) {                                                                       \
    (head)=(head)->NDRX_NEXT;                                                                  \
  } else {                                                                                     \
    char *_tmp = (char*)(head);                                                                \
    while (head->NDRX_NEXT && (head->NDRX_NEXT != (del))) {                                    \
      head = head->NDRX_NEXT;                                                                  \
    }                                                                                          \
    if (head->NDRX_NEXT) {                                                                     \
      head->NDRX_NEXT = ((del)->NDRX_NEXT);                                                    \
    }                                                                                          \
    {                                                                                          \
      char **_head_alias = (char**)&(head);                                                    \
      *_head_alias = _tmp;                                                                     \
    }                                                                                          \
  }                                                                                            \
} while (0)
#ifdef NO_DECLTYPE
#undef LL_APPEND
#define LL_APPEND LL_APPEND_VS2008
#undef LL_DELETE
#define LL_DELETE LL_DELETE_VS2008
#endif
/* end VS2008 replacements */

#define LL_FOREACH(head,el)                                                                    \
    for(el=head;el;el=el->NDRX_NEXT)

#define LL_FOREACH_SAFE(head,el,tmp)                                                           \
  for((el)=(head);(el) && (tmp = (el)->NDRX_NEXT, 1); (el) = tmp)

#define LL_SEARCH_SCALAR(head,out,field,val)                                                   \
do {                                                                                           \
    LL_FOREACH(head,out) {                                                                     \
      if ((out)->field == (val)) break;                                                        \
    }                                                                                          \
} while(0) 

#define LL_SEARCH(head,out,elt,cmp)                                                            \
do {                                                                                           \
    LL_FOREACH(head,out) {                                                                     \
      if ((cmp(out,elt))==0) break;                                                            \
    }                                                                                          \
} while(0) 

/******************************************************************************
 * doubly linked list macros (non-circular)                                   *
 *****************************************************************************/
#define DL_PREPEND(head,add)                                                                   \
do {                                                                                           \
 (add)->NDRX_NEXT = head;                                                                      \
 if (head) {                                                                                   \
   (add)->NDRX_PREV = (head)->NDRX_PREV;                                                       \
   (head)->NDRX_PREV = (add);                                                                  \
 } else {                                                                                      \
   (add)->NDRX_PREV = (add);                                                                   \
 }                                                                                             \
 (head) = (add);                                                                               \
} while (0)

#define DL_APPEND(head,add)                                                                    \
do {                                                                                           \
  if (head) {                                                                                  \
      (add)->NDRX_PREV = (head)->NDRX_PREV;                                                    \
      (head)->NDRX_PREV->NDRX_NEXT = (add);                                                    \
      (head)->NDRX_PREV = (add);                                                               \
      (add)->NDRX_NEXT = NULL;                                                                 \
  } else {                                                                                     \
      (head)=(add);                                                                            \
      (head)->NDRX_PREV = (head);                                                              \
      (head)->NDRX_NEXT = NULL;                                                                \
  }                                                                                            \
} while (0);

#define DL_DELETE(head,del)                                                                    \
do {                                                                                           \
  if ((del)->NDRX_PREV == (del)) {                                                             \
      (head)=NULL;                                                                             \
  } else if ((del)==(head)) {                                                                  \
      (del)->NDRX_NEXT->NDRX_PREV = (del)->NDRX_PREV;                                          \
      (head) = (del)->NDRX_NEXT;                                                               \
  } else {                                                                                     \
      (del)->NDRX_PREV->NDRX_NEXT = (del)->NDRX_NEXT;                                          \
      if ((del)->NDRX_NEXT) {                                                                  \
          (del)->NDRX_NEXT->NDRX_PREV = (del)->NDRX_PREV;                                      \
      } else {                                                                                 \
          (head)->NDRX_PREV = (del)->NDRX_PREV;                                                \
      }                                                                                        \
  }                                                                                            \
} while (0);


#define DL_FOREACH(head,el)                                                                    \
    for(el=head;el;el=el->NDRX_NEXT)

/* For Each in reverse order, i is int*/
#define DL_REVFOREACH(head,el,i)                                                               \
    for(el=(head)->NDRX_PREV,i=1; (el) && (el->NDRX_NEXT || i); el=(el)->NDRX_PREV, i=0)

/* this version is safe for deleting the elements during iteration */
#define DL_FOREACH_SAFE(head,el,tmp)                                                           \
  for((el)=(head);(el) && (tmp = (el)->NDRX_NEXT, 1); (el) = tmp)

/* these are identical to their singly-linked list counterparts */
#define DL_SEARCH_SCALAR LL_SEARCH_SCALAR
#define DL_SEARCH LL_SEARCH

/******************************************************************************
 * circular doubly linked list macros                                         *
 *****************************************************************************/
#define CDL_PREPEND(head,add)                                                                  \
do {                                                                                           \
 if (head) {                                                                                   \
   (add)->NDRX_PREV = (head)->NDRX_PREV;                                                       \
   (add)->NDRX_NEXT = (head);                                                                  \
   (head)->NDRX_PREV = (add);                                                                  \
   (add)->NDRX_PREV->NDRX_NEXT = (add);                                                        \
 } else {                                                                                      \
   (add)->NDRX_PREV = (add);                                                                   \
   (add)->NDRX_NEXT = (add);                                                                   \
 }                                                                                             \
 (head)=(add);                                                                                 \
} while (0)
/* Added for Enduro/X Queues: */
#define CDL_APPEND(head,add)                                                                   \
do {                                                                                           \
 if (head) {                                                                                   \
   (add)->NDRX_PREV = (head)->NDRX_PREV;                                                       \
   (add)->NDRX_NEXT = (head);                                                                  \
   (head)->NDRX_PREV = (add);                                                                  \
   (add)->NDRX_PREV->NDRX_NEXT = (add);                                                        \
 } else {                                                                                      \
   (add)->NDRX_PREV = (add);                                                                   \
   (add)->NDRX_NEXT = (add);                                                                   \
   (head)=(add);                                                                               \
 }                                                                                             \
} while (0)

#define CDL_DELETE(head,del)                                                                   \
do {                                                                                           \
  if ( ((head)==(del)) && ((head)->NDRX_NEXT == (head))) {                                     \
      (head) = 0L;                                                                             \
  } else {                                                                                     \
     (del)->NDRX_NEXT->NDRX_PREV = (del)->NDRX_PREV;                                           \
     (del)->NDRX_PREV->NDRX_NEXT = (del)->NDRX_NEXT;                                           \
     if ((del) == (head)) (head)=(del)->NDRX_NEXT;                                             \
  }                                                                                            \
} while (0);

#define CDL_FOREACH(head,el)                                                                   \
    for(el=head;el;el=(el->NDRX_NEXT==head ? 0L : el->NDRX_NEXT)) 

#define CDL_FOREACH_SAFE(head,el,tmp1,tmp2)                                                    \
  for((el)=(head), ((tmp1)=(head)?((head)->NDRX_PREV):NULL);                                   \
      (el) && ((tmp2)=(el)->NDRX_NEXT, 1);                                                     \
      ((el) = (((el)==(tmp1)) ? 0L : (tmp2))))

#define CDL_SEARCH_SCALAR(head,out,field,val)                                                  \
do {                                                                                           \
    CDL_FOREACH(head,out) {                                                                    \
      if ((out)->field == (val)) break;                                                        \
    }                                                                                          \
} while(0) 

#define CDL_SEARCH(head,out,elt,cmp)                                                           \
do {                                                                                           \
    CDL_FOREACH(head,out) {                                                                    \
      if ((cmp(out,elt))==0) break;                                                            \
    }                                                                                          \
} while(0) 

#endif /* UTLIST_INT_H */

