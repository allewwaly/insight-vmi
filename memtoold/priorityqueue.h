/*
 * priorityqueue.h
 *
 *  Created on: 05.10.2010
 *      Author: chrschn
 */

#ifndef PRIORITYQUEUE_H_
#define PRIORITYQUEUE_H_

#include <QMultiMap>

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
    typedef QMap<Key, T> map_type;

    /// Iterator type for this container
    typedef typename map_type::iterator iterator;

    /// Iterator type for this container, const version
    typedef typename map_type::const_iterator const_iterator;

public:

    /**
     * Constructor
     */
    PriorityQueue()
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
    inline void clear()
    {
        _map.clear();
    }

    /**
     * Returns the element of the list, that was inserted with the smallest key.
     * @return the element with the smallest key
     * \note This function assumes that the list is not empty.
     */
    inline T& smallest()
    {
        return _map.begin().value();
    }

    /**
     * This is an overloaded function.
     * @return the element with the smallest key
     * \note This function assumes that the list is not empty.
     */
    inline const T& smallest() const
    {
        return _map.constBegin().value();
    }

    /**
     * Returns the smallest key that is stored with an item.
     * @return the smallest key
     * \note This function assumes that the list is not empty.
     */
    inline Key smallestKey() const
    {
        return _map.constBegin().key();
    }

    /**
     * Returns the element of the list, that was inserted with the largest key.
     * @return the element with the largest key
     */
    inline T& largest()
    {
        typename map_type::iterator it = _map.end();
        while (--it == _map.end());
        return it.value();
    }

    /**
     * This is an overloaded function.
     * @return the last element of the list
     */
    inline const T& largest() const
    {
        typename map_type::const_iterator it = _map.constEnd();
        while (--it == _map.constEnd());
        return it.value();
    }

    /**
     * Returns the largest key that is stored with an item.
     * @return the largest key
     * \note This function assumes that the list is not empty.
     */
    inline Key largestKey() const
    {
        typename map_type::const_iterator it = _map.constEnd();
        while (--it == _map.constEnd());
        return it.key();
    }

    /**
     * Inserts \a value into the list, sorted by \a key ascending.
     * @param key the sorting key
     * @param value the value to insert
     */
    inline void insert(Key key, T value)
    {
        _map.insert(key, value);
    }

    /**
     * Returns the item associated with the smallest key and removes it from the
     * list.
     * @return the item with the smallest key
     * \sa smallest(), largest, takeLargest()
     * \note This function assumes that the list is not empty.
     */
    inline T takeSmallest()
    {
        typename map_type::iterator it = _map.begin();
        T item = it.value();
        _map.erase(it);
        return item;
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
        typename map_type::iterator it = _map.end();
        while (--it == _map.end());
        T item = it.value();
        _map.erase(it);
        return item;
    }

    /**
     * This is the same as size().
     * @return number of items in the list
     * \sa size()
     */
    inline int count() const
    {
        return _map.size();
    }

    /**
     * This is the same as count().
     * @return number of items in the list
     * \sa count()
     */
    inline int size() const
    {
        return _map.size();
    }

    /**
     * @return \c true if the queue is emtpy, \c false otherwise
     */
    inline bool isEmpty() const
    {
        return _map.isEmpty();
    }

    /**
     * Returns an STL-style iterator pointing to the item associated with the
     * smallest key.
     * @return iterator
     */
    inline iterator begin()
    {
        return _map.begin();
    }

    /**
     * Returns an STL-style iterator pointing to the imaginary item after the
     * last item in the queue.
     * @return iterator
     */
    inline iterator end()
    {
        return _map.end();
    }

    /**
     * This is an overloaded function.
     * @return iterator
     */
    inline const_iterator begin() const
    {
        return _map.begin();
    }

    /**
     * This is an overloaded function.
     * @return iterator
     */
    inline const_iterator end() const
    {
        return _map.end();
    }

    /**
     * Returns an STL-style iterator pointing to the item associated with the
     * smallest key.
     * @return iterator
     */
    inline const_iterator constBegin() const
    {
        return _map.constBegin();
    }

    /**
     * Returns an STL-style iterator pointing to the imaginary item after the
     * last item in the queue.
     * @return iterator
     */
    inline const_iterator constEnd() const
    {
        return _map.constEnd();
    }

private:
    map_type _map;
};

#endif /* PRIORITYQUEUE_H_ */
