#pragma once

/**
 * @namespace corelink
 * @details Root namespace for everything corelink. All modules are in nested namespaces
 */
namespace corelink
{
    /**
     * @typedef ptr
     * @tparam t type of pointer
     * @brief Typedef native pointer type.
     * @details template expands to @code t *var @endcode <br/>
     * Example: @code
     * int a = 10;
     * ptr<int> b = &a;
     * @endcode
     */
    template<typename t> using ptr = t *;

    /**
     * @typedef ptr_to_const_val
     * @tparam t type of pointer
     * @brief Typedef native pointer-to-const-value type.
     * @details template expands to @code const t *var @endcode <br/>
     * Example: @code
     * int a = 10;
     * int b = 11;
     * ptr_to_const_val<int> p = &a; // eq. to  int const *b = &a;
     * // *p = 12; // illegal. will generate compiler error
     * p = &b; // OK.
     * @endcode
     */
    template<typename t> using ptr_to_const_val = t const *;

    /**
     * @typedef const_ptr_to_val
     * @tparam t type of pointer
     * @brief Typedef native const-pointer-to-value type.
     * @details template expands to @code t *const var @endcode <br/>
     * Example: @code
     * int a = 10;
     * int b = 11;
     * const_ptr_to_val<int> p = &a; // eq. to  int const *b = &a;
     * *p = 12; // OK
     * // p = &b; // illegal. will generate compiler error
     * @endcode
     */
    template<typename t> using const_ptr_to_val = t *const;

    /**
     * @typedef const_ptr_to_const_val
     * @tparam t type of pointer
     * @brief Defines a constant pointer type to a constant value.
     * @details Equivalent expansion would be const t *const val
     * Example: @code
     * int a = 10;
     * int b = 11;
     * const_ptr_to_const_val<int> p = &a; // eq. to const int const *b = &a;
     * // *p = 12; // illegal. will generate compiler error
     * // p = &b; // illegal. will generate compiler error
     * @endcode
     */
    template<typename t> using const_ptr_to_const_val = const t *const;

    /**
     * @typedef fref
     * @tparam t value type
     * @brief Forwarding reference
     * @details Please refer to https://en.cppreference.com/w/cpp/language/reference
     */
    template<typename t> using fref = t &&;

    /**
     * @typedef rvref
     * @tparam t native or secondary type
     * @brief R-value reference typedef
     * Example @code
     * void foo(rvref<std::string> s)
     * {
     *  // ...
     * }
     * ...
     * foo("Hello"); //OK
     * std::string b = "Bye";
     * foo(std::move(b)); // OK
     * @endcode
     */
    template<typename t> using rvref = fref<t &&>;

    /**
     * @typedef lvref
     * @tparam t native or secondary type
     * @brief L-value reference typedef. equivalent to type &var
     * Example @code
     * void foo(lvref<std::string> s)
     * {
     *  // ...
     * }
     * ...
     * std::string b = "Bye";
     * foo(b); // OK
     * @endcode
     */
    template<typename t> using lvref = fref<t &>;

    /**
     * @typedef clvref
     * @tparam t native or secondary type
     * @brief const l-value reference typedef.
     * @details usage: clvref\<type\> var. equivalent to const type &var
     * Example @code
     * void foo(clvref<std::string> s)
     * {
     *  // ...
     * }
     * ...
     * foo("Hello"); // OK
     * std::string b = "Bye";
     * foo(b); // OK
     * @endcode
     */
    template<typename t> using clvref = fref<const t &>;

    /**
     * @typedef in
     * @tparam t native or secondary type
     * @brief Wraps a type and declares a value as an in parameter, which is a @ref clvref.
     * @details this is useful for parameters which should not be mutated when passed to a function
     * @code
     * void foo(in<int> num)
     * {
     *    // ... refer to num
     * }
     *
     * foo(2);
     * @endcode
     */
    template<typename t> using in = clvref<t>;

    /**
     * @typedef out
     * @tparam t native or secondary type
     * @brief Wraps a type and declares a value as an out parameter, which is a @ref lvref.
     * @details this is useful for parameters which should be mutated when passed to a function
     * @code
     * void foo(out<int> num)
     * {
     *    // ... change num and that will be propagated to a
     * }
     * int a = 2;
     * foo(a);
     * @endcode
     */
    template<typename t> using out = lvref<t>;
}