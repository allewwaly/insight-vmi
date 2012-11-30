#ifndef EMBEDRESULT_H
#define EMBEDRESULT_H

/**
 * Describes how to objects in a shared memory area relate to one another.
 * \sa BaseType::embeds(), Instance::embeds()
 */
enum ObjectRelation
{
    /// objects don't share any memory
    orNoOverlap         = 0,
    /// objects overlap in memory but don't share the same type
    orOverlap           = (1 << 0),
    /// objects claim the same memory area but are different
    orCover             = (1 << 1),
    /// objects share the same memory area and are of the same type
    orEqual             = (1 << 2),
    /// first object is contained within the second and matches the second's type within that area
    orFirstEmbedsSecond = (1 << 3),
    /// second object is contained within the first and matches the first's type within that area
    orSecondEmbedsFirst = (1 << 4)
};


#endif // EMBEDRESULT_H
