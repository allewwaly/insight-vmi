/*
 * priorityqueue.h
 *
 *  Created on: 05.10.2010
 *      Author: chrschn
 */

#ifndef PRIORITYQUEUE_H_
#define PRIORITYQUEUE_H_

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
    /// Queue item class
    class Item {
    public:
        Item() : _key(Key()), _value(T()) {}
        Item(const Key& key, const T& value) : _key(key), _value(value) {}
        inline const Key& key() const { return _key; }
        inline const T& value() const { return _value; }
        inline T& value() { return _value; }
    private:
        Key _key;
        T _value;
    };

    typedef QLinkedList<Item> list_type;

    /// Iterator type for this container
    typedef typename list_type::iterator iterator;

    /// Iterator type for this container, const version
    typedef typename list_type::const_iterator const_iterator;

public:

    /**
     * Constructor
     */
    PriorityQueue() : _cur(_list.end())
    {
    }

    /**
     * Clears all data.
     */
    inline void clear()
    {
        _list.clear();
        _cur = _list.end();
    }

    /**
     * Returns the element of the list, that was inserted with the smallest key.
     * @return the element with the smallest key
     * \note This function assumes that the list is not empty.
     */
    inline T& smallest()
    {
        return _list.first().value();
    }

    /**
     * This is an overloaded function.
     * @return the element with the smallest key
     * \note This function assumes that the list is not empty.
     */
    inline const T& smallest() const
    {
        return _list.first().value();
    }

    /**
     * Returns the smallest key that is stored with an item.
     * @return the smallest key
     * \note This function assumes that the list is not empty.
     */
    inline Key smallestKey() const
    {
        return _list.first().key();
    }

    /**
     * Returns the element of the list, that was inserted with the largest key.
     * @return the element with the largest key
     */
    inline T& largest()
    {
        return _list.last().value();
    }

    /**
     * This is an overloaded function.
     * @return the last element of the list
     */
    inline const T& largest() const
    {
        return _list.last().value();
    }

    /**
     * Returns the largest key that is stored with an item.
     * @return the largest key
     * \note This function assumes that the list is not empty.
     */
    inline Key largestKey() const
    {
        return _list.last().key();
    }

    /**
     * Inserts \a value into the list, sorted by \a key ascending.
     * @param key the sorting key
     * @param value the value to insert
     */
    inline void insert(const Key& key, const T& value)
    {
        // Insert sorted with the smallest key at the beginning and the largest
        // key at the end
        if (_list.isEmpty())
            _list.append(Item(key, value));
        else if (_list.last().key() < key)
            _list.append(Item(key, value));
        else if (_list.first().key() >= key)
            _list.prepend(Item(key, value));
        else {
            // Use _cur iterator or start from beginning, depending which
            // distance is smaller
            if (_cur == _list.end() ||
                qAbs(key - _cur->key()) > qAbs(key - _list.first().key()))
                _cur = _list.begin();
            // Which direction to go?
            if (key < _cur->key()) {
                while (key < _cur->key() && _cur != _list.begin())
                    --_cur;
                // Value is inserted BEFORE _cur, so we may need to go back one step
                if (key >= _cur->key())
                    ++_cur;
                _cur = _list.insert(_cur, Item(key, value));
            }
            else {
                while (_cur != _list.end() && key > _cur->key())
                    ++_cur;
                _cur = _list.insert(_cur, Item(key, value));
            }
        }
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
        if (_list.begin() == _cur)
            ++_cur;
        return _list.takeFirst().value();
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
        if (--_list.end() == _cur)
            --_cur;
        return _list.takeLast().value();
    }

    /**
     * This is the same as size().
     * @return number of items in the list
     * \sa size()
     */
    inline int count() const
    {
        return _list.size();
    }

    /**
     * This is the same as count().
     * @return number of items in the list
     * \sa count()
     */
    inline int size() const
    {
        return _list.size();
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
    iterator _cur;
};


#endif /* PRIORITYQUEUE_H_ */
