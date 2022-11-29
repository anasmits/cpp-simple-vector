#pragma once

#include <cassert>
#include <initializer_list>
#include "array_ptr.h"
#include <algorithm>
#include <stdexcept>


//template <typename Type>
class ReserveProxyObj{
private:
    size_t capacity_to_reserve_;
public:
    ReserveProxyObj() = delete;
    explicit ReserveProxyObj( size_t capacity_to_reserve)
            : capacity_to_reserve_(capacity_to_reserve)
    {}
    size_t GetCapacity() {
        return capacity_to_reserve_;
    };
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
private:
    size_t size_ = 0;
    size_t capacity_ = 0;
    ArrayPtr<Type> items_;
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size)
    : size_(size)
    , capacity_(size)
    , items_(size)
    {
        for (auto it = begin(); it != end(); ++it){
            *it = Type{};
        }
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value)
    : size_(size)
    , capacity_(size)
    , items_(size)
    {
        std::fill(begin(), end(), std::move(value));
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init)
    : size_(init.size())
    , capacity_(init.size())
    , items_(init.size())
    {
        std::copy(init.begin(), init.end(), begin());
    }

    SimpleVector(ReserveProxyObj proxy_obj){
        capacity_ = proxy_obj.GetCapacity();
    }

    SimpleVector(const SimpleVector& other)
    : size_(other.size_)
    , capacity_(other.capacity_)
    , items_(other.size_){
            std::copy(other.begin(), other.end(), begin());
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (&rhs != this){
            SimpleVector tmp(rhs);
            tmp.swap(*this);
        }
        return *this;
    }

    SimpleVector(SimpleVector&& other){
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
        items_ = std::exchange(other.items_, ArrayPtr<Type>());
    }

    SimpleVector& operator=(SimpleVector&& other){
        if (&other != this) {
            size_ = std::exchange(other.size_, 0);
            capacity_ = std::exchange(other.capacity_, 0);
            items_ = std::exchange(other.items_, ArrayPtr<Type>());
            return *this;
        }
    }

    ~SimpleVector(){}

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        std::swap(items_, other.items_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    void swap(SimpleVector&& other) noexcept {
        items_ = std::exchange(other.items_, items_);
        std::swap(other.size_, size_);
        std::swap(other.capacity_, capacity_);
    }


    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if(size_ < capacity_){
            items_[size_++] = item;
        } else{
            Resize(size_+1);
            items_[size_-1] = item;
        }
    }

    void PushBack(Type&& item){
        if(size_ < capacity_){
            items_[size_++] = std::move(item);
        } else{
            Resize(size_+1);
            items_[size_-1] = std::move(item);
        }
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(!IsEmpty());
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(pos >= cbegin() && pos < cend());
        Iterator it_pos = begin() + (pos - cbegin());
        if (it_pos == begin()){
            std::move(it_pos+1, end(), begin());
        }else if(it_pos < end()-1){
            std::move_backward(it_pos+1, end(), end()-1);
        }
        --size_;
        return size_ == 0 ? end() : it_pos;
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= cbegin() && pos <= cend());
        if(IsEmpty()){
            PushBack(value);
            return begin();
        };
        size_t it_dist = pos - cbegin();
        if(size_ == capacity_){
            Resize(size_);
        }
        Iterator it_pos = begin() + it_dist;
        std::copy_backward(it_pos, end(), end() + 1);
        *it_pos = value;
        ++size_;
        return it_pos;
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= cbegin() && pos <= cend());
        if(IsEmpty()){
            PushBack(std::move(value));
            return begin();
        };
        size_t it_dist = pos - cbegin();
        if(size_ == capacity_){
            Resize(size_);
        }
        Iterator it_pos = begin() + it_dist;
        std::move_backward(it_pos, end(), end() + 1);
        *it_pos = std::move(value);
        ++size_;
        return it_pos;
    }

    void Reserve(size_t new_capacity){
        if(new_capacity > capacity_){
            auto old_size = size_;
            SimpleVector<Type> new_items(new_capacity);
            std::move(&items_[0], &items_[size_], new_items.begin());
            swap(new_items);
            capacity_ = new_capacity;
            size_ = old_size;
        }
    };


    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index >= 0 && index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index >= 0 && index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_){
            throw std::out_of_range("The index is bigger than simple vector size");
        }
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_){
            throw std::out_of_range("The index is bigger than simple vector size");
        }
        return items_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
//     При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size < size_){
            size_ = new_size;
        } else if (new_size > size_ && new_size < capacity_){
            for (auto it = end(); it != &items_[new_size-1]; ++it){
                *it = std::move(Type{});
            }
            size_ = new_size;
        } else{
            auto new_capacity =  std::max(new_size, capacity_ *2);
            Reserve(new_capacity);
            size_ = new_size;
            capacity_ = new_capacity;
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return (items_.Get() + size_);
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return IsEmpty() ? nullptr : &items_[0];
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return IsEmpty() ? nullptr : &items_[size_];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return begin();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return end();
    }
};



template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs > lhs);
}

