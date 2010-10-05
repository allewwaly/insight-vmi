/*
 * priorityqueue.h
 *
 *  Created on: 05.10.2010
 *      Author: chrschn
 */

#ifndef PRIORITYQUEUE_H_
#define PRIORITYQUEUE_H_

#include <QMap>
#include <QLinkedList>

/**
 * This container class stores items associated with a priority in an ordered
 * list. With the functions largest(), smallest(), takeLargest() or
 * takeSmallest(), the items with the highest or lowest priority can be easily
 * accessed. There may be any number of items associated with the same priority.
 *
 * When iterating over the list with begin() and end(), the items appear in
 * the order of their priority, starting with the smallest priority and ending
 * with the largest. Items with the same priority appear in a random order.
 *
 * Internally, this class uses a QMap to store the different priorities and a
 * QLinkedList to store the items themselves. When the number of items per
 * priority is very low or even just one, then the PriorityQueue is in fact
 * slower than just using a QMap for that purpose. But when there are lots of
 * items with the same priority, then PriorityQueue is much faster than a QMap.
 */
template<class Key, class T>
class PriorityQueue
{
public:
    typedef QLinkedList<T> list_type;

    /// Iterator type for this container
    typedef typename list_type::iterator iterator;

    /// Iterator type for this container, const version
    typedef typename list_type::const_iterator const_iterator;

private:
    typedef QMap<Key, iterator> map_type;

public:

    /**
     * Constructor
     */
    PriorityQueue()
        : _count(0)
    {
    }

    /**
     * Destructor
     */
    virtual ~PriorityQueue()
    {
    }

    /**
     * Clears all data.
     */
    void clear()
    {
        _keys.clear();
        _list.clear();
        _count = 0;
    }

    /**
     * Returns the element of the list, that was inserted with the smallest key.
     * @return the element with the smallest key
     * \note This function assumes that the list is not empty.
     */
    T& smallest()
    {
        return _list.first();
    }

    /**
     * This is an overloaded function.
     * @return the element with the smallest key
     * \note This function assumes that the list is not empty.
     */
    const T& smallest() const
    {
        return _list.first();
    }

    /**
     * Returns the smallest key that is stored with an item.
     * @return the smallest key
     * \note This function assumes that the list is not empty.
     */
    Key smallestKey() const
    {
        return _keys.constBegin().key();
    }

    /**
     * Returns the element of the list, that was inserted with the largest key.
     * @return the element with the largest key
     */
    T& largest()
    {
        return _list.last();
    }

    /**
     * This is an overloaded function.
     * @return the last element of the list
     */
    const T& largest() const
    {
        return _list.last();
    }

    /**
     * Returns the largest key that is stored with an item.
     * @return the largest key
     * \note This function assumes that the list is not empty.
     */
    Key largestKey() const
    {
        typename map_type::const_iterator it = --_keys.constEnd();
        return it.key();
    }

    /**
     * Inserts \a value into the list, sorted by \a key ascending.
     * @param key the sorting key
     * @param value the value to insert
     */
    void insert(Key key, T value)
    {
        // Is this the very first item?
        if (_list.isEmpty()) {
            // Add value to the list and key with list iterator to the map
            _list.append(value);
            _keys.insert(key, _list.begin());
        }
        else {
            // Returns item with given key, or next larger element if key is
            // not contained in the map
            typename map_type::iterator map_it = _keys.lowerBound(key);

            iterator list_it;
            // If this is the largest key so far, append value to the end of the
            // list
            if (map_it == _keys.end())
                list_it = _list.end();
            // If this is the smallest key so far, prepend value at beginning
            // of the list
            else if (map_it == _keys.begin() && map_it.key() > key)
                list_it = _list.begin();
            else
                list_it = map_it.value();
            // Insert value before list_it
            list_it = _list.insert(list_it, value);
            // Add key with list iterator to the map, if it doesn't exist yet
            if (map_it == _keys.end() || map_it.key() != key)
                _keys.insert(key, list_it);
        }
        ++_count;
    }

    /**
     * Returns the item associated with the smallest key and removes it from the
     * list.
     * @return the item with the smallest key
     * \sa smallest(), largest, takeLargest()
     * \note This function assumes that the list is not empty.
     */
    T takeSmallest()
    {
        --_count;
        // Do we have to delete the key? If the iterator mapped to the first
        // key equals the list iterator at the first position, then this is the
        // last item with that key, thus we delete that key.
        typename map_type::iterator it = _keys.begin();
        if (!_count || _list.begin() == it.value())
            _keys.erase(it);
        return _list.takeFirst();
    }

    /**
     * Returns the item associated with the largest key and removes it from the
     * list.
     * @return the item with the largest key
     * \sa largest(), smallest(), takeSmallest()
     * \note This function assumes that the list is not empty.
     */
    T takeLargest()
    {
        --_count;
        T item = _list.takeLast();

        // If this was the last item, then clear all keys
        if (_list.isEmpty()) {
            _keys.clear();
        }
        // If only items with the same key are remaining, then we only have to
        // update the corresponding list iterator
        else if (_keys.size() == 1) {
            _keys.begin().value() = --_list.end();
        }
        // With more than one key, we have to check if this was the last item
        // with that key of if there are more in the list. As the list iterators
        // mapped with each key point to the last item with the same key, we
        // can compare the iterator stored with the prev-to-last key and the
        // list iterator pointing to the last list element.
        else {
            typename map_type::iterator last_key_it = --_keys.end();
            typename map_type::iterator prev_last_key_it = last_key_it - 1;
            iterator last_list_it = --_list.end();
            // If end of the list equals prev-to-last key, then erase last key
            if (prev_last_key_it.value() == last_list_it)
                _keys.erase(last_key_it);
            // Otherwise update the iterator associated with the largest key
            else
                last_key_it.value() = last_list_it;
        }
        return item;
    }

    /**
     * This is the same as size().
     * @return number of items in the list
     * \sa size()
     */
    inline int count() const
    {
        return _count;
    }

    /**
     * This is the same as count().
     * @return number of items in the list
     * \sa count()
     */
    inline int size() const
    {
        return _count;
    }

    /**
     * @return \c true if the queue is emtpy, \c false otherwise
     */
    inline bool isEmpty() const
    {
        return _list.isEmpty();
    }

    /**
     * Returns an STL-style iterator pointing to the item associated with the
     * smallest key.
     * @return iterator
     */
    inline iterator begin()
    {
        return _list.begin();
    }

    /**
     * Returns an STL-style iterator pointing to the imaginary item after the
     * last item in the queue.
     * @return iterator
     */
    inline iterator end()
    {
        return _list.end();
    }

    /**
     * This is an overloaded function.
     * @return iterator
     */
    inline const_iterator begin() const
    {
        return _list.begin();
    }

    /**
     * This is an overloaded function.
     * @return iterator
     */
    inline const_iterator end() const
    {
        return _list.end();
    }

    /**
     * Returns an STL-style iterator pointing to the item associated with the
     * smallest key.
     * @return iterator
     */
    inline const_iterator constBegin() const
    {
        return _list.constBegin();
    }

    /**
     * Returns an STL-style iterator pointing to the imaginary item after the
     * last item in the queue.
     * @return iterator
     */
    inline const_iterator constEnd() const
    {
        return _list.constEnd();
    }

private:
    list_type _list;
    map_type _keys;
    int _count;
};

#endif /* PRIORITYQUEUE_H_ */
