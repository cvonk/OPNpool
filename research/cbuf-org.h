/****************************************************************************
*
*   Since this code originated from code which is public domain, I
*   hereby declare this code to be public domain as well.
*
*   Derived more functionality from macros to C++ template.
*   Coert Vonk, 2015
*
*   loosely based on http://www.keil.com/download/docs/200.asp
****************************************************************************/

/****************************************************************************
*
*   Since this code originated from code which is public domain, I
*   hereby declare this code to be public domain as well.
*
*   Dave Hylands - dhylands@gmail.com
*
****************************************************************************/
/**
*
*   @file   CBUF.h
*
*   @defgroup   CBUF Circular Buffer
*   @{
*
*   @brief  A simple and efficient set of circular buffer manipulations.
*
*   These macros implement a circular buffer which employs get and put
*   pointers, in such a way that mutual exclusion is not required
*   (assumes one reader & one writer).
*
*   It requires that the circular buffer size be a power of two, and the
*   size of the buffer needs to smaller than the index. So an 8 bit index
*   supports a circular buffer up to ( 1 << 7 ) = 128 entries, and a 16 bit index
*   supports a circular buffer up to ( 1 << 15 ) = 32768 entries.
*
*   The basis for these routines came from an article in Jack Ganssle's
*   Embedded Muse: http://www.ganssle.com/tem/tem110.pdf
*
*   In order to offer the most amount of flexibility for embedded environments
*   you need to define a macro for the size.
*
*   First, you need to name your circular buffer. For this example, we'll
*   call it @c myQ.
*
*   The size macro that needs to be defined will be the name of the
*   circular buffer followed by @c _SIZE. The size must be a power of two
*   and it needs to fit in the get/put indexes. i.e. if you use an
*   8 bit index, then the maximum supported size would be 128.
*
*   The structure which defines the circular buffer needs to have 3 members
*   <tt>m_popIdx, m_pushIdx,</tt> and @c m_entry.
*
*   @c m_popIdx and @c m_pushIdx need to be unsigned integers of the same size.
*
*   @c m_entry needs to be an array of @c xxx_SIZE entries, or a pointer to an
*   array of @c xxx_SIZE entries. The type of each entry is entirely up to the
*   caller.
*
*   @code
*   #define myQ_SIZE    64
*
*   volatile struct
*   {
*       uint8_t     m_popIdx;
*       uint8_t     m_pushIdx;
*       uint8_t     m_entry[ myQ_SIZE ];
*
*   } myQ;
*   @endcode
*
*   You could then use CBUF_Push to add a character to the circular buffer:
*
*   @code
*   CBUF_Push( myQ, 'x' );
*   @endcode
*
*   And CBUF_Pop to retrieve an entry from the buffer:
*
*   @code
*   ch = CBUF_Pop( myQ );
*   @endcode
*
*   If you happen to prefer to use C++ instead, there is a template
*   version which requires no macros. You just declare 3 template parameters:
*
*       - The type that should be used for the index
*       - The size of the circular buffer
*       - The type that should be used for the entry
*
*   For example:
*   @code
*   volatile CBUF< uint8_t, 64, char >   myQ;
*   @endcode
*
****************************************************************************/

#pragma once

#if defined(__cplusplus)

template < class IndexType, unsigned size, class EntryType >
class CBUF {

public:
    // initializes the circular buffer for use
    CBUF()
    {
        popIdx = pushIdx = 0;
    }


    /******
     * push
     ******/

    // inserts an entry at the head
    void push( EntryType val )
    {
        entry[pushIdx++ & (size - 1)] = val;
    }

    // returns a pointer to the next entry to push
    //   use in conjunction with pushAdvanceIdx() to fill out an entry before indicating that it's
    //   available.
    EntryType * const pushNextPtr()
    {
        return &entry[pushIdx & (size - 1)];
    }

    void pushAdvanceIdx( void )
    {
        pushIdx++;
    }

    /*****
     * pop
     *****/

    // returns entry at the tail (least recent)
    EntryType const pop()
    {
        return entry[popIdx++ & (size - 1)];
    }

    // returns ptr to entry at the tail (least recent)
    EntryType * const popPtr( void)
    {
        return &(entry[popIdx++ & (size - 1)]);
    }

    // advances the get index
    //   slightly more efficient than popping and tossing the result
    void popAdvanceIdx( void )
    {
        popIdx++;
    }

    /******
     * util
     ******/

    // returns ptr to the n'th entry from the tail (least recent)
    EntryType * const getTailPtr( IndexType const n)
    {
        return &entry[(popIdx + n) & (size - 1)];
    }

    // returns ptr to the n'th entry from the head (most recent)
    EntryType * const getHeadPtr( IndexType const n)
    {
        return &entry[(pushIdx - n - 1) & (size - 1)];
    }

    // returns the number of entries which are currently contained in the circular buffer
    IndexType len() const
    {
        return pushIdx - popIdx;
    }

    // determines if the circular buffer is empty.
    bool isEmpty() const
    {
        return len() == 0;
    }
    
    // determines if the circular buffer is full
    bool isFull() const
    {
        return len() == size;
    }

    // determines if the circular buffer is currently overflowed or underflowed
    bool error() const
    {
        return len() > size;
    }

#if 0
    // retrieves the n'th entry from the tail (least recent)
    EntryType getFromTail( IndexType const n ) const
    {
        return entry[(popIdx + n) & (size - 1)];
    }

    // retrieves the n'th entry from the head (most recent)
    EntryType getFromHead( IndexType const n ) const
    {
        return entry[(pushIdx - n - 1) & (size - 1)];
    }
#endif

private:

    volatile IndexType  popIdx;   // index of least recent entry in the (tail)
    volatile IndexType  pushIdx;  // index of 1 ahead of the most recent entry in the buffer (head+1)
    EntryType           entry[size];
};

#else // __cplusplus

// initializes the circular buffer for use
#define CBUF_Init(cbuf)  cbuf.m_popIdx = cbuf.m_pushIdx = 0

// returns the number of entries which are currently contained in the circular buffer
#define CBUF_Len(cbuf)  ((typeof( cbuf.m_pushIdx ))(( cbuf.m_pushIdx ) - ( cbuf.m_popIdx )))

// appends an entry to the end of the circular buffer.
// The entry is expected to be of the same type as the m_entry member
#define CBUF_Push(cbuf, elem)  (cbuf.m_entry)[ cbuf.m_pushIdx++ & (( cbuf##_SIZE ) - 1 )] = (elem)

// retrieves an entry from the beginning of the circular buffer
#define CBUF_Pop(cbuf)  (cbuf.m_entry)[ cbuf.m_popIdx++ & (( cbuf##_SIZE ) - 1 )]

// returns a pointer to the last spot that was pushed
#define CBUF_GetLastEntryPtr(cbuf)  &(cbuf.m_entry)[ ( cbuf.m_pushIdx - 1 ) & (( cbuf##_SIZE ) - 1 )]

// Returns a pointer to the next spot to push. This can be used in conjunction with
// CBUF_pushAdvanceIdx to fill out an entry before indicating that it's available.
// It is the caller's responsibility to ensure that space is available, and that no
// other items are pushed to overwrite the entry returned.
#define CBUF_pushNextPtr(cbuf)  &(cbuf.m_entry)[ cbuf.m_pushIdx & (( cbuf##_SIZE ) - 1 )]

// Advances the put index. This is useful if you need to reserve space for an item but can't fill
// in the contents yet. CBUG_GetLastEntryPtr can be used to get a pointer to the item.
// It is the caller's responsibility to ensure that the item isn't popped before the contents are
// filled in.
#define CBUF_pushAdvanceIdx(cbuf)  cbuf.m_pushIdx++

// advances the get index. This is slightly more efficient than popping and tossing the result
#define CBUF_popAdvanceIdx(cbuf)  cbuf.m_popIdx++

// retrieves the n'th entry from the beginning of the circular buffer
#define CBUF_Get(cbuf, n)  (cbuf.m_entry)[( cbuf.m_popIdx + n ) & (( cbuf##_SIZE ) - 1 )]

// retrieves the n'th entry from the end of the circular buffer
#define CBUF_GetEnd(cbuf, idx)  (cbuf.m_entry)[( cbuf.m_pushIdx - n - 1 ) & (( cbuf##_SIZE ) - 1 )]

// returns a pointer to the next spot to push
#define CBUF_tailPtr(cbuf)  &(cbuf.m_entry)[ cbuf.m_popIdx & (( cbuf##_SIZE ) - 1 )]

// determines if the circular buffer is empty.
#define CBUF_IsEmpty(cbuf)  ( CBUF_Len( cbuf ) == 0 )

// determines if the circular buffer is full
#define CBUF_IsFull(cbuf)  ( CBUF_Len( cbuf ) == ( cbuf##_SIZE ))

// determines if the circular buffer is currently overflowed or underflowed
#define CBUF_Error(cbuf)  ( CBUF_Len( cbuf ) > cbuf##_SIZE )

#endif  // __cplusplus
