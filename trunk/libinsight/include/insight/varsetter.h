/*
 * varsetter.h
 *
 *  Created on: 31.08.2010
 *      Author: chrschn
 */

#ifndef VARSETTER_H_
#define VARSETTER_H_

/**
 * This utility template class sets a given variable to a defined value in its
 * destructor.
 */
template<class T>
class VarSetter
{
public:
    /**
     * Constructor
     * @param variable the variable to set
     * @param valueOnExit the value to set \a variable to when the destructor
     * of this VarSetter object is invoked
     * is invoked
     */
    VarSetter(T* variable, const T& valueOnExit)
        : _var(variable), _valOnExit(valueOnExit)
    {
    }

    /**
     * Constructor
     * @param variable the variable to set
     * @param valueNow the value to set \a variable to immediately
     * @param valueOnExit the value to set \a variable to when the destructor
     * of this VarSetter object is invoked
     */
    VarSetter(T* variable, const T& valueNow, const T& valueOnExit)
        : _var(variable), _valOnExit(valueOnExit)
    {
        if (_var)
            *_var = valueNow;
    }

    /**
     * Destructor, sets the previously specified variable to the specified
     * value.
     */
    ~VarSetter()
    {
        if (_var)
            *_var = _valOnExit;
    }

private:
    T* _var;      ///< pointer to the variable to set
    T _valOnExit; ///< value to set _var to
};

#endif /* VARSETTER_H_ */
