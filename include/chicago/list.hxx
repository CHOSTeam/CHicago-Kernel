/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 05 of 2020, at 20:57 BRT
 * Last edited on August 06 of 2020, at 11:34 BRT */

#ifndef __CHICAGO_LIST_HXX__
#define __CHICAGO_LIST_HXX__

#include <chicago/math.hxx>
#include <chicago/mm.hxx>

/* Wow, now, with C++, we can actually make the list class be strongly-typed!
 * Just one thing, we actually have to define and declare all the functions here in the header.. Well, no problems, as this
 * class is pretty small.
 * Btw, we can't include string.hxx here, as it also includes us. */

Void StrCopyMemory(Void *Buffer, const Void *Source, UIntPtr Length);
Void StrSetMemory(Void *Buffer, UInt8 Value, UIntPtr Length);
Void StrMoveMemory(Void *Buffer, const Void *Source, UIntPtr Length);

template<class T> class List {
public:
	typedef T *Iterator;
	typedef const T *ConstIterator;
	
	List(Void) : Elements(Null), Length(0), Capacity(0) { }
	List(UIntPtr Size) : Elements(Null), Length(0), Capacity(0) { Reserve(Size); }
	List(const List &Source) : Elements(Null), Length(0), Capacity(0) { Reserve(Source.Length); Add(Source); }
	~List(Void) { if (Elements != Null) { Clear(); Fit(); } }
	
	List &operator =(const List &Source) {
		/* Basic rule: If you are overwriting the copy constructor and the destructor, you also need to overwrite the copy
		 * operator. Thankfully for us, we just need to call Clear() and Add(). */
		
		if (this != &Source) {
			Clear();
			Reserve(Source.Length);
			Add(Source);
		}
		
		return *this;
	}
	
	Status Reserve(UIntPtr Size) {
		T *buf = Null;
		
		/* Allocating with new[] would call the destructor for all the items everytime we delete[] it, we don't want
		 * that, we want to be able to manually call the destructors using Remove(), and later just deallocate the
		 * memory without calling the destructors again (that is, without causing UB), so let's use the Heap::Allocate
		 * function (which is our malloc function). */
		
		if (Size <= Capacity) {
			return Status::InvalidArg;
		} else if ((buf = (T*)Heap::Allocate(sizeof(T) * Size)) == Null) {
			return Status::OutOfMemory;
		}
		
		/* Don't do the same mistake I did when I first wrote this function. Remember to check if this isn't the first
		 * allocation we're doing, if that's the case, we don't need to copy the old elements nor deallocate the them. */
		
		if (Elements != Null) {
			StrCopyMemory(buf, Elements, Length * sizeof(T));
			Heap::Deallocate(Elements);
		}
		
		Elements = buf;
		Capacity = Size;
		
		return Status::Success;
	}
	
	Status Fit(Void) {
		T *buf = Null;
		
		/* While the Reserve function allocates a buffer that can contain at least all the items that the user specified,
		 * this function deallocates any extra space, and fits the element buffer to make the capacity=length. If the
		 * length is 0 (for example, we were called on the destructor), we just need to free the buffer. */
		
		if (Elements == Null || Capacity == Length) {
			return Status::Success;
		} else if (Length == 0) {
			Heap::Deallocate(Elements);
			
			Elements = Null;
			Capacity = 0;
			
			return Status::Success;
		} else if ((buf = (T*)Heap::Allocate(sizeof(T) * Length)) == Null) {
			return Status::OutOfMemory;
		}
		
		StrCopyMemory(buf, Elements, Length * sizeof(T));
		Heap::Deallocate(Elements);
		
		Elements = buf;
		Capacity = Length;
		
		return Status::Success;
	}
	
	inline Void Clear(Void) { while (Length) { Elements[--Length].~T(); } }
	
	inline Status Add(const List &Source) { return Add(Source, Length); }
	inline Status Add(const T &Data) { return Add(Data, Length); }
	
	Status Add(const List &Source, UIntPtr Index) {
		Status status;
		
		for (const T &data : Source) {
			if ((status = Add(data, Index++)) != Status::Success) {
				return status;
			}
		}
		
		return Status::Success;
	}
	
	Status Add(const T &Data, UIntPtr Index) {
		/* As the elements are stored in one big array, setting the value (or even adding a new one and relocating the
		 * others) isn't hard. First, we need to check if we have enough space for adding the new item and relocating
		 * the others that are on the position/after the position forward (of course, this last part isn't required
		 * when adding beyond the current length), if there isn't enough space, we can call Reserve(), if this is the
		 * first entry, allocate * 2 items than the last allocation, else, the default allocation size is 2 (we can
		 * change this later). Before actually copying the data into the array, we need to move everything on the
		 * index forward (if it's not beyond the current length), we need to use MoveMemory for this, as the memory
		 * locations ARE going to overlap. */
		
		Status status;
		
		if (Index + (Index > Length) >= Capacity &&
			(status = Reserve(Capacity == 0 ? 2 : Capacity * 2)) != Status::Success) {
			return status;
		} else if (Index < Length) {
			StrMoveMemory(&Elements[Index + 1], &Elements[Index], sizeof(T) * (Length - Index));
		}
		
		Elements[Index] = Data;
		Length = Math::Max(Index, Length) + 1;
		
		return Status::Success;
	}
	
	Status Remove(UIntPtr Index) {
		/* No need to dealloc any memory here (the destructor and the Fit() functions are responsible for deallocing
		 * the memory), but we DO need to call T's destructor, and move the items like we do on Add, but we need to
		 * move the items backwards one slot this time, instead of forward one slot. */
		
		if (Index >= Length) {
			return Status::InvalidArg;
		}
		
		Elements[Index].~T();
		
		if (Index < --Length) {
			StrMoveMemory(&Elements[Index], &Elements[Index + 1], sizeof(T) * (Length - Index));
			StrSetMemory(&Elements[Length], 0, sizeof(T));
		} else {
			StrSetMemory(&Elements[Index], 0, sizeof(T));
		}
		
		return Status::Success;
	}
	
	template<typename U> Void Sort(U &&Compare) {
		MergeSort(Elements, Length, Compare);
	}
	
	UIntPtr GetLength(Void) const { return Length; }
	UIntPtr GetCapacity(Void) const { return Capacity; }
	
	/* begin() and end() are required for using ranged-for loops (equivalent to C# foreach loops, but the format is
	 * for (val : list)' instead of 'foreach (val in list)'). */
	
	Iterator begin(Void) { return Elements; }
	ConstIterator begin(Void) const { return Elements; }
	Iterator end(Void) { return Elements + Length; }
	ConstIterator end(Void) const { return Elements + Length; }
	
	T &operator [](UIntPtr Index) {
		/* For the non-const index operator, we can auto increase the length if we're accessing a region inside the
		 * 0->Capacity-1 range (accesing things outside this range is UB, and you're probably just going to page
		 * fault the kernel). */
		
		if (Index < Capacity && Index >= Length) {
			Length = Index + 1;
		}
		
		return Elements[Index];
	}
	
	const T &operator [](UIntPtr Index) const { return Elements[Index]; }
private:
	template<typename U> inline static Void MergeSort(T *Array, UIntPtr Length, U &&Compare) {
		/* Merge sort is a divide and conquerer sorting algorithm, we subdivide the array until we the length is <= 1,
		 * and we sort the subdivided arrays, best/worst/avg cases are O(nlogn), but the space complexity is O(n),
		 * we're allocing space on the stack for the temp array, so we may need to increase the kernel stack size in
		 * the future. Other choice would be implementing something like quick sort, or trying to implement an in-place
		 * merge sort algorithm. */
		
		if (Length <= 1) {
			return;
		}
		
		UIntPtr i = 0, j = 0, k = 0, llen = Length / 2, rlen = Length - llen;
		T left[llen], right[rlen];
		
		StrCopyMemory(left, Array, sizeof(T) * llen);
		StrCopyMemory(right, Array + llen, sizeof(T) * rlen);
		MergeSort(left, llen, Compare);
		MergeSort(right, rlen, Compare);
		
		for (; j < llen && k < rlen; i++) {
			T *src;
			
			if (Compare(left[j], right[k])) {
				src = &left[j++];
			} else {
				src = &right[k++];
			}
			
			StrCopyMemory(&Array[i], src, sizeof(T));
		}
		
		for (; j < llen; i++, j++) {
			StrCopyMemory(&Array[i], &left[j], sizeof(T));
		}
		
		for (; k < rlen; i++, k++) {
			StrCopyMemory(&Array[i], &right[k], sizeof(T));
		}
	}
	
	T *Elements;
	UIntPtr Length, Capacity;
};

#endif
