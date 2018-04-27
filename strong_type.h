#include <type_traits>
#include <utility>
#include <string>

namespace wit
{
    namespace detail
    {
        template<typename STRONG_TYPE, template<typename> class MODIFIER_TYPE> // second type only because of multiple modifiers and avoid crash (CRTP)
        struct modifier
        {
        protected:
            // modifier is friend of strong_type but the children of modifier will not
            auto& get_value() { return strongly_typed_object().get_value(); }
            const auto& get_value() const { return strongly_typed_object().get_value(); }
            static const auto& get_value(const STRONG_TYPE& _object) { return _object.get_value(); }
            static auto& get_value(STRONG_TYPE& _object) { return _object.get_value(); }
            STRONG_TYPE& strongly_typed_object() { return static_cast<STRONG_TYPE&>(*this); }
            const STRONG_TYPE& strongly_typed_object() const { return static_cast<const STRONG_TYPE&>(*this); }
        };
    } // namespace detail

    // operators == and != only (need only operator == on the underlying type)
    template<typename STRONG_TYPE>
    struct equalable : detail::modifier<STRONG_TYPE, equalable>
    {
        bool operator==(const STRONG_TYPE& _rhs) const { return  this->get_value() == this->get_value(_rhs); }
        bool operator!=(const STRONG_TYPE& _rhs) const { return  !(*this == _rhs); }
    };

    // operators == , != , <, >, <= and >= (need only operator == and < on the underlying type)
    template<typename STRONG_TYPE>
    struct comparable : detail::modifier<STRONG_TYPE, comparable>
    {
        //static_assert(!std::is_base_of_v<equalable<STRONG_TYPE>, STRONG_TYPE>);
        bool operator==(const STRONG_TYPE& _rhs) const { return  this->get_value() == this->get_value(_rhs); } // should consider using !(a<b || b<a) to not use operator ==
        bool operator!=(const STRONG_TYPE& _rhs) const { return  !(*this == _rhs); }
        bool operator<(const STRONG_TYPE& _rhs) const { return  this->get_value() < this->get_value(_rhs); }
        bool operator>(const STRONG_TYPE& _rhs) const { return  this->get_value(_rhs) < this->get_value(); }
        bool operator<=(const STRONG_TYPE& _rhs) const { return  !(*this > _rhs); }
        bool operator>=(const STRONG_TYPE& _rhs) const { return  !(*this < _rhs); }
    };

    template<typename STRONG_TYPE>
    struct self_addable : detail::modifier<STRONG_TYPE, self_addable>
    {
        STRONG_TYPE operator+(const STRONG_TYPE& _rhs) const { return STRONG_TYPE{ this->get_value() + this->get_value(_rhs) }; }
    };

    template<typename STRONG_TYPE>
    struct self_subtractable : detail::modifier<STRONG_TYPE, self_subtractable>
    {
        STRONG_TYPE operator-(const STRONG_TYPE& _rhs) const { return STRONG_TYPE{ this->get_value() - this->get_value(_rhs) }; }
    };

    template<typename STRONG_TYPE>
    struct self_multipliable : detail::modifier<STRONG_TYPE, self_multipliable>
    {
        STRONG_TYPE operator-(const STRONG_TYPE& _rhs) const { return STRONG_TYPE{ this->get_value() * this->get_value(_rhs) }; }
    };

    template<typename STRONG_TYPE>
    struct self_dividable : detail::modifier<STRONG_TYPE, self_multipliable>
    {
        STRONG_TYPE operator/(const STRONG_TYPE& _rhs) const { return STRONG_TYPE{ this->get_value() / this->get_value(_rhs) }; }
    };

    template<typename STRONG_TYPE>
    struct incrementable : detail::modifier<STRONG_TYPE, incrementable>
    {
        STRONG_TYPE& operator++() { ++this->get_value(); return this->strongly_typed_object(); }
        STRONG_TYPE operator++(int) { auto res = this->strongly_typed_object(); ++this->get_value(); return res; }
        STRONG_TYPE& operator+=(const STRONG_TYPE& _rhs) { this->get_value() += this->get_value(_rhs); return this->strongly_typed_object(); }
    };

    template<typename STRONG_TYPE>
    struct decrementable : detail::modifier<STRONG_TYPE, decrementable>
    {
        STRONG_TYPE& operator--() { --this->get_value(); return this->strongly_typed_object(); }
        STRONG_TYPE operator--(int) { auto res = this->strongly_typed_object(); --this->get_value(); return res; }
        STRONG_TYPE& operator-=(const STRONG_TYPE& _rhs) { this->get_value() -= this->get_value(_rhs); return this->strongly_typed_object(); }
    };

    template<typename STRONG_TYPE>
    struct stringable : detail::modifier<STRONG_TYPE, stringable>
    {
        static constexpr bool is_stringable = true;
        std::string to_string() const { return std::to_string( this->get_value() ); }
    };

    template<typename TYPE>
    struct explicitly_convertible_to
    {
        template<typename STRONG_TYPE>
        struct modifier : detail::modifier<STRONG_TYPE, modifier>
        {
            explicit operator TYPE() const
            {
                return this->get_value();
            }
        };
    };

    // template<template<typename, typename, template<typename> class...> MULTIPLICAND_STRONG_TYPE, typename PRODUCT_STRONG_TYPE>
    // struct multipliable_by
    // {
    //     template<typename STRONG_TYPE>
    //     struct modifier : detail::modifier<STRONG_TYPE, modifier>
    //     {
    //         PRODUCT_STRONG_TYPE operator+(const MULTIPLICAND_STRONG_TYPE&) { this->get_value()}
    //     }
    // };

    // -------------------------------------

    template<typename UNDERLYING_TYPE, typename TAG_TYPE, template<typename> class... MODIFIER_TYPES>
    struct strong_type : MODIFIER_TYPES<strong_type<UNDERLYING_TYPE, TAG_TYPE, MODIFIER_TYPES...>>...
    {
        using underlying_type = UNDERLYING_TYPE;
        explicit constexpr strong_type(UNDERLYING_TYPE _value) : value_(std::move(_value)) {}
        template<typename ANOTHER_UNDERLYING_TYPE, typename ANOTHER_TAG_TYPE>
        strong_type(const strong_type<ANOTHER_UNDERLYING_TYPE, ANOTHER_TAG_TYPE>&) = delete; // conversion from another strong_type
        const UNDERLYING_TYPE& get() const { return value_; }
    private:
        template<typename STRONG_TYPE, template<typename> class MODIFIER_TYPE >
        friend struct detail::modifier;
        const UNDERLYING_TYPE& get_value() const { return value_; }
        UNDERLYING_TYPE& get_value() { return value_; }
        UNDERLYING_TYPE value_;
    };
} // namespace wit

namespace std
{
    template<typename UNDERLYING_TYPE, typename TAG_TYPE, template<typename> class... MODIFIER_TYPES, typename = std::enable_if_t<wit::strong_type<UNDERLYING_TYPE, TAG_TYPE, MODIFIER_TYPES...>::is_stringable>>
    string to_string(const wit::strong_type<UNDERLYING_TYPE, TAG_TYPE, MODIFIER_TYPES...>& _strongly_typed_object)
    {
        return _strongly_typed_object.to_string();
    }
}