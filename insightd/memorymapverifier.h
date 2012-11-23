#ifndef MEMORYMAPVERIFIER_H
#define MEMORYMAPVERIFIER_H

class MemoryMapVerifier;

#include "memorymap.h"
#include "memorymapnodesv.h"
#include "slubobjects.h"
#include <QVarLengthArray>

#include <log.h>

// Enable or disable the memory map verification globally
// 1 = Enabled, 0 = Disabled
#define MEMORY_MAP_VERIFICATION 1

// Do we process members with multiple candidate types?
#define MEMORY_MAP_PROCESS_NODES_WITH_ALT 0


/**
 * A watch node is added to a particularly \sa MemoryMapNode. During the memory map
 * generation the \sa MemoryMapNodeWatcher::check() function will be periodically
 * invoked, which verifies that the MemoryMapNode that this watcher node is attached
 * to meets the condition of the watcher. In case the check fails the memory map
 * generation may be interrupted (\sa MemoryMapNodeWatcher::_forcesHalt).
 */
class MemoryMapNodeWatcher
{
public:
    MemoryMapNodeWatcher(MemoryMapNodeSV *node, MemoryMapVerifier &verifier,
                         bool forcesHalt = false);

    virtual ~MemoryMapNodeWatcher();
    /**
     * Returns whether the watcher node will halt the memory map generation in
     * case its condition is not met.
     */
    bool getForceHalt() const;

    /**
     * The main function of a watcher node that is called whenever the condition
     * of the watcher node should be checked.
     */
    virtual bool check() = 0;

    /**
     * This functions is called to receive an individual description message
     * in case the check() function fails.
     *
     * @param lastNode the lastNode that was processed during the memory map creation
     * and may therefore be responsible for failing the check of the node.
     */
    virtual const QString failMessage(MemoryMapNodeSV *lastNode);

protected:
    MemoryMapNodeSV *_node;
    MemoryMapVerifier &_verifier;
    bool _forcesHalt;
};

/**
 * A simple condition that verifies that the probability of a \sa MemoryMapNode
 * remains in a the given interval (\sa MemoryMapNodeIntervalWatcher::_changeInterval)
 */
class MemoryMapNodeIntervalWatcher : public MemoryMapNodeWatcher
{
public:
    /**
     * Constructor.
     *
     * @param node the \sa MemoryMapNode that this watcher will be attached to
     * @param verifier the \sa MemoryMapVerifier that is responsible for this watcher
     * @param changeInterval the interval that the node is allowed to change
     * @param forcesHalt does the watcher stop the generation of the map in case
     * the probability of the node changes by a higher amount than the interval
     * allows
     * @param flexibleInterval is this a static (false) or flexible (true) interval?
     * In the first case the probability of the node should not change by more than
     * the given interval during all rounds. In the latter case the probability of the
     * node must not change by more than the given interval in each round, but may change
     * by an arbitrary amount over an infinite number of rounds.
     */
    MemoryMapNodeIntervalWatcher(MemoryMapNodeSV *node, MemoryMapVerifier &verifier,
                                 float changeInterval, bool forcesHalt = false,
                                 bool flexibleInterval = false);
    bool check();

private:
    float _changeInterval;
    float _lastProbability;
    bool _flexibleInterval;
};

/**
 * A class that contains various helper functions that can either give an indication of the
 * correctness of a memory map or can be used to debug a memory map.
 */

class MemoryMapVerifier
{
public:
    MemoryMapVerifier(MemoryMap *map);
    MemoryMapVerifier(MemoryMap *map, const char *slubFile);

    /**
     * Called every time a new node is fetched from the queue during the memory map
     * generation. Notice that in contrast to \sa performChecks the node has not been
     * processed when this function is called.
     *
     * @param currentNode the node that has been taken from the queue
     */
    void newNode(MemoryMapNode *currentNode);

    /**
     * This functions verifies the current state of a memory map. It is called every time
     * a new \sa MemoryMapNode was taken from the queue and processed. It will invoke all
     * \sa MemoryMapNodeWatcher that watch a node from this memory map.
     *
     * @param lastNode the last \sa MemoryMapNode that was processed.
     * @returns false in case the memory map generation should be interrupted, since a critical
     * check was failed or true in case generation should continue.
     */
    bool performChecks(MemoryMapNode *n);

    /**
     * Finds the least common ancestor of the two given nodes. In the case that
     * a is the parent of b, the function returns a. Similarly if b is the parent
     * of a, b is returned.
     */
    MemoryMapNode * leastCommonAncestor(MemoryMapNode *aa, MemoryMapNode *bb);

    /**
     * Returns the result of the last verification of the memory map.
     */
    bool lastVerification() const;

    /**
     * Destroy the existing watch nodes.
     */
    void resetWatchNodes();

    /**
     * Parse the given slub file. In case no slub file is given, this function will try
     * to parse the slub file that was provided to the constructor if any.
     * @param slubFile the full path to the slub file that should be parsed
     * @returns true if the slub file could be parsed, false otherwise.
     */
    bool parseSlubData(const QString &slubFile = QString());

    /**
     * Collect and print statistics about the current memory map.
     */
    void statistics();

private:
    /**
     * Internal helper function for the statistics function.
     */
    void statisticsHelper(MemoryMapNode *node);

    /**
     * Collects statistics about a single node
     */
    void statisticsCountNodeSV(MemoryMapNodeSV *node);

    /**
     * Collects statistics about a single node
     */
    void statisticsCountNodeCS(MemoryMapNode *node);

    MemoryMap *_map;
    QList<MemoryMapNodeWatcher *> _watchNodes;
    Log _log;
    bool _lastVerification;
    QString _slubFile;
    bool _slubDataParsed;
    bool _slubDataAvailable;
    SlubObjects _slub;

    quint32 _validObjects;
    quint32 _invalidObjects;
    quint32 _maybeValidObjects;
    quint32 _maybeValidSlubObjects;
    
    quint32 _seemValidObjects;

    quint32 _magicNumberValid;
    quint32 _magicNumberValid_withConst;
    quint32 _magicNumberValid_notSlub;
    quint32 _magicNumberInvalid;

    QVarLengthArray<quint32, 10> _slubValidDistribution;
    QVarLengthArray<quint32, 10> _slubInvalidDistribution;

    QVarLengthArray<quint32, 10> _magicnumberValidDistribution;
    QVarLengthArray<quint32, 10> _magicnumberInvalidDistribution;


    float _minValidProbability;
    float _maxInvalidProbability;

    QMutex verifierMutex;
};

#endif // MEMORYMAPVERIFIER_H
