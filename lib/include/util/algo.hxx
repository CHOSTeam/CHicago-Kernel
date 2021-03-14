/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 04 of 2021, at 11:50 BRT
 * Last edited on March 14 of 2021, at 13:28 BRT */

#pragma once

#include <util/misc.hxx>

#ifdef _LP64
#define LOG2(x) (sizeof(UInt64) * 8 - __builtin_clzll(x) - 1)
#else
#define LOG2(x) (sizeof(UInt32) * 8 - __builtin_clz(x) - 1)
#endif

namespace CHicago {

inline constexpr UInt64 ConstHash(const Char *Data, UIntPtr Length, UInt64 Seed = 0) {
    /* MurmurHash64A implementation (should be good and fast enough for basic non-crypto safe hashing). Also, this
     * function expects that if you're gonna call us in a constexpr, the Data is a const Char* (that is, a string, else
     * the compiler will complain and error out). */

    const Char *end = Data + (Length >> 5);
    UInt64 ret = Seed ^ (Length * 0xC6A4A7935BD1E995);

    for (; Data < end; Data += 8) {
        UInt64 ch = ((static_cast<UInt64>(Data[0]) << 56) | (static_cast<UInt64>(Data[1]) << 48) |
                     (static_cast<UInt64>(Data[2]) << 40) | (static_cast<UInt64>(Data[3]) << 32) |
                     (static_cast<UInt32>(Data[4]) << 24) | (static_cast<UInt32>(Data[5]) << 16) |
                     (static_cast<UInt16>(Data[6]) << 8) | Data[6]) * 0xC6A4A7935BD1E995;
        ret = (ret ^ (ch ^ (ch >> 47)) * 0xC6A4A7935BD1E995) * 0xC6A4A7935BD1E995;
    }

    switch (Length & 7) {
        case 7: ret ^= static_cast<UInt64>(Data[6]) << 48;
        case 6: ret ^= static_cast<UInt64>(Data[5]) << 40;
        case 5: ret ^= static_cast<UInt64>(Data[4]) << 32;
        case 4: ret ^= static_cast<UInt32>(Data[3]) << 24;
        case 3: ret ^= static_cast<UInt32>(Data[2]) << 16;
        case 2: ret ^= static_cast<UInt16>(Data[1]) << 8;
        case 1: ret = (ret ^ Data[0]) * 0xC6A4A7935BD1E995;
    }

    return ret = (ret ^ (ret >> 47)) * 0xC6A4A7935BD1E995, ret ^ (ret >> 47);
}

inline UInt64 Hash(const Void *Data, UIntPtr Length, UInt64 Seed = 0) {
    return ConstHash(static_cast<const Char*>(Data), Length, Seed);
}

template<typename T, typename U> Void InsertionSort(T *Start, T *End, U Compare) {
    /* It's like bubble sort, but a bit faster (and that is why we don't even have a BubbleSort function). */

    for (T *i = Start + 1; i < End; i++) {
        for (T *j = i; j > Start && Compare(*j, j[-1]); j--) {
            Swap(*j, j[-1]);
        }
    }
}

template<typename T, typename U> static inline Void MakeHeap(T *Start, UIntPtr Root, UIntPtr End, U Compare) {
    /* Sift down through the array, putting the entry on the right place (this is a max binary heap, so bigger elements
     * will be closer to the start of the array. */

    while (2 * Root + 1 <= End) {
        UIntPtr i = 2 * Root + 1;

        if (i + 1 <= End && Compare(Start[i], Start[i + 1])) {
            i++;
        }

        if (Compare(Start[Root], Start[i + 1])) {
            Swap(Start[Root], Start[i]);
        }

        Root = i;
    }
}

template<typename T, typename U> Void HeapSort(T *Start, T *End, U Compare) {
    /* Now onto the main sorting algorithm: For small (less or equal to length 2) arrays, we don't need to do anything
     * (except possibly one swap if the length is 2). */

    if (Start >= End || End - Start <= 2) {
        if (Start < End && End - Start == 2 && Compare(Start[1], Start[0])) {
            Swap(Start[0], Start[1]);
        }

        return;
    }

    /* Build up the heap (sifting down each element into the right place), and then pop each element into the end of the
     * array. */

    for (IntPtr i = (End - Start - 2) / 2; i >= 0; i--) {
        MakeHeap(Start, i, End - Start - 1, Compare);
    }

   for (UIntPtr i = End - Start - 1; i > 0;) {
       Swap(Start[0], Start[i--]);
       MakeHeap(Start, 0, i, Compare);
   }
}

template<typename T, typename U> T *SortPartition(T *Start, T *End, U Compare) {
    /* Quick sort selection algorithm: Simply iterate from start to end, swapping elements around (based on the
     * condition from the compare function), by the end, we should have everything that satisfies the compare on the
     * left, and everything that doesn't on the right. */

    if (Start >= End) {
        return Start;
    }

    T *i = Start, *j = End - 1;

    for (; i < j;) {
        for (; Compare(*i); i++);
        for (; i < j && !Compare(*j); j--);

        if (i < j) {
            Swap(*i, *j);
        }
    }

    return j;
}

template<typename T, typename U> static inline Void Sort(T *Start, T *End, UIntPtr Depth, U Compare) {
    /* Use a loop so that we can remove one of the recursive calls. */

    while (Start < End) {
        /* Quick Sort+Heap Sort+Insertion Sort (also known as Introsort): First, check if we haven't reached the point
         * where quick sort starts being too inefficient, if we have, we can switch to heap sort (if we just reached
         * depth 0), or to insertion sort (if the length is less than 16). */

        if (End - Start <= 2) {
            if (End - Start == 2 && Compare(Start[1], Start[0])) {
                Swap(Start[0], Start[1]);
            }

            return;
        } else if (!Depth--) {
            HeapSort(Start, End, Compare);
            return;
        } else if (End - Start < 16) {
            InsertionSort(Start, End, Compare);
            return;
        }

        /* Else, just perform a recursive quick sort: Put the pivot in the end, partition the area between the start and
         * one lesser than the end (as in the end we have now the pivot), put the pivot back in place, and recursively
         * call ourselves in one half (and let the loop handle the other half). */

        Swap(Start[(End - Start) / 2], End[-1]);

        T *pivot = SortPartition(Start, End - 1, [=](T &a) { return Compare(a, End[-1]); });

        Swap(*pivot, End[-1]);
        Sort(Start, pivot, Depth, Compare);

        Start = pivot + 1;
    }
}

template<typename T, typename U> Void Sort(T *Start, T *End, U Compare) {
    Sort(Start, End, LOG2(End - Start) * 1.38629436111989 + 0.5, Compare);
}

}

#undef LOG2
