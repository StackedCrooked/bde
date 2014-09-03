// bslh_spookyhashalgorithm.t.cpp                                     -*-C++-*-
#include <bslh_spookyhashalgorithm.h>

#include <bslmf_isbitwisemoveable.h>
#include <bslmf_issame.h>

#include <bsls_assert.h>
#include <bsls_asserttest.h>
#include <bsls_bsltestutil.h>
#include <bsls_platform.h>
#include <bsls_bsltestutil.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

using namespace BloombergLP;
using namespace bslh;


//=============================================================================
//                                  TEST PLAN
//-----------------------------------------------------------------------------
//                                  Overview
//                                  --------
// The component under test is a 'bslh' hashing algorithm.  The basic test plan
// is to compare the output of the function call operator with the expected
// output generated by a known-good implementation of the hashing algorithm.
// The component will also be tested for conformance to the requirements on
// 'bslh' hashing algorithms, outlined in the 'bslh' package level
// documentation.
//-----------------------------------------------------------------------------
// TYPEDEF
// [ 4] typedef bsls::Types::Uint64 result_type;
//
// CONSTANTS
// [ 5] enum { k_SEED_LENGTH = 16 };
//
// CREATORS
// [ 2] SpookyHashAlgorithm();
// [ 2] SpookyHashAlgorithm(const char *seed);
// [ 2] ~SpookyHashAlgorithm();
//
// MANIPULATORS
// [ 3] void operator()(void const* key, size_t len);
// [ 3] result_type computeHash();
// ----------------------------------------------------------------------------
// [ 1] BREATHING TEST
// [ 6] Trait IsBitwiseMoveable
// [ 7] USAGE EXAMPLE
//-----------------------------------------------------------------------------

// ============================================================================
//                      STANDARD BDE ASSERT TEST MACROS
// ----------------------------------------------------------------------------
// NOTE: THIS IS A LOW-LEVEL COMPONENT AND MAY NOT USE ANY C++ LIBRARY
// FUNCTIONS, INCLUDING IOSTREAMS.

namespace {

int testStatus = 0;

void aSsErT(bool b, const char *s, int i)
{
    if (b) {
        printf("Error " __FILE__ "(%d): %s    (failed)\n", i, s);
        if (testStatus >= 0 && testStatus <= 100) ++testStatus;
    }
}

}  // close unnamed namespace

// ============================================================================
//                      STANDARD BDE TEST DRIVER MACROS
// ----------------------------------------------------------------------------

#define ASSERT       BSLS_BSLTESTUTIL_ASSERT
#define LOOP_ASSERT  BSLS_BSLTESTUTIL_LOOP_ASSERT
#define LOOP0_ASSERT BSLS_BSLTESTUTIL_LOOP0_ASSERT
#define LOOP1_ASSERT BSLS_BSLTESTUTIL_LOOP1_ASSERT
#define LOOP2_ASSERT BSLS_BSLTESTUTIL_LOOP2_ASSERT
#define LOOP3_ASSERT BSLS_BSLTESTUTIL_LOOP3_ASSERT
#define LOOP4_ASSERT BSLS_BSLTESTUTIL_LOOP4_ASSERT
#define LOOP5_ASSERT BSLS_BSLTESTUTIL_LOOP5_ASSERT
#define LOOP6_ASSERT BSLS_BSLTESTUTIL_LOOP6_ASSERT
#define ASSERTV      BSLS_BSLTESTUTIL_ASSERTV

#define Q   BSLS_BSLTESTUTIL_Q   // Quote identifier literally.
#define P   BSLS_BSLTESTUTIL_P   // Print identifier and value.
#define P_  BSLS_BSLTESTUTIL_P_  // P(X) without '\n'.
#define T_  BSLS_BSLTESTUTIL_T_  // Print a tab (w/o newline).
#define L_  BSLS_BSLTESTUTIL_L_  // current Line number

// ============================================================================
//                  NEGATIVE-TEST MACRO ABBREVIATIONS
// ----------------------------------------------------------------------------

#define ASSERT_SAFE_PASS(EXPR) BSLS_ASSERTTEST_ASSERT_SAFE_PASS(EXPR)
#define ASSERT_SAFE_FAIL(EXPR) BSLS_ASSERTTEST_ASSERT_SAFE_FAIL(EXPR)
#define ASSERT_PASS(EXPR)      BSLS_ASSERTTEST_ASSERT_PASS(EXPR)
#define ASSERT_FAIL(EXPR)      BSLS_ASSERTTEST_ASSERT_FAIL(EXPR)
#define ASSERT_OPT_PASS(EXPR)  BSLS_ASSERTTEST_ASSERT_OPT_PASS(EXPR)
#define ASSERT_OPT_FAIL(EXPR)  BSLS_ASSERTTEST_ASSERT_OPT_FAIL(EXPR)

// ============================================================================
//                  PRINTF FORMAT MACRO ABBREVIATIONS
// ----------------------------------------------------------------------------

#define ZU BSLS_BSLTESTUTIL_FORMAT_ZU

//=============================================================================
//                             USAGE EXAMPLE
//-----------------------------------------------------------------------------
///Usage
///-----
// This section illustrates intended usage of this component.
//
///Example: Creating and Using a Hash Table
/// - - - - - - - - - - - - - - - - - - - -
// Suppose we have any array of types that define 'operator==', and we want a
// fast way to find out if values are contained in the array.  We can create a
// 'HashTable' data structure that is capable of looking up values in O(1)
// time.
//
// Further suppose that we will be storing futures (the financial instruments)
// in this table.  Since futures have standardized names, we don't have to
// worry about any malicious values causing collisions.  We will want to use a
// general purpose hashing algorithm with a good hash distribution and good
// speed.  This algorithm will need to be in the form of a hash functor -- an
// object that will take objects stored in our array as input, and yield a
// 64-bit int value.  The functor can pass the attributes of the 'TYPE' that
// are salient to hashing into the hashing algorithm, and then return the hash
// that is produced.
//
// We can use the result of the hash function to index into our array of
// 'buckets'.  Each 'bucket' is simply a pointer to a value in our original
// array of 'TYPE' objects.
//
// First, we define our 'HashTable' template class, with the two type
// parameters: 'TYPE' (the type being referenced) and 'HASHER' (a functor that
// produces the hash).

    template <class TYPE, class HASHER>
    class HashTable {
        // This class template implements a hash table providing fast lookup of
        // an external, non-owned, array of values of (template parameter)
        // 'TYPE'.
        //
        // The (template parameter) 'TYPE' shall have a transitive, symmetric
        // 'operator==' function.  There is no requirement that it have any
        // kind of creator defined.
        //
        // The 'HASHER' template parameter type must be a functor with a method
        // having the following signature:
        //..
        //  size_t operator()(TYPE)  const;
        //                   -OR-
        //  size_t operator()(const TYPE&) const;
        //..
        // and 'HASHER' shall have a publicly accessible default constructor
        // and destructor.
        //
        // Note that this hash table has numerous simplifications because we
        // know the size of the array and never have to resize the table.

        // DATA
        const TYPE       *d_values;          // Array of values table is to
                                             // hold
        size_t            d_numValues;       // Length of 'd_values'.
        const TYPE      **d_bucketArray;     // Contains ptrs into 'd_values'
        unsigned          d_bucketArrayMask; // Will always be '2^N - 1'.
        HASHER            d_hasher;          // User supplied hashing algorithm


      private:
        // PRIVATE ACCESSORS
        bool lookup(size_t      *idx,
                    const TYPE&  value,
                    size_t       hashValue) const;
            // Look up the specified 'value', having the specified 'hashValue',
            // and load its index in 'd_bucketArray' into the specified 'idx'.
            // If not found, return the vacant entry in 'd_bucketArray' where
            // it should be inserted.  Return 'true' if 'value' is found and
            // 'false' otherwise.

      public:
        // CREATORS
        HashTable(const TYPE *valuesArray,
                  size_t      numValues);
            // Create a hash table referring to the specified 'valuesArray'
            // having length of the specified 'numValues'.  No value in
            // 'valuesArray' shall have the same value as any of the other
            // values in 'valuesArray'

        ~HashTable();
            // Free up memory used by this hash table.

        // ACCESSORS
        bool contains(const TYPE& value) const;
            // Return true if the specified 'value' is found in the table and
            // false otherwise.
    };

// Then, we define a 'Future' class, which holds a c-string 'name', char
// 'callMonth', and short 'callYear'.

    class Future {
        // This class identifies a future contract.  It tracks the name, call
        // month and year of the contract it represents, and allows equality
        // comparison.

        // DATA
        const char *d_name;    // held, not owned
        const char  d_callMonth;
        const short d_callYear;

      public:
        // CREATORS
        Future(const char *name, const char callMonth, const short callYear)
        : d_name(name), d_callMonth(callMonth), d_callYear(callYear)
            // Create a 'Future' object out of the specified 'name',
            // 'callMonth', and 'callYear'.
        {}

        Future() : d_name(""), d_callMonth('\0'), d_callYear(0)
            // Create a 'Future' with default values.
        {}

        // ACCESSORS
        const char * getMonth() const
            // Return the month that this future expires.
        {
            return &d_callMonth;
        }

        const char * getName() const
            // Return the name of this future
        {
            return d_name;
        }

        const short * getYear() const
            // Return the year that this future expires
        {
            return &d_callYear;
        }

        bool operator==(const Future& other) const
            // Compare this to the specified 'other' object and return true if
            // they are equal
        {
            return (!strcmp(d_name, other.d_name))  &&
               d_callMonth == other.d_callMonth &&
               d_callYear  == other.d_callYear;
        }
    };

    bool operator!=(const Future& lhs, const Future& rhs)
        // Compare compare the specified 'lhs' and 'rhs' objects and return
        // true if they are not equal
    {
        return !(lhs == rhs);
    }

// Next, we need a hash functor for 'Future'.  We are going to use the
// 'SpookyHashAlgorithm' because it is a fast, general purpose hashing
// algorithm that will provide an easy way to combine the attributes of
// 'Future' objects that are salient to hashing into one reasonable hash that
// will distribute the items evenly throughout the hash table.

    struct HashFuture {
        // This struct is a  functor that will apply the SpookyHashAlgorithm to
        // objects of type 'Future'.

        size_t operator()(const Future& future) const
            // Return the hash of the of the specified 'future'.  Note that
            // this uses the 'SpookyHashAlgorithm' to quickly combine the
            // attributes of 'Future' objects that are salient to hashing into
            // a hash suitable for a hash table.
        {
            SpookyHashAlgorithm hash;

            hash(future.getName(),  strlen(future.getName()));
            hash(future.getMonth(), sizeof(char));
            hash(future.getYear(),  sizeof(short));

            return static_cast<size_t>(hash.computeHash());
        }
    };

//=============================================================================
//                     ELIDED USAGE EXAMPLE IMPLEMENTATIONS
//-----------------------------------------------------------------------------

// PRIVATE ACCESSORS
template <class TYPE, class HASHER>
bool HashTable<TYPE, HASHER>::lookup(size_t      *idx,
                                     const TYPE&  value,
                                     size_t       hashValue) const
{
    const TYPE *ptr;
    for (*idx = hashValue & d_bucketArrayMask; (ptr = d_bucketArray[*idx]);
                                   *idx = (*idx + 1) & d_bucketArrayMask) {
        if (value == *ptr) {
            return true;                                              // RETURN
        }
    }
    // value was not found in table

    return false;
}

// CREATORS
template <class TYPE, class HASHER>
HashTable<TYPE, HASHER>::HashTable(const TYPE *valuesArray,
                                   size_t      numValues)
: d_values(valuesArray)
, d_numValues(numValues)
, d_hasher()
{
    size_t bucketArrayLength = 4;
    while (bucketArrayLength < numValues * 4) {
        bucketArrayLength *= 2;

    }
    d_bucketArrayMask = bucketArrayLength - 1;
    d_bucketArray = new const TYPE *[bucketArrayLength];
    memset(d_bucketArray,  0, bucketArrayLength * sizeof(TYPE *));

    for (unsigned i = 0; i < numValues; ++i) {
        const TYPE& value = d_values[i];
        size_t idx;
        BSLS_ASSERT_OPT(!lookup(&idx, value, d_hasher(value)));
        d_bucketArray[idx] = &d_values[i];
    }
}

template <class TYPE, class HASHER>
HashTable<TYPE, HASHER>::~HashTable()
{
    delete [] d_bucketArray;
}

// ACCESSORS
template <class TYPE, class HASHER>
bool HashTable<TYPE, HASHER>::contains(const TYPE& value) const
{
    size_t idx;
    return lookup(&idx, value, d_hasher(value));
}


//=============================================================================
//                     GLOBAL TYPEDEFS FOR TESTING
//-----------------------------------------------------------------------------

typedef SpookyHashAlgorithm Obj;
typedef BloombergLP::bsls::Types::Uint64 Uint64;

// ============================================================================
//                            MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int                 test = argc > 1 ? atoi(argv[1]) : 0;
    bool             verbose = argc > 2;
    bool         veryVerbose = argc > 3;
    bool     veryVeryVerbose = argc > 4;
//  bool veryVeryVeryVerbose = argc > 5;

    switch (test) { case 0:
      case 7: {
        // --------------------------------------------------------------------
        // USAGE EXAMPLE
        //   The hashing algorithm can be used to create more powerful
        //   components such as functors that can be used to power hash tables.
        //
        // Concerns:
        //: 1 The usage example provided in the component header file compiles,
        //:   links, and runs as shown.
        //
        // Plan:
        //: 1 Incorporate usage example from header into test driver (C-1)
        //
        // Testing:
        //   USAGE EXAMPLE
        // --------------------------------------------------------------------

        if (verbose) printf("USAGE EXAMPLE\n"
                            "=============\n");

// Then, we want to actually use our hash table on 'Future' objects.  We
// create an array of 'Future's based on data that was originally from some
// external source:

        Future futures[] = { Future("Swiss Franc", 'F', 2014),
                             Future("US Dollar", 'G', 2015),
                             Future("Canadian Dollar", 'Z', 2014),
                             Future("British Pound", 'M', 2015),
                             Future("Deutsche Mark", 'X', 2016),
                             Future("Eurodollar", 'Q', 2017)};
        enum { NUM_FUTURES = sizeof futures / sizeof *futures };

// Next, we create our HashTable 'hashTable'.  We pass the functor that we
// defined above as the second argument:

        HashTable<Future, HashFuture> hashTable(futures, NUM_FUTURES);

// Now, we verify that each element in our array registers with count:
        for ( int i = 0; i < 6; ++i) {
            ASSERT(hashTable.contains(futures[i]));
        }

// Finally, we verify that futures not in our original array are correctly
// identified as not being in the set:

        ASSERT(!hashTable.contains(Future("French Franc", 'N', 2019)));
        ASSERT(!hashTable.contains(Future("Swiss Franc", 'X', 2014)));
        ASSERT(!hashTable.contains(Future("US Dollar", 'F', 2014)));

      } break;
      case 6: {
        // --------------------------------------------------------------------
        // TESTING BDE TYPE TRAITS
        //   The class is bitwise movable and should have a trait that
        //   indicates that.
        //
        // Concerns:
        //: 1 The class is marked as 'IsBitwiseMoveable'.
        //
        // Plan:
        //: 1 ASSERT the presence of the trait using the 'bslalg::HasTrait'
        //:   metafunction. (C-1)
        //
        // Testing:
        //   Trait IsBitwiseMoveable
        // --------------------------------------------------------------------

        if (verbose) printf("\nTESTING BDE TYPE TRAITS"
                            "\n=======================\n");

        if (verbose) printf("ASSERT the presence of the trait using the"
                            " 'bslalg::HasTrait' metafunction. (C-1)\n");
        {
            ASSERT(bslmf::IsBitwiseMoveable<SpookyHashAlgorithm>::value);
        }

      } break;
      case 5: {
        // --------------------------------------------------------------------
        // TESTING 'k_SEED_LENGTH'
        //   The class is a seeded algorithm and should expose a
        //   'k_SEED_LENGTH' enum.
        //
        // Concerns:
        //: 1 'k_SEED_LENGTH' is publicly accessible.
        //:
        //: 2 'k_SEED_LENGTH' is set to 16.
        //
        // Plan:
        //: 1 Access 'k_SEED_LENGTH' and ASSERT it is equal to the expected
        //:   value. (C-1,2)
        //
        // Testing:
        //   enum { k_SEED_LENGTH = 16 };
        // --------------------------------------------------------------------

        if (verbose) printf("\nTESTING 'k_SEED_LENGTH'"
                            "\n=======================\n");

        if (verbose) printf("Access 'k_SEED_LENGTH' and ASSERT it is equal to"
                            " the expected value. (C-1,2)\n");
        {
            ASSERT(16 == SpookyHashAlgorithm::k_SEED_LENGTH);
        }

      } break;
      case 4: {
        // --------------------------------------------------------------------
        // TESTING 'result_type' TYPEDEF
        //   Verify that the class offers the result_type typedef that needs to
        //   be exposed by all 'bslh' hashing algorithms
        //
        // Concerns:
        //: 1 The typedef 'result_type' is publicly accessible and an alias for
        //:   'bsls::Types::Uint64'.
        //:
        //: 2 'computeHash()' returns 'result_type'
        //
        // Plan:
        //: 1 ASSERT the typedef is accessible and is the correct type using
        //:   'bslmf::IsSame'. (C-1)
        //:
        //: 2 Declare the expected signature of 'computeHash()' and then assign
        //:   to it.  If it compiles, the test passes. (C-2)
        //
        // Testing:
        //   typedef bsls::Types::Uint64 result_type;
        // --------------------------------------------------------------------

        if (verbose) printf("\nTESTING 'result_type' TYPEDEF"
                            "\n=============================\n");

        if (verbose) printf("ASSERT the typedef is accessible and is the"
                            " correct type using 'bslmf::IsSame'. (C-1)\n");
        {
            ASSERT((bslmf::IsSame<bsls::Types::Uint64,
                                  Obj::result_type>::VALUE));
        }

        if (verbose) printf("Declare the expected signature of 'computeHash()'"
                            " and then assign to it.  If it compiles, the test"
                            " passes. (C-2)\n");
        {
            Obj::result_type (Obj::*expectedSignature) ();

            expectedSignature = &Obj::computeHash;
        }

      } break;
      case 3: {
        // --------------------------------------------------------------------
        // TESTING TESTING 'operator()' AND 'computeHash()'
        //   Verify the class provides an overload for the function call
        //   operator that can be called with some bytes and a length.  Verify
        //   that calling 'operator()' will permute the algorithm's internal
        //   state as specified by SpookyHash.  Verify that 'computeHash()'
        //   returns the final value specified by the canonical spooky hash
        //   implementation.
        //
        // Concerns:
        //: 1 The function call operator is callable.
        //:
        //: 2 Given the same bytes, the function call operator will permute the
        //:   internal state of the algorithm in the same way, regardless of
        //:   whether the bytes are passed in all at once or in pieces.
        //:
        //: 3 Byte sequences passed in to 'operator()' with a length of 0 will
        //:   not contribute to the final hash
        //:
        //: 4 'computeHash()' and returns the appropriate value
        //:   according to the SpookyHash specification.
        //:
        //: 5 'operator()' does a BSLS_ASSERT for null pointers.
        //
        // Plan:
        //: 1 Insert various lengths of c-strings into the algorithm both all
        //:   at once and char by char using 'operator()'.  Assert that the
        //:   algorithm produces the same result in both cases. (C-1,2)
        //:
        //: 2 Hash c-strings all at once and with multiple calls to
        //:   'operator()' with length 0.  Assert that both methods of hashing
        //:   c-strings produce the same values.(C-3)
        //:
        //: 3 Check the output of 'computeHash()' against the expected results
        //:   from a known good version of the algorithm. (C-4)
        //:
        //: 4 Call 'operator()' with a null pointer. (C-5)
        //
        // Testing:
        //   void operator()(void const* key, size_t len);
        //   result_type computeHash();
        // --------------------------------------------------------------------

        if (verbose) printf(
                       "\nTESTING TESTING 'operator()' AND 'computeHash()'"
                       "\n================================================\n");

        static const struct {
            int                  d_line;
            const char           d_value [21];
            bsls::Types::Uint64  d_expectedHash;
        } DATA[] = {
#ifdef BSLS_PLATFORM_IS_LITTLE_ENDIAN
        // LINE DATA               HASH
         {  L_,                     "1",  7002407594712107506ULL,},
         {  L_,                    "12", 16818667202782586754ULL,},
         {  L_,                   "123", 14962907208936606339ULL,},
         {  L_,                  "1234", 13841264755844795919ULL,},
         {  L_,                 "12345",  4217007344502432836ULL,},
         {  L_,                "123456", 15406725965008216808ULL,},
         {  L_,               "1234567",  9451242472090273442ULL,},
         {  L_,              "12345678", 10759732512414454861ULL,},
         {  L_,             "123456789",   972485933408677714ULL,},
         {  L_,            "1234567890",  5673121172557267903ULL,},
         {  L_,           "12345678901", 13862553145028760004ULL,},
         {  L_,          "123456789012", 15680761296024743980ULL,},
         {  L_,         "1234567890123",  9130538754397985015ULL,},
         {  L_,        "12345678901234", 18108216297703361154ULL,},
         {  L_,       "123456789012345", 14690529358019617166ULL,},
         {  L_,      "1234567890123456", 10018078431842207320ULL,},
         {  L_,     "12345678901234567", 18401305144349413810ULL,},
         {  L_,    "123456789012345678", 16182063539957820126ULL,},
         {  L_,   "1234567890123456789", 16302264721124054926ULL,},
         {  L_,  "12345678901234567890",  7953951658377832965ULL,},
#else
        // LINE DATA               HASH
         {  L_,                     "1",  7002407594712107506ULL,},
         {  L_,                    "12", 16818667202782586754ULL,},
         {  L_,                   "123", 14962907208936606339ULL,},
         {  L_,                  "1234",  6699426530780107312ULL,},
         {  L_,                 "12345",  8676331252213764608ULL,},
         {  L_,                "123456",  4550369474379901410ULL,},
         {  L_,               "1234567", 16791262952135217002ULL,},
         {  L_,              "12345678", 16425013147235105742ULL,},
         {  L_,             "123456789",  5214485795892544707ULL,},
         {  L_,            "1234567890", 12261113520408985235ULL,},
         {  L_,           "12345678901",  2168166662679930866ULL,},
         {  L_,          "123456789012", 12754308552622014786ULL,},
         {  L_,         "1234567890123", 14297370022220374180ULL,},
         {  L_,        "12345678901234", 16893477830408992202ULL,},
         {  L_,       "123456789012345",  9435207994519636836ULL,},
         {  L_,      "1234567890123456",  9862342523424011374ULL,},
         {  L_,     "12345678901234567",  1788935288602307262ULL,},
         {  L_,    "123456789012345678",  4717428848732367803ULL,},
         {  L_,   "1234567890123456789", 17814078940325658812ULL,},
         {  L_,  "12345678901234567890", 10441832432953046058ULL,},
#endif
        };
        const int NUM_DATA = sizeof DATA / sizeof *DATA;

        if (verbose) printf("Insert various lengths of c-strings into the"
                            " algorithm both all at once and char by char"
                            " using 'operator()'.  Assert that the algorithm"
                            " produces the same result in both cases. (C-1,2)"
                            "\n");
        {
            for (int i = 0; i != NUM_DATA; ++i) {
                const int   LINE  = DATA[i].d_line;
                const char *VALUE = DATA[i].d_value;

                if (veryVerbose) printf("Hashing: %s\n", VALUE);

                Obj contiguousHash;
                Obj dispirateHash;

                contiguousHash(VALUE, strlen(VALUE));
                for (unsigned int j = 0; j < strlen(VALUE); ++j){
                    if (veryVeryVerbose) printf("Hashing by char: %c\n",
                                                                     VALUE[j]);
                    dispirateHash(&VALUE[j], sizeof(char));
                }

                LOOP_ASSERT(LINE, contiguousHash.computeHash() ==
                                                  dispirateHash.computeHash());
            }
        }

        if (verbose) printf("Hash c-strings all at once and with multiple"
                            " calls to 'operator()' with length 0.  Assert"
                            " that both methods of hashing c-strings produce"
                            " the same values.(C-3)\n");
        {
            for (int i = 0; i != NUM_DATA; ++i) {
                const int   LINE  = DATA[i].d_line;
                const char *VALUE = DATA[i].d_value;

                if (veryVerbose) printf("Hashing: %s\n", VALUE);

                Obj contiguousHash;
                Obj dispirateHash;

                contiguousHash(VALUE, strlen(VALUE));
                for (unsigned int j = 0; j < strlen(VALUE); ++j){
                    if (veryVeryVerbose) printf("Hashing by char: %c\n",
                                                                     VALUE[j]);
                    dispirateHash(&VALUE[j], sizeof(char));
                    dispirateHash(VALUE, 0);
                }

                LOOP_ASSERT(LINE, contiguousHash.computeHash() ==
                                                  dispirateHash.computeHash());
            }
        }

        if (verbose) printf("Check the output of 'computeHash()' against the"
                            " expected results from a known good version of"
                            " the algorithm. (C-4)\n");
        {
            for (int i = 0; i != NUM_DATA; ++i) {
                const int                LINE  = DATA[i].d_line;
                const char              *VALUE = DATA[i].d_value;
                const unsigned long long HASH  = DATA[i].d_expectedHash;

                if (veryVerbose) printf("Hashing: %s, Expecting: %llu\n",
                                        VALUE,
                                        HASH);

                Obj hash;
                hash(VALUE, strlen(VALUE));
                LOOP_ASSERT(LINE, hash.computeHash() == HASH);
            }
        }

        if (verbose) printf("Call 'operator()' with null pointers. (C-5)\n");
        {
            const char data[5] = {'a', 'b', 'c', 'd', 'e'};

            bsls::AssertFailureHandlerGuard
                                           g(bsls::AssertTest::failTestDriver);

            ASSERT_FAIL(Obj().operator()(   0, 5));
            ASSERT_PASS(Obj().operator()(data, 5));
        }

      } break;
      case 2: {
        // --------------------------------------------------------------------
        // TESTING CREATORS
        //   Ensure that the implicit destructor as well as the explicit
        //   default and parameterized constructors are publicly callable.
        //   Verify that the algorithm can be instantiated with or without a
        //   seed.  Note that a null pointer is not tested here, because there
        //   is no way to perform a BSLS_ASSERT before dereferenceing the
        //   pointer (without a performance penalty).
        //
        // Concerns:
        //: 1 Objects can be created using the default constructor.
        //:
        //: 2 Objects can be created using the parameterized constructor.
        //:
        //: 3 Objects can be destroyed.
        //
        // Plan:
        //: 1 Create a default constructed 'SpookyHashAlgorithm' and allow it
        //:   to leave scope to be destroyed. (C-1,3)
        //:
        //: 2 Call the parameterized constructor with a seed. (C-2)
        //
        // Testing:
        //   SpookyHashAlgorithm();
        //   SpookyHashAlgorithm(const char *seed);
        //   ~SpookyHashAlgorithm();
        // --------------------------------------------------------------------

        if (verbose)
            printf("\nTESTING CREATORS"
                   "\n================\n");

        if (verbose) printf("Create a default constructed"
                            " 'SpookyHashAlgorithm' and allow it to leave"
                            " scope to be destroyed. (C-1,3)\n");
        {
            Obj alg1;
        }

        if (verbose) printf("Call the parameterized constructor with a seed."
                            " (C-2)\n");
        {
            Uint64 array[2] = {0,0};
            Obj alg1(reinterpret_cast<const char *>(array));
        }

      } break;
      case 1: {
        // --------------------------------------------------------------------
        // BREATHING TEST
        //   This case exercises (but does not fully test) basic functionality.
        //
        // Concerns:
        //: 1 The class is sufficiently functional to enable comprehensive
        //:   testing in subsequent test cases.
        //
        // Plan:
        //: 1 Create an instance of 'bsl::SpookyHashAlgorithm'. (C-1)
        //:
        //: 2 Verify different hashes are produced for different c-strings.
        //:   (C-1)
        //:
        //: 3 Verify the same hashes are produced for the same c-strings. (C-1)
        //:
        //: 4 Verify different hashes are produced for different 'int's. (C-1)
        //:
        //: 5 Verify the same hashes are produced for the same 'int's. (C-1)
        //
        // Testing:
        //   BREATHING TEST
        // --------------------------------------------------------------------

        if (verbose) printf("\nBREATHING TEST"
                            "\n==============\n");

        if (verbose) printf("Instantiate 'bsl::SpookyHashAlgorithm'\n");
        {
            SpookyHashAlgorithm hashAlg;
        }

        if (verbose) printf("Verify different hashes are produced for"
                            " different c-strings.\n");
        {
            SpookyHashAlgorithm hashAlg1;
            SpookyHashAlgorithm hashAlg2;
            const char * str1 = "Hello World";
            const char * str2 = "Goodbye World";
            hashAlg1(str1, strlen(str1));
            hashAlg2(str2, strlen(str2));
            ASSERT(hashAlg1.computeHash() != hashAlg2.computeHash());
        }

        if (verbose) printf("Verify the same hashes are produced for the same"
                            " c-strings.\n");
        {
            SpookyHashAlgorithm hashAlg1;
            SpookyHashAlgorithm hashAlg2;
            const char * str1 = "Hello World";
            const char * str2 = "Hello World";
            hashAlg1(str1, strlen(str1));
            hashAlg2(str2, strlen(str2));
            ASSERT(hashAlg1.computeHash() == hashAlg2.computeHash());
        }

        if (verbose) printf("Verify different hashes are produced for"
                            " different 'int's.\n");
        {
            SpookyHashAlgorithm hashAlg1;
            SpookyHashAlgorithm hashAlg2;
            int int1 = 123456;
            int int2 = 654321;
            hashAlg1(&int1, sizeof(int));
            hashAlg2(&int2, sizeof(int));
            ASSERT(hashAlg1.computeHash() != hashAlg2.computeHash());
        }

        if (verbose) printf("Verify the same hashes are produced for the same"
                            " 'int's.\n");
        {
            SpookyHashAlgorithm hashAlg1;
            SpookyHashAlgorithm hashAlg2;
            int int1 = 123456;
            int int2 = 123456;
            hashAlg1(&int1, sizeof(int));
            hashAlg2(&int2, sizeof(int));
            ASSERT(hashAlg1.computeHash() == hashAlg2.computeHash());
        }
      } break;
      default: {
        fprintf(stderr, "WARNING: CASE `%d' NOT FOUND.\n", test);
        testStatus = -1;
      }
    }
    return testStatus;
}

// ----------------------------------------------------------------------------
// Copyright (C) 2014 Bloomberg Finance L.P.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------- END-OF-FILE ----------------------------------

