//
// Copyright (c) 2016-2018 Martin Moene
//
// https://github.com/martinmoene/any-lite
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "any-main.t.hpp"

// Suppress:
// - unused parameter, for cases without assertions such as [.std...]
#if defined(__clang__)
# pragma clang diagnostic ignored "-Wunneeded-member-function"
#elif defined __GNUC__
//# pragma GCC   diagnostic ignored ""
#endif

// Note: use any_cast( ptr-to-any ) to preserve move-state.

using namespace nonstd;

namespace {

// ensure comparison of pointers for lest:

//const void * lest_nullptr = 0;

// The following tracer code originates as Oracle from Optional by
// Andrzej Krzemienski, https://github.com/akrzemi1/Optional.

enum State
{
    /* 0 */ default_constructed,
    /* 1 */ value_copy_constructed,
    /* 2 */ value_move_constructed,
    /* 3 */ copy_constructed,
    /* 4 */ move_constructed,
    /* 5 */ move_assigned,
    /* 6 */ copy_assigned,
    /* 7 */ value_copy_assigned,
    /* 8 */ value_move_assigned,
    /* 9 */ moved_from,
    /*10 */ value_constructed
};

struct V
{
    State state;
    int   value;

    V(             ) : state( default_constructed ), value( deflt() ) {}
    V( int       v ) : state( value_constructed   ), value( v       ) {}
    V( V const & v ) : state( copy_constructed    ), value( v.value ) {}

    V & operator=( int       v ) { state = value_copy_assigned; value = v; return *this; }
    V & operator=( V const & v ) { state = copy_assigned      ; value = v.value; return *this; }

#if any_CPP11_OR_GREATER
    V(             V && v ) : state( move_constructed   ), value(  std::move( v.value ) ) { v.state = moved_from; }
    V & operator=( V && v ) { state = move_assigned      ; value = std::move( v.value ); v.state = moved_from; return *this; }
#endif

    static int deflt() { return 42; }

    bool operator==( V const & other ) const { return state == other.state && value == other.value; }
};

struct S
{
    State state;
    V     value;

    S(             ) : state( default_constructed    ) {}
    S( V const & v ) : state( value_copy_constructed ), value( v ) {}
    S( S const & s ) : state( copy_constructed       ), value( s.value        ) {}

    S & operator=( V const & v ) { state = value_copy_assigned; value = v; return *this; }
    S & operator=( const S & s ) { state = copy_assigned      ; value = s.value; return *this; }

#if any_CPP11_OR_GREATER
    S(             V && v ) : state(  value_move_constructed ), value(  std::move( v )     ) { v.state = moved_from; }
    S(             S && s ) : state(  move_constructed       ), value(  std::move( s.value ) ) { s.state = moved_from; }

    S & operator=( V && v ) { state = value_move_assigned     ; value = std::move( v       ); v.state = moved_from; return *this; }
    S & operator=( S && s ) { state = move_assigned           ; value = std::move( s.value ); s.state = moved_from; return *this; }
#endif

    bool operator==( S const & rhs ) const { return state == rhs.state && value == rhs.value; }
};

inline std::ostream & operator<<( std::ostream & os, V const & v )
{
    using lest::to_string;
    return os << "[V:" << to_string( v.value ) << "]";
}

inline std::ostream & operator<<( std::ostream & os, S const & s )
{
    using lest::to_string;
    return os << "[S:" << to_string( s.value ) << "]";
}

#if any_CPP11_OR_GREATER
struct InitList
{
    std::vector<int> vec;
    char c;
    S s;

    InitList( std::initializer_list<int> il, char k, S const & t)
    : vec( il ), c( k ), s( t ) {}

    InitList( std::initializer_list<int> il, char k, S && t)
    : vec( il ), c( k ), s( std::move(t) ) {}
};
#endif

} // anonymous namespace

//
// any member operations:
//

CASE( "any: Allows to default construct any" )
{
    any a;

    EXPECT_NOT( a.has_value() );
}

CASE( "any: Allows to copy-construct from any" )
{
    any a( 7 );

    any b( a );

    EXPECT( a.has_value() );
    EXPECT( b.has_value() );
    EXPECT( any_cast<int>( b ) == 7 );
}

CASE( "any: Allows to move-construct from any (C++11)" )
{
#if any_CPP11_OR_GREATER
    any b( any( 7 ) );

    EXPECT( any_cast<int>( b ) == 7 );
#else
    EXPECT( !!"any: move-construction is not available (no C++11)" );
#endif
}

CASE( "any: Allows to copy-construct from literal value" )
{
    any a( 7 );

    EXPECT( any_cast<int>( a ) == 7 );
}

CASE( "any: Allows to copy-construct from value" )
{
    const int i = 7;
    any a( i );

    EXPECT( any_cast<int>( a ) == i );
}

CASE( "any: Copy constructs from lvalue references" )
{
    std::string i = "Test";
    any a( i );

    EXPECT( any_cast<std::string>( a ) == i );
}

CASE( "any: Allows to move-construct from value (C++11)" )
{
#if any_CPP11_OR_GREATER
    S s( 7 );
    any a( std::move( s ) );

    EXPECT( any_cast<S>( &a )->value.value == 7          );
    EXPECT( any_cast<S>( &a )->state == move_constructed );
    EXPECT(                  s.state == moved_from       );
#else
    EXPECT( !!"any: move-construction is not available (no C++11)" );
#endif
}

CASE( "any: Allows to in-place construct from literal value (C++11)" )
{
#if any_CPP11_OR_GREATER
    using pair_t = std::pair<char, int>;

#if any_USES_STD_ANY
    any a( in_place_type<pair_t>, 'a', 7 );
#else
    any a( in_place_type<pair_t>, 'a', 7 );
//  any a( in_place<     pair_t>, 'a', 7 );
#endif
    EXPECT( any_cast<pair_t>( a ).first  == 'a' );
    EXPECT( any_cast<pair_t>( a ).second ==  7  );
#else
    EXPECT( !!"any: in-place construction is not available (no C++11)" );
#endif
}

CASE( "any: Allows to in-place copy-construct from value (C++11)" )
{
#if any_CPP11_OR_GREATER
    char c = 'a'; V v( 7 );
    using pair_t = std::pair<char, V>;

#if any_USES_STD_ANY
    any a( in_place_type<pair_t>, c, v );
#else
    any a( in_place_type<pair_t>, c, v );
//  any a( in_place<     pair_t>, c, v );
#endif

    EXPECT( any_cast<pair_t>( &a )->first        == 'a' );
    EXPECT( any_cast<pair_t>( &a )->second.value ==  7  );
#if any_USES_STD_ANY
    EXPECT( any_cast<pair_t>( &a )->second.state == copy_constructed );
#else
    EXPECT( any_cast<pair_t>( &a )->second.state == move_constructed );
#endif
    EXPECT(                              v.state != moved_from       );
#else
    EXPECT( !!"any: in-place construction is not available (no C++11)" );
#endif
}

CASE( "any: Allows to in-place move-construct from value (C++11)" )
{
#if any_CPP11_OR_GREATER
    char c = 'a'; V v( 7 );
    using pair_t = std::pair<char, V>;

#if any_USES_STD_ANY
    any a( in_place_type<pair_t>, c, std::move(v) );
#else
    any a( in_place_type<pair_t>, c, std::move(v) );
//  any a( in_place<     pair_t>, c, std::move(v) );
#endif
    EXPECT( any_cast<pair_t>( &a )->first        == 'a' );
    EXPECT( any_cast<pair_t>( &a )->second.value ==  7  );
    EXPECT( any_cast<pair_t>( &a )->second.state == move_constructed );
    EXPECT(                              v.state == moved_from       );
#else
    EXPECT( !!"any: in-place construction is not available (no C++11)" );
#endif
}

CASE( "any: Allows to in-place copy-construct from initializer-list (C++11)" )
{
#if any_CPP11_OR_GREATER
    S s( 7 );
#if any_USES_STD_ANY
    any a( in_place_type<InitList>, { 7, 8, 9, }, 'a', s );
#else
    any a( in_place_type<InitList>, { 7, 8, 9, }, 'a', s );
//  any a( in_place<     InitList>, { 7, 8, 9, }, 'a', s );
#endif

    EXPECT( any_cast<InitList>( &a )->vec[0]  ==  7  );
    EXPECT( any_cast<InitList>( &a )->vec[1]  ==  8  );
    EXPECT( any_cast<InitList>( &a )->vec[2]  ==  9  );
    EXPECT( any_cast<InitList>( &a )->c       == 'a' );
    EXPECT( any_cast<InitList>( &a )->s.value.value ==  7               );
#if any_USES_STD_ANY
    EXPECT( any_cast<InitList>( &a )->s.state       == copy_constructed );
#else
    EXPECT( any_cast<InitList>( &a )->s.state       == move_constructed );
#endif
    EXPECT(                           s.state       != moved_from       );
#else
    EXPECT( !!"any: in-place construction is not available (no C++11)" );
#endif
}

CASE( "any: Allows to in-place move-construct from initializer-list (C++11)" )
{
#if any_CPP11_OR_GREATER
    S s( 7 );
#if any_USES_STD_ANY
    any a( in_place_type<InitList>, { 7, 8, 9, }, 'a', std::move(s) );
#else
    any a( in_place_type<InitList>, { 7, 8, 9, }, 'a', std::move(s) );
//  any a( in_place<     InitList>, { 7, 8, 9, }, 'a', std::move(s) );
#endif

    EXPECT( any_cast<InitList>( &a )->vec[0]  ==  7  );
    EXPECT( any_cast<InitList>( &a )->vec[1]  ==  8  );
    EXPECT( any_cast<InitList>( &a )->vec[2]  ==  9  );
    EXPECT( any_cast<InitList>( &a )->c       == 'a' );
    EXPECT( any_cast<InitList>( &a )->s.value.value == 7                );
    EXPECT( any_cast<InitList>( &a )->s.state       == move_constructed );
    EXPECT(                           s.state       == moved_from       );
    EXPECT(                           s.value.state == moved_from       );
#else
    EXPECT( !!"any: in-place construction is not available (no C++11)" );
#endif
}

CASE( "any: Allows to copy-assign from any" )
{
    any a = 7;
    any b;

    b = a;

    EXPECT( any_cast<int>( b ) == 7 );
}

CASE( "any: Allows to move-assign from any (C++11)" )
{
#if any_CPP11_OR_GREATER
    any a;

    a = any( 7 );

    EXPECT( any_cast<int>( a ) == 7 );
#else
    EXPECT( !!"any: move semantics are not available (no C++11)" );
#endif
}

CASE( "any: Allows to copy-assign from literal value" )
{
    any a;

    a = 7;

    EXPECT( any_cast<int>( a ) == 7 );
}

CASE( "any: Allows to copy-assign from value" )
{
    const int i = 7;
    any a;

    a = i;

    EXPECT( any_cast<int>( a ) == i );
}

CASE( "any: Allows to move-assign from value (C++11)" )
{
#if any_CPP11_OR_GREATER
    V v( 7 );
    any a;

    a = std::move( v );

    EXPECT( any_cast<V>( &a )->value == 7                );
    EXPECT( any_cast<V>( &a )->state == move_constructed );
    EXPECT(                  v.state == moved_from       );
#else
    EXPECT( !!"any: in-place construction is not available (no C++11)" );
#endif
}

CASE( "any: Allows to copy-emplace content (C++11)" )
{
#if any_CPP11_OR_GREATER
    using pair_t = std::pair<char, V>;
    V v( 7 );
    any a;

    a.emplace<pair_t>( 'a', v );

    EXPECT( any_cast<pair_t>( &a )->first        == 'a'              );
    EXPECT( any_cast<pair_t>( &a )->second.value ==  7               );
#if any_USES_STD_ANY
    EXPECT( any_cast<pair_t>( &a )->second.state == copy_constructed );
#else
    EXPECT( any_cast<pair_t>( &a )->second.state == move_constructed );
#endif
    EXPECT(                              v.state != moved_from       );
#else
    EXPECT( !!"any: in-place construction is not available (no C++11)" );
#endif
}

CASE( "any: Allows to move-emplace content (C++11)" )
{
#if any_CPP11_OR_GREATER
    using pair_t = std::pair<char, V>;
    V v( 7 );
    any a;

    a.emplace<pair_t>( 'a', std::move( v ) );

    EXPECT( any_cast<pair_t>( &a )->first        == 'a'              );
    EXPECT( any_cast<pair_t>( &a )->second.value ==  7               );
    EXPECT( any_cast<pair_t>( &a )->second.state == move_constructed );
    EXPECT(                              v.state == moved_from       );
#else
    EXPECT( !!"any: in-place construction is not available (no C++11)" );
#endif
}

CASE( "any: Allows to copy-emplace content from intializer-list (C++11)" )
{
#if any_CPP11_OR_GREATER
    S s( 7 );
    any a;

    a.emplace<InitList>( { 7, 8, 9, }, 'a', s );

    EXPECT( any_cast<InitList>( &a )->vec[0]  ==  7  );
    EXPECT( any_cast<InitList>( &a )->vec[1]  ==  8  );
    EXPECT( any_cast<InitList>( &a )->vec[2]  ==  9  );
    EXPECT( any_cast<InitList>( &a )->c       == 'a' );
    EXPECT( any_cast<InitList>( &a )->s.value.value ==  7               );
#if any_USES_STD_ANY
    EXPECT( any_cast<InitList>( &a )->s.state       == copy_constructed );
#else
    EXPECT( any_cast<InitList>( &a )->s.state       == move_constructed );
#endif
    EXPECT(                           s.state       != moved_from       );
#else
    EXPECT( !!"any: in-place construction is not available (no C++11)" );
#endif
}

CASE( "any: Allows to move-emplace content from intializer-list (C++11)" )
{
#if any_CPP11_OR_GREATER
    S s( 7 );
    any a;

    a.emplace<InitList>( { 7, 8, 9, }, 'a', std::move( s ) );

    EXPECT( any_cast<InitList>( &a )->vec[0]  ==  7  );
    EXPECT( any_cast<InitList>( &a )->vec[1]  ==  8  );
    EXPECT( any_cast<InitList>( &a )->vec[2]  ==  9  );
    EXPECT( any_cast<InitList>( &a )->c       == 'a' );
    EXPECT( any_cast<InitList>( &a )->s.value.value ==  7               );
    EXPECT( any_cast<InitList>( &a )->s.state       == move_constructed );
    EXPECT(                           s.state       == moved_from       );
#else
    EXPECT( !!"any: in-place construction is not available (no C++11)" );
#endif
}

CASE( "any: Allows to reset content" )
{
    any a = 7;

    a.reset();

    EXPECT_NOT( a.has_value() );
}

CASE( "any: Allows to swap with other any (member)" )
{
    any a = 1;
    any b = 2;

    a.swap( b );

    EXPECT( any_cast<int>( a ) == 2 );
    EXPECT( any_cast<int>( b ) == 1 );
}

CASE( "any: Allows to inspect if any contains a value" )
{
    any a = 7;

    EXPECT( a.has_value() );
}

CASE( "any: Allows to obtain type_info of any's content" )
{
    any a = 7;
    any b = 3.14;

    EXPECT( ( a.type() == typeid(int   ) ) );
    EXPECT( ( b.type() == typeid(double) ) );
}

//
// any non-member functions:
//

CASE( "swap: Allows to swap with other any (non-member)" )
{
    any a = 1;
    any b = 2;

    swap( a, b );

    EXPECT( any_cast<int>( a ) == 2 );
    EXPECT( any_cast<int>( b ) == 1 );
}

CASE( "make_any: Allows to in-place copy-construct any from arguments (C++11)" )
{
#if any_CPP11_OR_GREATER
    using pair_t = std::pair<char, S>;

    S s( 7 );
    any a = make_any<pair_t>( 'a', s );

    EXPECT( any_cast<pair_t>( &a )->first              == 'a' );
    EXPECT( any_cast<pair_t>( &a )->second.value.value ==  7  );
#if any_USES_STD_ANY
    EXPECT( any_cast<pair_t>( &a )->second.state       == copy_constructed );
#else
    EXPECT( any_cast<pair_t>( &a )->second.state       == move_constructed );
#endif
    EXPECT(                              s.state       != moved_from       );
#else
    EXPECT( !!"any: in-place construction is not available (no C++11)" );
#endif
}

CASE( "make_any: Allows to in-place move-construct any from arguments (C++11)" )
{
#if any_CPP11_OR_GREATER
    using pair_t = std::pair<char, S>;
    S s( 7 );

    any a = make_any<pair_t>( 'a', std::move( s ) );

    EXPECT( any_cast<pair_t>( &a )->first              == 'a' );
    EXPECT( any_cast<pair_t>( &a )->second.value.value ==  7  );
    EXPECT( any_cast<pair_t>( &a )->second.state       ==  move_constructed );
    EXPECT(                              s.state       ==  moved_from       );
#else
    EXPECT( !!"any: in-place construction is not available (no C++11)" );
#endif
}

CASE( "make_any: Allows to in-place copy-construct any from initializer-list and arguments (C++11)" )
{
#if any_CPP11_OR_GREATER
    S s( 7 );
    any a = make_any<InitList>( { 7, 8, 9, }, 'a', s );

    EXPECT( any_cast<InitList>( &a )->vec[0]  ==  7  );
    EXPECT( any_cast<InitList>( &a )->vec[1]  ==  8  );
    EXPECT( any_cast<InitList>( &a )->vec[2]  ==  9  );
    EXPECT( any_cast<InitList>( &a )->c       == 'a' );
    EXPECT( any_cast<InitList>( &a )->s.value.value ==  7               );
#if any_USES_STD_ANY
    EXPECT( any_cast<InitList>( &a )->s.state       == copy_constructed );
#else
    EXPECT( any_cast<InitList>( &a )->s.state       == move_constructed );
#endif
    EXPECT(                           s.state       != moved_from       );
#else
    EXPECT( !!"any: in-place construction is not available (no C++11)" );
#endif
}

CASE( "make_any: Allows to in-place move-construct any from initializer-list and arguments (C++11)" )
{
#if any_CPP11_OR_GREATER
    S s( 7 );
    any a = make_any<InitList>( { 7, 8, 9, }, 'a', std::move( s ) );

    EXPECT( any_cast<InitList>( &a )->vec[0]  ==  7  );
    EXPECT( any_cast<InitList>( &a )->vec[1]  ==  8  );
    EXPECT( any_cast<InitList>( &a )->vec[2]  ==  9  );
    EXPECT( any_cast<InitList>( &a )->c       == 'a' );
    EXPECT( any_cast<InitList>( &a )->s.value.value ==  7               );
    EXPECT( any_cast<InitList>( &a )->s.state       == move_constructed );
    EXPECT(                           s.state       == moved_from       );
#else
    EXPECT( !!"any: in-place construction is not available (no C++11)" );
#endif
}

CASE( "any_cast: Allows to obtain any's content by value (any const &)" )
{
    struct F { static any const & ident( any const & a ) { return a; } };
    any a = 7;

    EXPECT( any_cast<int>( F::ident(a) ) == 7 );
}

CASE( "any_cast: Allows to obtain any's content by value (any &)" )
{
    struct F { static any & ident( any & a ) { return a; } };
    any a = 7;

    EXPECT( any_cast<int>( F::ident(a) ) == 7 );
}

CASE( "any_cast: Allows to obtain any's content by value (any &&)" )
{
#if any_CPP11_OR_GREATER
    struct F { static any && ident( any && a ) { return std::move(a); } };

    EXPECT( any_cast<int>( F::ident( any(7) ) ) == 7 );
#else
    EXPECT( !!"any: move semantics not available (no C++11)" );
#endif
}

CASE( "any_cast: Allows to obtain any's content by pointer (any const *)" )
{
    struct F { static any const * ident( any const * a ) { return a; } };
    any a = 7;

    EXPECT( *any_cast<int>( F::ident( &a ) ) == 7 );
}

CASE( "any_cast: Allows to obtain any's content by pointer (any *)" )
{
    struct F { static any * ident( any * a ) { return a; } };
    any a = 7;

    EXPECT( *any_cast<int>( F::ident( &a ) ) == 7 );
}

CASE( "any_cast: Throws bad_any_cast if requested type differs from content type (any const &)" )
{
    struct F { static any const & ident( any const & a ) { return a; } };
    any a = 7;

    EXPECT_THROWS_AS( any_cast<double>( F::ident(a) ), bad_any_cast );
//  EXPECT_THROWS_WITH( any_cast<double>( F::ident(a) ), "..." )

}

CASE( "any_cast: Throws bad_any_cast if requested type differs from content type (any &)" )
{
    struct F { static any & ident( any & a ) { return a; } };
    any a = 7;

    EXPECT_THROWS_AS( any_cast<double>( F::ident(a) ), bad_any_cast );
}

CASE( "any_cast: Throws bad_any_cast if requested type differs from content type (any &&)" )
{
#if any_CPP11_OR_GREATER
    struct F { static any && ident( any && a ) { return std::move(a); } };

    EXPECT_THROWS_AS( any_cast<double>( F::ident( any(7) ) ), bad_any_cast );
#else
    EXPECT( !!"any: move semantics not available (no C++11)" );
#endif
}

CASE( "any_cast: Throws bad_any_cast with non-empty what()" )
{
    struct F { static any const & ident( any const & a ) { return a; } };
    any a = 7;

    try
    {
        (void) any_cast<double>( F::ident(a) );
    }
    catch( bad_any_cast const & e )
    {
        EXPECT( ! std::string( e.what() ).empty() );
    }
}

// end of file
